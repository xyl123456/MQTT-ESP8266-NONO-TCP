/*
 * wifi.c
 *
 *  Created on: Dec 30, 2014
 *      Author: Minh
 */
#include "wifi.h"
#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"

#include "debug.h"
#include "user_config.h"
#include "config.h"

static ETSTimer WiFiLinker;
WifiCallback wifiCb = NULL;
uint8_t config_timer=0;//config smartconfig timer is $config_timer

uint8_t config_key_flag=0;//1----step in config    0------no step in config

static uint8_t wifiStatus = STATION_IDLE, lastWifiStatus = STATION_IDLE;


void ICACHE_FLASH_ATTR
smartconfig_done(sc_status status, void *pdata){
    switch(status) {
        case SC_STATUS_WAIT:
#ifdef	DBUG_MODE
			INFO("SC_STATUS_WAIT\r\n");
#endif
            break;
        case SC_STATUS_FIND_CHANNEL:
#ifdef	DBUG_MODE
			INFO("SC_STATUS_FIND_CHANNEL\r\n");
#endif
            break;
        case SC_STATUS_GETTING_SSID_PSWD:
        {
#ifdef	DBUG_MODE
			INFO("SC_STATUS_GETTING_SSID_PSWD\r\n");
#endif
        	sc_type *type = pdata;
			//config_time = configtime;
            if (*type == SC_TYPE_ESPTOUCH)
            {
#ifdef	DBUG_MODE
			INFO("SC_TYPE:SC_TYPE_ESPTOUCH\r\n");
#endif
			}
            else
            {
#ifdef	DBUG_MODE
			INFO("SC_TYPE:SC_TYPE_AIRKISS\r\n");
#endif
			}
        }
            break;
        case SC_STATUS_LINK:
        {
#ifdef	DBUG_MODE
			INFO("SC_STATUS_LINK\r\n");
#endif
		 	  	wifi_station_disconnect();

		        struct station_config *sta_conf = pdata;
			    wifi_station_set_config(sta_conf);
//获取最新的SSID和PASSWD
			    os_memcpy(sysCfg.sta_ssid,sta_conf->ssid,sizeof(sta_conf->ssid));
			    os_memcpy(sysCfg.sta_pwd,sta_conf->password,sizeof(sta_conf->password));
			    CFG_Save();
#ifdef	DBUG_MODE
			INFO("I GOT THE SSID IS:%S\r\n",sysCfg.sta_ssid);
			INFO("I GOT THE PASSWD IS:%S\r\n",sysCfg.sta_pwd);
#endif
			    wifi_station_connect();

        }
            break;
        case SC_STATUS_LINK_OVER:
#ifdef	DBUG_MODE
			INFO("SC_TYPE_LINK_OVER\r\n");
#endif
            if (pdata != NULL)
            {
                uint8 phone_ip[4] = {0};
                os_memcpy(phone_ip, (uint8*)pdata, 4);
#ifdef DBUG_MODE
                INFO("Phone ip: %d.%d.%d.%d\n",phone_ip[0],phone_ip[1],phone_ip[2],phone_ip[3]);
#endif
            }
		    //STOP CONFIG_TIMER
            config_timer=0;
            config_key_flag=0;
            smartconfig_stop();
            break;
    }
}



static void ICACHE_FLASH_ATTR wifi_check_ip(void *arg)
{
	struct ip_info ipConfig;

	os_timer_disarm(&WiFiLinker);
	wifi_get_ip_info(STATION_IF, &ipConfig);
	wifiStatus = wifi_station_get_connect_status();
	if (wifiStatus == STATION_GOT_IP && ipConfig.ip.addr != 0)
	{
		os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
		os_timer_arm(&WiFiLinker, 2000, 0);
	}
	else
	{

		if(wifi_station_get_connect_status() == STATION_WRONG_PASSWORD)
		{
#ifdef	DBUG_MODE
			INFO("STATION_WRONG_PASSWORD\r\n");
#endif
			wifi_station_connect();

		}
		else if(wifi_station_get_connect_status() == STATION_NO_AP_FOUND)
		{
#ifdef	DBUG_MODE
			INFO("STATION_NO_AP_FOUND\r\n");
#endif
			wifi_station_connect();

		}
		else if(wifi_station_get_connect_status() == STATION_CONNECT_FAIL)
		{
#ifdef	DBUG_MODE
			INFO("STATION_CONNECT_FAIL\r\n");
#endif
			wifi_station_connect();

		}
		else
		{
#ifdef	DBUG_MODE
			INFO("STATION_IDLE\r\n");
#endif
		}
		os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
		os_timer_arm(&WiFiLinker,2000, 0);

	}
	if(wifiStatus != lastWifiStatus){
		lastWifiStatus = wifiStatus;
		if(wifiCb)
			wifiCb(wifiStatus);
	}
#ifdef STATIC_WIFI_PASSWED
#else
	if(!GPIO_INPUT_GET(GPIO_ID_PIN(0)))
	{
		config_key_flag=1;
		config_timer=WIFI_SMART_CONFIG_TIME;
		smartconfig_stop();//停止配置,为了以后用户突然的中断
		smartconfig_start(smartconfig_done);//开启一键连接监听

	}
	if(config_timer>0)
		{
			config_timer--;
		}
	else
		{
			if(wifiStatus == STATION_GOT_IP && ipConfig.ip.addr != 0)
			{
				config_timer=0;
			}
			else
			{
				GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 1);//没有连接到wifi
				//长时间用户没有配置，停止配置模式.或者用户的网络断开了，也会进入到该状态
#ifdef DBUG_MODE
				INFO("USER NOT SEND PASSWD AND APNAME STOP SMARTCONFIG\r\n");
#endif

				if(config_key_flag==1){
					smartconfig_stop();
					config_key_flag=0;
					wifi_station_connect();
					INFO("stop smart_config\r\n");
				}

			}
		}
#endif

}

void ICACHE_FLASH_ATTR WIFI_Connect(uint8_t* ssid, uint8_t* pass, WifiCallback cb)
{
	struct station_config stationConf;
#ifdef	DBUG_MODE
	INFO("WIFI_INIT\r\n");
#endif
	wifi_set_opmode_current(STATION_MODE);
	wifi_station_set_auto_connect(1);//设置上电自动连接已经记录的AP信息
	wifi_station_set_reconnect_policy(true);//AP断开重连
	//wifi_station_set_auto_connect(FALSE);
	wifiCb = cb;

	os_memset(&stationConf, 0, sizeof(struct station_config));

	os_sprintf(stationConf.ssid, "%s", ssid);
	os_sprintf(stationConf.password, "%s", pass);

	wifi_station_set_config_current(&stationConf);

	os_timer_disarm(&WiFiLinker);
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiLinker, 1000, 0);

	//wifi_station_set_auto_connect(TRUE);
	wifi_station_connect();
}


void ICACHE_FLASH_ATTR WIFI_Start_config(WifiCallback cb)
{
	wifi_set_opmode_current(STATION_MODE);
	wifi_station_set_auto_connect(1);//设置上电自动连接已经记录的AP信息
	wifi_station_set_reconnect_policy(true);//AP断开重连
	wifiCb = cb;

	os_timer_disarm(&WiFiLinker);
	os_timer_setfn(&WiFiLinker, (os_timer_func_t *)wifi_check_ip, NULL);
	os_timer_arm(&WiFiLinker, 1000, 0);

}
