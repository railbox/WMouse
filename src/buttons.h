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
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define STOP_BUTTON_ID    10
#define OK_BUTTON_ID      11
#define MODE_BUTTON_ID    12
#define NEXT_BUTTON_ID    13
#define PREV_BUTTON_ID    14
#define MENU_BUTTON_ID    15
#define SHIFT_BUTTON_ID   16
#define NULL_BUTTON_ID    17
#define MAX_BUTTON_ID     17

typedef void (*buttons_callback_t)(uint8_t id, bool state);

void buttons_init(buttons_callback_t callback);
bool buttons_getstate(uint8_t id);
void buttons_stop(void);

#ifdef __cplusplus
}
#endif

#endif
