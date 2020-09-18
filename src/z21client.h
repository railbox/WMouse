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
#ifndef Z21_CLIENT_H
#define Z21_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

#define LAN_HEADER_LEN               4
#define LAN_X_Header                 0x40
#define LAN_GET_SERIAL_NUMBER        0x10
#define LAN_GET_CODE                 0x18
#define LAN_LOGOFF                   0x30
#define LAN_SET_BROADCASTFLAGS       0x50
#define LAN_GET_BROADCASTFLAGS       0x51
#define LAN_SYSTEMSTATE_DATACHANGED  0x84
#define LAN_SYSTEMSTATE_GETDATA      0x85
#define LAN_GET_HWINFO               0x1A
#define LAN_GET_LOCOMODE             0x60
#define LAN_SET_LOCOMODE             0x61
#define LAN_GET_TURNOUTMODE          0x70
#define LAN_SET_TURNOUTMODE          0x71

#define LAN_X_GET_FIRMWARE_VERSION   0xF1
#define LAN_X_FIRMWARE_VERSION       0xF3
#define LAN_X_GET_LOCO_INFO          0xE3
#define LAN_X_SET_LOCO               0xE4
#define LAN_X_SET_LOCO_FUNCTION      0xF8  
#define LAN_X_LOCO_INFO              0xEF
#define LAN_X_GET_TURNOUT_INFO       0x43 
#define LAN_X_SET_TURNOUT            0x53
#define LAN_X_TURNOUT_INFO           0x43 
#define LAN_X_CV_READ                0x23
#define LAN_X_CV_WRITE               0x24
#define LAN_X_CV_NACK_SC             0x61
#define LAN_X_CV_NACK                0x61
#define LAN_X_CV_RESULT              0x64
#define LAN_X_CV_POM                 0xE6
#define LAN_X_CV_POM_IO              0x30
#define LAN_X_GET_STATUS             0x21

// certain global Xpressnet status indicators
#define CS_NORMAL         0x00 // Normal Operation Resumed
#define CS_ESTOP          0x01 // Emergency stop
#define CS_TRACK_OFF      0x02 // Track voltage off
#define CS_TRACK_SHORTED  0x04 // Track short circuit
#define CS_SERV_MODE      0x08 // Service Mode

#define NUM_OF_LOCO_FUNC  29

typedef struct
{
    void (*notifyXNetPower)(uint8_t status);
    void (*notifyXNetExtControl)(uint16_t locoAddress);
    void (*notifyXNetExtSpeed)(uint16_t locoAddress, uint8_t steps, uint8_t value);
    void (*notifyXNetExtFunc)(uint16_t locoAddress, uint32_t funcMask, uint32_t funcStatus);
    void (*notifyXNetService)(bool directMode, uint16_t CV, uint8_t value);
    void (*notifyXNetServiceError)();
    void (*notifyXNetFeedback)(uint16_t address, uint8_t stateMask, uint8_t state);
} z21client_callback_t;
typedef void (*dataCallback_t)(uint8_t *data, uint8_t len);

void z21Client_setSendDataCallback(dataCallback_t callback);
void z21Client_setEventCallbacks(z21client_callback_t callback);
void z21Client_parseReceived(uint8_t* packet, uint8_t size);

bool z21Client_requestStatus(void);
bool z21Client_setPower(uint8_t power);
bool z21Client_setCV(uint16_t cv, uint8_t value);
bool z21Client_setPoMCV(uint16_t address, uint16_t cv, uint8_t value);
bool z21Client_requestReadCV(uint16_t address);
bool z21Client_setTrntPos(uint16_t address, bool state, bool active);
bool z21Client_setSpeed(uint16_t locoAddress, uint8_t steps, uint8_t speed);
bool z21Client_setLocoFunc(uint16_t locoAddress, uint8_t num, uint32_t funcStates);

#ifdef __cplusplus
}
#endif

#endif
