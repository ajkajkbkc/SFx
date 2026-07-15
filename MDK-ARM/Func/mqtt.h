#ifndef _MQTT_H
#define _MQTT_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private defines -----------------------------------------------------------*/
#define  MQTT_NO_PING           0
#define  MQTT_PING_SEND_OK      1
#define  MQTT_PING_SEND_ERR     2
#define  MQTT_PING_RESP_OK      3
#define  MQTT_PING_RESP_ERR     4

#define  MQTT_SEND_OK           0      //MQTT发送成功
#define  MQTT_SEND_ERR          1      //MQTT发送失败
#define  MQTT_SEND_ERROVER      2      //MQTT发送失败并且超过失败次数

#define  MQTT_ALL_ALM_INTERVAL  2      //轮询报警时间间隔(秒s)

#define  MQTTINIT_ERROR_TIMES   12     //MQTT初始化失败多少次即复位W5500
#define  PUBLISH_ERROR_TIMES    3      //publish失败多少次即关闭socket重新初始化MQTT
#define  MQTTPING_ERROR_TIMES   3      //MQTT ping失败多少次即置位NET_STATE_MQTT
#define  NEXT_TIME_OF_MQTTINIT  15000  //mqtt初始化失败后，下次重试初始化的时间(ms)
#define  MQTT_PING_RECV_MAXTIME 5000   //MQTT ping最大接收时间(ms)，超过时间没有收到ping包则错误
/* Exported types ------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
extern uint8_t gMqttInitErrorTimes;
extern uint8_t gPublishErrorTimes;
extern uint8_t gMqttPingErrorTimes;

extern __IO uint8_t gMqttPingState;

/* Exported functions prototypes ---------------------------------------------*/
void osThreadNew_mqttTask(void);

void MqttPingTimr_Start(void);

#endif // _MQTT_H
