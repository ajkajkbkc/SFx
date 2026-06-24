/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "FreeRTOS.h"
#include "timers.h"
#include "queue.h"
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;

extern UART_HandleTypeDef huart2;

extern UART_HandleTypeDef huart3;

/* USER CODE BEGIN Private defines */
#define UART1_RX_TIMER_ID   1
#define UART2_RX_TIMER_ID   2
#define UART3_RX_TIMER_ID   3

#define RX_LEN_UART1      1024
#define RX_LEN_UART2      1024
#define RX_LEN_UART3      1024

/*UART状态定义*/
enum __UART_STATUS_E
{
    /*端口空闲*/
    UART_STATE_IDLE = 0x01,
    /*接收*/
    UART_STATE_RX = 0x02,
    /*发送*/
    UART_STATE_TX = 0x04,
};

/*UART端口定义*/
enum __UART_PORT_E
{
    /* 串口1 */
    UART_PORT1 = 1,
    /* 串口2 */
    UART_PORT2 = 2,
    /* 串口3 */
    UART_PORT3 = 3,
    /*  */
    MAX_SUPPORT_UART_PORT,
};

/*UART工作模式定义*/
enum __UART_MODE_E
{
    /*默认*/
    UART_MODE_DEFAULT,
    /*寻找flk终端状态*/
    UART_MODE_FINDFLKRTU,
    /*寻找hlk终端状态*/
    UART_MODE_FINDHLKRTU,
};

/* Exported types ------------------------------------------------------------*/
/*uart 缓冲区定义*/
typedef struct __BSP_UART_STATUS_INFO_ST
{
    /*串口模式：*/
    unsigned char mcv_Mode;
    /*当前端口状态*/
    unsigned char mcv_Status;
    /*接收数据长度计数*/
    unsigned short msv_RxCount;
} bsp_uart_status_info_st;

/* uart任务消息队列结构体 */
typedef struct __UART_MSG_ST
{
    /*设备UART端口号*/
    unsigned char mcv_UartPort;
    /*数据长度*/
    unsigned short msv_MsgLength;
    /*数据缓存区指针*/
    unsigned char *mcp_DataBuff;
} uart_msg_st;

/*串口系统块配置字解析结构体*/
typedef struct __BSP_UART_CONFIG_ST
{
    /*端口波特率配置
    000 = 1200, 001 = 2400, 010 = 4800, 011 = 9600, 100 = 19200, 101 = 38400, 110 = 57600; 111 = 115200
    */
    unsigned short BaudRate     : 3;
    /*停止位 0：1位, 1: 2位*/
    unsigned short StopBits     : 1;
    /*校验位 0: 偶校验, 1: 奇校验*/
    unsigned short Parity       : 1;
    /*校验使能     0：不校验, 1: 校验*/
    unsigned short ParityEnable : 1;
    /*数据长度      0: 8, 1: 7*/
    unsigned short WordLength   : 1;

    /*自由口起始字节允许*/
    unsigned short StartWordEn  : 1;
    /*自由口结束字节允许*/
    unsigned short EndWordEn    : 1;
    /*自由口字符间超时时间使能*/
    unsigned short CharTimeOutEn    : 1;
    /*自由口帧间超时时间使能*/
    unsigned short FrameTimeOutEn   : 1;

    /*Modbus传输模式 0：RTU, 1: ASCII*/
    unsigned short ModbusTransMode  : 1;
    /*Modbus主从模式 0：从站, 1：主站*/
    unsigned short ModebusMode      : 1;

    /*协议类型
    001: 自由口 010：Modbus 011：KCBus
    */
    unsigned short ProtocolType     : 3;
} bsp_uart_config_st;

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);
void MX_USART3_UART_Init(void);

/* USER CODE BEGIN Prototypes */
static void bsp_uart_rx_time_init(TimerHandle_t *pxTimerHandle,uint32_t ulTimerID,const char *pcTimerName,unsigned short usRxPeriod);
static void bsp_timer_callback_func(TimerHandle_t ltv_TimeHandle);
void uartReceive_IDLE_FromISR(UART_HandleTypeDef *huart);

void HandleUART1RecvData(unsigned char *lcp_Buff, unsigned short lsv_Length);
void HandleUART2RecvData(unsigned char *lcp_Buff, unsigned short lsv_Length);
void HandleUART3RecvData(unsigned char *lcp_Buff, unsigned short lsv_Length);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

