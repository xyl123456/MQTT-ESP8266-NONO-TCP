/*
 * wifi.h
 *
 *  Created on: Dec 30, 2014
 *      Author: Minh
 */

#ifndef USER_WIFI_H_
#define USER_WIFI_H_
#include "os_type.h"

#define  WIFI_SMART_CONFIG_TIME  30
typedef enum {
    SC_STATUS_WAIT = 0,
    SC_STATUS_FIND_CHANNEL,
    SC_STATUS_GETTING_SSID_PSWD,
    SC_STATUS_LINK,
    SC_STATUS_LINK_OVER,
} sc_status;

typedef enum {
    SC_TYPE_ESPTOUCH = 0,
    SC_TYPE_AIRKISS,
} sc_type;


typedef void (*WifiCallback)(uint8_t);
void ICACHE_FLASH_ATTR WIFI_Connect(uint8_t* ssid, uint8_t* pass, WifiCallback cb);
void ICACHE_FLASH_ATTR WIFI_Start_config(WifiCallback cb);
void ICACHE_FLASH_ATTR smartconfig_done(sc_status status, void *pdata);

typedef void (*sc_callback_t)(sc_status status, void *pdata);

const char *smartconfig_get_version(void);
bool smartconfig_start(sc_callback_t cb, ...);
bool smartconfig_stop(void);
bool esptouch_set_timeout(uint8 time_s); //15s~255s, offset:45s



#endif /* USER_WIFI_H_ */
