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
#ifndef EEPROM_CONF_H
#define EEPROM_CONF_H

#include <IPAddress.h>
#include <EEPROM.h>

void EEPROMwrite(uint8_t start, uint8_t * data, uint16_t len);
void EEPROMread(uint8_t start, uint8_t * data, uint16_t len);
void EEPROMwriteIP(uint8_t start, IPAddress ip);
IPAddress EEPROMreadIP(uint8_t start);

#endif
