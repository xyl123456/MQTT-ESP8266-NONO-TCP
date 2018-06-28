/*
 * process_data.h
 *
 *  Created on: 2018Äê6ÔÂ6ÈÕ
 *      Author: Administrator
 */

#ifndef APP_INCLUDE_PROCESS_DATA_H_
#define APP_INCLUDE_PROCESS_DATA_H_

#include "os_type.h"
#include "mem.h"
#include "tcp/tcp.h"

typedef struct process_data
{
  uint8_t* data;
  uint16_t length;

} process_data_t;

extern TCP_Client *process_client;
extern void fill_in_keeplive_message(tcp_message_t tcp_keep_mess);

extern void tcp_data_process_byte(uint8 *psent, uint16 length);
extern void data_process_byte(tcp_message_t* process);

extern void data_process( tcp_message_t*process_data);
extern void tcp_data_process(uint8 *psent, uint16 length);
#endif /* APP_INCLUDE_PROCESS_DATA_H_ */
