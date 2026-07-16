#ifndef _EC20TASK_H
#define _EC20TASK_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private defines -----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
extern osThreadId_t EC20TaskHandle;
extern const osThreadAttr_t EC20Task_attributes;

/* Exported functions prototypes ---------------------------------------------*/
void EC20Task(void *argument);
void EC20_UART_RxCallback(uint8_t *pData, uint16_t len);

#endif //_EC20TASK_H
