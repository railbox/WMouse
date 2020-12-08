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
#include "main_page.h"
#include "page.h"
#include "menu_ll.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h> //for sprintf
#include "lcd_hl.h"
#include "log.h"
#include "z21client.h"
#include "callback.h"

#define INC(x,low,high)    (((x)==high)?(low):((x)+1))
#define DEC(x,low,high)    (((x)==low)?(high):((x)-1))
#define DEFAULT_IDLE_TIME_M 5
#define SET_IP(_arr, _x1, _x2, _x3, _x4) {_arr[0]=_x1;_arr[1]=_x2;_arr[2]=_x3;_arr[3]=_x4;}
#define MAX_TURNOUT_ID      1023
#define PROG_TIMEOUT        5000

static uint8_t U16ToStr(uint16_t val, char *str);

/* Callback functions declarations /////////////////////////////////////////// */
static bool callback_cv(void * param, uint32_t flags, bool read);
static bool callback_cv_value(void * param, uint32_t flags, bool read);
static bool callback_pom_addr(void * param, uint32_t flags, bool read);
static bool callback_pom_cv(void * param, uint32_t flags, bool read);
static bool callback_pom_cv_value(void * param, uint32_t flags, bool read);
static bool callback_loco_name(void * param, uint32_t flags, bool read);
static bool callback_loco_addr(void * param, uint32_t flags, bool read);
static bool callback_loco_ss(void * param, uint32_t flags, bool read);
static bool callback_stopmode(void * param, uint32_t flags, bool read);
static bool callback_ssid(void * param, uint32_t flags, bool read);
static bool callback_pass(void * param, uint32_t flags, bool read);
static bool callback_dhcp(void * param, uint32_t flags, bool read);
static bool callback_ip(void * param, uint32_t flags, bool read);
static bool callback_ipmask(void * param, uint32_t flags, bool read);
static bool callback_gateway(void * param, uint32_t flags, bool read);
static bool callback_ip_z21(void * param, uint32_t flags, bool read);
static bool callback_language(void * param, uint32_t flags, bool read);
static bool callback_reset(void * param, uint32_t flags, bool read);
static bool callback_childlock(void * param, uint32_t flags, bool read);
static bool callback_idle_time(void * param, uint32_t flags, bool read);
static bool callback_webpage_en(void * param, uint32_t flags, bool read);
static bool callback_contrast(void * param, uint32_t flags, bool read);

/* STATIC CONST DECLARATIONS //////////////////////////////////////////////// */
//Text strings
#define NUM_OF_LANGUAGES    2
#define DEFAULT_LANGUAGE    1
static text_list_t ss_language[NUM_OF_LANGUAGES] = {"ENGLISH", "POLSKI"};
#define DECLARE_TEXT(__name, _l1, _l2) static text_list_t __name[NUM_OF_LANGUAGES] = {_l1, _l2}
DECLARE_TEXT(text_loco, "LOCO", "LOKOMOTYWA");
DECLARE_TEXT(text_programing, "PROGRAM", "PROGRAMOWANIE");
DECLARE_TEXT(text_settings, "SETTINGS", "USTAWIENIA");
DECLARE_TEXT(text_userinterface, "USER INTERFACE", "OBSLUGA");
DECLARE_TEXT(text_new, "NEW", "NOWA");
DECLARE_TEXT(text_new_q, "NEW?", "NOWA?");
DECLARE_TEXT(text_edit, "EDIT", "EDYCJA");
DECLARE_TEXT(text_delete, "DELETE", "KASOWANIE");
DECLARE_TEXT(text_cvmenu, "CV MODIFICATION", "ZMIANA CV");
DECLARE_TEXT(text_pom_cvmenu, "PROG ON MAIN", "PROG NA TOROWISKU");
DECLARE_TEXT(text_stopmode, "STOP MODE", "TRYB STOP");
DECLARE_TEXT(text_wifi, "WIFI", "WIFI");
DECLARE_TEXT(text_language, "LANGUAGE", "JEZYK");
DECLARE_TEXT(text_ssid, "SSID", "SSID");
DECLARE_TEXT(text_password, "PASSWORD", "HASLO");
DECLARE_TEXT(text_dhcp, "DHCP", "DHCP");
DECLARE_TEXT(text_ip, "IP", "IP");
DECLARE_TEXT(text_ipmask, "MASK", "MASK");
DECLARE_TEXT(text_gateway, "GATEWAY", "GATEWAY");
DECLARE_TEXT(text_ip_z21, "IP Z21", "IP Z21");
DECLARE_TEXT(text_name, "NAME", "NAZWA");
DECLARE_TEXT(text_reset, "RESET", "RESET");
DECLARE_TEXT(text_childlock, "CHILD LOCK", "BLOKADA");
DECLARE_TEXT(text_idle_time, "POWER DOWN TIME", "CZAS WYLACZANIA");
DECLARE_TEXT(text_webpage_en, "SHOW WEB PAGE", "POKAZ WEB STRONE");
DECLARE_TEXT(text_contrast, "CONTRAST", "KONTRAST");

DECLARE_TEXT(prefix_value, "V", "W");
DECLARE_TEXT(text_value, "VALUE", "WARTOSC");
DECLARE_TEXT(prefix_turnout, "T", "Z");
DECLARE_TEXT(prefix_locoaddr, "L", "L");
DECLARE_TEXT(text_address, "ADDRESS", "ADRES");
DECLARE_TEXT(prefix_cvvalue, "CV", "CV");
DECLARE_TEXT(text_cvvalue, "CV NUMBER", "NUMER CV");
DECLARE_TEXT(prefix_ss, "SS", "SS");
DECLARE_TEXT(text_ss, "SPEED STEPS", "KROKI PREDKOSCI");
DECLARE_TEXT(prefix_password, "P", "H");
DECLARE_TEXT(prefix_min, "M", "M");

//DECLARE_TEXT(text_trackoff, "TRACK OFF", "NAPIECIE WYL.");
//DECLARE_TEXT(text_locostop, "LOCO STOP", "LOCO STOP");
//DECLARE_TEXT(text_english, "ENGLISH", "ANGELSKI");
//DECLARE_TEXT(text_polish, "POLISH", "POLSKI");
//DECLARE_TEXT(text_all, "ALL ", "WSYSTKO");
//DECLARE_TEXT(text_cancel, "CANCEL", "ANULOWANIE");
DECLARE_TEXT(text_off, "OFF ", "OFF ");
DECLARE_TEXT(text_prog, "PROG ", "PROG ");
DECLARE_TEXT(text_libprog, "LIB-PROG", "LIB-PROG");

DECLARE_TEXT(text_noresp, "NO RESPONCE", "BRAK ODPOWIEDZI");
DECLARE_TEXT(text_noack, "NO ACK", "NO ACK");
DECLARE_TEXT(text_lib_empty, "LIB EMPTY", "BIBL. JEST PUSTA");
DECLARE_TEXT(text_lib_full, "LIB FULL", "BIBL. JEST PELNA");
DECLARE_TEXT(text_exist, "LOCO EXIST", "LOCO JUZ ISTNIEJE");
DECLARE_TEXT(text_value_err, "WRONG VALUE", "BLEDNA WARTOSC");
DECLARE_TEXT(text_conn_fault, "CONNECTION ERR", "BLAD POLACZENIA");

static const uint8_t ss_val[3] = {14,28,128};
static text_list_t ss_list[3] = {"14", "28", "128"};
#define SEQ_LOCO_NAME_POS	0
#define SEQ_LOCO_ADDR_POS	1
#define SEQ_LOCO_SS_POS		2
static const seqitem_t seq_loco[] = {
    {
        .len = 3,
        .name = text_name,
        .init_val = "",
        .max_len = 6,
        .type = EDIT_TEXT,
        .callback = callback_loco_name,
    },
    {
        .name = text_address,
        .prefix = prefix_locoaddr,
        .init_val = "3",
        .max_len = 4,
        .type = EDIT_INTEGER,
        .callback = callback_loco_addr,
    },
    {
        .name = text_ss,
        .prefix = prefix_ss,
        .init_pos = 2,
        .list_len = sizeof(ss_list)/sizeof(ss_list[0]),
        .list = ss_list,
        .type = EDIT_CHOOSE,
        .callback = callback_loco_ss,
    },
};

static const seqitem_t seq_cv[] = {
  {
      .len = 2,
      .name = text_cvvalue,
      .prefix = prefix_cvvalue,
      .init_val = "1",
      .max_len = 4,
      .type = EDIT_INTEGER,
      .callback = callback_cv,
  },
  {
      .name = text_value,
      .prefix = prefix_value,
      .init_val = "",
      .max_len = 3,
      .type = EDIT_INTEGER,
      .callback = callback_cv_value,
  }
};

static const seqitem_t seq_pom_cv[] = {
  {
      .len = 3,
      .name = text_address,
      .prefix = prefix_locoaddr,
      .init_val = "3",
      .max_len = 4,
      .type = EDIT_INTEGER,
      .callback = callback_pom_addr,
  },
  {
      .name = text_cvvalue,
      .prefix = prefix_cvvalue,
      .init_val = "1",
      .max_len = 4,
      .type = EDIT_INTEGER,
      .callback = callback_pom_cv,
  },
  {
      .name = text_value,
      .prefix = prefix_value,
      .init_val = "",
      .max_len = 3,
      .type = EDIT_INTEGER,
      .callback = callback_pom_cv_value,
  }
};

static const seqitem_t seq_wifi_ssid[] = {
  {
      .len = 1,
      .name = text_ssid,
      .init_val = 0,
      .max_len = 32,
      .type = EDIT_TEXT,
      .callback = callback_ssid,
  }
};
static const seqitem_t seq_wifi_pass[] = {
  {
      .len = 1,
      .name = text_password,
      .init_val = "",
      .max_len = 32,
      .type = EDIT_TEXT,
      .callback = callback_pass,
  }
};
static text_list_t dhcp_list[2] = {"OFF", "ON"};
static const seqitem_t seq_wifi_dhcp[] = {
  {
    .len = 1,
    .name = text_dhcp,
    .init_pos = 0,
    .list_len = sizeof(dhcp_list)/sizeof(dhcp_list[0]),
    .list = dhcp_list,
    .type = EDIT_CHOOSE,
    .callback = callback_dhcp,
  },
};
static const seqitem_t seq_wifi_ip[] = {
  {
      .len = 1,
      .name = text_ip,
      .init_val = "",
      .max_len = 32,
      .type = EDIT_IP,
      .callback = callback_ip,
  }
};
static const seqitem_t seq_wifi_ipmask[] = {
  {
      .len = 1,
      .name = text_ipmask,
      .init_val = "",
      .max_len = 32,
      .type = EDIT_IP,
      .callback = callback_ipmask,
  }
};
static const seqitem_t seq_wifi_gateway[] = {
  {
      .len = 1,
      .name = text_gateway,
      .init_val = "",
      .max_len = 32,
      .type = EDIT_IP,
      .callback = callback_gateway,
  }
};
static const seqitem_t seq_wifi_ip_z21[] = {
  {
      .len = 1,
      .name = text_ip_z21,
      .init_val = "",
      .max_len = 32,
      .type = EDIT_IP,
      .callback = callback_ip_z21,
  }
};

static text_list_t ss_stop[2] = {"TRACK OFF", "LOCO STOP"};
static const seqitem_t seq_stop[] = {
  {
    .len = 1,
    .name = text_stopmode,
    .init_pos = 0,
    .list_len = sizeof(ss_stop)/sizeof(ss_stop[0]),
    .list = ss_stop,
    .type = EDIT_CHOOSE,
    .callback = callback_stopmode,
  },
};

static const seqitem_t seq_language[] = {
  {
    .len = 1,
    .name = text_language,
    .init_pos = 0,
    .list_len = sizeof(ss_language)/sizeof(ss_language[0]),
    .list = ss_language,
    .type = EDIT_CHOOSE,
    .callback = callback_language,
  },
};

#define RESET_CANCEL    0
#define RESET_SETT      1
#define RESET_ALL       2
static text_list_t ss_reset[3] = {"CANCEL", "SETTINGS", "ALL "};
static const seqitem_t seq_reset[] = {
  {
    .len = 1,
    .name = text_reset,
    .init_pos = 0,
    .list_len = sizeof(ss_reset)/sizeof(ss_reset[0]),
    .list = ss_reset,
    .type = EDIT_CHOOSE,
    .callback = callback_reset,
  }
};

static const seqitem_t item_password = {
    .prefix = prefix_password,
    .name = text_password,
    .max_len = 4,
    .type = EDIT_INTEGER,
};

#define CHILDLOCK_OFF           0
#define CHILDLOCK_SETPROG       1
#define CHILDLOCK_LIBSETPROG    2
static const seqitem_t seq_childlock[] = {
  {
    .len = 1,
    .name = text_password,
    .list = ss_stop,
    .max_len = 4,
    .type = EDIT_INTEGER,
    .callback = callback_childlock,
  },
};

static const seqitem_t seq_idle_time[] = {
  {
      .len = 1,
      .name = text_idle_time,
      .prefix = prefix_min,
      .init_val = "5",
      .max_len = 2,
      .type = EDIT_INTEGER,
      .callback = callback_idle_time,
  }
};

static text_list_t ss_webpage_en[2] = {"NO", "YES"};
static const seqitem_t seq_webpage_en[] = {
  {
    .len = 1,
    .name = text_webpage_en,
    .init_pos = 0,
    .list_len = sizeof(ss_webpage_en)/sizeof(ss_webpage_en[0]),
    .list = ss_webpage_en,
    .type = EDIT_CHOOSE,
    .callback = callback_webpage_en,
  },
};

static const seqitem_t seq_contrast[] = {
  {
    .len = 1,
    .name = text_contrast,
    .init_val = "64",
    .max_len = 3,
    .type = EDIT_INTEGER,
    .callback = callback_contrast,
  },
};

//Errors
const erritem_t err_noresp = {
    .id = 1,
    .name = text_noresp,
};
const erritem_t err_noack = {
    .id = 2,
    .name = text_noack,
};
const erritem_t err_lib_empty = {
    .id = 7,
    .name = text_lib_empty,
};
const erritem_t err_lib_full = {
    .id = 8,
    .name = text_lib_full,
};
const erritem_t err_exist = {
    .id = 10,
    .name = text_exist,
};
const erritem_t err_value_err = {
    .id = 11,
    .name = text_value_err,
};
const erritem_t err_conn_fault = {
    .id = 13,
    .name = text_conn_fault,
};

//Dynamic loco list menu
static const char * text_loconame[LOCO_LIST_LEN];
mitem_t loco_list[LOCO_LIST_LEN];

static const mitem_t loco_menu[] = {
    {
        .name = text_new,
        .len = 3,
        .flags = FLAGS_NEW_LOCO,
        .subseq = seq_loco,
    },
    {
        .name = text_edit,
        .flags = FLAGS_EDIT_LOCO,
        .submenu = loco_list,
    },
    {
        .name = text_delete,
        .flags = FLAGS_DELETE_LOCO,
        .submenu = loco_list,
    },
};

static const mitem_t prog_menu[] = {
    {
        .name = text_cvmenu,
        .len = 2,
        .flags = 0,
        .subseq = seq_cv,
    },
    {
        .name = text_pom_cvmenu,
        .len = 2,
        .flags = 0,
        .subseq = seq_pom_cv,
    },
};

static const mitem_t childlock_menu[] = {
  {
    .len = 3,
    .name = text_off,
    .flags = FLAGS_LOCK_OFF,
    .callback = callback_childlock,
  },
  {
    .name = text_prog,
    .flags = FLAGS_LOCK_SET,
    .subseq = seq_childlock,
    .skip_on_return = true,
  },
  {
    .name = text_libprog,
    .flags = FLAGS_LOCK_LIB,
    .subseq = seq_childlock,
    .skip_on_return = true,
  },
};

static const mitem_t wifi_menu[] = {
  {
    .len = 8,
    .name = text_ssid,
    .subseq = seq_wifi_ssid,
  },
  {
    .name = text_password,
    .subseq = seq_wifi_pass,
  },
  {
    .name = text_ip,
    .subseq = seq_wifi_ip,
  },
  {
    .name = text_dhcp,
    .subseq = seq_wifi_dhcp,
  },
  {
    .name = text_ipmask,
    .subseq = seq_wifi_ipmask,
  },
  {
    .name = text_gateway,
    .subseq = seq_wifi_gateway,
  },
  {
    .name = text_ip_z21,
    .subseq = seq_wifi_ip_z21,
  },
  {
    .name = text_webpage_en,
    .subseq = seq_webpage_en,
  }
};

static const mitem_t user_menu[] = {
  {
      .len = 3,
      .name = text_language,
      .subseq = seq_language,
  },
      {
      .name = text_childlock,
      .submenu = childlock_menu,
  },
  {
      .name = text_idle_time,
      .subseq = seq_idle_time,
  },
};

static const mitem_t settings_menu[] = {
    {
        .name = text_stopmode,
        .len = 5,
        .subseq = seq_stop,
    },
    {
        .name = text_wifi,
        .submenu = wifi_menu,
    },
    {
        .name = text_userinterface,
        .submenu = user_menu,
    },
    {
        .name = text_reset,
        .subseq = seq_reset,
    },

    {
        .name = text_contrast,
        .subseq = seq_contrast,
    }
};

const mitem_t menu_base[] = {
    {
        .name = text_loco,
        .submenu = loco_menu,
        .len = 3,
        .child_level = CHILDLOCK_SETPROG,
    },
    {
        .name = text_programing,
        .submenu = prog_menu,
        .child_level = CHILDLOCK_LIBSETPROG,
    },
    {
        .name = text_settings,
        .submenu = settings_menu,
        .child_level = CHILDLOCK_LIBSETPROG,
    },
};

/* VARIABLE DEFINITIONS /////////////////////////////////////////////////////*/
static config_update_callback_t config_update_callback;
config_db_t config_db;
static loco_t loco_current;
static bool loco_choose;
static uint8_t loco_func_shift;
static prog_cv_t prog_cv;
typedef enum {
    TRACK_NORMAL,
    TRACK_STOP,
    TRACK_SCIRCUIT,
    TRACK_PROG
} track_state_t;
static track_state_t track_state = TRACK_NORMAL;
static page_t err_call_page;
static uint8_t err_id;
#ifdef ESP8266
static callback_handler_t prog_timeout_timer;
#endif

/* FUNCTION DEFINITIONS /////////////////////////////////////////////////////*/
static inline void config_update(config_flags_t flags)
{
    if (config_update_callback) config_update_callback(flags);
}

void main_set_config_update_callback(config_update_callback_t callback)
{
    config_update_callback = callback;
}

#ifdef ESP8266
static void prog_timeout_handler(void * arg)
{
  main_show_error(&err_noresp);
  LOG_ERR("Prog timeout");
}
#endif

#define CV_MAGIC_VALUE  0xDEAD
static bool callback_cv(void * param, uint32_t flags, bool read)
{
    uint16_t magic_val;
    if (!read) {
        if (!param) return false;
        memcpy((uint8_t*)&magic_val, param, sizeof(magic_val));
        if (magic_val != CV_MAGIC_VALUE) {
            prog_cv.cv = atoi((char*)param);
            prog_cv.val = 0xFFFF;
            bool res = z21Client_requestReadCV(prog_cv.cv);
#ifdef ESP8266
            callback_timer_start(prog_timeout_timer, PROG_TIMEOUT, false, prog_timeout_handler, 0);
#endif
            /* Return false to stop sequence as we are waiting for command station response */
            return !res;
        }
        return true;
    }
    return false;
}

static void cv_response_notification(uint16_t CV, uint8_t value)
{
    uint16_t param = 0xDEAD;
#ifdef ESP8266
    callback_timer_stop(prog_timeout_timer);
#endif

    if (prog_cv.cv == CV) {
        prog_cv.val = value;
        page_return_back(PAGE_SEQUENCE, &param);
    } else if ((CV == 0) && (prog_cv.val == 0xFFFF)){ /* Error handling on CV reading */
        prog_cv.val = 0;
        page_return_back(PAGE_SEQUENCE, &param);
    }
}

static bool callback_cv_value(void * param, uint32_t flags, bool read)
{
    uint16_t magic_val;
    if (!read)  {
        if (!param) return false;
        memcpy((uint8_t*)&magic_val, param, sizeof(magic_val));
        if (magic_val != CV_MAGIC_VALUE) {
            prog_cv.val = atoi((char*)param);
            bool res = z21Client_setCV(prog_cv.cv, prog_cv.val);
#ifdef ESP8266
            callback_timer_start(prog_timeout_timer, PROG_TIMEOUT, false, prog_timeout_handler, 0);
#endif
            LOG_INFO_PRINTF("  Prog CV=%u val=%u", prog_cv.cv, prog_cv.val);
            /* Return false to stop sequence as we are waiting for command station response */
            return !res;
        }
        return true;
    } else if (prog_cv.val != 0xFFFF) {
        U16ToStr(prog_cv.val, param);
        return true;
    }
    return false;
}

static bool callback_pom_addr(void * param, uint32_t flags, bool read)
{
    if (!read) {
        if (!param) return false;
        prog_cv.addr = atoi((char*)param);
        return true;
    }
    return false;
}

static bool callback_pom_cv(void * param, uint32_t flags, bool read)
{
    if (!read) {
        if (!param) return false;
        prog_cv.cv = atoi((char*)param);
        return true;
    }
    return false;
}

static bool callback_pom_cv_value(void * param, uint32_t flags, bool read)
{
    if (!read)  {
        if (!param) return false;
        prog_cv.val = atoi((char*)param);
        z21Client_setPoMCV(prog_cv.addr, prog_cv.cv, prog_cv.val);
        LOG_INFO_PRINTF("  POM Prog CV=%u val=%u", prog_cv.cv, prog_cv.val);
        return true;
    }
    return false;
}

static bool check_addr(loco_t *item, int8_t id)
{
    for (uint8_t i=0; i<config_db.loco_db_len; i++) {
      if ((config_db.loco_db[i].addr == item->addr) && (id != i)) {
        main_show_error(&err_exist);
        LOG_ERR(" LOCO Addr exist");
        return true;
      }
    }
    return false;
}

static bool AddLoco(loco_t *item)
{
    if (config_db.loco_db_len < LOCO_LIST_LEN-1) {
        if (check_addr(item, -1)) return false;
        memcpy(&config_db.loco_db[config_db.loco_db_len], item, sizeof(loco_t));
        config_db.loco_db_len++;
        loco_list[0].len = config_db.loco_db_len;
        LOG_INFO_PRINTF("  LOCO added name=%s, addr=%u, ss=%s", item->name, item->addr, ss_list[item->ss]);
        config_update(DB_LOCO_DB);
    } else {
      main_show_error(&err_lib_full);
      LOG_ERR(" LOCO DB is full");
      return false;
    }
    return true;
}

static bool EditLoco(uint8_t id, loco_t *item)
{
    if (check_addr(item, id)) return false;
    memcpy(&config_db.loco_db[id], item, sizeof(loco_t));
    LOG_INFO_PRINTF("  LOCO updated name=%s, addr=%u, ss=%s", config_db.loco_db[id].name, config_db.loco_db[id].addr, ss_list[config_db.loco_db[id].ss]);
    config_update(DB_LOCO_DB);
    return true;
}

/* Called by menu_ll.c */
bool DeleteLoco(uint8_t id)
{
    if (config_db.loco_db_len) {
        for (uint8_t i=id+1; i<config_db.loco_db_len; i++) {
            config_db.loco_db[i-1] = config_db.loco_db[i];
        }
        config_db.loco_db_len--;
        loco_list[0].len = config_db.loco_db_len;
        LOG_INFO_PRINTF("  Delete loco %u", id);
        config_update(DB_LOCO_DB);
    } else {
        main_show_error(&err_lib_empty);
        LOG_ERR(" LOCO DB is empty");
        return false;
    }
    return true;
}

static bool callback_loco_name(void * param, uint32_t flags, bool read)
{
    uint8_t loco_id;
    if (!param) return false;

    if (!read) {
        strncpy(loco_current.name, param, STRING_LEN);
    } else if ((flags & FLAGS_ACTION_MASK) == FLAGS_EDIT_LOCO) {
        loco_id = (flags & FLAGS_LOCO_ID_MASK)>>8;
        strncpy(param, config_db.loco_db[loco_id].name, STRING_LEN);
    } else {
        /* New loco -> reset current_loco data */
        strncpy(loco_current.name, seq_loco[SEQ_LOCO_NAME_POS].init_val, STRING_LEN);
        loco_current.addr = atoi(seq_loco[SEQ_LOCO_ADDR_POS].init_val);
        loco_current.ss = seq_loco[SEQ_LOCO_SS_POS].init_pos;
        return false;
    }
    return true;
}

static bool callback_loco_addr(void * param, uint32_t flags, bool read)
{
    uint8_t loco_id;
    if (!param) return false;

    if (!read) {
        loco_current.addr = atoi((char*)param);
    } else if ((flags & FLAGS_ACTION_MASK) == FLAGS_EDIT_LOCO) {
        loco_id = (flags & FLAGS_LOCO_ID_MASK)>>8;
        U16ToStr(config_db.loco_db[loco_id].addr, param);
    } else {
        /* New loco */
        U16ToStr(loco_current.addr, param);
    }
    return true;
}

static bool callback_loco_ss(void * param, uint32_t flags, bool read)
{
    uint8_t loco_id;
    if (!param) return false;

    if (!read) {
        loco_current.ss = *(uint8_t*)param;
        if ((flags & FLAGS_ACTION_MASK) == FLAGS_EDIT_LOCO) {
            loco_id = (flags & FLAGS_LOCO_ID_MASK)>>8;
            return EditLoco(loco_id, &loco_current);
        } else {
            return AddLoco(&loco_current);
        }
    } else if ((flags & FLAGS_ACTION_MASK) == FLAGS_EDIT_LOCO) {
        loco_id = (flags & FLAGS_LOCO_ID_MASK)>>8;
        ((uint8_t*)param)[0] = config_db.loco_db[loco_id].ss;
    } else {
        /* New loco */
        ((uint8_t*)param)[0] = loco_current.ss;
        return false;
    }
    return true;
}

static bool callback_stopmode(void * param, uint32_t flags, bool read)
{
    if (!param) return false;

    if (!read) {
        config_db.stop_mode = ((uint8_t*)param)[0];
        //TODO: add reaction on stop mode change
        config_update(DB_CONFIG);
    } else ((uint8_t*)param)[0] = config_db.stop_mode;
    return true;
}

static bool callback_ssid(void * param, uint32_t flags, bool read)
{
    if (!param) return false;

    if (!read) {
        strncpy(config_db.ssid, param, STRING_LEN);
        config_update(DB_WIFI);
    } else strncpy(param, config_db.ssid, STRING_LEN);
    return true;
}

static bool callback_pass(void * param, uint32_t flags, bool read)
{
    if (!param) return false;

    if (!read) {
        strncpy(config_db.pass, param, STRING_LEN);
        config_update(DB_WIFI);
    } else strncpy(param, config_db.pass, STRING_LEN);
    return true;
}

static bool callback_ip(void * param, uint32_t flags, bool read)
{
    if (!param) return false;

    if (!read) {
        memcpy(config_db.ipaddr, param, 4);
        config_update(DB_WIFI);
    } else {
        memcpy(param, config_db.ipaddr, 4);
    }
    return true;
}

static bool callback_dhcp(void * param, uint32_t flags, bool read)
{
    if (!param) return false;

    if (!read) {
        config_db.dhcp = ((uint8_t*)param)[0];
        config_update(DB_WIFI);
    } else ((uint8_t*)param)[0] = config_db.dhcp;
    return true;
}

static bool callback_ipmask(void * param, uint32_t flags, bool read)
{
    if (!param) return false;

    if (!read) {
        memcpy(config_db.maskaddr, param, 4);
        config_update(DB_WIFI);
    } else {
        memcpy(param, config_db.maskaddr, 4);
    }
    return true;
}

static bool callback_gateway(void * param, uint32_t flags, bool read)
{
    if (!param) return false;

    if (!read) {
        memcpy(config_db.gwaddr, param, 4);
        config_update(DB_WIFI);
    } else {
        memcpy(param, config_db.gwaddr, 4);
    }
    return true;
}

static bool callback_ip_z21(void * param, uint32_t flags, bool read)
{
    if (!param) return false;

    if (!read) {
        memcpy(config_db.ip_z21, param, 4);
        config_update(DB_WIFI);
    } else {
        memcpy(param, config_db.ip_z21, 4);
    }
    return true;
}

static bool callback_language(void * param, uint32_t flags, bool read)
{
    if (!param) return false;

    if (!read) {
        config_db.language_id = ((uint8_t*)param)[0];
        menu_set_language(config_db.language_id);
        config_update(DB_CONFIG);
    } else ((uint8_t*)param)[0] = config_db.language_id;
    return true;
}

static bool callback_reset(void * param, uint32_t flags, bool read)
{
    if (!param) return false;

    if (!read) {
        switch (((uint8_t*)param)[0]) {
        case RESET_CANCEL:
            break;
        case RESET_SETT:
            LOG_INFO("RESET_SETT\n\r");
            main_reset_data(DB_CONFIG|DB_WIFI);
            break;
        case RESET_ALL:
            LOG_INFO("RESET_ALL\n\r");
            main_reset_data(DB_CONFIG|DB_LOCO_DB|DB_WIFI);
            break;
        }
        return true;
    }
    return false;
}

static bool callback_childlock(void * param, uint32_t flags, bool read)
{
    if (!read) {
        if (param) config_db.child_password = atoi((char*)param);
        if ((flags & FLAGS_ACTION_MASK) == FLAGS_LOCK_LIB) config_db.child_level = CHILDLOCK_LIBSETPROG;
        else if ((flags & FLAGS_ACTION_MASK) == FLAGS_LOCK_SET) config_db.child_level = CHILDLOCK_SETPROG;
        else config_db.child_level = CHILDLOCK_OFF;
        menu_set_childlock(config_db.child_level, config_db.child_password, &item_password);
        config_update(DB_CONFIG);
    } else {
      U16ToStr(config_db.child_password, param);
    }
    return true;
}

static bool callback_idle_time(void * param, uint32_t flags, bool read)
{
    if (!read) {
        if (param) config_db.idle_time_min = atoi((char*)param);
        config_update(DB_CONFIG);
    } else {
      U16ToStr(config_db.idle_time_min, param);
    }
    return true;
}

static bool callback_webpage_en(void * param, uint32_t flags, bool read)
{
    if (!read) {
        if (param) config_db.webpage_en = ((uint8_t*)param)[0];
        config_update(DB_CONFIG);
    } else {
      ((uint8_t*)param)[0] = config_db.webpage_en;
    }
    return true;
}

static bool callback_contrast(void * param, uint32_t flags, bool read)
{
  uint16_t contrast;
    if (!read) {
        if (param) {
          contrast = atoi((char*)param);
          if (contrast > 0xFF) contrast = 0xFF;
          config_db.contrast = contrast;
        }
        config_update(DB_CONFIG);
    } else {
      U16ToStr(config_db.contrast, param);
    }
    return true;
}

/* ///////////////////////////////////////////////////////////// */
static void loco_show(void)
{
    if (config_db.loco_db_pos < config_db.loco_db_len) {
        lcd_main_print(config_db.loco_db[config_db.loco_db_pos].name, 0, ALIGN_CENTER);
        LOG_INFO_PRINTF("\rL:%s", config_db.loco_db[config_db.loco_db_pos].name);
    } else {
        lcd_main_print(text_new_q[config_db.language_id], 0, ALIGN_CENTER);
    }
}

static void loco_change_dir(void)
{
    if (config_db.loco_db[config_db.loco_db_pos].speed > 0) {
        config_db.loco_db[config_db.loco_db_pos].dir_left = false;
        lcd_set_pause(false);
        lcd_set_left(false);
        lcd_set_mode(true, config_db.loco_db[config_db.loco_db_pos].dir_left, false, true);
        lcd_set_right(true);
    } else if (config_db.loco_db[config_db.loco_db_pos].speed < 0) {
        config_db.loco_db[config_db.loco_db_pos].dir_left = true;
        lcd_set_pause(false);
        lcd_set_right(false);
        lcd_set_mode(true, config_db.loco_db[config_db.loco_db_pos].dir_left, false, true);
        lcd_set_left(true);
    } else {
        lcd_set_right(false);
        lcd_set_left(false);
        lcd_set_pause(true);
        lcd_set_mode(true, config_db.loco_db[config_db.loco_db_pos].dir_left, false, true);
    }
}

static void loco_show_func(void)
{
    uint32_t func_mask, func;

    if (config_db.loco_db_pos < config_db.loco_db_len) {
        func = config_db.loco_db[config_db.loco_db_pos].func;
    } else func = 0;
    lcd_set_loco_func(0, (func & (1<<0)) ? true : false);
    for (uint8_t key=loco_func_shift+1; key<=loco_func_shift+10; key++) {
        func_mask = 1<<key;
        if (key <= 28) lcd_set_loco_func(key, (func & func_mask) ? true : false);
        else lcd_set_loco_func(key - loco_func_shift, false); 
    }
}

static void set_loco_throttle(void)
{
  static int8_t last_dir = 0x10;
  lcd_set_loco_throttle(abs(config_db.loco_db[config_db.loco_db_pos].speed), true);
  int8_t cur_dir = 0;
  if (config_db.loco_db[config_db.loco_db_pos].speed > 0) cur_dir = 1;
  else if (config_db.loco_db[config_db.loco_db_pos].speed < 0) cur_dir = -1;

  if (cur_dir != last_dir) loco_change_dir();
  last_dir = cur_dir;
}

void loco_begin(void)
{
    lcd_begin();
    set_loco_throttle();
    lcd_bottom_print(NULL, ALIGN_NONE);
    loco_show_func();
    loco_show();
    lcd_commit();
}

static void loco_exit_local(void)
{
    lcd_set_loco_throttle(0, false);
    for (uint8_t key=0; key<=10; key++) {
        lcd_set_loco_func(key, false);
    }
    lcd_set_mode(false, false, false, false);
    lcd_set_pause(false);
    lcd_set_right(false);
    lcd_set_left(false);
}

void loco_exit(void)
{
    lcd_begin();
    loco_exit_local();
    lcd_commit();
}

static void SendSpeed(bool emergency_stop)
{
    static uint8_t dir = 0x80;
    uint8_t steps, speed;
    steps = 128;
    if (config_db.loco_db[config_db.loco_db_pos].ss < sizeof(ss_val))
        steps = ss_val[config_db.loco_db[config_db.loco_db_pos].ss];
    /* Calculate speed step according to steps configuration
     *  NOTE: 0 - normal stop
     *        1 - E-stop
     *        2 - step-1
     */
    speed = 0;
    if (config_db.loco_db[config_db.loco_db_pos].speed) {
        switch (steps) {
          case 128:
            speed = (abs(config_db.loco_db[config_db.loco_db_pos].speed) * 126 + LOCO_MAX_STEP/2) / LOCO_MAX_STEP + (0x7F-126);
            break;
          case 14:
            speed = (abs(config_db.loco_db[config_db.loco_db_pos].speed) * 14 + LOCO_MAX_STEP/2) / LOCO_MAX_STEP + (0xF-14);
            break;
          case 28:
            speed = (abs(config_db.loco_db[config_db.loco_db_pos].speed) * 28 + LOCO_MAX_STEP/2) / LOCO_MAX_STEP + (0x1F-28);
            break;
          default:
            break;
        }
    } else if (emergency_stop) {
      speed = 1;
    }
    if (config_db.loco_db[config_db.loco_db_pos].speed > 0) dir = 0x80;
    else if (config_db.loco_db[config_db.loco_db_pos].speed < 0) dir = 0x00;
    speed |= dir;
    z21Client_setSpeed(config_db.loco_db[config_db.loco_db_pos].addr, steps, speed);
}

void loco_next(void)
{
    lcd_begin();
    if (loco_choose) {
        config_db.loco_db_pos = INC(config_db.loco_db_pos, 0, config_db.loco_db_len);
        loco_show();
    } else if ((config_db.loco_db_pos < config_db.loco_db_len)
        && (config_db.loco_db[config_db.loco_db_pos].speed < LOCO_MAX_STEP)) {
        config_db.loco_db[config_db.loco_db_pos].speed++;
        set_loco_throttle();
        SendSpeed(false);
#if LOG_PRINTF_ENABLED
        loco_show();
        LOG_INFO_PRINTF("  LOCO Step %+3i", config_db.loco_db[config_db.loco_db_pos].speed);
#endif
    }
    lcd_commit();
}

void loco_prev(void)
{
    lcd_begin();
    if (loco_choose) {
        config_db.loco_db_pos = DEC(config_db.loco_db_pos, 0, config_db.loco_db_len);
        loco_show();
    } else if ((config_db.loco_db_pos < config_db.loco_db_len)
        && (config_db.loco_db[config_db.loco_db_pos].speed > -LOCO_MAX_STEP)) {
        config_db.loco_db[config_db.loco_db_pos].speed--;
        set_loco_throttle();
        SendSpeed(false);
#if LOG_PRINTF_ENABLED
        loco_show();
        LOG_INFO_PRINTF("  LOCO Step %+3i", config_db.loco_db[config_db.loco_db_pos].speed);
#endif
    }
    lcd_commit();
}

void loco_shift(bool status)
{
    static uint32_t last_button_time = 0;
#ifdef ESP8266
    uint32_t curr_time = millis();
#else
    #include <time.h>
    uint32_t curr_time = clock();
#endif
    uint32_t delta = curr_time - last_button_time;
    if (status) {
        if (delta < 1000)
            loco_func_shift = 20;
        else loco_func_shift = 10;
        last_button_time = curr_time;
    } else loco_func_shift = 0;

    lcd_begin();
    lcd_set_shift(status, loco_func_shift == 20);
    loco_show_func();
    lcd_commit();
    loco_choose = status;
}

void loco_key(uint8_t key)
{
    key += loco_func_shift;
    uint32_t func_mask = 1<<key;
    bool status;

    if (config_db.loco_db_pos < config_db.loco_db_len) {
        if ((config_db.loco_db[config_db.loco_db_pos].func & func_mask) != 0)
            config_db.loco_db[config_db.loco_db_pos].func &=~ func_mask;
        else config_db.loco_db[config_db.loco_db_pos].func |= func_mask;

        status = (config_db.loco_db[config_db.loco_db_pos].func & func_mask) ? true : false;
        lcd_set_loco_func(key, status);
        z21Client_setLocoFunc(config_db.loco_db[config_db.loco_db_pos].addr, key, config_db.loco_db[config_db.loco_db_pos].func);
        LOG_INFO_PRINTF("FUNC %u = %u", key, status);
    }
}

void loco_enter(void)
{
    if (config_db.loco_db_pos < config_db.loco_db_len) {
        loco_key(0);
    } else {
        seq_start(PAGE_LOCO, FLAGS_NEW_LOCO, seq_loco);
    }
}

void turnout_show(void)
{
    char str[SHOW_LEN+1];
    snprintf(str, sizeof(str), "%-2s%04u", prefix_turnout[config_db.language_id], config_db.turnout_id);
    lcd_main_print(str, 0, ALIGN_NONE);
    LOG_INFO_PRINTF("\rT:%s", str);
}

void turnout_begin(void)
{
    lcd_begin();
    loco_exit_local();
    lcd_set_mode(false, false, false, true);
    turnout_show();
    lcd_commit();
}

void turnout_exit(void)
{
    lcd_begin();
    lcd_set_mode(false, false, false, true);
    lcd_commit();
}

void turnout_set_id(uint16_t id)
{
    if (id > MAX_TURNOUT_ID)
      id = MAX_TURNOUT_ID;

    lcd_begin();
    config_db.turnout_id = id;
    turnout_show();
    lcd_commit();
    config_update(DB_CONFIG);
}

void turnout_set(bool state)
{
    if (state) {
        lcd_begin();
        turnout_show();
        lcd_set_mode(false, true, false, true);
        lcd_commit();
        z21Client_setTrntPos(config_db.turnout_id-1, true, true);
        LOG_INFO_PRINTF("  Set turnout %u", config_db.turnout_id);
    } else {
        z21Client_setTrntPos(config_db.turnout_id-1, true, false);
    }
}

void turnout_reset(bool state)
{
    if (state) {
        lcd_begin();
        turnout_show();
        lcd_set_mode(false, false, false, true);
        lcd_commit();
        z21Client_setTrntPos(config_db.turnout_id-1, false, true);
        LOG_INFO_PRINTF("  Reset turnout %u", config_db.turnout_id);
    } else {
        z21Client_setTrntPos(config_db.turnout_id-1, false, false);
    }
}

void turnout_key(uint8_t key)
{
    value_edit(PAGE_TURNOUT, EDIT_INTEGER, prefix_turnout[config_db.language_id], "", 4);
    value_edit_set(key);
}

void track_stop(void)
{
    if (current_page == PAGE_LOCO) {
        if (loco_func_shift) {
          config_db.loco_db[config_db.loco_db_pos].speed = 0;
          set_loco_throttle();
          SendSpeed(true);
          return;
        }
        if (config_db.loco_db[config_db.loco_db_pos].speed) {
          config_db.loco_db[config_db.loco_db_pos].speed = 0;
          set_loco_throttle();
          SendSpeed(false);
          return;
        }
    }
    lcd_begin();
    if (track_state == TRACK_NORMAL) {
        track_state = TRACK_STOP;
        lcd_set_stop(true);
        z21Client_setPower((!config_db.stop_mode) ? CS_TRACK_OFF : CS_ESTOP);
    } else if (track_state != TRACK_SCIRCUIT){
        track_state = TRACK_NORMAL;
        lcd_set_stop(false);
        z21Client_setPower(CS_NORMAL);
    } else{
        track_state = TRACK_NORMAL;
        lcd_set_shortcircuit(false);
        lcd_set_stop(false);
        z21Client_setPower(CS_NORMAL);
    }
    lcd_commit();
}

void track_shortcircuit(void)
{
    lcd_begin();
    track_state = TRACK_SCIRCUIT;
    lcd_set_shortcircuit(true);
    lcd_set_stop(true);
    lcd_commit();
}
/////////////////////////////////////////////////////////////////////////////////
static void notifyXNetPower(uint8_t status)
{
    //LOG_INFO("notifyXNetPower\n\r");
    lcd_begin();
#if DEBUG
    char sts[5];
    sts[0] = '0' + ((status & 0x1) ? 1 : 0);
    sts[1] = '0' + ((status & 0x2) ? 1 : 0);
    sts[2] = '0' + ((status & 0x4) ? 1 : 0);
    sts[3] = '0' + ((status & 0x8) ? 1 : 0);
    sts[4] = 0;
    lcd_bottom_print(sts, ALIGN_NONE);
#endif
    if (!status) {
        track_state = TRACK_NORMAL;
        if ((current_page == PAGE_LOCO) || (current_page == PAGE_TURNOUT)) {
          lcd_set_shortcircuit(false);
          lcd_set_stop(false);
        }
    } else if (status & (CS_ESTOP|CS_TRACK_OFF)) {
        track_state = TRACK_STOP;
        if ((current_page == PAGE_LOCO) || (current_page == PAGE_TURNOUT)) {
          lcd_set_stop(true);
        }
    } else if (status & CS_TRACK_SHORTED) {
        track_state = TRACK_SCIRCUIT;
        if ((current_page == PAGE_LOCO) || (current_page == PAGE_TURNOUT)) {
          lcd_set_shortcircuit(true);
          lcd_set_stop(true);
        }
    }
    lcd_commit();
}

static void notifyXNetExtControl(uint16_t locoAddress)
{
  LOG_INFO("notifyXNetExtControl\n\r");
  if ((current_page == PAGE_LOCO) && (config_db.loco_db[config_db.loco_db_pos].addr == locoAddress)) {
      lcd_begin();
      lcd_set_mode(true, config_db.loco_db[config_db.loco_db_pos].dir_left, true, true);
      lcd_commit();
  }
}

static void notifyXNetExtSpeed(uint16_t locoAddress, uint8_t steps, uint8_t value)
{
    //LOG_INFO("notifyXNetExtSpeed\n\r");
    if (config_db.loco_db[config_db.loco_db_pos].addr == locoAddress){
        config_db.loco_db[config_db.loco_db_pos].speed = ((value & 0x80) ? 1 : -1) * ((int16_t)LOCO_MAX_STEP * (value & 0x7F) + (steps-1)/2) / (steps-1);
        if (current_page == PAGE_LOCO) {
          lcd_begin();
          set_loco_throttle();
          lcd_commit();
        }
    }
}

static void notifyXNetExtFunc(uint16_t locoAddress, uint32_t funcMask, uint32_t funcStatus)
{
    //LOG_INFO("notifyXNetExtFunc\n\r");
    uint32_t newFunctionStates, functionChanged, mask;
    if (config_db.loco_db[config_db.loco_db_pos].addr == locoAddress){
        newFunctionStates = config_db.loco_db[config_db.loco_db_pos].func & (~funcMask);
        newFunctionStates |= funcStatus;
        functionChanged = newFunctionStates ^ config_db.loco_db[config_db.loco_db_pos].func;
        config_db.loco_db[config_db.loco_db_pos].func = newFunctionStates;
        if (current_page == PAGE_LOCO) {
          lcd_begin();
          //Check Light function
          mask = 1<<0;
          if ((functionChanged & mask) != 0) {
              lcd_set_loco_func(0, (config_db.loco_db[config_db.loco_db_pos].func & mask) ? true : false);
          }
          //Check the rest based on current shift state
          for (uint8_t id=loco_func_shift+1; id<loco_func_shift+10; id++) {
            mask = 1<<id;
            if ((functionChanged & mask) != 0) {
              lcd_set_loco_func(id, (config_db.loco_db[config_db.loco_db_pos].func & mask) ? true : false);
            }
          }
          lcd_commit();
        }
    }
}

#ifdef _WIN32
void notifyXNetService(bool directMode, uint16_t CV, uint8_t value)
#else
static void notifyXNetService(bool directMode, uint16_t CV, uint8_t value)
#endif
{
    cv_response_notification(CV, value);
    LOG_INFO("notifyXNetService\n\r");
}

#ifdef _WIN32
void notifyXNetServiceError(void)
#else
static void notifyXNetServiceError(void)
#endif
{
    cv_response_notification(0, 0);
    /* Must be after cv_response_notification */
    main_show_error(&err_noack);
    LOG_INFO("notifyXNetServiceError\n\r");
}

static const z21client_callback_t z21client_callback = {
    .notifyXNetPower = notifyXNetPower,
    .notifyXNetExtControl = notifyXNetExtControl,
    .notifyXNetExtSpeed = notifyXNetExtSpeed,
    .notifyXNetExtFunc = notifyXNetExtFunc,
    .notifyXNetService = notifyXNetService,
    .notifyXNetServiceError = notifyXNetServiceError,
    .notifyXNetFeedback = NULL
};

/////////////////////////////////////////////////////////////////////////////////
void main_reset_data(config_flags_t flags)
{
    if (flags & DB_CONFIG) {
        LOG_INFO("Reset user config\n\r");
        config_db.language_id = DEFAULT_LANGUAGE;
        config_db.child_level = 0;
        config_db.child_password = 0;
        config_db.turnout_id = 1;
        config_db.stop_mode = 0;
        config_db.idle_time_min = DEFAULT_IDLE_TIME_M;
        config_db.webpage_en = false;
        config_db.contrast = 127;
    }
    if (flags & DB_LOCO_DB) {
      LOG_INFO("Reset loco_db\n\r");
      memset(config_db.loco_db, 0, sizeof(config_db.loco_db));
      config_db.loco_db_pos = 0;
      config_db.loco_db_len = 1;
      loco_list[0].len = config_db.loco_db_len;
      strcpy(config_db.loco_db[0].name, "DEFLT");
      config_db.loco_db[0].addr = 3;
      config_db.loco_db[0].ss = 2;
    }
    if (flags & DB_WIFI) {
      LOG_INFO("Reset wifi data\n\r");
    #ifndef _WIN32
    void WiFi_ResetToDefaults(void);
          WiFi_ResetToDefaults();
    #endif
    }
    config_update(flags);
}

void main_page_init(void)
{
#ifdef ESP8266
    prog_timeout_timer = callback_timer_create();
#endif

    prog_cv.cv = 0;
    menu_set_language(config_db.language_id);
    menu_set_childlock(config_db.child_level, config_db.child_password, &item_password);
    loco_list[0].len = config_db.loco_db_len;
    for (uint8_t i=0; i < LOCO_LIST_LEN; i++) {
        loco_list[i].subseq = seq_loco;
        text_loconame[i] = (const char *)&config_db.loco_db[i].name;
        loco_list[i].name = (text_list_t *)&text_loconame[i];
        loco_list[i].skip_on_return = true;
    }
    z21Client_setEventCallbacks(z21client_callback);
}

void main_exit_error(void)
{
    if (current_page != PAGE_ERROR) return;
     
    if (err_id == err_exist.id) {
        /* In case the loco address exist move back to address entering */
        seq_move_to(SEQ_LOCO_ADDR_POS);
        return;
    }
    lcd_bottom_print(0, ALIGN_NONE);
    page_return_back(err_call_page, NULL);
}

void main_show_error(const erritem_t * err_item)
{
    char buf[SHOW_LEN+1];
    if (!err_item) return;

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
    err_call_page = current_page;
    current_page = PAGE_ERROR;
    err_id = err_item->id;

    snprintf(buf, sizeof(buf), "ERR %2u", err_item->id);
    lcd_main_print(buf, 0, ALIGN_NONE);
    lcd_bottom_print(err_item->name[config_db.language_id], ALIGN_CENTER);
}

/* UTILITY */
static uint8_t U16ToStr(uint16_t val, char *str)
{
  uint16_t div = 10000;
  uint16_t pos = 0;
  bool leading = true;
  while (1)
  {
    uint8_t res = val/div;
    if ((res>0) || !leading)
    {
      str[pos++] = '0' + res;
      leading = false;
    }
    val -= res*div;
    if (div == 1) {
        if (!pos) str[pos++] = '0';
        break;
    }
    div /=10;
  }
  str[pos] = 0;
  return pos;
}

#define ADD_TO_BUF(_buf, _len, _x, ...) do {            \
    uint8_t res = snprintf(_buf, _len, _x, __VA_ARGS__);\
    buf += res; len -= res; if (len <= 0) return false; \
}while(0)

int print_config_db(char * buf, uint16_t len)
{
    char *b_ptr = buf;
    config_db_t *db = &config_db;

    ADD_TO_BUF(buf, len, "%s;%s;""%u;\n", db->ssid, db->pass, db->dhcp);
    ADD_TO_BUF(buf, len, "%u;%u;%u;%u;""%u;%u;%u;%u;""%u;%u;%u;%u;\n",
            db->ipaddr[0], db->ipaddr[1], db->ipaddr[2], db->ipaddr[3],
            db->gwaddr[0], db->gwaddr[1], db->gwaddr[2], db->gwaddr[3],
            db->maskaddr[0], db->maskaddr[1], db->maskaddr[2], db->maskaddr[3] );
    ADD_TO_BUF(buf, len, "%u;%u;%u;%u;\n", db->ip_z21[0], db->ip_z21[1], db->ip_z21[2], db->ip_z21[3]);
    ADD_TO_BUF(buf, len, "%u;%u;%u;%u;%u;%u;\n", db->stop_mode, db->language_id, db->child_level, db->child_password, db->idle_time_min, db->contrast);
    ADD_TO_BUF(buf, len, "%u;%u;\n", db->turnout_id, db->loco_db_len);
    for (uint8_t i=0; i<db->loco_db_len; i++) {
        ADD_TO_BUF(buf, len, "%s;%u;%u;\n", db->loco_db[i].name, db->loco_db[i].addr, db->loco_db[i].ss);
    }
    *buf = '\0';
    return buf - b_ptr;
}

#define PARSE_IP(_pos,_strt_pos, _ptr, _str) \
    case _strt_pos:                          \
    case _strt_pos+1:                        \
    case _strt_pos+2:                        \
    case _strt_pos+3:                        \
        _ptr[_pos-_strt_pos] = atoi(_str);   \
        break;
#define PARSE_INT(_strt_pos, _val, _str) \
    case _strt_pos:                      \
        _val = atoi(strt);               \
        break;
#define PARSE_BOOL(_strt_pos, _val, _str)         \
    case _strt_pos:                               \
        _val = (*(strt+1) == '1') ? true : false; \
        break;
#define PARSE_STR(_strt_pos, _ptr, _len, _str) \
    case _strt_pos:                            \
        strncpy(_ptr, _str, _len);             \
        break;

bool parse_config_db(char * buf, uint16_t len)
{
    config_db_t *db = &config_db;
    char * strt = buf;
    char * end;
    uint8_t pos = 0;

    do {
        if ((strt) && (*(strt) == '\n')) strt++;
        end = strchr(strt+1,';');
        if (end) *end = 0;
        else break;

        if (pos < 25) {
            switch (pos) {
            PARSE_STR(0, db->ssid, sizeof(db->ssid), strt)
            PARSE_STR(1, db->pass, sizeof(db->pass), strt)
            PARSE_BOOL(2, db->dhcp, strt)
            PARSE_IP(pos, 3, db->ipaddr, strt)
            PARSE_IP(pos, 7, db->gwaddr, strt)
            PARSE_IP(pos, 11, db->maskaddr, strt)
            PARSE_IP(pos, 15, db->ip_z21, strt)
            PARSE_INT(19, db->stop_mode, strt)
            PARSE_INT(20, db->language_id, strt)
            PARSE_INT(21, db->child_level, strt)
            PARSE_INT(22, db->child_password, strt)
            PARSE_INT(23, db->idle_time_min, strt)
            PARSE_INT(24, db->contrast, strt)
            PARSE_INT(25, db->turnout_id, strt)
            PARSE_INT(26, db->loco_db_len, strt)
            }
        } else {
            uint8_t loco_id = (pos - 27)/3;
            uint8_t loco_item = (pos - 27) % 3;
            switch (loco_item) {
                PARSE_STR(0, db->loco_db[loco_id].name, sizeof(db->loco_db[loco_id].name), strt)
                PARSE_INT(1, db->loco_db[loco_id].addr, strt)
                PARSE_INT(2, db->loco_db[loco_id].ss, strt)
            }
        }
        strt = end+1;
        pos++;
    }
    while (1);

    loco_list[0].len = config_db.loco_db_len;
    return true;
}
