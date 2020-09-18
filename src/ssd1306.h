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
#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DCDCCTRLMODESET     0xAD
#define DCDCSETON           0x8B
#define COLUMNADDR          0x21
#define COMSCANDEC          0xC8
#define COMSCANINC          0xC0
#define DISPLAYALLON        0xA5
#define DISPLAYALLON_RESUME 0xA4
#define DISPLAYOFF          0xAE
#define DISPLAYON           0xAF
#define EXTERNALVCC         0x1
#define INVERTDISPLAY       0xA7
#define MEMORYMODE          0x20
#define NORMALDISPLAY       0xA6
#define PAGEADDR            0x22
#define PAGESTARTADDRESS    0xB0
#define SEGREMAP            0xA1
#define SETCOMPINS          0xDA
#define SETCONTRAST         0x81
#define SETDISPLAYCLOCKDIV  0xD5
#define SETDISPLAYOFFSET    0xD3
#define SETHIGHCOLUMN       0x10
#define SETLOWCOLUMN        0x00
#define SETMULTIPLEX        0xA8
#define SETPRECHARGE        0xD9
#define SETSEGMENTREMAP     0xA1
#define SETSTARTLINE        0x40
#define SETVCOMDETECT       0xDB
#define SWITCHCAPVCC        0x2

#define SSD1306_HEIGHT          64
#define SSD1306_WIDTH           128
#define SSD1306_XSHIFT          2

// Enumeration for screen colors
typedef enum {
    OLED_BLACK = 0x00, // Black color, no pixel
    OLED_WHITE = 0x01  // Pixel is set. Color depends on OLED
} SSD1306_COLOR;

// Procedure definitions
bool ssd1306_Init(uint8_t contrast);
void ssd1306_SetContrast(uint8_t contrast);
void ssd1306_PowerDown(void);
void ssd1306_Fill(SSD1306_COLOR color);
void ssd1306_UpdateScreen(void);
void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color);

// Low-level procedures
void ssd1306_Reset(void);
void ssd1306_WriteCommand(uint8_t byte);
void ssd1306_WriteData(uint8_t* buffer, size_t buff_size);

#ifdef __cplusplus
}
#endif

#endif
