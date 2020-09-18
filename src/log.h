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
#ifndef LOG_H
#define LOG_H

#include "config.h"

#ifdef __cplusplus
#ifdef DEBUG_PRINT
#define LOG_PRINTF_ENABLED 1
#include <Arduino.h>
extern "C" void DebugPrint(char *data);
#define LOG_INFO(...)         Serial.print(__VA_ARGS__)
#define LOG_INFO_PRINTF(...)  Serial.printf(__VA_ARGS__)
#else
#define LOG_INFO(...)
#endif
#else //__cplusplus
#if defined(DEBUG_PRINT)&&defined(ESP8266)
#define LOG_PRINTF_ENABLED 0
#define LOG_INFO_PRINTF(...)
void DebugPrint(char *data);
#define LOG_INFO(x)  DebugPrint(x)
#define LOG_ERR(x)   DebugPrint(x)
#else
#define LOG_PRINTF_ENABLED 0
#define LOG_INFO_PRINTF(...)
#define LOG_INFO(...)
#define LOG_ERR(...)
#endif
#endif

#endif // LOG_H
