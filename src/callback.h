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
#ifndef CALLBACK_H
#define CALLBACK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*callback_funcion_t)(void * arg);
typedef void * callback_handler_t;

callback_handler_t callback_timer_create(void);
void callback_timer_start(callback_handler_t handler, uint32_t ms, bool repeat, callback_funcion_t callback, void * arg);
void callback_timer_stop(callback_handler_t handler);
void callback_timer_delete(callback_handler_t handler);

#ifdef __cplusplus
}
#endif

#endif
