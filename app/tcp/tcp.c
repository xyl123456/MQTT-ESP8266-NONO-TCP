/* mqtt.c
*  Protocol: http://docs.oasis-open.org/mqtt/mqtt/v3.1.1/os/mqtt-v3.1.1-os.html
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

#include "user_interface.h"
#include "osapi.h"
#include "espconn.h"
#include "os_type.h"
#include "mem.h"

#include "user_config.h"
#include "tcp/tcp.h"
#include "tcp/debug.h"

#include "process_data.h"



#define TCP_TASK_PRIO                2
#define TCP_TASK_QUEUE_SIZE        1
#define TCP_SEND_TIMOUT            5


os_event_t tcp_procTaskQueue[TCP_TASK_QUEUE_SIZE];




#ifndef QUEUE_BUFFER_SIZE
#define QUEUE_BUFFER_SIZE             2048
#endif

void ICACHE_FLASH_ATTR
TCP_InitConnection(TCP_Client *tcpClient, uint8_t* host, uint32_t port, uint8_t security)
{
    uint32_t temp;
#ifdef DBUG_MODE
    INFO("TCP_InitConnection\r\n");
#endif
    os_memset(tcpClient, 0, sizeof(TCP_Client));
    temp = os_strlen(host);
    tcpClient->host = (uint8_t*)os_zalloc(temp + 1);
    os_strcpy(tcpClient->host, host);
    tcpClient->host[temp] = 0;
    tcpClient->port = port;
    tcpClient->security = security;
}


/**
  * @brief  Delete tcp client and free all memory
  * @param  tcpClient:
  * @retval None
  */
void ICACHE_FLASH_ATTR
tcp_tcpclient_delete(TCP_Client *tcpClient)
{
    if (tcpClient->pCon != NULL) {
        INFO("Free memory\r\n");
        espconn_delete(tcpClient->pCon);
        if (tcpClient->pCon->proto.tcp)
            os_free(tcpClient->pCon->proto.tcp);
        os_free(tcpClient->pCon);
        tcpClient->pCon = NULL;
        //
        process_client=NULL;
    }

}

void ICACHE_FLASH_ATTR
tcp_tcpclient_discon_cb(void *arg)
{
    struct espconn *pespconn = (struct espconn *)arg;
    TCP_Client* client = (TCP_Client *)pespconn->reverse;
#ifdef DBUG_MODE
    INFO("TCP: Disconnected callback\r\n");
#endif
    if(TCP_DISCONNECTING == client->connState) {
    	//连接的信息错误，需要用户重新配置
        client->connState = TCP_DISCONNECTED;
    }
    else {
    	//连接中断开，需要重新连接
        client->connState = TCP_RECONNECT_REQ;
    }
    //用户是否注册了断开后的处理函数,默认没有定义
    /*
    if (client->disconnectedCb)
        client->disconnectedCb((uint32_t*)client);
    */
    system_os_post(TCP_TASK_PRIO, 0, (os_param_t)client);
}

static tcp_message_t* ICACHE_FLASH_ATTR fill_in_message(tcp_state_t * tcp_state)
{
	tcp_state->uart_data.in_message.length=tcp_state->in_buffer_length;
	tcp_state->uart_data.in_message.data=tcp_state->in_buffer;
	return &tcp_state->uart_data.in_message;

}
/**
  * @brief  Client received callback function.
  * @param  arg: contain the ip link information
  * @param  pdata: received data
  * @param  len: the lenght of received data
  * @retval None
  */
void ICACHE_FLASH_ATTR
tcp_tcpclient_recv(void *arg, char *pdata, unsigned short len)
{
	 struct espconn *pCon = (struct espconn*)arg;
	 TCP_Client *client = (TCP_Client *)pCon->reverse;

	 client->keepAliveTick = 0;

	 if (len < TCP_BUF_SIZE && len > 0) {

		 os_memcpy(client->tcp_state.in_buffer, pdata, len);
		 client->tcp_state.in_buffer_length=len;

		 client->tcp_state.inbound_message=fill_in_message(&client->tcp_state);

	 }
	 switch (client->connState){
	 	 case TCP_DATA:
	 	 case TCP_KEEPALIVE_SEND:
	 		 /*
	 		  * 用户定义的处理接收函数
	 		  */
	 		 /*
	 		  *
	 		 if (client->tcprecvedCb)
	 		    client->tcprecvedCb((uint32_t*)client);
	 		            */
	 		 if (QUEUE_Puts(&client->msgQueue, client->tcp_state.inbound_message->data,
	 				 client->tcp_state.inbound_message->length) == -1) {
	 			INFO("TCP: Queue full\r\n");
	 		 }
	 		//tcp_data_process(client->tcp_state.outbound_message);

	 		 break;
	 	 default:
			 break;
	 }
	 system_os_post(TCP_TASK_PRIO, 0, (os_param_t)client);
}


BOOL ICACHE_FLASH_ATTR
TCP_send_data(TCP_Client *client, char* data, int data_length)
{

    uint8_t dataBuffer[TCP_BUF_SIZE];
    uint16_t dataLen;
    err_t result = ESPCONN_OK;

	client->keepAliveTick = 0;

	 result = espconn_send(client->pCon,data,data_length);

    if(ESPCONN_OK == result) {
           client->keepAliveTick = 0;
           client->connState = TCP_DATA;

       }
       else {
    	   INFO("TCP IS MISSING!!!\r\n");
           client->connState = TCP_RECONNECT_DISCONNECTING;
       }
    system_os_post(TCP_TASK_PRIO, 0, (os_param_t)client);
    return TRUE;
}



/**
  * @brief  Client send over callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
void ICACHE_FLASH_ATTR
tcp_tcpclient_sent_cb(void *arg)
{
    struct espconn *pCon = (struct espconn *)arg;
    TCP_Client* client = (TCP_Client *)pCon->reverse;
#ifdef DBUG_MODE
    INFO("TCP: Sent\r\n");
#endif
    client->sendTimeout = 0;
    client->keepAliveTick =0;//keepactivetick recont times

    if (client->connState == TCP_DATA || client->connState == TCP_KEEPALIVE_SEND) {
    	//如果用户定义发送完成，处理函数
    	/*
        if (client->publishedCb)
            client->publishedCb((uint32_t*)client);
            */
    }
    //system_os_post(TCP_TASK_PRIO, 0, (os_param_t)client);
}

/**
  * @brief  Tcp client connect success callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
void ICACHE_FLASH_ATTR
tcp_tcpclient_connect_cb(void *arg)
{
    struct espconn *pCon = (struct espconn *)arg;
    TCP_Client* client = (TCP_Client *)pCon->reverse;
    process_client=client;

    espconn_regist_disconcb(client->pCon, tcp_tcpclient_discon_cb);/////主动断开回调
    espconn_regist_recvcb(client->pCon, tcp_tcpclient_recv);////////接收回调
    espconn_regist_sentcb(client->pCon, tcp_tcpclient_sent_cb);///////发送回调函数
#ifdef DBUG_MODE
    INFO("TCP: Connected to broker %s:%d\r\n", client->host, client->port);
#endif
    client->sendTimeout = TCP_SEND_TIMOUT;
    client->connState = TCP_DATA;
    system_os_post(TCP_TASK_PRIO, 0, (os_param_t)client);
}

/**
  * @brief  Tcp client connect repeat callback function.
  * @param  arg: contain the ip link information
  * @retval None
  */
void ICACHE_FLASH_ATTR
tcp_tcpclient_recon_cb(void *arg, sint8 errType)
{
    struct espconn *pCon = (struct espconn *)arg;
    TCP_Client* client = (TCP_Client *)pCon->reverse;

    INFO("TCP: Reconnect to %s:%d\r\n", client->host, client->port);

    client->connState = TCP_RECONNECT_REQ;

    system_os_post(TCP_TASK_PRIO, 0, (os_param_t)client);

}

void ICACHE_FLASH_ATTR tcp_timer(void *arg)
{
    TCP_Client* client = (TCP_Client*)arg;

    if (client->connState == TCP_DATA) {
        client->keepAliveTick ++;
        if (client->keepAliveTick > TCP_KEEPALIVE) {
            client->connState =TCP_KEEPALIVE_SEND;//TCP超时连接，发送心跳数据，证明设备在线
            system_os_post(TCP_TASK_PRIO, 0, (os_param_t)client);
        }

    } else if (client->connState == TCP_RECONNECT_REQ) {
        client->reconnectTick ++;
        //等待MQTT_RECONNECT_TIMEOUT时间以后，如果还没有连接成功，再次发送消息给MQTT_WORKER
        //并且调用超时函数,建立TCP重新连接
        if (client->reconnectTick > TCP_RECONNECT_TIMEOUT) {
        	INFO("SEND RECONNECT SINGLE\r\n");
            client->reconnectTick = 0;
            client->connState = TCP_RECONNECT;
            system_os_post(TCP_TASK_PRIO, 0, (os_param_t)client);
            /*如果用户定义了，重连函数，执行程序连接函数
            if (client->timeoutCb)
                client->timeoutCb((uint32_t*)client);
                */
        }
    }
    if (client->sendTimeout > 0)
        client->sendTimeout --;
}


LOCAL void ICACHE_FLASH_ATTR
tcp_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *pConn = (struct espconn *)arg;
    TCP_Client* client = (TCP_Client *)pConn->reverse;


    if (ipaddr == NULL)
    {
#ifdef DBUG_MODE
        INFO("DNS: Found, but got no ip, try to reconnect\r\n");
#endif
        client->connState = TCP_RECONNECT_REQ;
        return;
    }

    INFO("DNS: found ip %d.%d.%d.%d\n",
         *((uint8 *) &ipaddr->addr),
         *((uint8 *) &ipaddr->addr + 1),
         *((uint8 *) &ipaddr->addr + 2),
         *((uint8 *) &ipaddr->addr + 3));

    if (client->ip.addr == 0 && ipaddr->addr != 0)
    {
        os_memcpy(client->pCon->proto.tcp->remote_ip, &ipaddr->addr, 4);
        if (client->security) {
            espconn_secure_connect(client->pCon);

        }
        else {
            espconn_connect(client->pCon);
        }

        client->connState = TCP_CONNECTING;
#ifdef DBUG_MODE
        INFO("TCP: connecting...\r\n");
#endif
    }
    system_os_post(TCP_TASK_PRIO, 0, (os_param_t)client);
}



/**
  * @brief  Begin connect to TCP broker
  * @param  client: TCP_Client reference
  * @retval None
  */
void ICACHE_FLASH_ATTR
TCP_Connect(TCP_Client *tcpClient)
{
    //espconn_secure_set_size(0x01,6*1024);       // try to modify memory size 6*1024 if ssl/tls handshake failed
	if (tcpClient->pCon) {
        // Clean up the old connection forcefully - using MQTT_Disconnect
        // does not actually release the old connection until the
        // disconnection callback is invoked.
		tcp_tcpclient_delete(tcpClient);
    }
	tcpClient->pCon = (struct espconn *)os_zalloc(sizeof(struct espconn));
	tcpClient->pCon->type = ESPCONN_TCP;
	tcpClient->pCon->state = ESPCONN_NONE;
	tcpClient->pCon->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
	tcpClient->pCon->proto.tcp->local_port = espconn_port();
	tcpClient->pCon->proto.tcp->remote_port = tcpClient->port;//远程tcp服务器端口
	tcpClient->pCon->reverse = tcpClient;//espconn 中的user 参数

    //注册连接回调函数和重新连接回调函数
    espconn_regist_connectcb(tcpClient->pCon, tcp_tcpclient_connect_cb);
    espconn_regist_reconcb(tcpClient->pCon, tcp_tcpclient_recon_cb);

    tcpClient->keepAliveTick = 0;
    tcpClient->reconnectTick = 0;

    //生成定时器维护TCP的连接状态
    os_timer_disarm(&tcpClient->tcpTimer);
    os_timer_setfn(&tcpClient->tcpTimer, (os_timer_func_t *)tcp_timer, tcpClient);
    os_timer_arm(&tcpClient->tcpTimer, 1000, 1);


    //将点分的IP字符，转换为IP数组整数格式
    if (UTILS_StrToIP(tcpClient->host, &tcpClient->pCon->proto.tcp->remote_ip)) {

    	INFO("TCP: Connect to ip  %s:%d\r\n", tcpClient->host, tcpClient->port);

    	if (tcpClient->security)
        {
            espconn_secure_connect(tcpClient->pCon);
        }
        else
        {
        	espconn_connect(tcpClient->pCon);
        }
    }
    else {
#ifdef DBUG_MODE
        INFO("TCP: Connect to domain %s:%d\r\n", tcpClient->host, tcpClient->port);
#endif
        //DNS解析,解析正确返回ESPCONN_OK，ESPCONN_ISCONN表示错误代码，但是已经连接，ESPCONN_ARG表示网络错误
        espconn_gethostbyname(tcpClient->pCon, tcpClient->host, &tcpClient->ip, tcp_dns_found);
    }
    tcpClient->connState = TCP_CONNECTING;
}


void ICACHE_FLASH_ATTR
TCP_Disconnect(TCP_Client *tcpClient)
{
    tcpClient->connState = TCP_DISCONNECTING;
    system_os_post(TCP_TASK_PRIO, 0, (os_param_t)tcpClient);
    os_timer_disarm(&tcpClient->tcpTimer);
}


void ICACHE_FLASH_ATTR
TCP_Task(os_event_t *e)
{
	 TCP_Client* client = (TCP_Client*)e->par;
	 uint8_t dataBuffer[TCP_BUF_SIZE];
	 uint16_t dataLen;
	 err_t result;
	    if (e->par == 0)
	        return;
	 switch (client->connState) {

	    case TCP_RECONNECT_REQ:
	        break;
	    case TCP_CONNECTED:
	    	break;
	    case TCP_RECONNECT:
	        tcp_tcpclient_delete(client);
	        TCP_Connect(client);
	#ifdef DBUG_MODE
	        INFO("TCP: Reconnect to: %s:%d\r\n", client->host, client->port);
	#endif
	        client->connState = TCP_CONNECTING;
	        break;
	    case TCP_DISCONNECTING:
	    case TCP_RECONNECT_DISCONNECTING:
	        if (client->security) {
	            espconn_secure_disconnect(client->pCon);
	        }
	        else {
	        	//断开当前连接，重新连接
	        	result=espconn_disconnect(client->pCon);//回调用对应的回调函数

	        	if(ESPCONN_OK == result){
	        		INFO("DISCONNECT IS OK\r\n");
	        	}else
	        	{
	        		//断开失败，清空原来状态，重新连接，发送重新练级请求
	        		client->connState = TCP_RECONNECT_REQ;
	        		INFO("DISCONNECT IS BAD\r\n");
	        	}

	        }
	        break;
	    case TCP_DISCONNECTED:
	#ifdef DBUG_MODE
	        INFO("TCP: Disconnected\r\n");
	#endif
	        tcp_tcpclient_delete(client);
	        break;
	    case TCP_KEEPALIVE_SEND:
	        tcp_send_keepalive(client);
	        break;
	        //TCP_DATA is send data state
	    case TCP_DATA:
	        if (QUEUE_IsEmpty(&client->msgQueue)) {
	            break;
	        }
	        if (QUEUE_Gets(&client->msgQueue, dataBuffer, &dataLen, TCP_BUF_SIZE) == 0) {

	            client->sendTimeout = TCP_SEND_TIMOUT;
	            //处理TCP的接收数据，通过串口发送
	            tcp_data_process(dataBuffer, dataLen);

	            client->tcp_state.inbound_message = NULL;
	            break;
	        }
	        break;
	    }
}


void ICACHE_FLASH_ATTR
TCP_InitClient(TCP_Client *tcpClient)
{
	INFO("TCP CLIENT INIT\r\n");
	//初始化buffer
	 tcpClient->tcp_state.in_buffer = (uint8_t *)os_zalloc(TCP_BUF_SIZE);
	 tcpClient->tcp_state.in_buffer_length = TCP_BUF_SIZE;
	 tcpClient->tcp_state.out_buffer =  (uint8_t *)os_zalloc(TCP_BUF_SIZE);
	 tcpClient->tcp_state.out_buffer_length = TCP_BUF_SIZE;

	 QUEUE_Init(&tcpClient->msgQueue, QUEUE_BUFFER_SIZE);

	//创建系统任务，最多可以创建3个优先级系统任务0.1.2,参数为任务函数，任务优先级，消息指针队列，消息深度1
	 system_os_task(TCP_Task, TCP_TASK_PRIO, tcp_procTaskQueue, TCP_TASK_QUEUE_SIZE);
	 system_os_post(TCP_TASK_PRIO, 0, (os_param_t)tcpClient);

}

