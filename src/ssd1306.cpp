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
#include "config.h"
#include "ssd1306.h"

#include <Wire.h>
#include <ESP8266WiFi.h>

static uint8_t SSD1306_Buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8];

void ssd1306_WriteCommand(uint8_t command)
{
    Wire.beginTransmission(OLED_I2C_ADDR);
    Wire.write(0x00);
    Wire.write(command);
    Wire.endTransmission();
}

void ssd1306_WriteData(uint8_t* data, size_t len)
{
    uint8_t burst_len;
    while (len) {
      burst_len = (len > 127) ? 127 : len;
      Wire.beginTransmission(OLED_I2C_ADDR);
      Wire.write(0x40);
      for (size_t i=0; i<burst_len; i++) {
        Wire.write(data[i]);
      }
      Wire.endTransmission();
      len -= burst_len;
      data += burst_len;
    }
}

// Write the screenbuffer with changed to the screen
void ssd1306_UpdateScreen(void) 
{
    uint8_t i;
    for(i = 0; i < 8; i++) {
        ssd1306_WriteCommand(0xB0 + i);
        ssd1306_WriteCommand(SSD1306_XSHIFT&0xF);
        ssd1306_WriteCommand(0x10);
        ssd1306_WriteData(&SSD1306_Buffer[SSD1306_WIDTH*i],SSD1306_WIDTH);
    }
}

void ssd1306_Fill(SSD1306_COLOR color)
{
    uint32_t i;

    for(i = 0; i < sizeof(SSD1306_Buffer); i++) {
        SSD1306_Buffer[i] = (color == OLED_BLACK) ? 0x00 : 0xFF;
    }
}

void ssd1306_SetContrast(uint8_t contrast)
{
  ssd1306_WriteCommand(SETCONTRAST);
  ssd1306_WriteCommand(contrast);
}

bool ssd1306_Init(uint8_t contrast)
{  
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN, 0); // join i2c bus (address optional for master)
  Wire.setClock(400000L);

  Wire.beginTransmission(OLED_I2C_ADDR);
  if (Wire.endTransmission()) return false;
  
  /* Init LCD */
  delay(100);
  ssd1306_WriteCommand(DISPLAYOFF); //display off  
  ssd1306_WriteCommand(SSD1306_XSHIFT&0xF);
  ssd1306_WriteCommand(0x10);
  ssd1306_WriteCommand(PAGESTARTADDRESS); //Set Page Start Address for Page Addressing Mode,0-7
  ssd1306_WriteCommand(COMSCANDEC); //Set COM Output Scan Direction
  ssd1306_WriteCommand(SETSTARTLINE | 0x00); //--set start line address
  ssd1306_WriteCommand(SEGREMAP); //--set segment re-map 0 to 127
  ssd1306_WriteCommand(NORMALDISPLAY); //--set normal display
  ssd1306_WriteCommand(SETMULTIPLEX); //--set multiplex ratio(1 to 64)
  ssd1306_WriteCommand(0x3F); //
  ssd1306_WriteCommand(0xA4); //0xa4,Output follows RAM content;0xa5,Output ignores RAM content
  ssd1306_WriteCommand(SETDISPLAYOFFSET); //-set display offset
  ssd1306_WriteCommand(0x00); //-not offset
  ssd1306_WriteCommand(SETDISPLAYCLOCKDIV); //--set display clock divide ratio/oscillator frequency
  ssd1306_WriteCommand(0xF0); //--set divide ratio
  ssd1306_WriteCommand(SETPRECHARGE); //--set pre-charge period
  ssd1306_WriteCommand(0x22); //
  ssd1306_WriteCommand(SETCOMPINS); //--set com pins hardware configuration
  ssd1306_WriteCommand(0x12);
  ssd1306_WriteCommand(SETVCOMDETECT); //--set vcomh
  ssd1306_WriteCommand(0x20); //0x20,0.77xVcc
  ssd1306_WriteCommand(SETCONTRAST); //--set contrast control register
  ssd1306_WriteCommand((contrast) ? contrast : 64);
  ssd1306_WriteCommand(0x30|0); //--set pump voltage
  ssd1306_WriteCommand(DCDCCTRLMODESET);
  ssd1306_WriteCommand(DCDCSETON); //--set DC-DC enable
  // Clear screen
  ssd1306_Fill(OLED_BLACK);
  ssd1306_UpdateScreen();
  ssd1306_WriteCommand(DISPLAYON); //--turn on SSD1306 panel
  return true;
}

void ssd1306_PowerDown(void)
{
  ssd1306_WriteCommand(DISPLAYOFF);
}

void ssd1306_DrawPixel(uint8_t x, uint8_t y, SSD1306_COLOR color)
{
  if (x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) {
    return;
  }
  if (color == OLED_WHITE){
    SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] |= 1 << (y % 8);
  } else{
    SSD1306_Buffer[x + (y / 8) * SSD1306_WIDTH] &= ~(1 << (y % 8));
  }
}
