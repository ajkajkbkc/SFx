#ifndef _NTC_H
#define _NTC_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
/* Private defines -----------------------------------------------------------*/

/* Exported types ------------------------------------------------------------*/

/* Exported macro ------------------------------------------------------------*/
extern osThreadId_t ntcTaskHandle;
extern const osThreadAttr_t ntcTask_attributes;

/* Exported functions prototypes ---------------------------------------------*/
void NtcTask(void *argument);

#endif /* _NTC_H */
