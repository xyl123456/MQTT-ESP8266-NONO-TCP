/* mqtt.h
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
#ifndef USER_AT_TCP_H_
#define USER_AT_TCP_H_
#include "tcp_config.h"



#include "tcp_config.h"
#include "user_interface.h"

#include "queue.h"

typedef struct tcp_message
{
  uint8_t* data;
  uint16_t length;

} tcp_message_t;


typedef struct tcp_connect_data
{
  tcp_message_t in_message;
  //tcp_message_t out_message;
  uint8_t* data;
  uint16_t length;

} tcp_connect_data_t;

typedef enum {
	WIFI_INIT,
	WIFI_CONNECTING,
	WIFI_CONNECTING_ERROR,
	WIFI_CONNECTED,
	DNS_RESOLVE,
	TCP_DISCONNECTING,
	TCP_DISCONNECTED,
	TCP_RECONNECT_DISCONNECTING,
	TCP_RECONNECT_REQ,
	TCP_RECONNECT,
	TCP_CONNECTING,
	TCP_CONNECTING_ERROR,
	TCP_CONNECTED,
	TCP_DATA,
	TCP_KEEPALIVE_SEND,
} tcpConnState;

typedef struct tcp_state_t
{
  uint8_t* in_buffer;
  uint8_t* out_buffer;
  int in_buffer_length;
  int out_buffer_length;
  //tcp_connect_data_t tcp_data;
 //tcp_message_t* outbound_message;

  tcp_connect_data_t uart_data;
  tcp_message_t* inbound_message;
} tcp_state_t;

typedef struct  {
	struct espconn *pCon;
	uint8_t security;
	uint8_t* host;
	uint32_t port;
	ip_addr_t ip;

	tcp_state_t  tcp_state;//

	ETSTimer tcpTimer;
	uint32_t keepAliveTick;
	uint32_t reconnectTick;
	uint32_t sendTimeout;//sendTimeout是判断处于发送，大于0表示不发送。

	tcpConnState connState;//tcp的连接状态

	QUEUE msgQueue;

	void* user_data;

} TCP_Client;


#endif /* USER_AT_TCP_H_ */
