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
#ifndef MAIN_PAGE_H
#define MAIN_PAGE_H

#include "menu_ll.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char name[STRING_LEN];
    uint16_t addr;
    uint8_t ss;
    uint32_t func;
    int8_t speed;
    bool dir_left;
} loco_t;

typedef struct {
    uint16_t addr;
    uint16_t cv;
    uint16_t val;
} prog_cv_t;

typedef struct {
    char ssid[32];
    char pass[32];
    uint8_t stop_mode;
    uint8_t language_id;
    uint8_t child_level;
    uint16_t child_password;
    uint8_t ipaddr[4];
    uint8_t gwaddr[4];
    uint8_t maskaddr[4];
    bool dhcp;
    uint8_t ip_z21[4];
    uint16_t turnout_id;
    bool turnout_state;
    uint8_t loco_db_len;
    uint8_t loco_db_pos;
    loco_t loco_db[LOCO_LIST_LEN];
    uint8_t idle_time_min;
    bool webpage_en;
    uint8_t contrast;
    uint16_t magic;
}config_db_t;
extern config_db_t config_db;

#define DB_LOCO_DB  1
#define DB_WIFI     2
#define DB_CONFIG   4
typedef uint8_t config_flags_t;

typedef void (*config_update_callback_t)(config_flags_t flags);

//Errors
typedef struct {
    uint8_t id;
    text_list_t * name;
} erritem_t;
extern const erritem_t err_noack;
extern const erritem_t err_lib_empty;
extern const erritem_t err_lib_full;
extern const erritem_t err_exist;
extern const erritem_t err_value_err;
extern const erritem_t err_conn_fault;

extern const mitem_t menu_base[];
void main_page_init(void);
void main_show_error(const erritem_t * err_item);
void main_exit_error(void);
void main_set_config_update_callback(config_update_callback_t callback);
void main_reset_data(config_flags_t flags);

int print_config_db(char * buf, uint16_t len);
bool parse_config_db(char * buf, uint16_t len);
void turnout_begin(void);
void turnout_exit(void);
void turnout_set_id(uint16_t id);
void turnout_set(bool state);
void turnout_reset(bool state);
void turnout_key(uint8_t key);
void turnout_show(void);

void loco_begin(void);
void loco_exit(void);
void loco_shift(bool status);
void loco_enter(void);
void loco_key(uint8_t key);
void loco_next(void);
void loco_prev(void);

void track_stop(void);
void track_shortcircuit(void);

#ifdef __cplusplus
}
#endif

#endif // MAIN_PAGE_H
