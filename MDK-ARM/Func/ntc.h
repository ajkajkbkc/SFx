#ifndef _NTC_H
#define _NTC_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/


/* Private defines -----------------------------------------------------------*/
extern osThreadId_t ntcTaskHandle;
extern const osThreadAttr_t ntcTask_attributes;

/* Private functions ---------------------------------------------------------*/
void NtcTask(void *argument);

#endif /* _NTC_H */
