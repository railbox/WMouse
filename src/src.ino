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
#include <SD.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <EEPROM.h>

#define ASYNC_UDP
#ifdef ASYNC_UDP
#include <ESPAsyncUDP.h>
#else
#include <WiFiUDP.h>
#endif 

#include "Ticker.h"
#include "callback.h"
#include "config.h"
#include "log.h"
#include "eeprom_hl.h"
#include "ssd1306.h"
#include "lcd_hl.h"
#include "main_page.h"
#include "buttons.h"
#include "page.h"
#include "Z21client.h"
#include "update_server.h"

//client config
#ifdef RAILBOX_WIFI
static const IPAddress defaultIpZ21(192, 168, 1, 34); //default Subnet
static const IPAddress defaultIpCL(192, 168, 1, 10); //default IP
static const IPAddress defaultGwCL(192, 168, 1, 1); //default Gateway
#else
static const IPAddress defaultIpZ21(192, 168, 4, 111); //default Subnet
static const IPAddress defaultIpCL(192, 168, 4, 10); //default IP
static const IPAddress defaultGwCL(192, 168, 4, 1); //default Gateway
#endif
static const IPAddress defaultMaskCL(255, 255, 255, 0); //default Subnet

#ifdef ASYNC_UDP
static AsyncUDP Z21UDPClient;
#else
static WiFiUDP Z21UDPClient;
#endif
ESP8266WebServer updateServer(80);
static bool server_enabled = false;
static void fw_notification_handler(fw_update_status_t status);
ESP8266HTTPUpdateServer httpUpdater(false, fw_notification_handler);

extern "C" void WiFi_ResetToDefaults(void);

static byte connectionTries = 0;
static callback_handler_t wifi_timer, key_timeout_timer, powerdown_timer, status_timer, bat_timer, idle_timer;

/**********************************************************************************/
void DebugPrint(char *data) {
  Serial.print(data);
}

static void key_timeout_handler(void *arg)
{
  page_event_timeout();
}

static void powerdown_handler(void *arg)
{
  LOG_INFO("config_db saving to memory\n\r");
  EEPROMwrite(EE_CONFIG_DB, (uint8_t*)&config_db, sizeof(config_db));
  EEPROM.commit();
  digitalWrite(BUILTIN_LED_PIN, HIGH);
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  ssd1306_PowerDown();
  ESP.deepSleep(0);
}

static void buttons_event(uint8_t id, bool state)
{
  if (config_db.idle_time_min) 
    callback_timer_start(idle_timer, config_db.idle_time_min*60000, false, powerdown_handler, 0);
  
  if (id < 10) {
    if (state) callback_timer_start(key_timeout_timer, 1000, false, key_timeout_handler, 0);
    page_event_key(id, state);
  } else {
    switch (id) {
    case STOP_BUTTON_ID:
      page_event_back(state);
      break;
    case OK_BUTTON_ID:
      if (state) callback_timer_start(powerdown_timer, 2000, false, powerdown_handler, 0);
      else callback_timer_stop(powerdown_timer);
      page_event_enter(state);
      break;
    case MODE_BUTTON_ID:
      page_event_mode(state);
      break;
    case NEXT_BUTTON_ID:
      page_event_next(state);
      break;
    case PREV_BUTTON_ID:
      page_event_prev(state);
      break;
    case MENU_BUTTON_ID:
      page_event_menu(state);
      break;
    case SHIFT_BUTTON_ID:
      page_event_shift(state);
      break;
    }
  }
}

static inline void receiveEvent(uint8_t *data, uint8_t len) {
#ifdef DATA_DEBUG
  LOG_INFO("UDP receive: ");
  for (uint8_t i=0; i<len; i++) {
    if (data[i] < 16) LOG_INFO("0");
    LOG_INFO(data[i], HEX);
  }
  LOG_INFO("\n\r");
#endif
  z21Client_parseReceived(data, len);
}

#ifdef ASYNC_UDP
static void onPacket(AsyncUDPPacket &packet)
{
  uint8_t remoteIp[4];
  memcpy(remoteIp, packet.remoteIP(), 4);
  
  if (!memcmp(remoteIp, config_db.ip_z21, 4)) {
    receiveEvent(packet.data(), packet.length());
  }
}
#endif

static void SendDataToZ21(byte *data, byte len)
{
#ifdef DATA_DEBUG
  LOG_INFO("UDP send: ");
  for (uint8_t i=0; i<len; i++) {
    if (data[i] < 16) LOG_INFO("0");
    LOG_INFO(data[i], HEX);
  }
  LOG_INFO("\n\r");
#endif
#ifdef ASYNC_UDP
  Z21UDPClient.writeTo(data, len, config_db.ip_z21, Z21_PORT);
#else
  Z21UDPClient.beginPacket(config_db.ip_z21, Z21_PORT);
  Z21UDPClient.write(data, len);
  Z21UDPClient.endPacket();
#endif
}

#define DIODE_SHIFT   0//250
#define BAT_MAX_VOLT  (1450*2-DIODE_SHIFT)
#define BAT_MIN_VOLT  (1000*2)

#define ADC_REF_VOLT  1000
#define DIV_R1        1000
#define DIR_R2        470
static void bat_handler(void * arg)
{
  uint32_t adc = 0;
  uint16_t voltage;
  uint8_t step;
  
  for (uint8_t i=0; i<4; i++)
    adc += analogRead(BAT_IN);
  adc /=4;
  voltage = (adc * (ADC_REF_VOLT*(DIV_R1+DIR_R2)/DIR_R2)) / 1024;
  if (voltage < BAT_MIN_VOLT) voltage = BAT_MIN_VOLT;
  step = (uint32_t)LCD_BAT_MAX_VAL * (voltage - BAT_MIN_VOLT) / (BAT_MAX_VOLT-BAT_MIN_VOLT);
  lcd_set_battery(step, true);
#if 0  
  String msg = "ADC=";
  msg += adc;
  msg += " VBAT=";
  msg += voltage;
  Serial.println(msg);
#endif
}

static void config_update_callback(config_flags_t flags)
{
  LOG_INFO("config_db saving to memory\n\r");
  EEPROMwrite(EE_CONFIG_DB, (uint8_t*)&config_db, sizeof(config_db));
  EEPROM.commit();
  if (flags & DB_WIFI) {
    WiFi_ClientConfigUpdated();
  }
  if (flags & DB_CONFIG) {
    if (config_db.webpage_en && !server_enabled) {
      main_start_server();
    }
    ssd1306_SetContrast(config_db.contrast);
  }
}

static void status_handler(void * arg) 
{
  if (WiFi.status() == WL_CONNECTED) {
    z21Client_requestStatus();
  }
}

static void fw_notification_handler(fw_update_status_t status)
{
  if (status == FW_UPDATE_START) {
    callback_timer_stop(wifi_timer);
    switch (current_page) {
    case PAGE_LOCO:
        loco_exit();
        break;
    case PAGE_TURNOUT:
        turnout_exit();
        break;
    default:
        break;
    }
    lcd_main_print("FW UPD", 0, ALIGN_NONE);
    lcd_bottom_print("Downloading...", ALIGN_CENTER);
  } else if (status == FW_UPDATE_FINISH) {
    lcd_bottom_print("Rebooting...", ALIGN_CENTER);
  } else if (status == FW_UPDATE_RESTART) {
    WiFi.forceSleepBegin();
    lcd_bottom_print("Flashing...", ALIGN_CENTER);
    delay(10);
    ESP.restart();
  }
}

static void powerup_handler(void * arg) 
{
  lcd_begin();
  lcd_clear();
  bat_handler(NULL);
  if (WiFi.status() != WL_CONNECTED) lcd_set_signal(0, true);
  page_start(PAGE_LOCO);
  buttons_init(buttons_event);
}

void setup() {
  bool memory_fault = false;
  static_assert(sizeof(config_db) <= EE_SIZE, "config DB size is bigger the EEPROM page");
  
  key_timeout_timer = callback_timer_create();
  powerdown_timer = callback_timer_create();
  idle_timer = callback_timer_create();
  bat_timer = callback_timer_create();
  
  EEPROM.begin(EE_SIZE);  //init EEPROM
  main_set_config_update_callback(config_update_callback);
  EEPROMread(EE_CONFIG_DB, (uint8_t*)&config_db, sizeof(config_db));
#ifdef DEBUG_PRINT
  Serial.begin(115200, SERIAL_8N1);
  String msg = "sizeof(config_db)=";
  msg += sizeof(config_db);
  Serial.println(msg);
#endif
  if (config_db.magic != MAGIC_VALUE) {
    config_db.magic = MAGIC_VALUE;
    main_reset_data(0xFF);
    EEPROM.commit();
    memset(&config_db, 0, sizeof(config_db));
    EEPROMread(EE_CONFIG_DB, (uint8_t*)&config_db, sizeof(config_db));
    if (config_db.magic != MAGIC_VALUE) 
      memory_fault = true;
  }
  FlashMode_t ideMode = ESP.getFlashChipMode();
  if ((ideMode != FM_DIO) && (ideMode != FM_DOUT)) {
    memory_fault = true;
  }

  lcd_init(config_db.contrast);
  main_page_init();
  if (!memory_fault) {
    lcd_show_logo("ver " STR(FW_MAJOR) "." STR(FW_MINOR));
    callback_timer_start(powerdown_timer, 3000, false, powerup_handler, 0);
    callback_timer_start(bat_timer, 10000, true, bat_handler, 0);
  } else {
    lcd_show_logo("MEMORY FAULT");
    while(1);
  }

  if (config_db.idle_time_min) 
    callback_timer_start(idle_timer, config_db.idle_time_min*60000, false, powerdown_handler, 0);

  WiFi_Init();
  if (config_db.webpage_en) {
    main_start_server();
  }
#ifdef ASYNC_UDP
  Z21UDPClient.listen(Z21_PORT);
  Z21UDPClient.onPacket(onPacket);
#else
  Z21UDPClient.begin(Z21_PORT);
#endif
  z21Client_setSendDataCallback(SendDataToZ21);
  status_timer = callback_timer_create();
  callback_timer_start(status_timer, 2000, true, status_handler, 0); 
  LOG_INFO("Initialization done\n\r");
}

#ifdef DEBUG_PRINT
static void parseChar(char c)
{
  static bool shift = false;
  
  if (config_db.idle_time_min) 
    callback_timer_start(idle_timer, config_db.idle_time_min*60000, false, powerdown_handler, 0);
  
  switch (c) {
  case '.':
      page_event_next(true);
      page_event_next(false);
      break;
  case ',':
      page_event_prev(true);
      page_event_prev(false);
      break;
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
  default:
      break;
  }
}
#endif

void loop() {
#ifndef ASYNC_UDP
  /* Z21 UDP receive data parsing */
  byte packetSize = Z21UDPClient.parsePacket();
  if (packetSize) {
    /* Send and save client Identity */
    IPAddress remoteIp = Z21UDPClient.remoteIP();
    if (remoteIp == config_db.ip_z21) {
      uint8_t packetBuffer[packetSize];
      byte len = Z21UDPClient.read(packetBuffer, packetSize);
      if (len > 0) receiveEvent(packetBuffer, len);
    }
  }
#endif
#ifdef DEBUG_PRINT
  while (Serial.available()) parseChar(Serial.read());
#endif
  if (config_db.webpage_en) updateServer.handleClient();
  delay(1);
}

/**********************************************************************************/
static const char webpage_begin[] PROGMEM =
  "<html><body><title>RailBOX WMouse config</title><h2>RailBOX WiMouse config</h2>"
  "<form method='POST' action='' enctype='multipart/form-data'>"
  "<textarea rows='8' cols='100' name='config' maxlength='200'>";
static const char webpage_end[] PROGMEM =
  "</textarea><br>"
  "<input type='submit' value='Update'>"
  "</form>"
  "</body></html>";
ESP8266WebServer *web_server;
 
static void main_webpage_get(void)
{
  char buf[1024];
  int len = print_config_db(buf, sizeof(buf));
  web_server->setContentLength(sizeof(webpage_begin)-1 + sizeof(webpage_end)-1 + len);
  web_server->send_P(200,"text/html", webpage_begin);
  web_server->sendContent(buf);
  web_server->sendContent_P(webpage_end);
}

static void main_webpage_post(void)
{
  int8_t arg_id = -1;
  for (uint8_t i=0; i<web_server->args(); i++){
    if (!strcmp(web_server->argName(i).c_str(), "config")) {
      arg_id = i;
      break;
    }
  }
  if (arg_id != -1) {
    Serial.println("Parsing config argument");
    char *buf = (char*)(web_server->arg(arg_id).c_str());
    uint16_t len = web_server->arg(arg_id).length();
    if (parse_config_db(buf, len)) {
      Serial.println("Parsed successfully. Saving to memory");
      EEPROMwrite(EE_CONFIG_DB, (uint8_t*)&config_db, sizeof(config_db));
      EEPROM.commit();
    } else {
      Serial.println("Parsing error");
    }
    main_webpage_get();
  } else {
    Serial.println("Argument not found in POST request");
    web_server->send(200, "text/html", "Argument not found in POST request");
  }
}
  
void main_webpage_setup(ESP8266WebServer *server)
{
    web_server = server;
    web_server->on("/", HTTP_GET, main_webpage_get);
    web_server->on("/", HTTP_POST, main_webpage_post);
}

void main_start_server(void)
{
  server_enabled = true;
  httpUpdater.setup(&updateServer);
  main_webpage_setup(&updateServer);
  updateServer.begin();
}

/**********************************************************************************/
void WiFi_ResetToDefaults(void) {
  memcpy(config_db.ssid, CL_SSID, sizeof(CL_SSID));
  memcpy(config_db.pass, CL_PASS, sizeof(CL_PASS));
  config_db.dhcp = true;
  memcpy(config_db.ipaddr, defaultIpCL, 4);
  memcpy(config_db.gwaddr, defaultGwCL, 4);
  memcpy(config_db.maskaddr, defaultMaskCL, 4);
  memcpy(config_db.ip_z21, defaultIpZ21, 4);
  LOG_INFO("config_db saving to memory");
  EEPROMwrite(EE_CONFIG_DB, (uint8_t*)&config_db, sizeof(config_db));
  EEPROM.commit();
}

static void wifi_handler(void * arg) {
  static int status;
  static uint8_t counter;
  if (WiFi.status() != WL_CONNECTED) {
      if (status == WL_CONNECTED) {
          lcd_set_signal(0, true);
          main_show_error(&err_conn_fault);
          LOG_INFO("Wifi connection lost\n\r");
      }
      if ( (strlen(config_db.ssid) > 0) && (connectionTries < 5) ) {
        connectionTries++;
        WiFi.begin(config_db.ssid, config_db.pass);
      }
  } else {
      int8_t rssi = 0;
      if ((status != WL_CONNECTED) || (counter > 20)) counter = 0;
      
      if (!counter) {
        rssi = WiFi.RSSI();
        lcd_set_signal(WIFI_getQuality(rssi), true);
      }
      counter++;
      
      if (status != WL_CONNECTED) {
        LOG_INFO("Connected to Wifi with IP ");
        LOG_INFO(WiFi.localIP());
        LOG_INFO(" RSSI: ");
        LOG_INFO(rssi);
        LOG_INFO("\n\r");
        memcpy(config_db.ipaddr, WiFi.localIP(), 4);
        memcpy(config_db.maskaddr, WiFi.subnetMask(), 4);
        memcpy(config_db.gwaddr, WiFi.gatewayIP(), 4);
        main_exit_error();
      }
  }
  status = WiFi.status();
}

void WiFi_Init(void) {
  //--------------WIFI CLIENT---------------
  if (config_db.dhcp > 1) WiFi_ResetToDefaults();
  
  WiFi.mode(WIFI_STA);  //client
  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);
  WiFi.persistent(false);

  if (strlen(config_db.ssid) > 0) {
    if (config_db.dhcp == false)
      WiFi.config(config_db.ipaddr,config_db.gwaddr,config_db.maskaddr,config_db.gwaddr,IPAddress(8,8,8,8));
    else {
      memset(config_db.ipaddr, 0, sizeof(config_db.ipaddr));
    }
    WiFi.begin(config_db.ssid, config_db.pass);
    LOG_INFO("Connecting to WiFi...\n\r");
  }
  wifi_timer = callback_timer_create();
  callback_timer_start(wifi_timer, 100, true, wifi_handler, 0); 
}

void WiFi_ClientConfigUpdated(void) {
  lcd_set_signal(0, true);
  LOG_INFO("WiFi client config updated\n\r");
  WiFi.disconnect();
  if (config_db.dhcp == false)
    WiFi.config(config_db.ipaddr,config_db.gwaddr,config_db.maskaddr,config_db.gwaddr,IPAddress(8,8,8,8));
  else WiFi.config(0U,0U,0U,0U,0U);        
  WiFi.begin(config_db.ssid, config_db.pass);
}

#define LOW_RSSI  (-100)
#define HIGH_RSSI (-70)
uint8_t WIFI_getQuality(int8_t rssi)
{
  if (rssi <= LOW_RSSI) return 0;
  if (rssi >= HIGH_RSSI) return LCD_SIG_MAX_VAL;

  return (rssi - LOW_RSSI) * LCD_SIG_MAX_VAL / (HIGH_RSSI - LOW_RSSI);
}

//Callback implementation //////////////////////////////////////////////////////////////////////////////////////////////
callback_handler_t callback_timer_create(void)
{
  Ticker * ticker = new Ticker;
  return (callback_handler_t)ticker;
}

void callback_timer_start(callback_handler_t handler, uint32_t ms, bool repeat, callback_funcion_t callback, void * arg)
{
  if (!handler) return;
  Ticker * ticker = ((Ticker*)handler);
  if (repeat) ticker->attach_ms(ms, callback, arg);
  else ticker->once_ms(ms, callback, arg);
}

void callback_timer_stop(callback_handler_t handler)
{
  if (!handler) return;
  Ticker * ticker = ((Ticker*)handler);
  if (handler) ticker->detach();
}

void callback_timer_delete(callback_handler_t handler)
{
  if (!handler) return;
  Ticker * ticker = ((Ticker*)handler);
  delete ticker;
}
