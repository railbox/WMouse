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
#ifndef LCD_HL_H
#define LCD_HL_H

#include <stdbool.h>
#include <stdint.h>

#define LCD_BAT_MAX_VAL 4
#define LCD_SIG_MAX_VAL 3

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    ALIGN_NONE,
    ALIGN_CENTER,
    ALIGN_RIGHT,
}alignment_t;

void lcd_set_stop(bool state);
void lcd_set_shortcircuit(bool state);
void lcd_set_left(bool state);
void lcd_set_pause(bool state);
void lcd_set_right(bool state);
void lcd_set_battery(uint8_t level, bool state);
void lcd_set_signal(uint8_t level, bool state);
void lcd_set_loco_throttle(uint8_t level, bool state);
void lcd_set_loco_func(uint8_t func, bool state);
void lcd_set_mode(bool train, bool left, bool blink, bool state);
void lcd_set_shift(bool state, bool filled);
void lcd_show_logo(const char* version);
void lcd_main_print(const char* str, uint8_t blink_pos, alignment_t aligment);
void lcd_bottom_print(const char* str, alignment_t aligment);

bool lcd_init(uint8_t contrast);
void lcd_begin(void);
void lcd_commit(void);
void lcd_clear(void);


#ifdef __cplusplus
}
#endif

#endif // LCD_HL_H
