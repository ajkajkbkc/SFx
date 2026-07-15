
/* Private includes ----------------------------------------------------------*/
#include "mqtt.h"
#include "log.h"
#include "event_groups.h"
#include "parameter.h"
#include "W5500Task.h"
#include "socket.h"
#include "mqtt_interface.h"
#include "MQTTClient.h"
#include <string.h>
#include "semphr.h"
/* Private define ------------------------------------------------------------*/
static uint8_t mqttReadBuffer[1024];
static uint8_t mqttSendBuffer[1024];

static MQTTClient mqttClient;
static Network mqttNetwork;
/* Private variables ---------------------------------------------------------*/
uint8_t gMqttInitErrorTimes = 0;
uint8_t gPublishErrorTimes = 0;
uint8_t gMqttPingErrorTimes = 0;

__IO uint8_t gPyldRtu_Idx = 0;  //终端（RTU）索引，用于遍历当前正在处理的是第几个 RTU 终端设备
__IO uint8_t gPyldP_Idx = 0;    //总控制点索引（全局索引），用于遍历 所有终端的全部控制点
__IO uint8_t gPyldOneP_Idx = 0; //单个终端内的控制点子索引（局部索引），用于记录当前正在遍历的是 当前 RTU 终端内部的第几个控制点

__IO uint8_t gMqttPingState = MQTT_NO_PING;

TimerHandle_t MqttPingTimr_Handle = NULL;
SemaphoreHandle_t  CtrlP_MuxSem = NULL;
/* Private function prototypes -----------------------------------------------*/
osThreadId_t mqttTaskHandle;
const osThreadAttr_t mqttTask_attributes =
{
    .name = "mqttTask",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 512 * 4
};

void mqttTask(void *argument);
/* Private user code ---------------------------------------------------------*/

/**
  * @brief  MQTT接收到达
  * @param  MessageData *md 数据包
  * @retval None
  */
static void messageArrived(MessageData *md)
{
    LOGV("mqtt", "Enter %s()", __func__);
    MQTTMessage *message = md->message;
    char *topic = (char *)pvPortMalloc(md->topicName->lenstring.len + 128);
    memset(topic, 0, md->topicName->lenstring.len + 128);
    memcpy(topic, md->topicName->lenstring.data, md->topicName->lenstring.len);
    LOGD("mqtt", "Topic is : %s", topic);

    char *content = (char *)pvPortMalloc(message->payloadlen + 128);
    memset(content, 0, message->payloadlen + 128);
    memcpy(content, message->payload, message->payloadlen);
    LOGI("mqtt", "Payload is : %s", content);

    // UnpackPayLoad(content);  //解析MQTT下发的数据
    vPortFree(topic);
    vPortFree(content);
}

/**
  * @brief  MQTT初始化
  * @param  None
  * @retval true / false
  */
bool Mqtt_Init(void)
{
    LOGV("mqtt", "Enter %s()", __func__ );

    //portENTER_CRITICAL();
    W5500MutexLock();

    uint16_t targetPort = 1883;
    uint8_t targetIP[4] = {8, 129, 232, 136};

    memset(&mqttNetwork, 0, sizeof(mqttNetwork));
    memset(&mqttClient, 0, sizeof(mqttClient));
    NewNetwork(&mqttNetwork, APP_SOCKET_MQTT);  //初始化并配置 MQTT 网络接口,注册底层 I/O 函数指针

    memcpy(targetIP, gFlashParam.st.s1TargetIP, 4);
    targetPort = gFlashParam.st.s1TargetPort;
    LOGW("mqtt", "MQTT target IP address : %d.%d.%d.%d, port : %d", targetIP[0], targetIP[1], targetIP[2], targetIP[3], targetPort);

    ConnectNetwork(&mqttNetwork, targetIP, targetPort); //建立tcp连接
    MQTTClientInit(&mqttClient, &mqttNetwork, 1000, mqttSendBuffer, sizeof(mqttSendBuffer), 
    mqttReadBuffer, sizeof(mqttReadBuffer)); //初始化 MQTT 客户端

    char client[24] = {0};
    memcpy(client, gFlashParam.st.idInfo, 16);
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer; //默认值
    data.willFlag = 0;                              // 不启用遗嘱
    data.MQTTVersion = 3;                           // MQTT 3.1
    data.clientID.cstring = client;                 // 设备 ID
    data.username.cstring = (char *)gFlashParam.st.mqttUserName;
    data.password.cstring = (char *)gFlashParam.st.mqttPassword;
    data.keepAliveInterval = 120;                   // 120 秒心跳
    data.cleansession = 1;                          // 清理旧会话

    int rcCon = MQTTConnect(&mqttClient, &data);
    LOGI("mqtt", "Connected %d, tips:0 sucess, -1 failure, -2 buffer pverflow", rcCon);

    LOGD("mqtt", "Subscribing to %s", gFlashParam.st.mqttSub);
    int rcSub = MQTTSubscribe(&mqttClient, (char *)gFlashParam.st.mqttSub, QOS2, messageArrived);
    LOGI("mqtt", "Subscribed %d,  tips:0 sucess, -1 failure, -2 buffer pverflow", rcSub);

    W5500MutexUnlock();

    if (rcCon == SUCCESSS && rcSub == SUCCESSS)
    {
        PAR_CLEAR_BIT(gParam.st.NetLink_State, NET_STATE_MQTT);

        //portEXIT_CRITICAL();  //退出临界段
        return true;
    }
    else
    {
        PAR_SET_BIT(gParam.st.NetLink_State, NET_STATE_MQTT);

        //portEXIT_CRITICAL();  //退出临界段
        return false;
    }
}

/**
 * @brief  MQTT ping 定时器回调函数，用于检测是否收到服务器的 ping 响应
 * @param  None
 * @retval None
 */
static void MqttPingTimr_Callback(TimerHandle_t ltv_TimeHandle)
{
    LOGW("mqtt", "Enter %s, gMqttPingState = %d", __func__, gMqttPingState);

    if(gMqttPingState != MQTT_PING_RESP_OK)
    {
        LOGE("mqtt", "MQTT ping ERROR!!!");

        PAR_SET_BIT(gParam.st.NetLink_State, NET_STATE_MQTT);
    }
    else
    {
        if(gMqttPingErrorTimes != 0)
        {
            gMqttPingErrorTimes = 0;
        }
    }

    gMqttPingState = MQTT_NO_PING;
}

/**
 * @brief  MQTT ping 定时器回调函数
 * @param  None
 * @retval None
 */
void MqttPingTimr_Start(void)
{
    xTimerStart(MqttPingTimr_Handle, 0);  //开启周期定时器
}

/**
  * @brief  创建传感点互斥量
  */
void CtrlPMutexInit(void)
{
    BaseType_t xReturn = pdPASS;/* 定义一个创建信息返回值，默认为pdPASS */

    /* 创建MuxSem */
    CtrlP_MuxSem = xSemaphoreCreateMutex();
    if(NULL != CtrlP_MuxSem)
    {
        LOGD("internet", "CtrlP_MuxSem mutex create success !\r\n");

        xSemaphoreTake(CtrlP_MuxSem, portMAX_DELAY);

        xReturn = xSemaphoreGive( CtrlP_MuxSem );//给出互斥量
        if( xReturn == pdTRUE )
        {
            LOGD("internet", "CtrlP_MuxSem give success !\r\n");
        }
    }
    else
    {
        LOGE("internet", "CtrlP_MuxSem mutex create FAILED !\r\n");
    }
}
/**
  * @brief  获取传感点互斥锁，保护传感点相关操作不被多任务并发访问
  * @note   配合 CtrlPMutexUnlock() 成对使用，所有访问传感点硬件的操作都应先获取此锁
  * @retval pdTRUE  - 成功获取互斥锁
  *         pdFALSE - 互斥锁未初始化，获取失败
  */
int CtrlPMutexLock(void)
{
    if(NULL == CtrlP_MuxSem)
    {
        return pdFALSE;
    }
    return xSemaphoreTake(CtrlP_MuxSem, portMAX_DELAY);
}
/**
  * @brief  释放传感点互斥锁
  * @note   与 CtrlPMutexLock() 成对使用
  * @retval pdTRUE  - 成功释放互斥锁
  *         pdFALSE - 互斥锁未初始化或释放失败
  */
int CtrlPMutexUnlock(void)
{
    if(NULL == CtrlP_MuxSem)
    {
        return pdFALSE;
    }
    return xSemaphoreGive(CtrlP_MuxSem);
}

/**
  * @brief  新建线程（任务）
  * @param  None
  * @retval None
  */
void osThreadNew_mqttTask(void)
{
    mqttTaskHandle = osThreadNew(mqttTask, NULL, &mqttTask_attributes);
}

/**
  * @brief  MQTT定时发送任务
  * @param  None
  * @retval None
  */
void Mqtt_TrsmNormalTask(void)
{
    static uint32_t TrsmItvSecCnt = 0;
    uint8_t ret;

    //不是第一次并且没到时间发送
    if( (TrsmItvSecCnt != 0) && (gParam.st.SecCnt - TrsmItvSecCnt < gFlashParam.st.mqttPublishInterval * 60) )
    {
        return;
    }

    CtrlPMutexLock();

    TrsmItvSecCnt = gParam.st.SecCnt;

    LOGW("mqtt", "Start to mqtt normal transmit !!!!!!!!!!!!\r\n");
    char *payload = (char *)pvPortMalloc(1024);
    char *topic = (char *)pvPortMalloc(128);
    int payloadlen = 0;

    gPyldRtu_Idx = 0;
    gPyldP_Idx = 0;
    gPyldOneP_Idx = 0;

    memset(topic, 0, 128);
    memcpy(topic, gFlashParam.st.mqttPub, 100);

    for(;;)
    {
        memset(payload, 0, 1024);

        if(gPyldP_Idx == gCtrlP_Num)  //payload结束
        {
            break ;
        }

        GetOnePayload(payload);
        payloadlen = strlen(payload);
        LOGD("mqtt_TrsmNomal", "Publish Topic  : %s", topic);
        LOGI("mqtt_TrsmNomal", "Publish Payload: %s", payload);

        ret = Mqtt_Transmit(topic, payload, payloadlen);
        if(ret == MQTT_SEND_OK)
        {
            gPyldP_Idx++;
            gPyldOneP_Idx++;
            if(gPyldOneP_Idx == CtrlPNum_Table[gRtu_InfoPacket.Rtu_Info[gPyldRtu_Idx].RtuType])
            {
                gPyldRtu_Idx++;
                gPyldOneP_Idx = 0;
            }
        }
        else
        {
            if(ret == MQTT_SEND_ERROVER)
            {
                TrsmItvSecCnt = 0;
                break ;
            }
            else if(ret == MQTT_SEND_ERR)
            {
                continue;
            }
        }
    }

    vPortFree(topic);
    vPortFree(payload);

    CtrlPMutexUnlock();
}

/**
  * @brief  Function implementing the mqttTask thread.
  * @param  argument: Not used
  * @retval None
  */
void mqttTask(void *argument)
{
    LOGD("mqtt", "%s RUN. Free heap size is %d bytes", __func__, xPortGetFreeHeapSize());

    MqttPingTimr_Handle = xTimerCreate((const char *          )"OneShotTimer",
                                       (TickType_t            )MQTT_PING_RECV_MAXTIME, /* 定时器周期 */
                                       (UBaseType_t           )pdFALSE, /* 单次模式 */
                                       (void *                )1, /* 为每个计时器分配一个索引的唯一ID */
                                       (TimerCallbackFunction_t)MqttPingTimr_Callback);
    if(MqttPingTimr_Handle != NULL)
    {
        LOGD("mqtt", "MqttPingTimr_Handle timer create success !\r\n");
    }

    for(;;)
    {
        osDelay(100);

        if((gParam.st.NetLink_State & NET_STATE_OK) != 0)
        {
            PAR_SET_BIT(gParam.st.NetLink_State, NET_STATE_MQTT);
            continue ;
        }

        if((gParam.st.NetLink_State & NET_STATE_MQTT) != 0)
        {
            if(!Mqtt_Init())  //初始化失败
            {
                gMqttInitErrorTimes++;
                disconnect(APP_SOCKET_MQTT);
                LOGE("mqtt", "Mqtt init error, now error times is %d", gMqttInitErrorTimes);
                osDelay(NEXT_TIME_OF_MQTTINIT);

                continue ;
            }
            else
            {
                if(gMqttInitErrorTimes != 0)
                {
                    gMqttInitErrorTimes = 0;
                }
            }
        }

        if(gMqttPingErrorTimes > MQTTPING_ERROR_TIMES)
        {
            gMqttPingErrorTimes = 0;
            PAR_SET_BIT(gParam.st.NetLink_State, NET_STATE_MQTT);
        }

        Mqtt_TrsmNormalTask();

        // Mqtt_TrsmAlmTask();

        // Mqtt_RecvTask();

    }
}
