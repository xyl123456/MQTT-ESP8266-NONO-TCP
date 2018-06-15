/*
 * process_data.c
 *
 *  Created on: 2018年6月6日
 *      Author: Administrator
 */

#include "process_data.h"
#include "tcp_config.h"

#include "tcp/debug.h"
#include "tcp/tcp.h"

#include "string.h"
#include "cJSON.h"

#include "user_config.h"

#include "user_interface.h"


TCP_Client *process_client;

tcp_message_t tcp_keeplive_message;

void ICACHE_FLASH_ATTR
tcp_send_keepalive(TCP_Client *client)
{

		char *out = NULL;

		cJSON *cjson_message = NULL;
		cJSON *mes_data = NULL;
		cJSON *data_item = NULL;

		cjson_message = cJSON_CreateObject();
		cJSON_AddStringToObject(cjson_message, "message", "success");
		cJSON_AddNumberToObject(cjson_message,"code",200);
		cJSON_AddStringToObject(cjson_message, "type", "keeplive_data");
		cJSON_AddItemToObject(cjson_message, "data", mes_data = cJSON_CreateArray());
		cJSON_AddItemToArray(mes_data, data_item=cJSON_CreateObject());
		cJSON_AddNumberToObject(data_item, "pm25", 345);

		out = cJSON_Print(cjson_message);
		tcp_keeplive_message.data=out;

		tcp_keeplive_message.length = os_strlen(out);

		if(client!=NULL)
		{
			TCP_send_data(client,tcp_keeplive_message.data,tcp_keeplive_message.length);
		}else{
			INFO("the tcp connecting is null\r\n");
		}

		cJSON_Delete(cjson_message);
		os_free(out);

}



void ICACHE_FLASH_ATTR data_process( tcp_message_t* process)
{
	cJSON * root = cJSON_Parse(process->data);
	if (NULL != root) {
			if (cJSON_HasObjectItem(root, "message")) {
				cJSON *string = cJSON_GetObjectItem(root, "message");
				if (cJSON_IsString(string)) {
					char *s = cJSON_Print(string);

					if(memcmp(s,"\"success\"",9)==0)
					{
						//数据正常调用发送函数,time>500ms
						if(process_client!=NULL){
						TCP_send_data(process_client,process->data,process->length);
						}else{
							INFO("the tcp connecting is null\r\n");
						}
					}
					cJSON_free((void *) s);
				}
			}
			cJSON_Delete(root);
		} else {
#ifdef DBUG_MODE
			INFO("\r\nparse error!\r\n");
#endif
		}
}

void ICACHE_FLASH_ATTR tcp_data_process(uint8 *psent, uint16 length)
{
	cJSON * root = cJSON_Parse(psent);
		if (NULL != root) {
				if (cJSON_HasObjectItem(root, "message")) {
					cJSON *string = cJSON_GetObjectItem(root, "message");
					if (cJSON_IsString(string)) {
						char *s = cJSON_Print(string);

						if(memcmp(s,"\"success\"",9)==0)
						{
						//数据正常，发送到对应的设备解析
							uart0_tx_buffer(psent,length);
						}
						cJSON_free((void *) s);
					}
				}
				cJSON_Delete(root);
			}
		else {

			}
}

