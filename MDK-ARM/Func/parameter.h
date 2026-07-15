#ifndef PARAMETER_H
#define PARAMETER_H

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"

/* Private defines -----------------------------------------------------------*/

/* -------------------------- TOOLS --------------------------- */
#define PAR_SET_BIT(REG, BIT)     ((REG) |= (BIT))       /* 置位寄存器中指定的位         */
#define PAR_CLEAR_BIT(REG, BIT)   ((REG) &= ~(BIT))      /* 清除寄存器中指定的位         */
#define PAR_READ_BIT(REG, BIT)    ((REG) & (BIT))        /* 读取寄存器中指定位的状态     */
#define PAR_CLEAR_REG(REG)        ((REG) = (0x0))        /* 将寄存器清零                 */
#define PAR_WRITE_REG(REG, VAL)   ((REG) = (VAL))        /* 向寄存器写入指定值           */
#define PAR_READ_REG(REG)         ((REG))                /* 读取寄存器的当前值           */

/*取指针地址值*/
#define GET_POINT_ADDR(x)         ((unsigned long)(x))   /* 获取指针的地址值（将指针转为无符号整数）*/

#define GET_PU8_DATA(x)           (*((unsigned char *)(x)))  /* 取指针指向的无符号8位数据    */
#define GET_PS8_DATA(x)           (*((char *)(x)))           /* 取指针指向的有符号8位数据    */

#define GET_PU16_DATA(x)          (*((unsigned short *)(x))) /* 取指针指向的无符号16位数据  */
#define GET_PS16_DATA(x)          (*((short *)(x)))          /* 取指针指向的有符号16位数据  */

#define GET_PU32_DATA(x)          (*((unsigned long *)(x)))  /* 取指针指向的无符号32位数据  */
#define GET_PS32_DATA(x)          (*((long *)(x)))           /* 取指针指向的有符号32位数据  */

/* 按照小端序（Little-Endian）取值：低地址存低字节 */
#define GET_SMLPU16_DATA(x)       (unsigned short)((*(x + 1)<<8) + (*(x)))      /* 字节数组转小端16位无符号整数 */
#define GET_SMLPU32_DATA(x)       (unsigned long)((*(x+3)<<24) + (*(x+2)<<16) + (*(x+1)<<8) + (*(x)))  /* 字节数组转小端32位无符号整数 */

/*按照大端序（Big-Endian）取值：低地址存高字节 */
#define GET_BIGPU16_DATA(x)       (unsigned short)((*(x)<<8) + (*(x+1)))        /* 字节数组转大端16位无符号整数 */
#define GET_BIGPU32_DATA(x)       (unsigned long)((*(x)<<24) + (*(x+1)<<16) + (*(x+2)<<8) + (*(x+3)))  /* 字节数组转大端32位无符号整数 */
/* ------------------------------------------------------------ */

/* paramters ------------------------------------------------------*/
#define FLASH_ONEPAGE_HALFWORDSIZE  1024 //1024 half-word(16-bit) = 2048 byte(8-bit) Flash 的擦除操作是按页（2KB）为最小单位的
#define PARAM_HALFWORDSIZE          200  //param size 200 half-word(16-bit)

/* Exported types ------------------------------------------------------------*/

//存入flash的变量
typedef union
{
    uint16_t flash_buff[FLASH_ONEPAGE_HALFWORDSIZE];  //size: 1024 half-word(16-bit)
    struct
    {
        uint32_t Prod_Protocol;                       //(D1025)04 01  产品通讯协议
        uint32_t Module_Reg32_reserve[99];            //(D1025)04 03  预留32位模组寄存器

        uint16_t localUDPPort;                        //(D1225)04 C9  本机UDP端口
        uint16_t s0LocalPort;                         //(D1226)04 CA  S0本机端口
        uint16_t s0TargetPort;                        //(D1227)04 CB  S0目标端口
        uint16_t s1LocalPort;                         //(D1228)04 CC  S1本机端口
        uint16_t s1TargetPort;                        //(D1229)04 CD  S1目标端口（服务器 端口）
        uint16_t mqttPublishInterval;                 //(D1230)04 CE  MQTT上传时间间隔（单位：分钟）
        uint16_t Module_Reg16_reserve[494];           //(D1231)04 CF  预留16位模组寄存器 

        uint8_t idInfo[24];                           //(D1725)06 BD  ID序列号
        uint8_t macAddr[6];                           //(D1737)06 C9  本机MAC地址
        uint8_t gatewayIP[4];                         //(D1740)06 CC  网关IP地址
        uint8_t maskIP[4];                            //(D1742)06 CE  子网掩码
        uint8_t localIP[4];                           //(D1744)06 D0  本机IP地址
        uint8_t s0TargetIP[4];                        //(D1746)06 D2  S0目标IP地址
        uint8_t s1TargetIP[4];                        //(D1748)06 D4  S1目标IP地址（服务器IP）
        uint8_t mqttUserName[20];                     //(D1750)06 D6  MQTT用户名
        uint8_t mqttPassword[20];                     //(D1760)06 E0  MQTT密码
        uint8_t mqttPub[100];                         //(D1770)06 EA  MQTT发布主题
        uint8_t mqttSub[100];                         //(D1820)07 1C  MQTT订阅主题
        uint8_t mqttSubReply[100];                    //(D1870)07 4E  MQTT订阅回复主题
        uint8_t mqttAlarmPub[100];                    //(D1920)07 80  MQTT报警发布主题
        uint8_t DnsServerIP[4];                       //(D1970)07 B2  DNS服务器IP
        uint8_t domainName[50];                       //(D1972)07 B4  域名
        uint8_t Module_Reg8_reserve[104];             //(D1997)07 CD  预留8位模组寄存器 
    } st;
} flash_param_t;

//不存入flash的变量
typedef union
{
    uint16_t param_buff[PARAM_HALFWORDSIZE];         //size: 400 half-word(16-bit)
    struct
    {
        uint32_t SecCnt;                             //(D2049)08 01  运行的秒数
        uint32_t Module_Reg32_reserve[24];           //预留32位模组寄存器

        uint16_t NetLink_State;                      //(D2099)08 33  网络连接状态
        uint16_t Module_Reg16_reserve[99];           //(D2100)08 34  预留16位模组寄存器 

        uint8_t Module_Reg8_reserve[100];            //(D2199)08 97  预留8位模组寄存器 

    } st;
} param_t;

/* Exported macro ------------------------------------------------------------*/
extern flash_param_t gFlashParam;
extern volatile param_t gParam;

/* Exported functions prototypes ---------------------------------------------*/
void Parameter_Init(void);

#endif
