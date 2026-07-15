/* Includes ------------------------------------------------------------------*/
#include "parameter.h"
#include "W5500Task.h"
#include <string.h>
/* Private define ------------------------------------------------------------*/
flash_param_t gFlashParam;
volatile param_t gParam; //全局变量，存放在SRAM中，运行时使用

/* Private variables ---------------------------------------------------------*/

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

void flashparam_init(void)
{
    if(gFlashParam.st.Prod_Protocol == 0xFFFFFFFF)
    {
            gFlashParam.st.Prod_Protocol = 0;
#if   PROTOCOL_TYPE & PROTOCOL_MBTCP
            gFlashParam.st.Prod_Protocol |= PROTOCOL_MBTCP;
#endif
#if PROTOCOL_TYPE & PROTOCOL_MBTCP
            gFlashParam.st.Prod_Protocol |= PROTOCOL_MBTCP;
#endif
#if PROTOCOL_TYPE & PROTOCOL_FLKMQTT
            gFlashParam.st.Prod_Protocol |= PROTOCOL_FLKMQTT;
#endif
#if PROTOCOL_TYPE & PROTOCOL_HZUDP
            gFlashParam.st.Prod_Protocol |= PROTOCOL_HZUDP;
#endif
#if PROTOCOL_TYPE & PROTOCOL_LLTUDP
            gFlashParam.st.Prod_Protocol |= PROTOCOL_LLTUDP;
#endif
#if PROTOCOL_TYPE & PROTOCOL_DHCP
            gFlashParam.st.Prod_Protocol |= PROTOCOL_DHCP;
#endif
#if PROTOCOL_TYPE & PROTOCOL_DNS
            gFlashParam.st.Prod_Protocol |= PROTOCOL_DNS;
#endif
#if PROTOCOL_TYPE & PROTOCOL_SNTP
            gFlashParam.st.Prod_Protocol |= PROTOCOL_SNTP;
#endif
#if PROTOCOL_TYPE & PROTOCOL_NETBIOS
            gFlashParam.st.Prod_Protocol |= PROTOCOL_NETBIOS;
#endif
#if PROTOCOL_TYPE & PROTOCOL_HTTPS
            gFlashParam.st.Prod_Protocol |= PROTOCOL_HTTPS;
#endif
    }
    /**************************** TCP/UDP *******************************/
    //获取芯片ID
    gFlashParam.st.macAddr[0] = (HAL_GetUIDw2() & 0x000000FF) + ((HAL_GetUIDw2() & 0x0000FF00) >> 8);     //物理MAC地址
    gFlashParam.st.macAddr[1] = ((HAL_GetUIDw2() & 0x00FF0000) >> 16) + ((HAL_GetUIDw2() & 0xFF000000) >> 24);
    gFlashParam.st.macAddr[2] = (HAL_GetUIDw1() & 0x000000FF) + ((HAL_GetUIDw1() & 0x0000FF00) >> 8);
    gFlashParam.st.macAddr[3] = ((HAL_GetUIDw1() & 0x00FF0000) >> 16) + ((HAL_GetUIDw1() & 0xFF000000) >> 24);
    gFlashParam.st.macAddr[4] = (HAL_GetUIDw0() & 0x000000FF) + ((HAL_GetUIDw0() & 0x0000FF00) >> 8);
    gFlashParam.st.macAddr[5] = ((HAL_GetUIDw0() & 0x00FF0000) >> 16) + ((HAL_GetUIDw0() & 0xFF000000) >> 24);
    if(gFlashParam.st.macAddr[0] % 2 != 0) //MAC地址开头必须是偶数
    {
    gFlashParam.st.macAddr[0] += 1;
    }

    gFlashParam.st.DnsServerIP[0] = 114;        //DNS Server ip
    gFlashParam.st.DnsServerIP[1] = 114;
    gFlashParam.st.DnsServerIP[2] = 114;
    gFlashParam.st.DnsServerIP[3] = 114;

    gFlashParam.st.maskIP[0] = 255;       //子网掩码
    gFlashParam.st.maskIP[1] = 255;
    gFlashParam.st.maskIP[2] = 255;
    gFlashParam.st.maskIP[3] = 0;

    gFlashParam.st.gatewayIP[0] = 192;    //本机网关IP
    gFlashParam.st.gatewayIP[1] = 168;    //192.168.1.1
    gFlashParam.st.gatewayIP[2] = 0;
    gFlashParam.st.gatewayIP[3] = 1;

    gFlashParam.st.localIP[0] = 192;      //本机IP
    gFlashParam.st.localIP[1] = 168;      //192.168.1.99
    gFlashParam.st.localIP[2] = 0;
    gFlashParam.st.localIP[3] = 45;
    gFlashParam.st.localUDPPort = 6000;   //本机UDP端口

    gFlashParam.st.s0TargetIP[0] = 192;   //S0目标IP
    gFlashParam.st.s0TargetIP[1] = 168;   //192.168.0.149
    gFlashParam.st.s0TargetIP[2] = 1;
    gFlashParam.st.s0TargetIP[3] = 149;

    gFlashParam.st.s0LocalPort = 6000;    //S0本机端口
    gFlashParam.st.s0TargetPort = 4000;   //S0目标端口

    gFlashParam.st.s1TargetIP[0] = 8;   //S1目标IP
    gFlashParam.st.s1TargetIP[1] = 129;   //8.129.232.136
    gFlashParam.st.s1TargetIP[2] = 232;
    gFlashParam.st.s1TargetIP[3] = 136;

    gFlashParam.st.s1LocalPort = 13036;   //S1本机端口
    gFlashParam.st.s1TargetPort = 1883;  //S1目标端口

    /**************************** MQTT *******************************/
    gFlashParam.st.mqttPublishInterval = 1;

    memset(gFlashParam.st.mqttUserName, 0, sizeof(gFlashParam.st.mqttUserName));
    memcpy(gFlashParam.st.mqttUserName, "admin", 5);
    memset(gFlashParam.st.mqttPassword, 0, sizeof(gFlashParam.st.mqttPassword));
    memcpy(gFlashParam.st.mqttPassword, "password", 8);

    memset(gFlashParam.st.mqttPub, 0, sizeof(gFlashParam.st.mqttPub));
    strncpy((char *)gFlashParam.st.mqttPub, "FEXCLOUD/SENSOR/POL/RPT", sizeof(gFlashParam.st.mqttPub) - 1);			

    memset(gFlashParam.st.mqttAlarmPub, 0, sizeof(gFlashParam.st.mqttAlarmPub));
    strncpy((char *)gFlashParam.st.mqttAlarmPub, "FEXCLOUD/SENSOR/ALR/RPT", sizeof(gFlashParam.st.mqttAlarmPub) - 1);

    memset(gFlashParam.st.mqttSub, 0, sizeof(gFlashParam.st.mqttSub));
    strncpy((char *)gFlashParam.st.mqttSub, "FEXCLOUD/COMMAND/SET/REQ", sizeof(gFlashParam.st.mqttSub) - 1);


}
/**
  * @brief  参数初始化
  * @param  None
  * @retval None
  */
void Parameter_Init(void)
{
    portENTER_CRITICAL();

    flashparam_init();
    // thunderparam_init();
    // gatewayparam_init();
    // param_init();
    // udpparam_init();

    portEXIT_CRITICAL();
}
