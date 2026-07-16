/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "EC20Task.h"      /* 转发USART2数据给EC20任务 */

/* USER CODE BEGIN 0 */
volatile bsp_uart_status_info_st gtv_UartPortStatus[MAX_SUPPORT_UART_PORT];

uint8_t gcv_Uart1RecvBuf[RX_LEN_UART1];
uint8_t gcv_Uart2RecvBuf[RX_LEN_UART2];
uint8_t gcv_Uart3RecvBuf[RX_LEN_UART3];

QueueHandle_t gtv_UartTaskMsgQueueHandle;

/*定时器句柄*/
TimerHandle_t ltv_Uart1RxTime;
TimerHandle_t ltv_Uart2RxTime;
TimerHandle_t ltv_Uart3RxTime;
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 9600;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */
  gtv_UartPortStatus[UART_PORT1].mcv_Status = UART_STATE_IDLE;
  gtv_UartPortStatus[UART_PORT1].mcv_Mode = UART_MODE_DEFAULT;
  bsp_uart_rx_time_init(&ltv_Uart1RxTime, UART1_RX_TIMER_ID, "Uart1 RxTime", 10);  // 10ms超时

  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
  HAL_NVIC_EnableIRQ(USART1_IRQn);  //使能串口中断
  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
  gtv_UartPortStatus[UART_PORT2].mcv_Status = UART_STATE_IDLE;
  gtv_UartPortStatus[UART_PORT2].mcv_Mode = UART_MODE_DEFAULT;
 bsp_uart_rx_time_init(&ltv_Uart2RxTime, UART2_RX_TIMER_ID, "Uart2 RxTime", 10);  // 10ms超时

  __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);
  HAL_NVIC_EnableIRQ(USART2_IRQn);  //使能串口中断
  /* USER CODE END USART2_Init 2 */

}
/* USART3 init function */

void MX_USART3_UART_Init(void)
{

  /* USER CODE BEGIN USART3_Init 0 */

  /* USER CODE END USART3_Init 0 */

  /* USER CODE BEGIN USART3_Init 1 */

  /* USER CODE END USART3_Init 1 */
  huart3.Instance = USART3;
  huart3.Init.BaudRate = 9600;
  huart3.Init.WordLength = UART_WORDLENGTH_8B;
  huart3.Init.StopBits = UART_STOPBITS_1;
  huart3.Init.Parity = UART_PARITY_NONE;
  huart3.Init.Mode = UART_MODE_TX_RX;
  huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart3.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART3_Init 2 */
  gtv_UartPortStatus[UART_PORT3].mcv_Status = UART_STATE_IDLE;
  gtv_UartPortStatus[UART_PORT3].mcv_Mode = UART_MODE_DEFAULT;
  bsp_uart_rx_time_init(&ltv_Uart3RxTime, UART3_RX_TIMER_ID, "Uart3 RxTime", 10);  // 10ms超时

  __HAL_UART_ENABLE_IT(&huart3, UART_IT_RXNE);
  HAL_NVIC_EnableIRQ(USART3_IRQn);  //使能串口中断
  /* USER CODE END USART3_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */
    HAL_NVIC_DisableIRQ(USART1_IRQn);  //先禁止串口中断，创建定时器后再开启中断
  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */
    HAL_NVIC_DisableIRQ(USART2_IRQn);  //先禁止串口中断，创建定时器后再开启中断
  /* USER CODE END USART2_MspInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspInit 0 */

  /* USER CODE END USART3_MspInit 0 */
    /* USART3 clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    /**USART3 GPIO Configuration
    PC10     ------> USART3_TX
    PC11     ------> USART3_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

    __HAL_AFIO_REMAP_USART3_PARTIAL();

    /* USART3 interrupt Init */
    HAL_NVIC_SetPriority(USART3_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspInit 1 */
    HAL_NVIC_DisableIRQ(USART3_IRQn);  //先禁止串口中断，创建定时器后再开启中断
  /* USER CODE END USART3_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART3)
  {
  /* USER CODE BEGIN USART3_MspDeInit 0 */

  /* USER CODE END USART3_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART3_CLK_DISABLE();

    /**USART3 GPIO Configuration
    PC10     ------> USART3_TX
    PC11     ------> USART3_RX
    */
    HAL_GPIO_DeInit(GPIOC, GPIO_PIN_10|GPIO_PIN_11);

    /* USART3 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART3_IRQn);
  /* USER CODE BEGIN USART3_MspDeInit 1 */

  /* USER CODE END USART3_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/**
 * @brief 通用串口接收超时定时器初始化
 * @param pxTimerHandle   指向定时器句柄变量的指针（例如 &ltv_Uart1RxTime）
 * @param ulTimerID       定时器ID，用于回调中识别端口（例如 UART1_RX_TIMER_ID）
 * @param pcTimerName     定时器名称字符串
 * @param usRxPeriod      超时周期（毫秒），若为0则使用默认100ms
 * @retval None
 */
static void bsp_uart_rx_time_init(TimerHandle_t *pxTimerHandle,
                                  uint32_t ulTimerID,
                                  const char *pcTimerName,
                                  unsigned short usRxPeriod)
{
    if (usRxPeriod == 0) {
        usRxPeriod = 100;
    }

    // 仅当定时器句柄尚未创建（为NULL）且周期有效时创建
    if (*pxTimerHandle == NULL && usRxPeriod > 0) {
        *pxTimerHandle = xTimerCreate(
            pcTimerName,
            (TickType_t)(usRxPeriod / portTICK_PERIOD_MS),
            (UBaseType_t)pdFALSE,
            (void *)ulTimerID,
            (TimerCallbackFunction_t)bsp_timer_callback_func
        );
        // 可添加创建失败的断言或错误处理
        configASSERT(*pxTimerHandle);
    }
}
/**
  * @brief  定时器回调函数（统一处理三个串口接收超时）
  * @param  ltv_TimeHandle 定时器句柄
  * @retval None
  */
static void bsp_timer_callback_func(TimerHandle_t ltv_TimeHandle)
{
    unsigned int liv_TimerId;
    uart_msg_st ltv_UartMsg;
    BaseType_t ltv_Err;
    unsigned char ucPort = 0;
    unsigned char *pDataBuf = NULL;
    unsigned short usDataLen = 0;

    liv_TimerId = (unsigned int)pvTimerGetTimerID(ltv_TimeHandle);

    // 1. 根据定时器ID确定端口号和对应的接收缓冲区
    if (liv_TimerId == UART1_RX_TIMER_ID) {
        ucPort = UART_PORT1;
        pDataBuf = gcv_Uart1RecvBuf;
    } else if (liv_TimerId == UART2_RX_TIMER_ID) {
        ucPort = UART_PORT2;
        pDataBuf = gcv_Uart2RecvBuf;
    } else if (liv_TimerId == UART3_RX_TIMER_ID) {
        ucPort = UART_PORT3;
        pDataBuf = gcv_Uart3RecvBuf;
    } else {
        // 未知定时器ID，直接退出
        return;
    }

    // 2. 检查端口是否处于接收状态，若不是则忽略本次超时
    if (gtv_UartPortStatus[ucPort].mcv_Status != UART_STATE_RX) {
        return;
    }

    // 3. 将状态改为空闲，并保存当前接收长度
    gtv_UartPortStatus[ucPort].mcv_Status = UART_STATE_IDLE;
    usDataLen = gtv_UartPortStatus[ucPort].msv_RxCount;

    // 4. 无论数据长度是否有效，先清空计数，避免影响下一帧
    gtv_UartPortStatus[ucPort].msv_RxCount = 0;

    // 5. 校验数据长度：必须大于0且不超过接收缓冲区大小
    if (usDataLen == 0 || usDataLen >= sizeof(gcv_Uart1RecvBuf)) {
        // 无效数据，直接丢弃（可增加无效包计数）
        // gtv_UartInvalidCnt[ucPort]++;
        return;
    }

    // 6. 填充消息结构体
    ltv_UartMsg.mcv_UartPort = ucPort;
    ltv_UartMsg.msv_MsgLength = usDataLen;
    ltv_UartMsg.mcp_DataBuff = pDataBuf;

    // 7. 发送到队列，阻塞时间为0（定时器回调中严禁阻塞）
    ltv_Err = xQueueSend(gtv_UartTaskMsgQueueHandle, &ltv_UartMsg, 0);
    if (ltv_Err != pdPASS) {
        // 发送失败（队列满），丢弃该包并累计丢包计数（外部定义）
        // gtv_UartDropCount[ucPort]++;
        // 这里不打印日志，避免在回调中执行耗时操作
    }
}
/**
  * @brief  串口接收中断处理（由UART的RXNE中断调用）
  * @param  huart  UART句柄指针
  * @retval None
  */
void uartReceive_IDLE_FromISR(UART_HandleTypeDef *huart)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t ucData;
    unsigned char ucPort = 0;              // 端口号（UART_PORT1等）
    uint8_t *pRecvBuf = NULL;              // 指向对应端口的接收缓冲区
    volatile uint16_t *pRxCount = NULL;    // 指向对应端口的接收计数
    TimerHandle_t *pTimerHandle = NULL;    // 指向对应端口的定时器句柄
    uint16_t usMaxLen = 0;                 // 对应端口的缓冲区最大长度

    // 1. 根据UART实例确定资源
    if (huart->Instance == USART1) {
        ucPort = UART_PORT1;
        pRecvBuf = gcv_Uart1RecvBuf;
        pRxCount = &gtv_UartPortStatus[UART_PORT1].msv_RxCount;
        pTimerHandle = &ltv_Uart1RxTime;
        usMaxLen = RX_LEN_UART1;
    } else if (huart->Instance == USART2) {
        ucPort = UART_PORT2;
        pRecvBuf = gcv_Uart2RecvBuf;
        pRxCount = &gtv_UartPortStatus[UART_PORT2].msv_RxCount;
        pTimerHandle = &ltv_Uart2RxTime;
        usMaxLen = RX_LEN_UART2;
    } else if (huart->Instance == USART3) {
        ucPort = UART_PORT3;
        pRecvBuf = gcv_Uart3RecvBuf;
        pRxCount = &gtv_UartPortStatus[UART_PORT3].msv_RxCount;
        pTimerHandle = &ltv_Uart3RxTime;
        usMaxLen = RX_LEN_UART3;
    } else {
        return;  // 不支持的UART
    }

    // 2. 读取数据寄存器（必须读，否则硬件不会清除RXNE标志）
    ucData = (uint8_t)(huart->Instance->DR & 0x00FF);

    // 3. 根据当前状态处理
    if (gtv_UartPortStatus[ucPort].mcv_Status == UART_STATE_IDLE) {
        // 首次收到数据：状态切换为RX，清空计数，保存数据，启动定时器
        gtv_UartPortStatus[ucPort].mcv_Status = UART_STATE_RX;
        *pRxCount = 0;
        pRecvBuf[(*pRxCount)++] = ucData;

        if (xTimerStartFromISR(*pTimerHandle, &xHigherPriorityTaskWoken) != pdPASS) {
            // 启动失败，可增加错误计数（避免在ISR中打印）
            // gtv_TimerStartErrorCnt[ucPort]++;
        }
    } else if (gtv_UartPortStatus[ucPort].mcv_Status == UART_STATE_RX) {
        // 正在接收过程中
        if (*pRxCount < usMaxLen) {
            pRecvBuf[(*pRxCount)++] = ucData;
        } else {
            // 缓冲区已满，只读数据以清除RXNE，但不保存（丢弃溢出字节）
            // 空操作：已读取ucData，无需额外操作
        }

        if (xTimerResetFromISR(*pTimerHandle, &xHigherPriorityTaskWoken) != pdPASS) {
            // 重置失败，可增加错误计数
            // gtv_TimerResetErrorCnt[ucPort]++;
        }
    }

    // 4. 如果有更高优先级的任务被唤醒，则进行上下文切换
    if (xHigherPriorityTaskWoken != pdFALSE) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

/**
  * @brief  处理从串口1收到的数据
  * @param  pBuf 数据起始地址
  * @param  cyLen 数据长度
  * @retval None
  */
void HandleUART1RecvData(unsigned char *lcp_Buff, unsigned short lsv_Length)
{
    if(gtv_UartPortStatus[UART_PORT1].mcv_Mode == UART_MODE_DEFAULT)
    {
#if (APP_IAP == 1)
        if(is_iap_update(lcp_Buff, lsv_Length))
        {
            HAL_NVIC_SystemReset();
        }
#endif
    }
}

/**
  * @brief  处理从串口2收到的数据
  * @param  pBuf 数据起始地址
  * @param  cyLen 数据长度
  * @retval None
  */
void HandleUART2RecvData(unsigned char *lcp_Buff, unsigned short lsv_Length)
{
    /* USART2连接EC20模块，转发数据给EC20任务处理 */
    EC20_UART_RxCallback(lcp_Buff, lsv_Length);
}

/**
  * @brief  处理从串口3收到的数据
  * @param  pBuf 数据起始地址
  * @param  cyLen 数据长度
  * @retval None
  */
void HandleUART3RecvData(unsigned char *lcp_Buff, unsigned short lsv_Length)
{
}
/* USER CODE END 1 */

