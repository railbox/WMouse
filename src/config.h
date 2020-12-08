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
#ifndef CONFIG_H
#define CONFIG_H

//#define DEBUG_PRINT
#define FW_MAJOR    1
#define FW_MINOR    3

#ifdef __cplusplus
#include <IPAddress.h>
#endif
#include "stdbool.h"
#include "stdint.h"
#include "string.h"
#include "main_page.h"
#include <assert.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

//WIFI configuration //////////////////////////////////////////////////////
#define CL_SSID           "Z21_ESP"   // Default Z21 AP (SSID)
#define CL_PASS           "12345678"  // Default Z21 network password
#define IP_MEM_SIZE       20          //Client IP that are stored
#define Z21_BUF_MAX_SIZE  24          //max UDP packet size
#define Z21_PORT          21105       //Z21 UDP port
#define UART_BAUDRATE     115200      //Default serial port baudrate
#define LOCO_MAX_STEP     21


//EEPROM Configuration /////////////////////////////////////////////////////
#define EE_SIZE           4096
#define MAGIC_VALUE       0xAB12
//Client:
#define EE_CONFIG_DB      0

//Pins configuration ////////////////////////////////////////////////////////
#define BT_COL_1          {0} //BOOT SEL PIN
#define BT_COL_2          {3} //DEBUG PRINT RX
#define BT_COL_3          {1} //DEBUG PRINT TX
#define BT_ROW_1          {2, true} //ESP-12 external LED pin
#define BT_ROW_2          {15, false}
#define BT_ROW_3          {13, true}
#define BT_ROW_4          {12, true}
#define BT_ROW_5          {14, true}
#define BT_ROW_6          {16, false}
#define OLED_I2C_ADDR     0x3C
#define OLED_SDA_PIN      5
#define OLED_SCL_PIN      4
#define BAT_IN            A0
#define BUILTIN_LED_PIN   2

#define ROW1_DISCHARGE
#define ROW1_PIN          2

#ifdef DEBUG_PRINT
#define BT_OUTPINS_NUM    1
#define BT_OUTPINS_CONF   {BT_COL_1}
#else
#define BT_OUTPINS_NUM    3
#define BT_OUTPINS_CONF   {BT_COL_1, BT_COL_2, BT_COL_3}
#endif
#define BT_INPINS_NUM     6
#define BT_INPINS_CONF    {BT_ROW_1,BT_ROW_2,BT_ROW_3,BT_ROW_4,BT_ROW_5,BT_ROW_6}

#endif
