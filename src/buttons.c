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

#include "Arduino.h"
#include "buttons.h"
#include "config.h" //for DEBUG_PRINT define
#include "callback.h"

#define CALLBACK_PERIOD   10
#define DEBOUNCE_TIMEOUT  (10/CALLBACK_PERIOD)

typedef struct
{
  uint8_t pin;
}output_t;

typedef struct
{
  uint8_t pin;
  bool pullup;
}input_t;

static const output_t outpins[BT_OUTPINS_NUM] = BT_OUTPINS_CONF;
static const input_t inpins[BT_INPINS_NUM] = BT_INPINS_CONF;
static const uint8_t convTable[BT_INPINS_NUM*3] = 
{
  STOP_BUTTON_ID, OK_BUTTON_ID,   1, 4, 7, SHIFT_BUTTON_ID,
  NULL_BUTTON_ID, MODE_BUTTON_ID, 2, 5, 8, 0,
  NEXT_BUTTON_ID, PREV_BUTTON_ID, 3, 6, 9, MENU_BUTTON_ID
};

static bool firstRun;
static buttons_callback_t callbackPtr;
static uint8_t curOutPos;
static bool lastStates[MAX_BUTTON_ID+1];
static uint8_t debounceTimeout[MAX_BUTTON_ID+1];
static callback_handler_t callback_handler;

static inline void set_inputs_mode(bool pullup)
{
   for (uint8_t i = 0; i<BT_INPINS_NUM; i++) {
      if (inpins[i].pullup) pinMode(inpins[i].pin, (pullup) ? INPUT_PULLUP : INPUT);
      else if (inpins[i].pin == 16) pinMode(inpins[i].pin, (pullup) ? INPUT : INPUT_PULLDOWN_16);
      else pinMode(inpins[i].pin, INPUT);
    }
}

static void buttons_handler(void * arg)
{	
#ifdef ROW1_DISCHARGE
  pinMode(ROW1_PIN, INPUT_PULLUP);
#endif
  for (uint8_t step=0; step<2; step++) {
    for (uint8_t out=0; out<BT_OUTPINS_NUM; out++) {
      if (out == curOutPos) {
        digitalWrite(outpins[out].pin, (step == 0) ? LOW : HIGH);
        pinMode(outpins[out].pin, OUTPUT);
      } else pinMode(outpins[out].pin, INPUT);
    }
    set_inputs_mode(step == 0);
    delayMicroseconds(5);
    for (uint8_t i = 0; i<BT_INPINS_NUM; i++) {
      if (inpins[i].pullup ^ (step == 0)) continue;
      bool curState = (digitalRead(inpins[i].pin) == HIGH) ^ inpins[i].pullup;
      uint8_t curNum = convTable[curOutPos*BT_INPINS_NUM + i];
      if (curState && (debounceTimeout[curNum] < DEBOUNCE_TIMEOUT)) debounceTimeout[curNum]++;
      else if (!curState && (debounceTimeout[curNum] > 0)) debounceTimeout[curNum]--;
      if (debounceTimeout[curNum] == 0) curState = false;
      else if (debounceTimeout[curNum] == DEBOUNCE_TIMEOUT) curState = true;
      else continue; //Transition state
      
      if (!firstRun)
      {
        if (curState && !lastStates[curNum])
        {
          lastStates[curNum] = curState;
          if (callbackPtr) callbackPtr(curNum, true);
        }
        else if (!curState && lastStates[curNum])
        {
          lastStates[curNum] = curState;
          if (callbackPtr) callbackPtr(curNum, false);
        }
      } else lastStates[curNum] = curState;
    }
  }
  for (uint8_t out=0; out<BT_OUTPINS_NUM; out++) {
    digitalWrite(outpins[out].pin, LOW);
  }
#ifdef ROW1_DISCHARGE
  digitalWrite(ROW1_PIN, LOW);
  pinMode(ROW1_PIN, OUTPUT);
#endif
  if (curOutPos >= BT_OUTPINS_NUM-1) {
    firstRun = false;
    curOutPos = 0;
  } else curOutPos++;
}

void buttons_stop(void)
{
  callback_timer_stop(callback_handler);
  for (uint8_t i = 0; i<BT_OUTPINS_NUM; i++) {
	  pinMode(outpins[i].pin, INPUT);
  }
}

bool buttons_getstate(uint8_t id)
{
  if (id > MAX_BUTTON_ID+1)
    return false;
  
  return lastStates[id];
}

void buttons_init(buttons_callback_t callback)
{
  callbackPtr = callback;
  curOutPos = 0;
  firstRun = true;
  
  callback_handler = callback_timer_create();
  callback_timer_start(callback_handler, CALLBACK_PERIOD, true, buttons_handler, 0);
}
