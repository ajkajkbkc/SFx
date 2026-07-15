#ifndef _W5500TASK_H
#define _W5500TASK_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private defines -----------------------------------------------------------*/
#define PROTOCOL_TYPE                 (PROTOCOL_FLKMQTT)
/* ----------------------- 通讯协议 --------------------------- */
#define PROTOCOL_XX                   0xFFFF
#define PROTOCOL_MB                   0x0000
#define PROTOCOL_MBTCP                0x0001  //modbusTCP协议
#define PROTOCOL_FLKMQTT              0x0002  //FexlinkMQTT协议
#define PROTOCOL_HZUDP                0x0004  //华咨UDP协议
#define PROTOCOL_LLTUDP               0x0008  //路路通UDP协议
/* ------------------------------------------------------------ */
/* ------------------------socket编号-------------------------- */
#define   APP_SOCKET_HUAZIUDP  0
#define   APP_SOCKET_MQTT      1
#define   APP_SOCKET_SNTP      2
#define   APP_SOCKET_MODBUS    3
#define   APP_SOCKET_DHCP      4
#define   APP_SOCKET_DNS       5
#define   APP_SOCKET_HTTPS     6
#define   APP_SOCKET_NETBIOS   7
/* ------------------------------------------------------------ */
/*
    NetLink_State 寄存器位定义 (对应 gParam.st.NetLink_State)
    每个位表示一个网络模块的状态：0 = 正常(normal)  1 = 异常(error)
    
    Bit 0 - SPI    : MCU与W5500的SPI通信状态
    Bit 1 - PHYLINK: 以太网物理连接状态
    Bit 2 - DHCP   : DHCP地址获取状态
    Bit 3 - DNS    : DNS解析状态
    Bit 4 - SNTP   : SNTP时间同步状态
    Bit 5 - MQTT   : MQTT客户端连接状态
    Bit 6 - UDP    : UDP通信状态
*/
/*!< NetLink_State registers */
/*******************  Bit definition for NetLink_State register  *******************/
#define NET_STATE_SPI_Pos                      (0U)                         /* SPI通信状态      位号 0 */
#define NET_STATE_SPI_Msk                      (0x1UL << NET_STATE_SPI_Pos) /* SPI通信状态      位掩码 */
#define NET_STATE_SPI                          NET_STATE_SPI_Msk            /* SPI通信状态      位值   */
#define NET_STATE_PHYLINK_Pos                  (1U)                         /* 物理连接状态     位号 1 */
#define NET_STATE_PHYLINK_Msk                  (0x1UL << NET_STATE_PHYLINK_Pos) /* 物理连接状态  位掩码 */
#define NET_STATE_PHYLINK                      NET_STATE_PHYLINK_Msk        /* 物理连接状态     位值   */
#define NET_STATE_DHCP_Pos                     (2U)                         /* DHCP状态         位号 2 */
#define NET_STATE_DHCP_Msk                     (0x1UL << NET_STATE_DHCP_Pos) /* DHCP状态        位掩码 */
#define NET_STATE_DHCP                         NET_STATE_DHCP_Msk           /* DHCP状态         位值   */
#define NET_STATE_DNS_Pos                      (3U)                         /* DNS解析状态      位号 3 */
#define NET_STATE_DNS_Msk                      (0x1UL << NET_STATE_DNS_Pos) /* DNS解析状态      位掩码 */
#define NET_STATE_DNS                          NET_STATE_DNS_Msk            /* DNS解析状态      位值   */
#define NET_STATE_SNTP_Pos                     (4U)                         /* SNTP时间同步状态 位号 4 */
#define NET_STATE_SNTP_Msk                     (0x1UL << NET_STATE_SNTP_Pos) /* SNTP时间同步状态 位掩码 */
#define NET_STATE_SNTP                         NET_STATE_SNTP_Msk           /* SNTP时间同步状态 位值   */
#define NET_STATE_MQTT_Pos                     (5U)                         /* MQTT连接状态     位号 5 */
#define NET_STATE_MQTT_Msk                     (0x1UL << NET_STATE_MQTT_Pos) /* MQTT连接状态    位掩码 */
#define NET_STATE_MQTT                         NET_STATE_MQTT_Msk           /* MQTT连接状态     位值   */
#define NET_STATE_UDP_Pos                      (6U)                         /* UDP通信状态      位号 6 */
#define NET_STATE_UDP_Msk                      (0x1UL << NET_STATE_UDP_Pos) /* UDP通信状态      位掩码 */
#define NET_STATE_UDP                          NET_STATE_UDP_Msk            /* UDP通信状态      位值   */

#define NET_STATE_OK                           (NET_STATE_SPI | NET_STATE_PHYLINK)
/* Exported types ------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
extern osThreadId_t W5500TaskHandle;
extern const osThreadAttr_t W5500Task_attributes;

/* Exported functions prototypes ---------------------------------------------*/
void W5500_Init(void);
void W5500Task(void *argument);
void W5500MutexInit(void);
int W5500MutexLock(void);
int W5500MutexUnlock(void);
#endif // _W5500TASK_H
