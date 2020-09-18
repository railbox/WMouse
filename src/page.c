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
#include "page.h"
#include "menu_ll.h"
#include "main_page.h"
#include <stdlib.h> //for atoi()

page_t current_page;
static bool key_shift;

void page_return_back(page_t call_page, void * param)
{
    current_page = call_page;
    switch (call_page) {
    case PAGE_MENU:
        menu_back();
        break;
    case PAGE_SEQUENCE:
        seq_end_item(param);
        break;
    case PAGE_EDIT:
        value_edit_begin();
        break;
    case PAGE_TURNOUT:
        if (param) {
          uint16_t id = atoi((char*)param);
          if (!id) id = 1;
          turnout_set_id(id);
        }
        else turnout_show();
        break;
    case PAGE_LOCO:
        loco_begin();
        break;
    case PAGE_PASSWORD:
        if (param) password_entered(atoi((char*)param));
        else password_entered(0xFFFF);
        break;
    default:
        break;
    }
}

void page_start(page_t base)
{
   current_page = base;
   switch (current_page) {
    case PAGE_TURNOUT:
        turnout_begin();
        break;
    case PAGE_LOCO:
        loco_begin();
        break;
    default:
        break;
    }
}

void page_event_next(bool state)
{
    if (!state && (current_page != PAGE_TURNOUT)) return;

    switch (current_page) {
    case PAGE_MENU:
        menu_next();
        break;
    case PAGE_EDIT:
        value_edit_next();
        break;
    case PAGE_LOCO:
        loco_next();
        break;
    case PAGE_TURNOUT:
        turnout_set(state);
        break;
    default:
        break;
    }
}

void page_event_prev(bool state)
{
    if (!state && (current_page != PAGE_TURNOUT)) return;

    switch (current_page) {
    case PAGE_MENU:
         menu_prev();
        break;
    case PAGE_EDIT:
        value_edit_prev();
        break;
    case PAGE_LOCO:
        loco_prev();
        break;
    case PAGE_TURNOUT:
        turnout_reset(state);
        break;
    default:
        break;
    }
}

void page_event_timeout(void)
{
    switch (current_page) {
    case PAGE_EDIT:
        value_edit_timeout();
        break;
    default:
        break;
    }
}

void page_event_enter(bool state)
{
    if (state) return;

    switch (current_page) {
    case PAGE_MENU:
        menu_enter();
        break;
    case PAGE_EDIT:
        value_edit_enter();
        break;
    case PAGE_LOCO:
        loco_enter();
        break;
    case PAGE_ERROR:
        main_exit_error();
        break;
    default:
        break;
    }
}

void page_event_back(bool state)
{
    if (!state) return;

    switch (current_page) {
    case PAGE_MENU:
         menu_back();
        break;
    case PAGE_EDIT:
        value_edit_exit();
        break;
    case PAGE_TURNOUT:
    case PAGE_LOCO:
        track_stop();
        break;
    case PAGE_ERROR:
        main_exit_error();
        break;
    case PAGE_SEQUENCE:
        seq_end_item(NULL);
        break;
    default:
        break;
    }
}

void page_event_key(uint8_t key, bool state)
{
    if (!state) return;

    switch (current_page) {
    case PAGE_EDIT:
        value_edit_set(key);
        break;
    case PAGE_LOCO:
        if (key==0) key+=10;
        loco_key(key);
        break;
    case PAGE_TURNOUT:
        turnout_key(key);
        break;
    default:
        break;
    }
}

void page_event_shift(bool state)
{
    key_shift = state;

    switch (current_page) {
    case PAGE_LOCO:
        loco_shift(key_shift);
        break;
    case PAGE_EDIT:
        value_edit_shift(state);
    default:
        break;
    }
}

void page_event_mode(bool state)
{
    if (!state) return;

    switch (current_page) {
    case PAGE_LOCO:
        current_page = PAGE_TURNOUT;
        turnout_begin();
        break;
    case PAGE_TURNOUT:
        current_page = PAGE_LOCO;
        loco_begin();
        break;
    default:
        break;
    }
}

void page_event_menu(bool state)
{
    if (!state) return;

    switch (current_page) {
    case PAGE_LOCO:
        loco_exit();
        menu_start(current_page, menu_base);
        break;
    case PAGE_TURNOUT:
        turnout_exit();
        menu_start(current_page, menu_base);
        break;
    default:
        menu_exit();
        break;
    }
}
