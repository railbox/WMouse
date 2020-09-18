/* 
 * This file is part of the WMouse distribution https://github.com/railbox/WMouse.
 * Copyright (c) 2020 Anton Nadezhdin.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include "z21client.h"
#include "log.h"
#include "string.h" //for memcpy

#pragma pack(1)
typedef struct {
  uint8_t railcom; //0=off, 1=on
  uint8_t reserved0;
  uint8_t powerButton; //0=Stop moving, 1=Emergency stop
  uint8_t selectionMode; //Selection-Mode: 0=No, 1=Bit, 2=uint8_t, 3=Both
  uint8_t reserved1[6];
  uint8_t resetPacketsStart; //(25-255)
  uint8_t resetPacketsCont;  //(6-64)
  uint8_t programPackets;    //(7-64)
  uint8_t reserved[9];
  uint16_t MainVoltage;      //mV
  uint16_t ProgVoltage;      //mV
}config_t;

#define Z21_BC_NONE         0x00000000
#define Z21_BC_ALL          0x00000001
#define Z21_BC_RBUS         0x00000002
#define Z21_BC_RAILCOM      0x00000004
#define Z21_BC_SYSINFO      0x00000100
#define Z21_BC_NETALL       0x00010000
#define Z21_BC_LN           0x01000000
#define Z21_BC_LN_LOCOS     0x02000000
#define Z21_BC_LN_SWITCHES  0x04000000
#define Z21_BC_LN_GBM       0x08000000
#define Z21_BC_RAILCOM_ALL  0x00040000
#define Z21_BC_CANDETECTOR  0x00080000

#define DCCSTEP14       0x01
#define DCCSTEP28       0x02
#define DCCSTEP128      0x03

#define SetAsU16(x)         *(uint16_t*)&(x)
#define SetAsU32(x)         *(uint32_t*)&(x)

//////////////////////////////////////////////////////////////////////////////////////////
#define XNET_HEADER   0  //Message header
#define XNET_DATA_1   1  //Data uint8_t 1
#define XNET_DATA_2   2  //Data uint8_t 2
#define XNET_DATA_3   3  //Data uint8_t 3
#define XNET_DATA_4   4  //Data uint8_t 4
#define XNET_DATA_5   5  //Data uint8_t 5
#define XNET_DATA_6   6  //Data uint8_t 6
#define XNET_DATA_7   7  //Data uint8_t 7
#define XNET_BUF_SIZE 8

static const uint16_t func_group_mask[] = {0x0F,0xFF,0xFF,0xFF};
static const uint8_t func_group_shift[] = {1,5,13,21};

static bool inServiceMode = false;
static bool programmingActive = false;

static z21client_callback_t mXpressNetCallback;
static dataCallback_t mSendDataCallback;

static bool sendXNetData(uint8_t *data, uint8_t len);
static void getXOR (uint8_t *data, uint8_t size);
static bool checkXOR (uint8_t *data, uint8_t size);
static int getFuncState(uint8_t group, uint8_t funcByte);
static int getFuncMask(uint8_t group);

//--------------------------------------------------------------------------------------------
static void ParseXNetMsg(uint8_t *XNetMsg, uint8_t len){
    if (checkXOR(XNetMsg, len)) {
        switch (XNetMsg[XNET_HEADER]) {
            case 0x61:  //Broadcast
                switch (XNetMsg[XNET_DATA_1]){
                    case 0x00: // Track power off
                        if (mXpressNetCallback.notifyXNetPower) mXpressNetCallback.notifyXNetPower(CS_TRACK_OFF);
                        break;
                    case 0x01: // Normal Operation Resumed
                        if (mXpressNetCallback.notifyXNetPower) mXpressNetCallback.notifyXNetPower(CS_NORMAL);
                        break;
                    case 0x02: // Service Mode Entry
                        inServiceMode = true;
                        if (mXpressNetCallback.notifyXNetPower) mXpressNetCallback.notifyXNetPower(CS_SERV_MODE);
                        break;
                    case 0x08: // Track Short
                        if (mXpressNetCallback.notifyXNetPower) mXpressNetCallback.notifyXNetPower(CS_TRACK_SHORTED);
                        break;
                    case 0x12: // Service mode: short circuit
                        if (programmingActive) {
                            if (mXpressNetCallback.notifyXNetServiceError)
                                mXpressNetCallback.notifyXNetServiceError();
                            if (inServiceMode)
                                z21Client_setPower(CS_NORMAL);
                            }
                        break;
                    case 0x13: // Service mode: no ACK
                        if (programmingActive) {
                            if (mXpressNetCallback.notifyXNetServiceError)
                                mXpressNetCallback.notifyXNetServiceError();
                            if (inServiceMode)
                                z21Client_setPower(CS_NORMAL);
                            programmingActive = false;
                        }
                        break;
                    case 0x80: //Transfer error

                        break;
                    case 0x81: //Command station busy

                        break;
                    case 0x82: //Instruction not supported

                        break;
                }
            case 0x62: //Command status response
                if ((XNetMsg[XNET_DATA_1] == 0x22) && mXpressNetCallback.notifyXNetPower) {
                    uint8_t status = 0;
                    if ((XNetMsg[XNET_DATA_2] & 0x04) != 0) // Track Short
                      status |= CS_TRACK_SHORTED;
                    else if ((XNetMsg[XNET_DATA_2] & 0x20) != 0) // Service Mode
                      status |= CS_SERV_MODE;
                    else if ((XNetMsg[XNET_DATA_2] & 0x02) != 0) // Track off
                      status |= CS_TRACK_OFF;
                    else if ((XNetMsg[XNET_DATA_2] & 0x01) != 0) // Emergency stop
                      status |= CS_ESTOP;
                    mXpressNetCallback.notifyXNetPower(status);
                }
                break;
            case 0x64: { //Service mode response
                if (XNetMsg[XNET_DATA_1] == 0x14) {
                    if (programmingActive) {
                        uint16_t CV = ((XNetMsg[XNET_DATA_2]<<8) + XNetMsg[XNET_DATA_3]) + 1;
                        uint8_t value = XNetMsg[XNET_DATA_4];
                        if (mXpressNetCallback.notifyXNetService) mXpressNetCallback.notifyXNetService(true,CV,value);
                        if (inServiceMode && programmingActive)
                            z21Client_setPower(CS_NORMAL);
                        programmingActive = false;
                    }
                }
                break;
            }
            case 0x81: //Emergency Stop
                if ((XNetMsg[XNET_DATA_1] == 0x00) && mXpressNetCallback.notifyXNetPower) {
                    mXpressNetCallback.notifyXNetPower(CS_ESTOP);
                }
                break;
            case 0xEF: {
              uint16_t address = (((XNetMsg[XNET_DATA_1]&0x3F)<<8) + XNetMsg[XNET_DATA_2]);
              uint8_t steps;
              uint8_t speed = XNetMsg[XNET_DATA_4];
              switch (XNetMsg[XNET_DATA_3]) {
                case 0:
                  steps = 14;
                  break;
                case 2:
                  steps = 28;
                  break;
                case 4:
                  steps = 128;
                  break;
                default:
                  steps = 28;
                  break;
              }
              if (mXpressNetCallback.notifyXNetExtSpeed)
                  mXpressNetCallback.notifyXNetExtSpeed(address,steps,speed);
              if (mXpressNetCallback.notifyXNetExtFunc) {
                  mXpressNetCallback.notifyXNetExtFunc(address,1+getFuncMask(0),
                                (((XNetMsg[XNET_DATA_5]&0x10)>>4) + getFuncState(0,XNetMsg[XNET_DATA_5])));
                  for (uint8_t funcGroup=1; funcGroup<4; funcGroup++) {
                      mXpressNetCallback.notifyXNetExtFunc(address,getFuncMask(funcGroup), getFuncState(funcGroup,XNetMsg[XNET_DATA_5+funcGroup]));
                  }
              }
            }
            default:
                break;
        }
#if 0
        if ((XNetMsg[XNET_HEADER] >= 0x42) && (XNetMsg[XNET_HEADER] <= 0x4E)){
            if (!mXpressNetCallback.notifyXNetFeedback) return;
            uint8_t size = (XNetMsg[XNET_HEADER]&0xF);
            for (uint8_t i=0; i < size; i+=2){
                if ((XNetMsg[size+2] & 0x10) != 0){
                    mXpressNetCallback.notifyXNetFeedback(XNetMsg[size+1], 0xF0,  ((XNetMsg[size+2] & 0x0F) << 4));
                }else {
                    mXpressNetCallback.notifyXNetFeedback(XNetMsg[size + 1],  0x0F,  (XNetMsg[size + 2] & 0x0F));
                }
            }
        }
#endif
    } else {
      LOG_ERR("Wrong XOR\n\r");
    }
}

//--------------------------------------------------------------------------------------------
bool z21Client_setPower(uint8_t power)
{
    bool ret = false;

    switch (power) {
        case CS_NORMAL: {
            //uint8_t PowerAn[] = {0x21, 0x81, 0xA0, 0x21, 0x81, 0xA0};
            uint8_t PowerAn[] = {0x21, 0x81, 0xA0};
            ret = sendXNetData(PowerAn, sizeof(PowerAn));
            break;
        }
        case CS_ESTOP: {
            //uint8_t EmStop[] = {0x80, 0x80, 0x80, 0x80};
            uint8_t EmStop[] = {0x80, 0x80};
            ret = sendXNetData(EmStop, sizeof(EmStop));
            break;
        }
        case CS_TRACK_OFF: {
            //uint8_t PowerAus[] = {0x21, 0x80, 0xA1, 0x21, 0x80, 0xA1};
            uint8_t PowerAus[] = {0x21, 0x80, 0xA1};
            ret = sendXNetData(PowerAus, sizeof(PowerAus));
            break;
        }
    }
    return ret;
}

bool z21Client_setSpeed(uint16_t locoAddress, uint8_t steps, uint8_t speed)
{
    uint8_t LocoInfo[] = {LAN_X_SET_LOCO, 0x13, 0x00, 0x00, speed, 0x00};

    switch (steps) {
        case 14: LocoInfo[1] = 0x10; break;
        case 28: LocoInfo[1] = 0x12; break;
        //case 128: LocoInfo[1] = 0x13; break; //default to 128 Steps!
    }
    LocoInfo[2] = ((locoAddress >> 8) & 0x3F) | ((locoAddress >= 128) ? 0xC0 : 0x00);
    LocoInfo[3] = locoAddress & 0xFF;
    getXOR(LocoInfo, sizeof(LocoInfo));
    return sendXNetData(LocoInfo, sizeof(LocoInfo));
}

bool z21Client_setLocoFunc(uint16_t locoAddress, uint8_t num, uint32_t funcStates)
{
    uint8_t LocoInfo[] = {LAN_X_SET_LOCO, LAN_X_SET_LOCO_FUNCTION, 0x00, 0x00, 0x00, 0x00};
    uint8_t stateByte = (funcStates>>num) & 0x1;

    LocoInfo[2] = ((locoAddress >> 8) & 0x3F) | ((locoAddress >= 128) ? 0xC0 : 0x00);
    LocoInfo[3] = locoAddress & 0xFF;
    LocoInfo[4] = (num & 0x3F) | (stateByte<<6);
    getXOR(LocoInfo, sizeof(LocoInfo));
    return sendXNetData(LocoInfo, sizeof(LocoInfo));
}

bool z21Client_setTrntPos(uint16_t address, bool state, bool active)
{
    uint8_t TrntInfo[] = {LAN_X_SET_TURNOUT, 0x00, 0x00, 0x80, 0x00};

    TrntInfo[1] = (address >> 8);
    TrntInfo[2] |= (address & 0xFF);
    TrntInfo[3] |= (active ? 1: 0) << 3;
    TrntInfo[3] |= state ? 1 : 0;
    getXOR(TrntInfo, sizeof(TrntInfo));
    return sendXNetData(TrntInfo, sizeof(TrntInfo));
}

bool z21Client_requestStatus(void)
{
  uint8_t data[] = {LAN_X_GET_STATUS, 0x24, 0x05};
  return sendXNetData(data, sizeof(data));
}

bool z21Client_setCV(uint16_t cv, uint8_t value) {
    uint8_t cvInfo[] = {LAN_X_CV_WRITE, 0x12, 0x00, 0x00, value, 0x00};
    if (cv == 0) return false;
    cv--;
    cvInfo[2] = cv>>8;
    cvInfo[3] = cv&0xFF;
    getXOR(cvInfo, sizeof(cvInfo));
    if (sendXNetData(cvInfo, sizeof(cvInfo))){
        programmingActive = true;
        return true;
    }
    return false;
}

bool z21Client_setPoMCV(uint16_t address, uint16_t cv, uint8_t value)
{
    uint8_t cvInfo[] = {LAN_X_CV_POM, LAN_X_CV_POM_IO, 0x00, 0x00, 0xEC, 0x00, value, 0x00};
    if (cv == 0) return false;
    cv--;
    cvInfo[2] = address>>8;
    cvInfo[3] = address&0xFF;
    cvInfo[4] |= (cv>>8)&0x3; 
    cvInfo[5] |= cv&0xFF; 
    getXOR(cvInfo, sizeof(cvInfo));
    if (sendXNetData(cvInfo, sizeof(cvInfo))){
        return true;
    }
    return false;
}

bool z21Client_requestReadCV(uint16_t address){
    uint8_t cvInfo[] = {LAN_X_CV_READ, 0x11, 0x00, 0x00, 0x00};
    if (address == 0) return false;
    address--;
    cvInfo[2] = address>>8;
    cvInfo[3] = address&0xFF;
    getXOR(cvInfo, sizeof(cvInfo));
    if (sendXNetData(cvInfo, sizeof(cvInfo))) {
        programmingActive = true;
        return true;
    }
    return false;
}

static void getXOR (uint8_t *data, uint8_t size) {
    uint8_t XOR = 0x00;
    for (uint16_t i = 0; i < size-1; i++) {
        XOR = (XOR ^ data[i]);
    }
  data[size-1] = XOR;
}

static bool checkXOR(uint8_t *data, uint8_t size) {
    uint8_t XOR = 0x00;
    for (uint16_t i = 0; i < size-1; i++) {
        XOR = (XOR ^ data[i]);
    }
    return data[size-1] == XOR;
}

static int getFuncState(uint8_t group, uint8_t funcByte){
    return (funcByte & func_group_mask[group]) << func_group_shift[group];
}

static int getFuncMask(uint8_t group){
    return func_group_mask[group] << func_group_shift[group];
}
//////////////////////////////////////////////////////////////////////////////////////////////
static void sendPacket(uint16_t dataLen, uint16_t header, uint8_t *data, bool withXOR)
{
  uint8_t packet[24];
  uint8_t i, packetLen = dataLen + LAN_HEADER_LEN + (withXOR?1:0);

  //Fill the packet structure
  packet[0] = packetLen & 0xFF;
  packet[1] = packetLen >> 8;
  packet[2] = header & 0xFF;
  packet[3] = header >> 8;
  packet[packetLen - 1] = 0;
  memcpy(packet + LAN_HEADER_LEN, data, dataLen);
  if (withXOR)
  {
    for (i = 0; i < dataLen; i++)
      packet[packetLen-1] ^= data[i];
  }
  if (mSendDataCallback) mSendDataCallback(packet, packetLen);
}

static bool sendXNetData(uint8_t *data, uint8_t len)
{
  sendPacket(len, LAN_X_Header, data, false);
  return true;
}

void z21Client_parseReceived(uint8_t* packet, uint8_t size)
{
  int header = (packet[3]<<8) + packet[2];
  //uint32_t buffer[4];
  //uint8_t * data = (uint8_t*)buffer;

  switch (header) {
  case LAN_X_Header:
    ParseXNetMsg(packet + LAN_HEADER_LEN, packet[0] - LAN_HEADER_LEN);
    break;
  }
}

void z21Client_setSendDataCallback(dataCallback_t callback)
{
  mSendDataCallback = callback;
}

void z21Client_setEventCallbacks(z21client_callback_t callback)
{
  mXpressNetCallback = callback;
}
