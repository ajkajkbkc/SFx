
/* Private includes ----------------------------------------------------------*/
#include "ntc.h"
#include "log.h"
#include <math.h>
/* Private define ------------------------------------------------------------*/
#define NTC_MAX_TEMPERATURE   (2000)   //测量最大温度200.0
#define NTC_MIN_TEMPERATURE   (-350)   //测量最小温度-35.0
#define NTC_DISCONNECT_TEMP   (0xFFFF) //未连接上后值

#define R_REF      10000.0  // Balance resistor(Pull-up resistor)
#define T_ROOM     298.15   // Room temp(uint: Kelvin) 273.15 Kelvin = 0 Celsius
#define	R_ROOM     10000.0  // Resistance at room temp
#define B          3455.0   // B coefficient
#define MAX_ADC    4095.0   // Maximum ADC value (10-bit ADC)
#define U_MCU      3300.0   // MCU standard voltage (uint : mV)
#define e          2.7183   // Natural number
/*
  T_ROOM : 298.15K (25°C)
        |
   GND.||_______| |_______
       ||       | |       |
        |                 |     R_ROOM : 10000 Ω
               _____      |         ___ /        |
  U_MCU |_____|     |_____|________|   / |_______||. GND
        |     |_____|   PA4,PA5    |_ /__|       ||
               R_REF              ___/           |
*/
/* Private variables ---------------------------------------------------------*/
uint16_t adc_data[4];  // ADC data buffer
extern ADC_HandleTypeDef hadc1;
/* Private function prototypes -----------------------------------------------*/
osThreadId_t ntcTaskHandle;
const osThreadAttr_t ntcTask_attributes =
{
    .name = "ntcTask",
    .priority = (osPriority_t) osPriorityBelowNormal,
    .stack_size = 128 * 4
};

/* Private user code ---------------------------------------------------------*/
/**
  * @brief  adc init
  * @param  None
  * @retval None
  */
static void ntc_adc_init(void)
{
    /* Run the ADC calibration */
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        /* Calibration Error */
        Error_Handler();
    }

    /* Start ADC conversion on regular group with transfer by DMA */
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_data, 4) != HAL_OK)
    {
        /* Start Error */
        Error_Handler();
    }
}

/**
  * @brief  通过采样的AD值计算出电压值
  * @param  adcVal：ad采样值
  * @retval 计算出的电压值(uint: mV)
  */
static uint16_t Calculate_Voltage(uint16_t adcVal)
{
    uint16_t voltage;

    if(adcVal > MAX_ADC)
    {
        adcVal = MAX_ADC;
    }
    voltage = adcVal * U_MCU / MAX_ADC;
    //LOGI("app_ntc", "voltage = %4d mV", voltage);

    return voltage;
}

/**
  * @brief  通过电压计算热敏电阻阻值
  * @param  voltage：电压(uint: mV)
  * @retval 阻值(uint: Ω)
  */
static uint32_t Calculate_Res(uint16_t voltage)
{
    uint32_t Res = 0;

    if(voltage >= U_MCU)
    {
        voltage = U_MCU - 1;
    }
    else if(voltage == 0)
    {
        voltage = 1;
    }
    Res = R_REF / (U_MCU - voltage) * voltage;
    //LOGI("app_ntc", "ntc Res = %8d", Res);

    return Res;
}
/**
  * @brief  通过热敏电阻阻值计算温度
  * @param  Res: 阻值
  * @retval 温度(放大10倍的温度，如返回250，则是25.0℃)
  */
static int16_t Calculate_NTCTemperature(uint32_t Res)
{
    double temperature = 0;
    int16_t temp;

    /*
    *               B
    *   T = ______________________
    *
    *         /                   \
    *         |      R_THERM      |
    *         | _________________ |
    *      ln |             -B    |
    *         |            ------ |
    *         |            T_ROOM |
    *         | R_ROOM * e        |
    *         \                   /
    */

    temperature = B / log(Res / (R_ROOM * pow(e, (-B / T_ROOM))));
    temperature -= 273.15;
    //LOGI("app_ntc", "ntc temperature = %3.1f", temperature);

    temp = (int16_t)(temperature * 10);

    return temp;
}

void first_get_temp(void)
{
    uint8_t i;
    uint32_t usTemp;
    int16_t temp;

    for(i = 0; i < 4; i++)
    {
        // adc_data[i] = 0;

        usTemp = Calculate_Voltage(adc_data[i]);
        //LOGI("app_ntc1", "Tmp[%d] = %6d", i, usTemp);
        usTemp = Calculate_Res(usTemp);
        //LOGI("app_ntc2", "Tmp[%d] = %6d", i, usTemp);
        temp = Calculate_NTCTemperature(usTemp);
        //LOGI("app_ntc3", "Tmp[%d] = %6d", i, temp);
        if(temp < NTC_MIN_TEMPERATURE || temp > NTC_MAX_TEMPERATURE)
        {
            temp = NTC_DISCONNECT_TEMP;
            // CLEAR_BIT(gParam.st.State_NtcConnect, 1 << i);
        }
        else
        {
            // SET_BIT(gParam.st.State_NtcConnect, 1 << i);
        }
        //LOGI("app_ntc", "Tmp[%d] = %6d", i, temp);
        
        // gESE_Elem.st.Tmp[i] = temp;
    }
}
static void Get_Temperature_Process(void)
{
    uint8_t i;
    uint32_t usTemp;
    int16_t temp;

    for(i = 0; i < 4; i++)
    {
        usTemp = Calculate_Voltage(adc_data[i]);
        usTemp = Calculate_Res(usTemp);
        temp = Calculate_NTCTemperature(usTemp);
        (void) temp;
        // if(temp < NTC_MIN_TEMPERATURE || temp > NTC_MAX_TEMPERATURE)  //断开连接时计算值约为-90~-100℃
        // {
        //     if(READ_BIT(gParam.st.State_NtcConnect, 1 << i))
        //     {
        //         if(gTempChgCnt[i] < TEMP_CNT_MAX)   //传感器连接过程的数据不要
        //         {
        //             gTempChgCnt[i]++;
        //             continue;
        //         }
        //         else
        //         {
        //             CLEAR_BIT(gParam.st.State_NtcConnect, 1 << i);
        //         }
        //     }
        //     else
        //     {
        //         gTempChgCnt[i] = 0;
        //     }
        // }
        // else
        // {
        //     if(!READ_BIT(gParam.st.State_NtcConnect, 1 << i))
        //     {
        //         if(gTempChgCnt[i] < TEMP_CNT_MAX)   //传感器连接过程的数据不要
        //         {
        //             gTempChgCnt[i]++;
        //             continue;
        //         }
        //         else
        //         {
        //             SET_BIT(gParam.st.State_NtcConnect, 1 << i);
        //         }
        //     }
        //     else
        //     {
        //         gTempChgCnt[i] = 0;
        //     }
        // }
        //LOGI("app_ntc", "Tmp[%d] = %6d", i, temp);

        // if(!READ_BIT(gParam.st.State_NtcConnect, 1 << i))
        // {
        //     gESE_Elem.st.Tmp[i] = NTC_DISCONNECT_TEMP;
        //     if(!READ_BIT(gFlashParam.st.AlmState_Keep[StateAlm3], StateAlm3_TMP1_Msk << i))
        //     {
        //         CLEAR_BIT(gParam.st.State_Alarm[StateAlm3], StateAlm3_TMP1_Msk << i);
        //         CLEAR_BIT(gParam.st.State_Alarm[StateAlm0], StateAlm0_TMP_Msk);
        //     }
        // }
        // else
        // {
        //     if(gESE_Elem.st.Tmp[i] == NTC_DISCONNECT_TEMP)
        //     {
        //         gESE_Elem.st.Tmp[i] = temp;
        //     }
        //     else
        //     {
        //         gESE_Elem.st.Tmp[i] = CalcAvTemp(gESE_Elem.st.Tmp[i], temp, gFlashParam.st.NTC_LowPassFilterBW);
        //     }

        //     if(gESE_Elem.st.Tmp[i] < gFlashParam.st.Temp_THDown || gESE_Elem.st.Tmp[i] > gFlashParam.st.Temp_THUp)
        //     {
        //         SET_BIT(gParam.st.State_Alarm[StateAlm3], StateAlm3_TMP1_Msk << i);
        //         SET_BIT(gParam.st.State_Alarm[StateAlm0], StateAlm0_TMP_Msk);
        //     }
        //     else
        //     {
        //         if(!READ_BIT(gFlashParam.st.AlmState_Keep[StateAlm3], StateAlm3_TMP1_Msk << i))
        //         {
        //             CLEAR_BIT(gParam.st.State_Alarm[StateAlm3], StateAlm3_TMP1_Msk << i);
        //             CLEAR_BIT(gParam.st.State_Alarm[StateAlm0], StateAlm0_TMP_Msk);
        //         }
        //     }
        // }
    }

}
/**
  * @brief  Function implementing the ntcTask thread.
  * @param  argument: Not used
  * @retval None
  */
void NtcTask(void *argument)
{
    LOGD("app_ntc", "%s RUN. Free heap size is %d bytes", __func__, xPortGetFreeHeapSize());

    ntc_adc_init();
    osDelay(1000);
    first_get_temp();

    for(;;)
    {
        osDelay(1000);

        Get_Temperature_Process();
    }
}


