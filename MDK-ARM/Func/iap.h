#ifndef _IAP_H_
#define _IAP_H_

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stdbool.h"

/* Exported types ------------------------------------------------------------*/



/* Exported constants --------------------------------------------------------*/


/* Private defines -----------------------------------------------------------*/
//App区域和Bootloader区域共享信息的地址(暂定大小为2K)
#define IAP_FLAG_ADDR               (uint32_t)(APPLICATION_ADDRESS - 1024 * 2)

#if (USE_BKP_SAVE_FLAG == 1)
  #define INIT_FLAG_DATA      0x0000  //默认标志的数据(空片子的情况)
#elif 1  //上一版本boot
  #define INIT_FLAG_DATA      0xFFFF   //默认标志的数据(空片子的情况)
#else  //新版boot
  #define INIT_FLAG_DATA      0xBBBB   //初始标志的数据
#endif
#define DEFAULT_FLAG_DATA           0xFFFF   //默认标志的数据(空片子的情况)
#define UPDATE_FLAG_DATA            0xEEEE   //下载标志的数据
#define UPLOAD_FLAG_DATA            0xDDDD   //上传标志的数据
#define ERASE_FLAG_DATA             0xCCCC   //擦除标志的数据
#define APPRUN_FLAG_DATA            0x5A5A   //APP不需要做任何处理，直接运行状态
#define IAP_BOOTLOADER_FLASH_SIZE   0x8000   //Bootloader区域大小
/* Define the address from where user application will be loaded,
  the application address should be a start sector address */
#define APPLICATION_ADDRESS         (FLASH_BASE + IAP_BOOTLOADER_FLASH_SIZE)


/* Private functions ---------------------------------------------------------*/
void IAP_Init(void);
bool is_iap_update(uint8_t *cStr, uint32_t baudrate);




#endif /* _IAP_H_ */
