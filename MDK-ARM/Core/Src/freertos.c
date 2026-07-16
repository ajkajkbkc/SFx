/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
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
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
#include <string.h>
#include "usart.h"
#include "ntc.h"
#include "W5500Task.h"
#include "parameter.h"
#include "log.h"
#include "iwdg.h"
#include "rtc.h"
#include "collect.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
extern volatile bsp_uart_status_info_st gtv_UartPortStatus[MAX_SUPPORT_UART_PORT];
/* uart recevie buffer */
extern uint8_t gcv_Uart1RecvBuf[RX_LEN_UART1];
extern uint8_t gcv_Uart2RecvBuf[RX_LEN_UART2];
extern uint8_t gcv_Uart3RecvBuf[RX_LEN_UART3];
/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for UartTask */
osThreadId_t UartTaskHandle;
const osThreadAttr_t UartTask_attributes = {
  .name = "UartTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern QueueHandle_t gtv_UartTaskMsgQueueHandle;
/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);
void vUartTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* creation of UartTask */
  UartTaskHandle = osThreadNew(vUartTask, NULL, &UartTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  ntcTaskHandle = osThreadNew(NtcTask, NULL, &ntcTask_attributes);
  W5500TaskHandle = osThreadNew(W5500Task, NULL, &W5500Task_attributes);
  collectTaskHandle = osThreadNew(collectTask, NULL, &collectTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
		osDelay(1000);
    HAL_IWDG_Refresh(&hiwdg);
    gParam.st.SecCnt++;
    gFlashParam.st.SecCntAll++;
    if(gParam.st.SecCnt % 3 == 0)  //save SecCntAll in BKP
    {
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR41, gFlashParam.st.SecCntAll);
        HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR42, (gFlashParam.st.SecCntAll >> 16));
    }

#if (PRINT_LOG_OPEN == 1)
    UBaseType_t uxArraySize, x;
    TaskStatus_t *pxTaskStatusArray;

    uxArraySize = uxTaskGetNumberOfTasks();
    pxTaskStatusArray = pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));
    if(pxTaskStatusArray != NULL)
    {
        uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, NULL);
        LOGE("app_main", "================================================");
        LOGE("app_main", "TotalHeap=%u  FreeHeap=%u  MinEverFree=%u",
            (unsigned int)configTOTAL_HEAP_SIZE,
            (unsigned int)xPortGetFreeHeapSize(),
            (unsigned int)xPortGetMinimumEverFreeHeapSize());
        LOGE("app_main", "Task Name            State  Priority  FreeStack");
        LOGE("app_main", "------------------------------------------------");
        for(x = 0; x < uxArraySize; x++)
        {
            const char *stateStr;
            switch(pxTaskStatusArray[x].eCurrentState)
            {
                case eRunning:   stateStr = "RUN";  break;
                case eReady:     stateStr = "RDY";  break;
                case eBlocked:   stateStr = "BLK";  break;
                case eSuspended: stateStr = "SUS";  break;
                case eDeleted:   stateStr = "DEL";  break;
                default:         stateStr = "UNK";  break;
            }
            LOGE("app_main", "%-20s %-5s %-5u %-5u",
                pxTaskStatusArray[x].pcTaskName,
                stateStr,
                pxTaskStatusArray[x].uxCurrentPriority,
                pxTaskStatusArray[x].usStackHighWaterMark);
        }
        LOGE("app_main", "================================================");
        vPortFree(pxTaskStatusArray);
    }
#endif
		
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_vUartTask */
/**
* @brief Function implementing the UartTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_vUartTask */
void vUartTask(void *argument)
{
  /* USER CODE BEGIN vUartTask */
  /* Infinite loop */
    //创建串口队列
    uart_msg_st ltv_UartMsg;
    gtv_UartTaskMsgQueueHandle = xQueueCreate(10, sizeof(uart_msg_st));
    configASSERT(gtv_UartTaskMsgQueueHandle != NULL);
    for(;;)
    {
       xQueueReceive(gtv_UartTaskMsgQueueHandle, &ltv_UartMsg, portMAX_DELAY);

       switch(ltv_UartMsg.mcv_UartPort)
       {
       case UART_PORT1:
           HandleUART1RecvData(ltv_UartMsg.mcp_DataBuff, ltv_UartMsg.msv_MsgLength);
           memset(gcv_Uart1RecvBuf, 0, RX_LEN_UART1);
           break;

       case UART_PORT2:
           HandleUART2RecvData(ltv_UartMsg.mcp_DataBuff, ltv_UartMsg.msv_MsgLength);
           memset(gcv_Uart2RecvBuf, 0, RX_LEN_UART2);
           break;

       case UART_PORT3:
           HandleUART3RecvData(ltv_UartMsg.mcp_DataBuff, ltv_UartMsg.msv_MsgLength);
           memset(gcv_Uart3RecvBuf, 0, RX_LEN_UART3);
           break;

       default:
           break;
       }
    }
  /* USER CODE END vUartTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

