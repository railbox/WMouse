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
#ifndef PAGE_H
#define PAGE_H
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PAGE_NONE = 0,
    PAGE_MENU,
    PAGE_EDIT,
    PAGE_SEQUENCE,
    PAGE_LOCO,
    PAGE_TURNOUT,
    PAGE_PASSWORD,
    PAGE_ERROR,
} page_t;

extern page_t current_page;

void page_return_back(page_t call_page, void * param);
void page_start(page_t base);
void page_event_next(bool state);
void page_event_prev(bool state);
void page_event_timeout(void);
void page_event_enter(bool state);
void page_event_back(bool state);
void page_event_key(uint8_t key, bool state);
void page_event_mode(bool state);
void page_event_shift(bool state);
void page_event_menu(bool state);
void page_repeat(void *arg);

#ifdef __cplusplus
}
#endif

#endif // PAGE_H
