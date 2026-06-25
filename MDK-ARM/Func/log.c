
/* Private includes ----------------------------------------------------------*/
#include "log.h"


/* Private define ------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
/* Private function prototypes -----------------------------------------------*/
void LogUart_Init(UART_HandleTypeDef *huart);

/* Private user code ---------------------------------------------------------*/
void LogUart_Init(UART_HandleTypeDef *huart)
{
    huart->Init.BaudRate = 460800;
    if (HAL_UART_Init(huart) != HAL_OK)
    {
        Error_Handler();
    }
}

void uartlog_init(void)
{
#if (PRINT_LOG_OPEN == 1)
    {
#if (UART1_AS_LOG == 1)
        LogUart_Init(&huart1);
#elif (UART2_AS_LOG == 1)
        LogUart_Init(&huart2);
#elif (UART3_AS_LOG == 1)
        LogUart_Init(&huart3);
#endif
    }
#endif
}

#if (UART1_AS_LOG == 1)
int fputc(int ch, FILE *f)
{
    /* 将printf内容发往串口1 */
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return (ch);
}
#elif (UART2_AS_LOG == 1)
int fputc(int ch, FILE *f)
{
    /* 将printf内容发往串口2 */
    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return (ch);
}
#elif (UART3_AS_LOG == 1)
int fputc(int ch, FILE *f)
{
    /* 将printf内容发往串口3 */
    HAL_UART_Transmit(&huart3, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return (ch);
}
#endif

#if (PRINT_LOG_OPEN == 1)
/********** 禁用半主机模式 **********/
/* And we should not use MicroLIB */;
#pragma import(__use_no_semihosting)

struct __FILE
{
    int a;
};

FILE __stdout;

void _sys_exit(int x)
{

}

void _ttywrch(int ch)
{
}


#endif /* PRINT_LOG_OPEN */

