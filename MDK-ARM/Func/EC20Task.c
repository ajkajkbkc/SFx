
/* Private includes ----------------------------------------------------------*/
#include "EC20Task.h"
#include "log.h"
#include "usart.h"
#include "rtc.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "semphr.h"
#include "parameter.h"

/* Private define ------------------------------------------------------------*/
/* EC20 AT指令响应缓冲区大小 */
#define EC20_RSP_BUF_SIZE       256

/* AT指令超时时间(ms) */
#define EC20_AT_TIMEOUT_MS      1000
#define EC20_AT_LONG_TIMEOUT_MS 5000

/* AT指令发送缓冲区大小 */
#define EC20_CMD_BUF_SIZE       1024

/* NTP服务器地址（默认） */
#define EC20_NTP_SERVER         "pool.ntp.org"

/* Private variables ---------------------------------------------------------*/

/* EC20 AT指令响应缓冲区 */
static uint8_t  g_EC20_RspBuf[EC20_RSP_BUF_SIZE];
static uint16_t g_EC20_RspLen = 0;
static SemaphoreHandle_t g_EC20_RspSem = NULL;

/* Private function prototypes -----------------------------------------------*/
osThreadId_t EC20TaskHandle;
const osThreadAttr_t EC20Task_attributes =
{
    .name = "EC20Task",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 512 * 4
};

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  EC20串口接收回调（由HandleUART2RecvData调用，在vUartTask上下文中执行）
  * @param  pData 数据指针
  * @param  len   数据长度
  * @retval None
  */
void EC20_UART_RxCallback(uint8_t *pData, uint16_t len)
{
    if (len > EC20_RSP_BUF_SIZE) {
        len = EC20_RSP_BUF_SIZE;
    }
    memcpy(g_EC20_RspBuf, pData, len);
    g_EC20_RspLen = len;

    /* 通知等待响应的EC20任务 */
    if (g_EC20_RspSem != NULL) {
        xSemaphoreGive(g_EC20_RspSem);
    }
}

/**
  * @brief  发送AT指令并等待响应（同步方式）
  * @param  cmd       AT指令字符串（不含\r\n），如 "AT"
  * @param  expect    期望的成功响应关键字，如 "OK"
  * @param  timeout   等待超时时间(ms)
  * @retval  0: 收到期望响应
  *         -1: 超时
  *         -2: 收到响应但未匹配期望关键字
  *         -3: 发送失败
  */
static int ec20_send_cmd(const char *cmd, const char *expect, uint32_t timeout)
{
    char send_buf[EC20_CMD_BUF_SIZE];
    int len;

    if (g_EC20_RspSem == NULL) {
        LOGE("EC20", "Semaphore not initialized");
        return -3;
    }

    /* 清空响应缓冲区 */
    g_EC20_RspLen = 0;
    memset(g_EC20_RspBuf, 0, EC20_RSP_BUF_SIZE);

    /* 构造指令：添加\r\n */
    len = snprintf(send_buf, sizeof(send_buf), "%s\r\n", cmd);
    if (len <= 0 || len >= (int)sizeof(send_buf)) {
        LOGE("EC20", "Cmd too long: %s", cmd);
        return -3;
    }

    /* 通过USART2发送 */
    if (HAL_UART_Transmit(&huart2, (uint8_t *)send_buf, len, 1000) != HAL_OK) {
        LOGE("EC20", "UART send failed");
        return -3;
    }

    LOGD("EC20", "Send: %s", cmd);

    /* 等待响应（先取一次清空可能的历史遗留信号量） */
    xSemaphoreTake(g_EC20_RspSem, 0);

    if (xSemaphoreTake(g_EC20_RspSem, pdMS_TO_TICKS(timeout)) == pdTRUE) {
        /* 收到响应，打印日志 */
        g_EC20_RspBuf[g_EC20_RspLen] = '\0';
        LOGD("EC20", "Rsp(%d): %s", g_EC20_RspLen, (char *)g_EC20_RspBuf);

        /* 检查是否包含期望关键字 */
        if (expect != NULL && *expect != '\0') {
            if (strstr((char *)g_EC20_RspBuf, expect) != NULL) {
                return 0;   /* 匹配成功 */
            }
            /* 检查是否包含ERROR */
            if (strstr((char *)g_EC20_RspBuf, "ERROR") != NULL) {
                LOGE("EC20", "Cmd[%s] response ERROR", cmd);
                return -2;
            }
            return -2;      /* 响应不匹配 */
        }
        return 0;           /* 不检查关键字，有响应即成功 */
    }

    /* 超时 */
    LOGE("EC20", "Cmd[%s] timeout(%dms)", cmd, (int)timeout);
    return -1;
}

/**
  * @brief  连续多次发送同一个AT指令，直至收到期望响应或达到最大重试次数
  * @param  cmd       AT指令字符串
  * @param  expect    期望响应关键字
  * @param  timeout   单次超时时间(ms)
  * @param  retry     最大重试次数
  * @retval  0: 成功   -1: 失败
  */
static int ec20_send_cmd_retry(const char *cmd, const char *expect,
                               uint32_t timeout, int retry)
{
    int i;
    for (i = 0; i < retry; i++) {
        if (ec20_send_cmd(cmd, expect, timeout) == 0) {
            return 0;
        }
        osDelay(200);
    }
    return -1;
}

/**
  * @brief  EC20初始化（AT指令部分）
  * @note   执行AT指令初始化序列，包括模块通信测试、SIM卡检测、网络注册等
  * @retval  0: 初始化成功
  *         -1: 初始化失败
  */
static int ec20_MQTT_init(void)
{
    int ret;

    LOGI("EC20", "=== EC20 Module Init Start ===");

    /*------------------------------------------------------------------------*/
    /* 1. 创建响应信号量（二值信号量）                                        */
    /*------------------------------------------------------------------------*/
    if (g_EC20_RspSem == NULL) {
        g_EC20_RspSem = xSemaphoreCreateBinary();
        if (g_EC20_RspSem == NULL) {
            LOGE("EC20", "Semaphore create failed");
            return -1;
        }
    }

    /*------------------------------------------------------------------------*/
    /* 2. 等待EC20模块上电稳定                                                */
    /*------------------------------------------------------------------------*/
    LOGI("EC20", "Waiting for module power-on...");
    osDelay(2000);  /* 模块上电后等待2s */

    /*------------------------------------------------------------------------*/
    /* 3. 同步波特率：多次发送AT直至模块响应                                  */
    /*------------------------------------------------------------------------*/
    LOGI("EC20", "Sync baudrate...");
    ret = ec20_send_cmd_retry("AT", "OK", EC20_AT_TIMEOUT_MS, 5);
    if (ret != 0) {
        LOGE("EC20", "Module not responding. Check power & UART connection.");
        return -1;
    }

    /*------------------------------------------------------------------------*/
    /* 4. 关闭回显                                                            */
    /*------------------------------------------------------------------------*/
    ec20_send_cmd("ATE0", "OK", EC20_AT_TIMEOUT_MS);

    /*------------------------------------------------------------------------*/
    /* 5. 查询SIM卡状态                                                       */
    /*------------------------------------------------------------------------*/
    ret = ec20_send_cmd_retry("AT+CPIN?", "READY", EC20_AT_LONG_TIMEOUT_MS, 3);
    if (ret != 0) {
        LOGE("EC20", "SIM card not ready (CPIN failed). Check SIM card.");
        /* 继续执行，不直接返回 */
    }

    /*------------------------------------------------------------------------*/
    /* 6. 查询信号质量                                                        */
    /*------------------------------------------------------------------------*/
    ec20_send_cmd("AT+CSQ", "OK", EC20_AT_TIMEOUT_MS);

    /*------------------------------------------------------------------------*/
    /* 7. 查询网络注册状态，等待注册                                          */
    /*------------------------------------------------------------------------*/
    ret = ec20_send_cmd_retry("AT+CREG?", "+CREG: 0,1", EC20_AT_LONG_TIMEOUT_MS, 10);
    if (ret != 0) {
        LOGW("EC20", "Network not registered yet (CREG). Will retry later.");
        /* CREG可能返回0,1(注册)或0,5(漫游)，尝试两种 */
        ret = ec20_send_cmd_retry("AT+CREG?", "+CREG: 0,5", EC20_AT_LONG_TIMEOUT_MS, 5);
        if (ret != 0) {
            LOGW("EC20", "Network registration not confirmed. Continuing...");
        }
    }

    /*------------------------------------------------------------------------*/
    /* 8. 查询GPRS附着状态，若未附着则发起附着                                */
    /*------------------------------------------------------------------------*/
    ret = ec20_send_cmd("AT+CGATT?", "+CGATT: 1", EC20_AT_TIMEOUT_MS);
    if (ret != 0) {
        LOGI("EC20", "GPRS not attached, sending CGATT=1...");
        ec20_send_cmd("AT+CGATT=1", "OK", EC20_AT_LONG_TIMEOUT_MS);
        /* 等待附着完成 */
        ret = ec20_send_cmd_retry("AT+CGATT?", "+CGATT: 1", EC20_AT_LONG_TIMEOUT_MS, 5);
        if (ret != 0) {
            LOGE("EC20", "GPRS attach failed");
            return -1;
        }
    }

    /*------------------------------------------------------------------------*/
    /* 9. 配置PDP上下文（APN）                                                */
    /*------------------------------------------------------------------------*/
    LOGI("EC20", "Configuring PDP context...");
    ec20_send_cmd("AT+CGDCONT=1,\"IP\",\"CMNET\"", "OK", EC20_AT_TIMEOUT_MS);

    /*------------------------------------------------------------------------*/
    /* 10. 激活PDP上下文（移动场景）                                          */
    /*------------------------------------------------------------------------*/
    ret = ec20_send_cmd("AT+QIACT=1", "OK", EC20_AT_LONG_TIMEOUT_MS);
    if (ret != 0) {
        /* 部分固件使用CGACT，尝试备用指令 */
        LOGW("EC20", "QIACT=1 failed, trying CGACT=1,1...");
        ec20_send_cmd("AT+CGACT=1,1", "OK", EC20_AT_LONG_TIMEOUT_MS);
    }

    /* 检查PDP是否激活 */
    ret = ec20_send_cmd_retry("AT+QIACT?", "+QIACT: 1", EC20_AT_LONG_TIMEOUT_MS, 3);
    if (ret != 0) {
        LOGE("EC20", "PDP context activation failed");
        return -1;
    }

    /*------------------------------------------------------------------------*/
    /* 11. 查询本地IP地址                                                     */
    /*------------------------------------------------------------------------*/
    ec20_send_cmd("AT+QIACT?", "OK", EC20_AT_TIMEOUT_MS);

    /*------------------------------------------------------------------------*/
    /* 12. NTP时间同步                                                        */
    /*------------------------------------------------------------------------*/
    {
        char ntp_cmd[128];
        int y, M, d, h, m, s;

        LOGI("EC20", "Syncing time via NTP...");
        snprintf(ntp_cmd, sizeof(ntp_cmd), "AT+QNTP=1,\"%s\"", EC20_NTP_SERVER);
        ret = ec20_send_cmd(ntp_cmd, "+QNTP: 1,", EC20_AT_LONG_TIMEOUT_MS);

        if (ret == 0) {
            /* 解析返回时间：+QNTP: 1,<year>/<month>/<day>,<hour>:<minute>:<second> */
            char *p = strstr((char *)g_EC20_RspBuf, "+QNTP: 1,");
            if (p != NULL) {
                p += 10; /* 跳过 "+QNTP: 1," */
                if (sscanf(p, "%d/%d/%d,%d:%d:%d", &y, &M, &d, &h, &m, &s) == 6) {
                    RTC_DateTypeDef rtc_date;
                    RTC_TimeTypeDef rtc_time;

                    rtc_date.Year = y - 2000;
                    rtc_date.Month = M;
                    rtc_date.Date = d;
                    rtc_date.WeekDay = RTC_WEEKDAY_MONDAY;
                    HAL_RTC_SetDate(&hrtc, &rtc_date, RTC_FORMAT_BIN);

                    rtc_time.Hours = h;
                    rtc_time.Minutes = m;
                    rtc_time.Seconds = s;
                    HAL_RTC_SetTime(&hrtc, &rtc_time, RTC_FORMAT_BIN);

                    LOGI("EC20", "NTP time: %04d-%02d-%02d %02d:%02d:%02d",
                         y, M, d, h, m, s);
                }
            }
        } else {
            LOGW("EC20", "NTP sync failed, continuing...");
        }
    }

    /*------------------------------------------------------------------------*/
    /* 13. 配置MQTT参数                                                        */
    /*------------------------------------------------------------------------*/
    /* 13a. 配置MQTT接收模式（push模式，自动ACK回复）                         */
    LOGI("EC20", "Configuring MQTT receive mode...");
    ec20_send_cmd("AT+QMTCFG=\"recv/mode\",0,0,1", "OK", EC20_AT_TIMEOUT_MS);

    /* 13b. 配置MQTT保活时间（60秒）                                          */
    LOGI("EC20", "Configuring MQTT keepalive...");
    ec20_send_cmd("AT+QMTCFG=\"keepalive\",0,60", "OK", EC20_AT_TIMEOUT_MS);

    LOGI("EC20", "=== EC20 Module Init OK ===");
    return 0;
}

/**
  * @brief  连接MQTT服务器（通过EC20内置MQTT AT指令）
  * @note   使用flash中保存的服务器地址、端口、用户名、密码等参数
  * @retval  0: 连接成功
  *         -1: 连接失败
  */
static int ec20_mqtt_connect(void)
{
    int ret;
    char cmd_buf[EC20_CMD_BUF_SIZE];

    LOGI("EC20", "=== EC20 MQTT Connect Start ===");

    /*------------------------------------------------------------------------*/
    /* 1. 构造MQTT服务器地址（优先使用域名，其次使用IP）                     */
    /*------------------------------------------------------------------------*/
    char *host = (char *)gFlashParam.st.domainName;
    uint16_t port = gFlashParam.st.s1TargetPort;

    /* 如果port未配置，使用默认1883 */
    if (port == 0) {
        port = 1883;
    }

    /* 若域名为空，则使用IP地址 */
    if (host[0] == '\0') {
        snprintf(cmd_buf, sizeof(cmd_buf), "AT+QMTOPEN=0,\"%d.%d.%d.%d\",%d",
                 gFlashParam.st.s1TargetIP[0],
                 gFlashParam.st.s1TargetIP[1],
                 gFlashParam.st.s1TargetIP[2],
                 gFlashParam.st.s1TargetIP[3],
                 port);
    } else {
        snprintf(cmd_buf, sizeof(cmd_buf), "AT+QMTOPEN=0,\"%s\",%d", host, port);
    }

    /*------------------------------------------------------------------------*/
    /* 2. 打开MQTT服务器TCP连接                                               */
    /*------------------------------------------------------------------------*/
    LOGI("EC20", "Opening MQTT connection...");
    ret = ec20_send_cmd(cmd_buf, "+QMTOPEN: 0,0", EC20_AT_LONG_TIMEOUT_MS);
    if (ret != 0) {
        LOGE("EC20", "MQTT TCP open failed");
        return -1;
    }

    /*------------------------------------------------------------------------*/
    /* 3. 构造ClientID（使用idInfo）                                          */
    /*------------------------------------------------------------------------*/
    char client_id[32];
    memset(client_id, 0, sizeof(client_id));

    if (gFlashParam.st.idInfo[0] != '\0') {
        /* 使用设备序列号作为ClientID */
        size_t id_len = strlen((char *)gFlashParam.st.idInfo);
        if (id_len > 23) id_len = 23;
        memcpy(client_id, gFlashParam.st.idInfo, id_len);
    } else {
        /* 无序列号时使用默认ID */
        strcpy(client_id, "EC20_MQTT");
    }

    /*------------------------------------------------------------------------*/
    /* 4. 连接MQTT服务器                                                      */
    /*------------------------------------------------------------------------*/
    LOGI("EC20", "Connecting MQTT...");
    snprintf(cmd_buf, sizeof(cmd_buf), "AT+QMTCONN=0,\"%s\",\"%s\",\"%s\"",
             client_id,
             gFlashParam.st.mqttUserName,
             gFlashParam.st.mqttPassword);

    ret = ec20_send_cmd(cmd_buf, "+QMTCONN: 0,0,0", EC20_AT_LONG_TIMEOUT_MS);
    if (ret != 0) {
        LOGE("EC20", "MQTT connect failed");
        return -1;
    }

    /*------------------------------------------------------------------------*/
    /* 5. 订阅MQTT主题（订阅命令下发、传感器上报、报警上报三个主题）           */
    /*------------------------------------------------------------------------*/
    LOGI("EC20", "Subscribing topics...");

    /* 订阅命令下发主题 */
    if (gFlashParam.st.mqttSub[0] != '\0') {
        snprintf(cmd_buf, sizeof(cmd_buf), "AT+QMTSUB=0,1,\"%s\",0",
                 (char *)gFlashParam.st.mqttSub);
        ec20_send_cmd(cmd_buf, "+QMTSUB: 0,1,0", EC20_AT_LONG_TIMEOUT_MS);
    }

    /* 订阅传感器上报主题 */
    if (gFlashParam.st.mqttPub[0] != '\0') {
        snprintf(cmd_buf, sizeof(cmd_buf), "AT+QMTSUB=0,1,\"%s\",0",
                 (char *)gFlashParam.st.mqttPub);
        ec20_send_cmd(cmd_buf, "+QMTSUB: 0,1,0", EC20_AT_LONG_TIMEOUT_MS);
    }

    /* 订阅报警上报主题 */
    if (gFlashParam.st.mqttAlarmPub[0] != '\0') {
        snprintf(cmd_buf, sizeof(cmd_buf), "AT+QMTSUB=0,1,\"%s\",0",
                 (char *)gFlashParam.st.mqttAlarmPub);
        ec20_send_cmd(cmd_buf, "+QMTSUB: 0,1,0", EC20_AT_LONG_TIMEOUT_MS);
    }

    LOGI("EC20", "=== EC20 MQTT Connected ===");
    return 0;
}

/**
  * @brief  Function implementing the EC20Task thread.
  * @param  argument: Not used
  * @retval None
  */
void EC20Task(void *argument)
{
    int init_ok = 0;
    int mqtt_ok = 0;

    for(;;)
    {
        if (init_ok == 0) {
            init_ok = (ec20_MQTT_init() == 0) ? 1 : 0;
            if (init_ok == 0) {
                LOGE("EC20", "Init failed, retry after 10s...");
                osDelay(10000);
                continue;
            }
        }

        if (mqtt_ok == 0) {
            mqtt_ok = (ec20_mqtt_connect() == 0) ? 1 : 0;
            if (mqtt_ok == 0) {
                LOGE("EC20", "MQTT connect failed, retry after 10s...");
                osDelay(10000);
                continue;
            }
        }

        /* MQTT已连接，后续可以在此处做数据发布/订阅 */
        osDelay(1000);
    }
}
