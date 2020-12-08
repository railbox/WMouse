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
#include "lcd_hl.h"
#include "img.h"
#include "font.h"
#include "string.h"
#include "config.h"

#ifdef _WIN32
#define _WIN32_WINNT 0x0500
#include<windows.h>
extern void SetImgPixel(unsigned int x, unsigned int y, unsigned char color);
#elif defined(ESP8266)
#include "ssd1306.h"
#include "callback.h"
#endif

#define ANT_X       119
#define ANT_Y       3
#define THROTTLE_X  112
#define THROTTLE_Y  25
#define FUNC_X      3
#define FUNC_Y      53
#define STOP_X      21
#define STOP_Y      3
#define SHORT_X     3
#define SHORT_Y     2
#define LEFT_X      53
#define LEFT_Y      12
#define PAUSE_X     63
#define PAUSE_Y     12
#define RIGHT_X     72
#define RIGHT_Y     12
#define BAT_X       57
#define BAT_Y       3
#define LIGHT_X     3
#define LIGHT_Y     30
#define SHIFT_X     3
#define SHIFT_Y     43
#define TRAIN_X     83
#define TRAIN_Y     7
#define SMOKE_X     83
#define SMOKE_Y     2
#define TURNOUT_X   82
#define TURNOUT_Y   3
#define MODE_CLR_X  81
#define MODE_CLR_Y  1
#define MODE_CLR_W  28
#define MODE_CLR_H  23

#define MAIN_LINE_X     14
#define MAIN_LINE_Y     27
#define MAIN_STR_LEN    6
#define BOTTOM_LINE_X   3
#define BOTTOM_LINE_Y   53
#define BOTTOM_STR_LEN  20

#ifdef _WIN32
#define X_SHIFT     0
#define Y_SHIFT     30
static HDC mydc;
static COLORREF white = RGB(255,255,255);
static COLORREF black = RGB(0,0,0);
#elif defined(ESP8266)
#define X_SHIFT     0
#define Y_SHIFT     0
#define COLORREF SSD1306_COLOR
static SSD1306_COLOR white = OLED_WHITE;
static SSD1306_COLOR black = OLED_BLACK;
#endif

#ifdef ESP8266
static callback_handler_t stop_blink_timer, signal_blink_timer, mode_blink_timer, bat_blink_timer, scroll_timer;
static bool stop_blink_state, signal_blink_state, mode_blink_state, bat_blink_state;
#endif
static bool last_mode, last_mode_left;
static bool isUpdating = false;
static uint8_t main_string_pos, main_string_len;
static char main_string[32];

static inline void set_pixel(unsigned int x, unsigned int y, COLORREF color)
{
#ifdef _WIN32
    for (uint8_t i=0; i<3; i++) {
        for (uint8_t j=0;j<3; j++) {
            SetPixel(mydc, 4*(X_SHIFT+x)+i, 4*(Y_SHIFT+y)+j, color);
        }
    }
    SetImgPixel(x, y, (color == white) ? 255 : 0);
#elif defined(ESP8266)
  ssd1306_DrawPixel(X_SHIFT+x, Y_SHIFT+y, color);
#endif
}

static void dwaw_image(bitmap_t *img, uint8_t x, uint8_t y, bool inverse)
{
    uint8_t byte_step, bitmask;
    uint16_t byte_id;
    if (!img) return;

    COLORREF color_on = (!inverse) ? white : black;

    byte_step = (img->width-1)/8 + 1;
    for (uint8_t j=y; j<y+img->height; j++) {
        byte_id = (j-y)*byte_step;
        bitmask = 1;
        for (uint8_t i=0; i<img->width; i++) {
            //set_pixel(x+i, j, ((img->data[byte_id] & bitmask) != 0) ? color_on : color_off);
            if ((img->data[byte_id] & bitmask) != 0) set_pixel(x+i, j, color_on);
            if ((i&0x7) != 0x7) {
                bitmask<<=1;
            } else {
                byte_id++;
                bitmask = 1;
            }
        }
    }
}

static void dwaw_image_clr(bitmap_t *img, uint8_t x, uint8_t y, bool inverse)
{
    uint8_t byte_step, byte_id, bitmask;
    if (!img) return;

    COLORREF color_off = (inverse) ? white : black;

#if 0
    for (uint8_t j=y; j<y+img->height; j++) {
        for (uint8_t i=x; i<x+img->width; i++) {
            set_pixel(i, j, color_off);
        }
    }
#else
    byte_step = (img->width-1)/8 + 1;
    for (uint8_t j=y; j<y+img->height; j++) {
        byte_id = (j-y)*byte_step;
        bitmask = 1;
        for (uint8_t i=0; i<img->width; i++) {
            if ((img->data[byte_id] & bitmask) != 0) set_pixel(x+i, j, color_off);
            if ((i&0x7) != 0x7) {
                bitmask<<=1;
            } else {
                byte_id++;
                bitmask = 1;
            }
        }
    }
#endif
}

static uint8_t current_x, current_y;
void draw_setpos(uint8_t x, uint8_t y)
{
    current_x = x;
    current_y = y;
}

void draw_char(char ch, font_t *font, bool inverse)
{
    if (!font) return;
  	uint32_t byte;
  	uint16_t *data = (uint16_t*)font->data;
  	COLORREF color_on = (!inverse) ? white : black;
  	COLORREF color_off = (!inverse) ? black : white;

  	if (font == &font_16x26) {
          for (uint8_t j = 0; j < font->height; j++) {
              byte = data[(ch - 32) * font->height + j];
              for (uint8_t i = 0; i < font->width; i++) {
                  set_pixel(current_x + i, current_y + j, ((byte << i) & 0x8000) ? color_on : color_off);
              }
          }
  	} else {
          for (uint8_t i = 0; i < font->width; i++) {
              for (uint8_t j = 0; j < font->height; j++) {
                  byte = font->data[(ch - 32) * font->width  + i];
                  set_pixel(current_x + i, current_y + j, (byte & (1<<j)) ? color_on : color_off);
              }
          }
          current_x++;
  	}
  	current_x += font->width;
}

void draw_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool state)
{
    COLORREF color = (state) ? white : black;

    for (uint8_t i=x; i<x+w; i++) {
        set_pixel(i, y, color);
        set_pixel(i, y+h-1, color);
    }
    for (uint8_t j=y; j<y+h; j++) {
        set_pixel(x, j, color);
        set_pixel(x+w-1, j, color);
    }
}

void draw_fill(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool state)
{
    COLORREF color = (state) ? white : black;

    for (uint8_t i=x; i<x+w; i++) {
        for (uint8_t j=y; j<y+h; j++) {
            set_pixel(i, j, color);
        }
    }
}

void draw_vline(uint8_t x, uint8_t y, uint8_t l, bool state)
{
    COLORREF color = (state) ? white : black;

    for (uint8_t j=y; j<y+l; j++) {
        set_pixel(x, j, color);
    }
}

void draw_hline(uint8_t x, uint8_t y, uint8_t l, bool state)
{
    COLORREF color = (state) ? white : black;

    for (uint8_t i=x; i<x+l; i++) {
        set_pixel(i, y, color);
    }
}

////////////////////////////////////////////////////////////////////////////////////
static void lcd_update(void)
{
    if (!isUpdating) {
#ifdef ESP8266
        ssd1306_UpdateScreen();
#endif
    }
}

static void align(const char* in, char *out, alignment_t aligment, uint8_t len)
{
    uint8_t text_len = strlen(in);
    if ((text_len>len) || (aligment == ALIGN_NONE)) {
        strncpy(out, in, len);
        out[len] = '\0';
    } else {
        uint8_t start_pos = (len - text_len)/2;
        memset(out, ' ', start_pos);
        strcpy(out + start_pos, in);
    }
}

static inline void lcd_print(uint8_t x, uint8_t y, const char* str, uint8_t blink_pos,
                             alignment_t aligment, font_t *font, uint8_t max_len)
{
    uint8_t len = strlen(str);
    char showstr[max_len+1];

    align(str, showstr, aligment, max_len);

    len = strlen(showstr);
    if (len>max_len) len = max_len;
    draw_setpos(x, y);
    if (!blink_pos) {
        for (uint8_t i=0; i<len; i++) {
            draw_char(showstr[i], font, false);
        }
        for (uint8_t i=len; i<max_len; i++) {
            draw_char(' ', font, false);
        }
    } else {
        for (uint8_t i=0; i<len; i++) {
            draw_char(showstr[i], font, blink_pos-1 == i);
        }
        for (uint8_t i=len; i<max_len; i++) {
            draw_char(' ', font, blink_pos-1 == i);
        }
    }
    lcd_update();
}

#ifdef ESP8266
static void scroll_handler(void * arg)
{
  if (!main_string_pos) callback_timer_start(scroll_timer, 500, true, scroll_handler, NULL);
  main_string_pos++;
  if (main_string_pos > main_string_len - MAIN_STR_LEN) main_string_pos = 0;
  lcd_print(MAIN_LINE_X, MAIN_LINE_Y, main_string+main_string_pos, 0, ALIGN_NONE, &font_16x26, MAIN_STR_LEN);
}
#endif

void lcd_main_print(const char* str, uint8_t blink_pos, alignment_t aligment)
{
    if (!str) return;

    main_string_len = strlen(str);
    if ((main_string_len > MAIN_STR_LEN) && !blink_pos) {
      strncpy(main_string, str, sizeof(main_string));
      main_string[main_string_len++] = ' ';
      main_string_pos = 0;
#ifdef ESP8266
      if (!scroll_timer) scroll_timer = callback_timer_create();
      callback_timer_start(scroll_timer, 2000, false, scroll_handler, NULL);
    } else callback_timer_stop(scroll_timer);
#else
    }
#endif
    lcd_print(MAIN_LINE_X, MAIN_LINE_Y, str, blink_pos, aligment, &font_16x26, MAIN_STR_LEN);
}

void lcd_bottom_print(const char* str, alignment_t aligment)
{
    if (!str) {
      draw_fill(BOTTOM_LINE_X, BOTTOM_LINE_Y, 5*BOTTOM_STR_LEN, 7, false);
      lcd_update();
    }else lcd_print(BOTTOM_LINE_X, BOTTOM_LINE_Y, str, 0, aligment, &font_5x7, BOTTOM_STR_LEN);
}

#ifdef ESP8266
static void stop_blink_handler(void * arg)
{
  if (stop_blink_state) dwaw_image_clr(&stop, STOP_X, STOP_Y, false);
  else dwaw_image(&stop, STOP_X, STOP_Y, false);
  stop_blink_state = !stop_blink_state;
  lcd_update();
}
#endif

void lcd_set_stop(bool state)
{
#ifdef ESP8266
    if (state) {
      if (!stop_blink_timer) stop_blink_timer = callback_timer_create();
      callback_timer_start(stop_blink_timer, 500, true, stop_blink_handler, NULL);
      dwaw_image(&stop, STOP_X, STOP_Y, false);
      stop_blink_state = true;
    }
    else {
      dwaw_image_clr(&stop, STOP_X, STOP_Y, false);
      callback_timer_stop(stop_blink_timer);
    }
#else
    if (state) dwaw_image(&stop, STOP_X, STOP_Y, false);
    else dwaw_image_clr(&stop, STOP_X, STOP_Y, false);
#endif
    lcd_update();
}


void lcd_set_shortcircuit(bool state)
{
    if (state) dwaw_image(&sc, SHORT_X, SHORT_Y, false);
    else dwaw_image_clr(&sc, SHORT_X, SHORT_Y, false);
    lcd_update();
}

void lcd_set_left(bool state)
{
    if (state) {
        dwaw_image(&left, LEFT_X, LEFT_Y, false);
        dwaw_image(&smoke, SMOKE_X, SMOKE_Y, false);
    }
    else {
        dwaw_image_clr(&smoke, SMOKE_X, SMOKE_Y, false);
        dwaw_image_clr(&left, LEFT_X, LEFT_Y, false);
    }
    lcd_update();
}

void lcd_set_pause(bool state)
{
    if (state) dwaw_image(&pause, PAUSE_X, PAUSE_Y, false);
    else dwaw_image_clr(&pause, PAUSE_X, PAUSE_Y, false);
    lcd_update();
}

void lcd_set_right(bool state)
{
    if (state) {
        dwaw_image(&right, RIGHT_X, RIGHT_Y, false);
        dwaw_image(&smoke_f, SMOKE_X, SMOKE_Y, false);
    } else {
        dwaw_image_clr(&smoke_f, SMOKE_X, SMOKE_Y, false);
        dwaw_image_clr(&right, RIGHT_X, RIGHT_Y, false);
    }
    lcd_update();
}

void lcd_set_battery(uint8_t level, bool state)
{
    static bool last_mode_state;

    if (level>LCD_BAT_MAX_VAL) level = LCD_BAT_MAX_VAL;

    if (state) {
        if (!last_mode_state) dwaw_image(&bat, BAT_X, BAT_Y, false);
        for (uint8_t i=0; i<LCD_BAT_MAX_VAL; i++) {
            draw_rect(BAT_X+(LCD_BAT_MAX_VAL-i)*3, BAT_Y+2, 2, 3, (i<level) ? true : false);
        }
    } else {
        dwaw_image_clr(&bat, BAT_X, BAT_Y, false);
        draw_fill(BAT_X, BAT_Y+2, LCD_BAT_MAX_VAL*3, 3, false);
    }
    last_mode_state = state;
    lcd_update();
}

#ifdef ESP8266
static void signal_blink_handler(void * arg)
{
  if (signal_blink_state) dwaw_image_clr(&ant, ANT_X, ANT_Y, false);
  else dwaw_image(&ant, ANT_X, ANT_Y, false);
  signal_blink_state = !signal_blink_state;
  lcd_update();
}
#endif

void lcd_set_signal(uint8_t level, bool state)
{
    static const uint8_t line_len[3] = {4, 7, 9};
    static bool last_mode_state;

    if (level>LCD_SIG_MAX_VAL) level = LCD_SIG_MAX_VAL;

    if (state) {
        if (!last_mode_state) dwaw_image(&ant, ANT_X, ANT_Y, false);
        for (uint8_t i=0; i<LCD_SIG_MAX_VAL; i++) {
            draw_rect(ANT_X+3-line_len[i], ANT_Y+18-i*4, line_len[i], 2, (i<level) ? true : false);
        }
#ifdef ESP8266
        if (level == 0) {
          if (!signal_blink_timer) signal_blink_timer = callback_timer_create();
          callback_timer_start(signal_blink_timer, 500, true, signal_blink_handler, NULL);
          signal_blink_state = true;
        }
        else {
          callback_timer_stop(signal_blink_timer);
          dwaw_image(&ant, ANT_X, ANT_Y, false);
        }
#endif
    } else {
#ifdef ESP8266
        callback_timer_stop(signal_blink_timer);
#endif
        dwaw_image_clr(&ant, ANT_X, ANT_Y, false);
        for (uint8_t i=0; i<3; i++) {
            draw_rect(ANT_X+3-line_len[i], ANT_Y+18-i*4, line_len[i], 2, false);
        }
    }
    last_mode_state = state;
    lcd_update();
}

void lcd_set_loco_throttle(uint8_t level, bool state)
{
    static bool last_mode_state;
    static uint8_t last_level;
    uint8_t min_lev, max_lev;

    if (level>LOCO_MAX_STEP) level = LOCO_MAX_STEP;
    if (level>last_level) {
        min_lev = last_level;
        max_lev = level;
    } else {
        min_lev = level;
        max_lev = last_level;
    }
    min_lev &= 0x1;
    max_lev |= 0x1;

    if (state) {
        if (!last_mode_state) draw_rect(THROTTLE_X, THROTTLE_Y, 15, 25, true);
        for (uint8_t i=min_lev; i<=max_lev; i++) {
            uint8_t line = i/2;
            if ((i&0x1) == 0) draw_hline(THROTTLE_X+2, THROTTLE_Y+(11-line)*2, 5, (i<=level) ? true : false);
            else draw_hline(THROTTLE_X+7, THROTTLE_Y+(11-line)*2, 6, (i<=level) ? true : false);
        }
    } else {
        draw_rect(THROTTLE_X, THROTTLE_Y, 15, 25, false);
        for (uint8_t i=0; i <= last_level; i++) {
            draw_hline(THROTTLE_X+2, THROTTLE_Y+(11-i)*2, 11, false);
        }
    }
    last_mode_state = state;
    last_level = (state) ? level : 0;
    lcd_update();
}

void lcd_set_loco_func(uint8_t func, bool state)
{
    uint8_t x;
    if (func > 28) return;

    uint8_t id;
    if (func == 10 || func == 20) {
        id = 10;
    } else id = func % 10;

    if (id == 0) {
        if (state) dwaw_image(&light, LIGHT_X, LIGHT_Y, false);
        else dwaw_image_clr(&light, LIGHT_X, LIGHT_Y, false);
    }
    else if (id != 10) {
        x = FUNC_X+(id-1)*12 + ((id < 6)? 0 : 1);
        if (state) {
            if (func < 20) {
                draw_fill(x, FUNC_Y, 11, 11, false);
                draw_rect(x, FUNC_Y, 11, 11, true);
                draw_setpos(x+4, FUNC_Y+2);
                draw_char('0'+id, &font_5x7, false);
                draw_vline(x+2, FUNC_Y+2, 7, id != func);
            } else {
                draw_fill(x, FUNC_Y, 11, 11, true);
                draw_setpos(x+4, FUNC_Y+2);
                draw_char('0'+id, &font_5x7, true);
            }
        } else {
            draw_fill(x, FUNC_Y, 11, 11, false);
        }
    } else {
        x = FUNC_X+(id-1)*12 + 1;
        if (state) {
            draw_setpos(x+2, FUNC_Y+2);
            if (id==func) draw_char('1', &font_5x7, false);
            else draw_char('2', &font_5x7, false);
            draw_rect(x, FUNC_Y, 15, 11, true);
            draw_char('0', &font_5x7, false);
        } else {
            draw_fill(x, FUNC_Y, 15, 11, false);
        }
    }
    lcd_update();
}

void lcd_set_shift(bool state, bool filled)
{
    if (state) {
      if (!filled) dwaw_image(&shift, SHIFT_X, SHIFT_Y, false);
      else dwaw_image(&shift_20, SHIFT_X, SHIFT_Y, false);
    }else draw_fill(SHIFT_X, SHIFT_Y, shift.width, shift.height, false);
    lcd_update();
}

#ifdef ESP8266
static void mode_blink_handler(void * arg)
{
  if (mode_blink_state) {
      if (last_mode) {
          dwaw_image((last_mode_left)? &train : &train_f, TRAIN_X, TRAIN_Y, false);
          dwaw_image((last_mode_left)? &smoke : &smoke_f, SMOKE_X, SMOKE_Y, false);
      } else {
          dwaw_image((last_mode_left)? &turnout_t : &turnout_s, TURNOUT_X, TURNOUT_Y, false);
      }
    } else {
        draw_fill(MODE_CLR_X, MODE_CLR_Y, MODE_CLR_W, MODE_CLR_H, false);
    }
  mode_blink_state = !mode_blink_state;
  lcd_update();
}
#endif

void lcd_set_mode(bool train_mode, bool left, bool blink, bool state)
{
    static bool last_mode_state;
    if ((last_mode_state != state) || (last_mode != train_mode) || (last_mode_left != left)) {
        draw_fill(MODE_CLR_X, MODE_CLR_Y, MODE_CLR_W, MODE_CLR_H, false);
    }

    if (state) {
        if (train_mode) {
            dwaw_image((left)? &train : &train_f, TRAIN_X, TRAIN_Y, false);
        } else {
            dwaw_image((left)? &turnout_t : &turnout_s, TURNOUT_X, TURNOUT_Y, false);
        }
    } else {
        draw_fill(MODE_CLR_X, MODE_CLR_Y, MODE_CLR_W, MODE_CLR_H, false);
    }
#ifdef ESP8266
    if (blink && state) {
      if (!mode_blink_timer) mode_blink_timer = callback_timer_create();
      callback_timer_start(mode_blink_timer, 500, true, mode_blink_handler, NULL);
      mode_blink_state = true;
    }
    else callback_timer_stop(mode_blink_timer);
#endif

    last_mode_state = state;
    last_mode = train_mode;
    last_mode_left = left;
    lcd_update();
}

void lcd_clear(void)
{
#if defined(ESP8266)
    ssd1306_Fill(OLED_BLACK);
    lcd_update();
#endif
}

void lcd_show_logo(const char* version)
{
    dwaw_image(&logo, 0, 0, false);
    lcd_bottom_print(version, ALIGN_CENTER);
    lcd_update();
}

void lcd_begin(void)
{
    isUpdating = true;
}

void lcd_commit(void)
{
    isUpdating = false;
    lcd_update();
}

bool lcd_init(uint8_t contrast)
{
#ifdef _WIN32
    HWND myconsole = GetConsoleWindow();
    mydc = GetDC(myconsole);
    draw_rect(0, 0, 128+1, 64+1, true);
#elif defined(ESP8266)
    if (!ssd1306_Init(contrast)) return false;
#endif

#if 0
    lcd_set_stop(true);
    lcd_set_shortcircuit(true);
    lcd_set_pause(true);
  #if 0
    lcd_set_mode(true, true, true);
    lcd_set_left(true);
  #else
    lcd_set_mode(true, false, true);
    lcd_set_right(true);
  #endif
    lcd_set_battery(3, true);
    lcd_set_signal(2, true);
    lcd_set_loco_throttle(8, true);
    lcd_set_shift(true);
    for (uint8_t i=0; i<=10; i++) {
        lcd_set_loco_func(i, true);
    }
    draw_setpos(MAIN_LINE_X, MAIN_LINE_Y);
    for (uint8_t i=0; i<6; i++) {
        draw_char('0'+i, &font_16x26, false);
    }
  #if 0
    lcd_set_stop(false);
    lcd_set_shortcircuit(false);
    lcd_set_left(false);
    lcd_set_pause(false);
    lcd_set_right(false);
    lcd_set_battery(4, false);
    lcd_set_signal(3, false);
    lcd_set_loco_throttle(10, false);
    lcd_set_mode(false, false, false);
    lcd_set_shift(false);
    for (uint8_t i=0; i<=10; i++) {
        lcd_set_loco_func(i, false);
    }
  #endif
#endif
    return true;
}
