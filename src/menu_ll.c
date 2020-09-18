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
#include "menu_ll.h"
#include <string.h>
#include <stdio.h>      /* snprintf */
#include <stdlib.h>     /* atoi */
#include "lcd_hl.h"
#include "log.h"

#define INC(x,low,high)    (((x)==high)?(low):((x)+1))
#define DEC(x,low,high)    (((x)==low)?(high):((x)-1))

typedef struct {
    uint8_t call_page;
    uint8_t pos;
    const mitem_t *current;
    uint8_t level;
    const mitem_t *parent[MAX_MENU_LEVEL];
    uint8_t parent_pos[MAX_MENU_LEVEL];
    uint16_t parent_flags;
    uint8_t child_level;
    uint16_t child_password;
    const seqitem_t *child_edit_item;
} menu_t;

typedef struct {
    page_t call_page;
    uint8_t pos;
    uint32_t flags;
    const seqitem_t *current;
} sequence_t;

typedef struct {
    page_t call_page;
    uint8_t pos;
     union {
        uint8_t max_len;
        uint8_t list_len;
    };
    uint8_t char_id;
    const char ** list;
    union {
      bool choosing;
      bool entering;
    };
    bool shift;
    int8_t ip_pos;
    edit_type_t type;
    uint8_t prefix_len;
    char show_str[STRING_LEN+1];
} value_t;

/* Keys definition declaration //////////////////// */
#define KEYS_0  "0 "
#define KEYS_1  "1_-/\\*[]<>"
#define KEYS_2  "2abc"
#define KEYS_3  "3def"
#define KEYS_4  "4ghi"
#define KEYS_5  "5jkl"
#define KEYS_6  "6mno"
#define KEYS_7  "7pqrs"
#define KEYS_8  "8tuv"
#define KEYS_9  "9wxyz"
typedef struct {
    const char * key;
    const uint8_t len;
} keys_t;
static const keys_t keys[] = {
    {KEYS_0, sizeof(KEYS_0)-1},
    {KEYS_1, sizeof(KEYS_1)-1},
    {KEYS_2, sizeof(KEYS_2)-1},
    {KEYS_3, sizeof(KEYS_3)-1},
    {KEYS_4, sizeof(KEYS_4)-1},
    {KEYS_5, sizeof(KEYS_5)-1},
    {KEYS_6, sizeof(KEYS_6)-1},
    {KEYS_7, sizeof(KEYS_7)-1},
    {KEYS_8, sizeof(KEYS_8)-1},
    {KEYS_9, sizeof(KEYS_9)-1},
};

/* VARIABLE DEFINITIONS */
static uint8_t lang_id;
static menu_t menu;
static value_t val_edit;
static sequence_t sequence;

extern mitem_t loco_list[];
/* Extern functions */
extern bool DeleteLoco(uint8_t id);

static void incStr(char *str, uint8_t max_len);
static void decStr(char *str, uint8_t max_len);
/* MENU Browsing functions ////////////////////////////////////////////////// */
static inline void menu_show_main(void)
{
    const char * str;
    const mitem_t * item;

    //main print
    item = &menu.current[menu.pos];
    if (item->name) {
        str = item->name[(menu.current == loco_list) ? 0 : lang_id];
        lcd_main_print(str, 0, ALIGN_CENTER);
    }else LOG_ERR("Name list for main is not provided");
}


static inline void menu_show_bottom(void)
{
    const char * str;
    const mitem_t * item;

    if (!menu.level) {
        lcd_bottom_print(0, ALIGN_NONE);
        return;
    }
    item = &menu.parent[menu.level-1][menu.parent_pos[menu.level-1]];
    if (item->name) {
        str = item->name[lang_id];
        lcd_bottom_print(str, ALIGN_CENTER);
    }else LOG_ERR("Name list for parent is not provided");
}


static void menu_show(void)
{
    menu_show_main();
    menu_show_bottom();
}

void menu_next(void)
{
    uint8_t max_len = menu.current[0].len;
    menu.pos = (menu.pos < (max_len-1)) ? (menu.pos+1) : (0);
    menu_show();
}

void menu_prev(void)
{
    uint8_t max_len = menu.current[0].len;
    menu.pos = (menu.pos > 0) ? (menu.pos-1) : (max_len-1);
    menu_show();
}

static void menu_enter_local(void)
{
    const mitem_t * item = &menu.current[menu.pos];
    if (menu.current == loco_list) {
        if ((menu.parent_flags & FLAGS_ACTION_MASK) == FLAGS_DELETE_LOCO) {
            if (!DeleteLoco(menu.pos)) return;
            page_return_back(PAGE_MENU, NULL);
            return;
        }
        if ((menu.parent_flags & FLAGS_ACTION_MASK) == FLAGS_EDIT_LOCO) {
            menu.parent_flags &=~ FLAGS_LOCO_ID_MASK;
            menu.parent_flags |= menu.pos<<8;
        }
    }
    if ((item->submenu && item->submenu->len)
         || item->subseq) {
        menu.parent[menu.level] = menu.current;
        menu.parent_pos[menu.level] = menu.pos;
        menu.level++;
        menu.parent_flags |= item->flags;
        if (item->submenu) {
            menu.current = item->submenu;
            menu.pos = 0;
            menu_show();
        } else {
            //TODO: menu_show_bottom();
            seq_start(PAGE_MENU, menu.parent_flags, item->subseq);
        }
    }
}

uint8_t * menu_get_levels(uint8_t *curr_pos, uint8_t *levels)
{
    if (!levels || !curr_pos) return 0;
    *levels = menu.level;
    *curr_pos = menu.pos;
    return menu.parent_pos;
}

void password_entered(uint16_t password)
{
    current_page = PAGE_MENU;
    if (password == menu.child_password) {
        menu_enter_local();
    } else menu_show();
}

void menu_enter(void)
{
    bool ret = false;
    const mitem_t * item = &menu.current[menu.pos];

    if (item->callback) ret = item->callback(0, item->flags, false);
    if (ret || item->back) {
        page_return_back(PAGE_MENU, NULL);
        return;
    }
    if ((item->child_level > menu.child_level) && menu.child_edit_item) {
        value_edit(PAGE_PASSWORD, menu.child_edit_item->type, menu.child_edit_item->prefix[lang_id],
                   menu.child_edit_item->init_val, menu.child_edit_item->max_len);
        lcd_bottom_print(menu.child_edit_item->name[lang_id], ALIGN_CENTER);
        return;
    }
    menu_enter_local();
}

void menu_start(page_t call_page, const mitem_t *base)
{
    menu.call_page = call_page;
    current_page = PAGE_MENU;
    menu.level = 0;
    menu.parent_flags = 0;
    menu.current = base;
    menu.pos = 0;
    menu_show();
}

void menu_back(void)
{
    uint8_t loops = 1;
    if (menu.current[menu.pos].skip_on_return) {
        loops++;
    }
    for (uint8_t loop=0; loop < loops; loop++) {
        if (menu.level) {
            menu.level--;
            menu.current = menu.parent[menu.level];
            menu.pos = menu.parent_pos[menu.level];
            menu.parent_flags &=~ menu.current[menu.pos].flags;
            if (loop == loops-1) menu_show();
        } else page_return_back(menu.call_page, NULL);
    }
}

void menu_exit(void)
{
    menu.level = 0;
    menu.parent_flags = 0;
    menu.pos = 0;
    menu.current = NULL;
    lcd_bottom_print(NULL, ALIGN_NONE);
    page_return_back(menu.call_page, NULL);
}

void menu_set_language(uint8_t id)
{
    lang_id = id;
}

void menu_set_childlock(uint8_t level, uint16_t pin, const seqitem_t * password_item)
{
    menu.child_level = (level < MAX_CHILD_LEVEL) ? MAX_CHILD_LEVEL - level : 0;
    menu.child_password = pin;
    menu.child_edit_item = password_item;
}

/* Edit sequence functions ////////////////////////////////////////////////// */
void seq_begin_item(void)
{
    char buf[STRING_LEN];
    const char * prefix;
    bool ret = false;
    const seqitem_t *item;
    item = &sequence.current[sequence.pos];
    prefix = (item->prefix) ? item->prefix[lang_id] : "";
    if (item->name) {
        lcd_bottom_print(item->name[lang_id], ALIGN_CENTER);
    }

    if (item->callback) {
        ret = item->callback(buf, sequence.flags, true);
    }
    if (item->type == EDIT_CHOOSE) {
        if (!ret) value_edit_choose(PAGE_SEQUENCE, prefix, item->init_pos, item->list, item->list_len);
        else value_edit_choose(PAGE_SEQUENCE, prefix, buf[0], item->list, item->list_len);
    } else {
        if (!ret && item->init_val) strncpy(buf, item->init_val, sizeof(buf));
        value_edit(PAGE_SEQUENCE, item->type, prefix, buf, item->max_len);
    }
}

void seq_end_item(void * param)
{
    if (!param) {
        page_return_back(sequence.call_page, NULL);
        return;
    }
    if (sequence.current[sequence.pos].callback) {
        if (!sequence.current[sequence.pos].callback(param, sequence.flags, false))
            return;
    }
    sequence.pos++;
    if (sequence.pos == sequence.current[0].len)
        page_return_back(sequence.call_page, NULL);
    else seq_begin_item();
}

void seq_start(page_t call_page, uint32_t flags, const struct seqitem_s * seq)
{
    sequence.call_page = call_page;
    current_page = PAGE_SEQUENCE;
    sequence.pos = 0;
    sequence.flags = flags;
    sequence.current = seq;
    seq_begin_item();
}

void seq_move_to(uint8_t seq_pos)
{
    sequence.pos = seq_pos;
    seq_begin_item();
}

/* MENU value editing functions ///////////////////////////////////////////// */
static void value_show(bool align_right)
{
    uint8_t len = strlen(val_edit.show_str);
    if (!align_right || (len > SHOW_LEN)) {
        uint8_t pos = val_edit.pos+val_edit.prefix_len+1;
        if (pos <= SHOW_LEN) {
            lcd_main_print(val_edit.show_str, pos, ALIGN_NONE);
        } else {
            lcd_main_print(val_edit.show_str + pos - SHOW_LEN, SHOW_LEN, ALIGN_NONE);
        }
    } else {
        char buf[SHOW_LEN+1];
        uint8_t pos;
        memcpy(buf, val_edit.show_str, val_edit.prefix_len);
        pos = SHOW_LEN - len + 1;
        memset(buf + val_edit.prefix_len, ' ', SHOW_LEN - len);
        strncpy(buf + val_edit.prefix_len + pos - 1, val_edit.show_str+val_edit.prefix_len, sizeof(buf) - pos+1);
        lcd_main_print(buf, 0, ALIGN_NONE);
    }
    lcd_set_shift(val_edit.shift, false);
}

void value_edit(page_t call_page, edit_type_t type, const char *prefix, void *init, uint8_t max_len)
{
    uint8_t len;

    if (max_len>STRING_LEN) return;

    val_edit.call_page = call_page;
    current_page = PAGE_EDIT;
    val_edit.max_len = max_len;
    val_edit.choosing = false;
    val_edit.ip_pos = -1;
    val_edit.type = type;
    val_edit.shift = false;
    memset(val_edit.show_str, 0, sizeof(val_edit.show_str));
    val_edit.prefix_len = strlen(prefix);
    if (val_edit.prefix_len <= STRING_LEN - max_len) {
        strcpy(val_edit.show_str, prefix);
    }else {
        val_edit.prefix_len = 0;
        LOG_ERR("prefix is too big");
    }

    if (val_edit.type != EDIT_IP) {
        if (init) {
            const char *ptr = init;
            len = strlen(ptr);
            if (len <= max_len) {
                strcpy(val_edit.show_str + val_edit.prefix_len, ptr);
            } else LOG_ERR("init value is too big\n\r");
        }
    } else {
        if (init) {
            uint8_t *ptr = init;
            snprintf(val_edit.show_str + val_edit.prefix_len, STRING_LEN, "%u.%u.%u.%u",
                      ptr[0], ptr[1], ptr[2], ptr[3]);
        } else strcpy(val_edit.show_str + val_edit.prefix_len, "0.0.0.0");
    }
    val_edit.pos = 0;
    value_show(val_edit.type == EDIT_INTEGER);
}

static void value_show_choose(void)
{
    uint8_t len;
    len = strlen(val_edit.list[val_edit.pos]);
    if (len + val_edit.prefix_len <= SHOW_LEN) {
        memset(val_edit.show_str + val_edit.prefix_len, ' ', SHOW_LEN - val_edit.prefix_len);
        strcpy(val_edit.show_str + SHOW_LEN - len, val_edit.list[val_edit.pos]);
    } else {
        if (len + val_edit.prefix_len > STRING_LEN)
            len = STRING_LEN - val_edit.prefix_len;
        strcpy(val_edit.show_str, val_edit.list[val_edit.pos]);
    }
    lcd_main_print(val_edit.show_str, 0, ALIGN_NONE);
    lcd_set_shift(false, false);
}

void value_edit_choose(page_t call_page, const char *prefix, uint8_t pos, const char ** list, uint8_t list_len)
{
    uint8_t len;
    if (!list) return;
    val_edit.call_page = call_page;
    current_page = PAGE_EDIT;
    val_edit.list_len = list_len;
    val_edit.choosing = false;
    val_edit.pos = pos;
    val_edit.list = list;
    len = strlen(prefix);
    val_edit.prefix_len = len;
    if (len <= STRING_LEN)
        strcpy(val_edit.show_str, prefix);
    val_edit.type = EDIT_CHOOSE;

    value_show_choose();
}

void value_edit_next(void)
{
    switch (val_edit.type) {
    case EDIT_TEXT:
        val_edit.pos = INC(val_edit.pos, val_edit.max_len-1, val_edit.max_len-1);
        value_show(false);
        break;
    case EDIT_CHOOSE:
        val_edit.pos = INC(val_edit.pos, 0, val_edit.list_len-1);
        value_show_choose();
        break;
    case EDIT_INTEGER:
        if (!val_edit.entering) {
            char *ptr = val_edit.show_str + val_edit.prefix_len;
            incStr(ptr, val_edit.max_len);
            value_show(true);
        }
        break;
    case EDIT_IP:
        val_edit.ip_pos = -1;
        if (val_edit.show_str[val_edit.prefix_len+val_edit.pos+1] != 0) {
            val_edit.pos = INC(val_edit.pos, val_edit.max_len-1, val_edit.max_len-1);
            if (val_edit.show_str[val_edit.prefix_len+val_edit.pos] == '.')
                val_edit.pos++;
            value_show(false);
        }
        break;
    default:
        break;
    }
}

void value_edit_prev(void)
{
    switch (val_edit.type) {
    case EDIT_TEXT:
        val_edit.pos = DEC(val_edit.pos, 0, 0);
        value_show(false);
        break;
    case EDIT_CHOOSE:
        val_edit.pos = DEC(val_edit.pos, 0, val_edit.max_len-1);
        value_show_choose();
        break;
    case EDIT_INTEGER:
        if (val_edit.entering) {
            val_edit.pos = DEC(val_edit.pos, 0, 0);
            val_edit.show_str[val_edit.prefix_len+val_edit.pos] = '\0';
        } else {
            char *ptr = val_edit.show_str + val_edit.prefix_len;
            decStr(ptr, val_edit.max_len);
        }
        value_show(true);
        break;
    case EDIT_IP:
        val_edit.ip_pos = -1;
        val_edit.pos = DEC(val_edit.pos, 0, 0);
        if (val_edit.show_str[val_edit.prefix_len+val_edit.pos] == '.')
            val_edit.pos--;
        value_show(false);
        break;
    default:
        break;
    }
}

void value_edit_begin(void)
{
    switch (val_edit.type) {
    case EDIT_TEXT:
    case EDIT_IP:
        value_show(false);
        break;
    case EDIT_CHOOSE:
        value_show_choose();
        break;
    case EDIT_INTEGER:
        value_show(true);
        break;
    default:
        break;
    }
}

void value_edit_exit(void)
{
    page_return_back(val_edit.call_page, NULL);
}

void value_edit_enter(void)
{
    char *ptr=NULL;
    switch (val_edit.type) {
    case EDIT_TEXT:
        ptr = val_edit.show_str + val_edit.prefix_len;
        for (uint8_t i=strlen(val_edit.show_str)-1; i>0; i--) {
            if (val_edit.show_str[i] == ' ')
                val_edit.show_str[i] = '\0';
            else break;
        }
        break;
    case EDIT_INTEGER:
        ptr = val_edit.show_str + val_edit.prefix_len;
        break;
    case EDIT_IP: {
        uint8_t ip[4], i=0;
        char *strt=val_edit.show_str;
        char *end = strt;
        while (*end) {
            end++;
            if ((*end == '.') || (*end == '\0')) {
                *end = '\0';
                ip[i++] = atoi(strt);
                strt = end+1;
                end = strt;
            }
        }
        printf("\r%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
        ptr = (char*)&ip;
        break;
    }
    case EDIT_CHOOSE:
        ptr = (char*)&val_edit.pos;
        break;
    default:
        break;
    }
    page_return_back(val_edit.call_page, ptr);
}

void value_edit_timeout(void)
{
    if (val_edit.choosing) {
        val_edit.choosing = false;
        value_edit_next();
    }
}

void value_edit_shift(bool state)
{
  if (!state) return;
  val_edit.shift = !val_edit.shift;
  lcd_set_shift(val_edit.shift, false);
}

void value_edit_set(uint8_t key)
{
    if (key>9) return;
    switch (val_edit.type) {
    case EDIT_INTEGER:
        if (val_edit.pos == 0) memset(val_edit.show_str + val_edit.prefix_len, 0, sizeof(val_edit.show_str) - val_edit.prefix_len);
        val_edit.show_str[val_edit.prefix_len + val_edit.pos] = keys[key].key[0];
        val_edit.pos = INC(val_edit.pos, 0, val_edit.max_len-1);
        value_show(true);
        break;
    case EDIT_IP: {
        char * ptr_l = val_edit.show_str + val_edit.prefix_len + val_edit.pos;
        char * ptr_r = ptr_l;
        uint8_t delta;
        val_edit.entering = true;

        if (val_edit.ip_pos == -1) {
            do {
                ptr_r++;
            } while ((*ptr_r != '.') && (*ptr_r != 0));
            while ((*ptr_l != '.') && (ptr_l >= val_edit.show_str)) {
                ptr_l--;
            }
            ptr_l++;
            *(ptr_l) = keys[key].key[0];
            delta = ptr_r - ptr_l - 1;
            for (uint8_t i=ptr_l+1-val_edit.show_str; i<strlen(val_edit.show_str)-delta+1; i++) {
                val_edit.show_str[i] = val_edit.show_str[i+delta];
            }
            val_edit.pos = ptr_l - val_edit.show_str;
            val_edit.ip_pos = val_edit.pos;
        } else {
            uint8_t len = strlen(val_edit.show_str);
            for (uint8_t i=strlen(val_edit.show_str)-1; i>val_edit.pos; i--) {
                val_edit.show_str[i+1] = val_edit.show_str[i];
            }
            val_edit.show_str[len+1] = '\0';
            val_edit.pos++;
            val_edit.show_str[val_edit.pos] = keys[key].key[0];
            if (val_edit.pos - val_edit.ip_pos == 2) {
                val_edit.ip_pos = -1;
                if (val_edit.show_str[val_edit.pos+1] != 0) val_edit.pos+=2;
            }
        }
        value_show(false);
        break;
    }
    case EDIT_TEXT: {
        char chr;
        if (!val_edit.choosing) {
            val_edit.choosing = true;
            val_edit.char_id = 1;
        } else if (val_edit.char_id < keys[key].len - 1) {
            val_edit.char_id++;
        } else val_edit.char_id = 0;

        chr = keys[key].key[val_edit.char_id];
        if ((chr>='a') && (chr<='z') && val_edit.shift) chr+='A'-'a';
        val_edit.show_str[val_edit.prefix_len + val_edit.pos] = chr;
        value_show(false);
        break;
    }
    default:
        break;
    }
}

static uint16_t loc_pow10(uint8_t x)
{
    uint16_t val = 1;
    for (uint8_t i=0; i<x; i++) {
        val *=10;
    }
    return val;
}

static void incStr(char *str, uint8_t max_len)
{
    uint16_t val = atoi(str);
    val++;
    val %= loc_pow10(max_len);
    sprintf(str,"%u", val);
}

static void decStr(char *str, uint8_t max_len)
{
    int16_t val = atoi(str);
    val--;
    if (val<0) val = 0;
    sprintf(str,"%u", val);
}
