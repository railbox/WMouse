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
#ifndef MENU_LL_H
#define MENU_LL_H
#include <stdint.h>
#include <stdbool.h>

#include "page.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FLAGS_ACTION_MASK   0x00FF
#define FLAGS_NEW_LOCO      0x0002
#define FLAGS_EDIT_LOCO     0x0003
#define FLAGS_DELETE_LOCO   0x0004
#define FLAGS_LOCK_OFF      0x0005
#define FLAGS_LOCK_SET      0x0006
#define FLAGS_LOCK_LIB      0x0007
#define FLAGS_LOCO_ID_MASK  0xFF00

/* PAGE definition and actions  */
#define MAX_MENU_LEVEL  4
#define STRING_LEN      32
#define SHOW_LEN        6
#define LOCO_LIST_LEN   64
#define MAX_CHILD_LEVEL 2

typedef enum {
    EDIT_NONE = 0,
    EDIT_TEXT,
    EDIT_INTEGER,
    EDIT_CHOOSE,
    EDIT_IP,
} edit_type_t;

typedef const char * text_list_t;

typedef struct seqitem_s {
    text_list_t * name;
    text_list_t * prefix;
    uint8_t len;
    edit_type_t type;
    union {
        uint8_t max_len;
        uint8_t list_len;
    };
    union {
        char * init_val;
        uint8_t init_pos;
    };
    text_list_t * list;
    bool (*callback)(void * param, uint32_t flags, bool read);
} seqitem_t;

typedef struct mitem_s {
    const text_list_t * name;
    uint8_t len;
    uint32_t flags;
    uint8_t child_level;
    const struct mitem_s * submenu;
    const struct seqitem_s * subseq;
    bool back;
    bool skip_on_return;
    bool (*callback)(void * param, uint32_t flags, bool read);
} mitem_t;


void menu_start(page_t call_page, const mitem_t *base);
void menu_set_language(uint8_t id);
void menu_set_childlock(uint8_t level, uint16_t pin, const seqitem_t * password_item);
void menu_next(void);
void menu_prev(void);
void menu_enter(void);
void menu_back(void);
void menu_exit(void);
uint8_t * menu_get_levels(uint8_t *curr_pos, uint8_t *levels);
void value_edit_begin(void);
void value_edit_next(void);
void value_edit_prev(void);
void value_edit_enter(void);
void value_edit_exit(void);
void value_edit_timeout(void);
void value_edit_set(uint8_t key);
void value_edit(page_t call_page, edit_type_t type, const char *prefix, void *init, uint8_t max_len);
void value_edit_choose(page_t call_page, const char *prefix, uint8_t pos, const char ** list, uint8_t list_len);
void value_edit_shift(bool state);
void seq_start(page_t call_page, uint32_t flags, const struct seqitem_s * subseq);
void seq_move_to(uint8_t seq_pos);
void seq_begin_item(void);
void seq_end_item(void * param);
void password_entered(uint16_t pin);

#ifdef __cplusplus
}
#endif

#endif // MENU_LL_H
