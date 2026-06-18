#ifndef __LOG_H__
#define __LOG_H__

#include "main.h"
#include <stdio.h>

#if PRINT_LOG_OPEN == 1
    #if ( ( (UART1_AS_LOG == 1) || (UART2_AS_LOG == 1) || (UART3_AS_LOG == 1)
        #define LOGV(tag, fmt, ...) printf("[V][%s] " fmt, tag, ##__VA_ARGS__) //最详细的调试信息
				#define LOGD(tag, fmt, ...) printf("[D][%s] " fmt, tag, ##__VA_ARGS__) //调试信息
				#define LOGI(tag, fmt, ...) printf("[I][%s] " fmt, tag, ##__VA_ARGS__) //常规运行信息
				#define LOGW(tag, fmt, ...) printf("[W][%s] " fmt, tag, ##__VA_ARGS__) //警告信息
				#define LOGE(tag, fmt, ...) printf("[E][%s] " fmt, tag, ##__VA_ARGS__) //错误信息
    #else
        #define LOGV(tag, fmt, ...)
        #define LOGD(tag, fmt, ...)
        #define LOGI(tag, fmt, ...)
        #define LOGW(tag, fmt, ...)
        #define LOGE(tag, fmt, ...)
    #endif
#endif

#endif /* __LOG_H__ */
