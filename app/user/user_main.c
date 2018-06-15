/* main.c -- MQTT client example
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/

#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "tcp/tcp.h"
#include "wifi.h"
#include "config.h"
#include "tcp/debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"
#include "sntp.h"


#include "user_config.h"
#include "cJSON.h"

#include "process_data.h"

#include "tcp/tcp.h"


typedef unsigned long u32_t;
static ETSTimer sntp_timer;


TCP_Client  tcpClient;


process_data_t  mqtt_receve_data;

os_timer_t mqtt_serial_timer;//MQtt周期定时器

void ICACHE_FLASH_ATTR mqtt_serial_cb(void);
void sntpfn()
{
    u32_t ts = 0;
    ts = sntp_get_current_timestamp();
#ifdef DBUG_MODE
    os_printf("current time : %s\n", sntp_get_real_time(ts));
#endif
//如果SNTP服务器时间同步错误，认为TCP没有连接，调用MQTT_Connect()连接
    if (ts == 0) {
        //os_printf("did not get a valid time from sntp server\n");
    } else {
            os_timer_disarm(&sntp_timer);
            TCP_Connect(&tcpClient);
    }
}

void wifiConnectCb(uint8_t status)
{
    if(status == STATION_GOT_IP){

#ifdef USE_NTP_ENABLE
        sntp_setservername(0, "0.cn.pool.ntp.org");        // set sntp server after got ip address
        sntp_setservername(1, "1.cn.pool.ntp.org");
        sntp_setservername(2, "2.cn.pool.ntp.org");
        //sntp_setservername(0,"ntp1.aliyun.com");
    	sntp_init();
        os_timer_disarm(&sntp_timer);
        os_timer_setfn(&sntp_timer, (os_timer_func_t *)sntpfn, NULL);
        os_timer_arm(&sntp_timer, 2000, 1);//2s
#else

        TCP_Connect(&tcpClient);
#endif

    } else {
    	TCP_Disconnect(&tcpClient);
    }
}

void mqttConnectedCb(uint32_t *args)
{
    TCP_Client* client = (TCP_Client*)args;
    process_client=client;

#ifdef DBUG_MODE
    INFO("MQTT: Connected\r\n");
#endif

	//test任务
    /*
	os_timer_disarm(&mqtt_serial_timer);
	os_timer_setfn(&mqtt_serial_timer, (os_timer_func_t *)mqtt_serial_cb, NULL);
	os_timer_arm(&mqtt_serial_timer, 10000, 0);
*/
   MQTT_Subscribe(client, "/xylmqtt/pm25_control", 0);
   // MQTT_Subscribe(client, "/mqtt/topic/1", 1);
   // MQTT_Subscribe(client, "/mqtt/topic/2", 2);

    //MQTT_Publish(client, "/mqtt/topic/0", "hello0", 6, 0, 0);
   // MQTT_Publish(client, "/mqtt/topic/1", "hello1", 6, 1, 0);
   // MQTT_Publish(client, "/mqtt/topic/2", "hello2", 6, 2, 0);

}

void mqttDisconnectedCb(uint32_t *args)
{
    TCP_Client* client = (TCP_Client*)args;
#ifdef DBUG_MODE
    INFO("TCP: Disconnected\r\n");
#endif
}

void mqttPublishedCb(uint32_t *args)
{
    TCP_Client* client = (TCP_Client*)args;
#ifdef DBUG_MODE
    INFO("TCP: Published\r\n");
#endif
}

void mqttDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{

    char *topicBuf = (char*)os_zalloc(topic_len+1),
            *dataBuf = (char*)os_zalloc(data_len+1);

    TCP_Client* client = (TCP_Client*)args;

    os_memcpy(topicBuf, topic, topic_len);
    topicBuf[topic_len] = 0;

    os_memcpy(dataBuf, data, data_len);
    dataBuf[data_len] = 0;
#ifdef DBUG_MODE
    INFO("Receive topic: %s, data: %s \r\n", topicBuf, dataBuf);
#endif

    mqtt_receve_data.data=dataBuf;
    mqtt_receve_data.length=data_len;
    mqtt_data_process(&mqtt_receve_data);
    os_free(topicBuf);
    os_free(dataBuf);
}


/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
 *******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR mqtt_serial_cb(void){

	// MQTT_Publish(&mqttClient, "/xylmqtt/pm25", "hello!", 6, 0, 0);

}

extern int cJSON_test(void);

void ICACHE_FLASH_ATTR to_scan(void) {

	//cJSON_test();

}
void ICACHE_FLASH_ATTR gpio_init(void) {
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U, FUNC_GPIO0);//设置为复位的按键  主要用于重新配置wifi
#ifdef UDP_ENABLE
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, FUNC_GPIO12);  //控制继电器IO
	GPIO_OUTPUT_SET(GPIO_ID_PIN(12), 0);
#endif
	PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);  //输出wifi的状态
	GPIO_OUTPUT_SET(GPIO_ID_PIN(14), 1);//WIFI模块未配置
}
void user_init(void)
{
	gpio_init();
	system_uart_de_swap();
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);
    //read and save config_data

    CFG_Load();
    //tcp初始化,配置接口信息
    TCP_InitConnection(&tcpClient,sysCfg.tcp_host, sysCfg.tcp_port, sysCfg.security);
    //初始化信息、任务事件TCP_TASK
    TCP_InitClient(&tcpClient);

#ifdef STATIC_WIFI_PASSWED
    WIFI_Connect(sysCfg.sta_ssid, sysCfg.sta_pwd, wifiConnectCb);
#else
    WIFI_Start_config(wifiConnectCb);//用户自动配置smartconfig,并且配置wifi连接成功后的回调函数
#endif

#ifdef DBUG_MODE
    INFO("\r\nSystem started ...\r\n");
#endif
    system_init_done_cb(to_scan);
}
