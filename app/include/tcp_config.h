#ifndef __TCP_CONFIG_H__
#define __TCP_CONFIG_H__

#define 		TCP_CFG_HOLDER      0x00FF55A4    /* Change this value to load default configurations */
#define 		TCP_CLIENT_ID      "cliend123"
#define 		TCP_HOST			"192.168.99.243"
#define         TCP_PORT			1883

#define 		STA_SSID 			"newwifi"    // your AP/router SSID to config your device networking
#define 		STA_PASS 			"xyl123456" // your AP/router password


typedef enum{
  NO_TLS = 0,                       // 0: disable SSL/TLS, there must be no certificate verify between MQTT server and ESP8266
  TLS_WITHOUT_AUTHENTICATION = 1,   // 1: enable SSL/TLS, but there is no a certificate verify
  ONE_WAY_ANTHENTICATION = 2,       // 2: enable SSL/TLS, ESP8266 would verify the SSL server certificate at the same time
  TWO_WAY_ANTHENTICATION = 3,       // 3: enable SSL/TLS, ESP8266 would verify the SSL server certificate and SSL server would verify ESP8266 certificate
}TLS_LEVEL;


#define DEFAULT_SECURITY    NO_TLS      // very important: you must config DEFAULT_SECURITY for SSL/TLS

#define CA_CERT_FLASH_ADDRESS      0x77              // CA certificate address in flash to read, 0x77 means address 0x77000
#define CLIENT_CERT_FLASH_ADDRESS  0x78          // client certificate and private key address in flash to read, 0x78 means address 0x78000
/***********************************************************************************************************************/

#define TCP_BUF_SIZE        1024
#define TCP_KEEPALIVE        120     /*second*/
#define TCP_RECONNECT_TIMEOUT     5    /*second*/

/*Please Keep the following configuration if you have no very deep understanding of ESP SSL/TLS*/
#define CFG_LOCATION    0x79    /* Please don't change or if you know what you doing */

#define TCP_BUF_SIZE        1024
#define QUEUE_BUFFER_SIZE                 2048

#define STA_TYPE AUTH_WPA2_PSK


#endif // __TCP_CONFIG_H__
