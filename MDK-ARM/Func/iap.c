
/* Private includes ----------------------------------------------------------*/
#include <string.h>

#include "main.h"
#include "iap.h"
#include "log.h"


/* Private define ------------------------------------------------------------*/



/* Private variables ---------------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
void bsp_erase_page(uint32_t pageAddress, uint16_t nbpages);
void iap_WriteFlag(uint16_t flag);
uint16_t iap_ReadFlag(void);


/* Private user code ---------------------------------------------------------*/

/**
  * @brief  擦除页
  * @param  pageAddress 起始地址
  * @param  nbpages 擦除页数
  * @retval None
  */
void bsp_erase_page(uint32_t pageAddress, uint16_t nbpages)
{
    FLASH_EraseInitTypeDef eit =
    {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = pageAddress,
        .NbPages = nbpages
    };
    uint32_t pageErr = 0;
    HAL_FLASHEx_Erase(&eit, &pageErr);
}


void iap_WriteFlag(uint16_t flag)
{
    HAL_FLASH_Unlock();
    bsp_erase_page(IAP_FLAG_ADDR, 1);
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, IAP_FLAG_ADDR, flag);
    HAL_FLASH_Lock();
}


uint16_t iap_ReadFlag(void)
{
    return *((uint16_t *)IAP_FLAG_ADDR);
}


bool is_iap_update(uint8_t *cStr, uint32_t baudrate)
{
    if(memcmp(cStr, "update", 6) == 0)
    {
        iap_WriteFlag(UPDATE_FLAG_DATA);
        return true;
    }
    else if (memcmp(cStr, "erase", 5) == 0)
    {
        iap_WriteFlag(ERASE_FLAG_DATA);
        return true;
    }
    else if (memcmp(cStr, "menu", 4) == 0)
    {
        iap_WriteFlag(INIT_FLAG_DATA);
        return true;
    }
    return false;
}


void NVIC_SetVectorTable(uint32_t Offset)
{
    SCB->VTOR = FLASH_BASE | (Offset & (uint32_t)0x1FFFFF80);
}


void IAP_Init(void)
{
    //设置中断向量表
    NVIC_SetVectorTable(IAP_BOOTLOADER_FLASH_SIZE);

    __enable_irq();  //开启中断
}




