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
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <conio.h>
#include "draw.h"
#endif

#include "src/main_page.h"
#include "src/menu_ll.h"
#include "src/page.h"

#include "src/lcd_hl.h"

extern void notifyXNetServiceError(void);
extern void notifyXNetService(bool directMode, short CV, short value);
#define SET_IP(_arr, _x1, _x2, _x3, _x4) {_arr[0]=_x1;_arr[1]=_x2;_arr[2]=_x3;_arr[3]=_x4;}

int main()
{
    config_db.language_id = 0;
    config_db.contrast = 128;
    strcpy(config_db.ssid, "Railbox");
    strcpy(config_db.pass, "oceanic997");
    SET_IP(config_db.ipaddr, 192,168,4,10);
    SET_IP(config_db.maskaddr, 255,255,255,0);
    SET_IP(config_db.gwaddr, 192,168,4,1);
    config_db.loco_db_len = 1;
    strcpy(config_db.loco_db[0].name, "DEFLT");
    config_db.loco_db[0].addr = 3;
    config_db.loco_db[0].ss = 2;


    lcd_init(config_db.contrast);
    lcd_set_battery(3, true);
    lcd_set_signal(3, true);

#if 0
    lcd_set_stop(true);
    lcd_set_shortcircuit(true);
    lcd_set_left(true);
    lcd_set_pause(true);
    lcd_set_right(true);
    lcd_set_loco_throttle(10, true);
    for (uint8_t i=0; i<=10; i++) {
        lcd_set_loco_func(i, true);
    }
    lcd_set_mode(true, true, false, true);
    lcd_set_shift(true);
    lcd_main_print("123456", 0, ALIGN_NONE);

#else
    main_page_init();
    page_start(PAGE_LOCO);
#endif


    bool special_char = false;
    bool shift = false;
    while (1) {
        uint8_t c = _getch( );
        if (special_char) {
            switch (c) {
            case 0x4D:
                page_event_next(true);
                page_event_next(false);
                break;
            case 0x4B:
                page_event_prev(true);
                page_event_prev(false);
                break;
            case 0x50:
                page_event_timeout();
                break;
            default:
                //printf("%X\n", c);
                break;
            }
        }else {
            switch (c) {
            case 0xE0:
                special_char = true;
                continue;
            case '\r':
                page_event_enter(true);
                page_event_enter(false);
                break;
            case 0x1B:
                page_event_back(true);
                page_event_back(false);
                break;
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            case '0':
                page_event_key(c-'0', true);
                page_event_key(c-'0', false);
                break;
            case 'z':
            case 'Z':
                page_event_mode(true);
                page_event_mode(false);
                break;
            case ' ':
                shift = !shift;
                page_event_shift(shift);
                break;
            case 'm':
            case 'M':
                page_event_menu(true);
                page_event_menu(false);
                break;
            case 'e':
            case 'E':
                main_show_error(&err_conn_fault);
                break;
            case 'c':
                notifyXNetService(false, 1, 8);
                break;
            case 'd':
                notifyXNetServiceError();
                break;
            case 'i': {
                char imageFileName[50];
                uint8_t level = 0, *levels, curr_pos, pos;
                levels = menu_get_levels(&curr_pos, &level);
                imageFileName[0] = 'm';
                imageFileName[1] = '_';
                if (levels) {
                    for (uint8_t i=0; i<level; i++) {
                        imageFileName[2+2*i] = '0'+levels[i];
                        imageFileName[2+2*i+1] = '_';
                    }
                    pos = 2+2*level;
                    imageFileName[pos++]='0'+curr_pos;
                    strcpy(imageFileName+pos, ".bmp");
                    SaveBitmapImage(imageFileName);
                }
                break;
            }
            default:
                //printf("%X\n", c);
                break;
            }
        }
        special_char = false;
    }

    return 0;
}
