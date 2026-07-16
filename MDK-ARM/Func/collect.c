
/* Private includes ----------------------------------------------------------*/
#include "collect.h"

/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
osThreadId_t collectTaskHandle;
const osThreadAttr_t collectTask_attributes =
{
    .name = "collectTask",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 512 * 4
};

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  Function implementing the collectTask thread.
  * @param  argument: Not used
  * @retval None
  */
void collectTask(void *argument)
{
    for(;;)
    {
        osDelay(100);

    }
}
