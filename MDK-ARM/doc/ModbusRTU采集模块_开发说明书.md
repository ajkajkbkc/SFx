# Modbus RTU 采集模块 — 开发说明书

## 一、概述

### 1.1 功能描述

本模块实现基于 STM32 + FreeRTOS 的 Modbus RTU 主站采集功能，通过 USART3（9600bps）轮询总线上的多个从站设备，读取保持寄存器（功能码 0x03），并将数据解析为统一格式的控制点数组供上层使用。

### 1.2 核心特性

- **多段不连续寄存器采集**：通过寄存器段映射表机制，支持寄存器地址不连续的设备
- **控制点偏移查表法**：替代传统 if-else 链，高效解析不同设备类型的响应帧
- **设备自动发现**：扫描总线地址 1~0x50，自动识别设备类型
- **中断驱动接收**：基于 UART RXNE 中断 + 超时定时器的帧接收
- **信号量同步**：采集任务与 UART 处理任务通过二值信号量同步

### 1.3 文件清单

| 文件 | 路径 | 说明 |
|---|---|---|
| `collect.h` | `Func/collect.h` | 数据结构定义、宏定义、函数声明 |
| `collect.c` | `Func/collect.c` | 全部函数实现（约 850 行） |
| `usart.c` | `Core/Src/usart.c` | UART 初始化与接收中断（仅需修改回调函数） |
| `freertos.c` | `Core/Src/freertos.c` | FreeRTOS 任务创建 |

---

## 二、UART 串口配置

### 2.1 USART3 硬件连接

| 引脚 | 功能 |
|---|---|
| PC10 | USART3_TX（发送至从站） |
| PC11 | USART3_RX（接收从站数据） |

### 2.2 参数配置

| 参数 | 值 |
|---|---|
| 波特率 | 9600 |
| 数据位 | 8 |
| 停止位 | 1 |
| 校验位 | NONE |
| 接收超时 | 10ms（软件定时器） |

### 2.3 UART 数据流

```
从站 USART3 TX → STM32 PC11(RX)
  → RXNE 中断 → uartReceive_IDLE_FromISR() 逐字节接收
  → 10ms 超时定时器 → bsp_timer_callback_func()
  → xQueueSend → UartTask (vUartTask)
  → HandleUART3RecvData() → HandleUART3RecvData_Modbus()
  → is_RtuReturn_protocol() / Handle_FindFlkRtu_RecvData()
  → xSemaphoreGive → 唤醒采集任务
```

### 2.4 需要修改 usart.c

在 `HandleUART3RecvData()` 中添加调用：

```c
void HandleUART3RecvData(unsigned char *lcp_Buff, unsigned short lsv_Length)
{
    /* USART3 连接 Modbus RTU 从站，转交给采集模块处理 */
    HandleUART3RecvData_Modbus(lcp_Buff, lsv_Length);
}
```

并在 `USER CODE BEGIN 0` 中添加：

```c
#include "EC20Task.h"       /* 解决 EC20_UART_RxCallback 隐式声明警告 */
void HandleUART3RecvData_Modbus(unsigned char *pBuf, unsigned short len);
```

---

## 三、数据结构定义

### 3.1 设备类型枚举（collect.h）

```c
#define TYPE_DES3       0
#define TYPE_DES4       1
#define TYPE_DES9       2
#define TYPE_DESA       3
#define TYPE_G1A3       4
#define TYPE_M1AI       5
#define TYPE_L2AI       6
#define TYPE_LDA1       7
#define TYPE_DEM2       8
#define TYPE_DEM4       9
#define TYPE_DEP4       10
#define TYPE_ESM0       11
#define TYPE_ESM3       12
#define TYPE_G2A3       13
#define MAX_PRODEUCT_TYPE  14
```

### 3.2 控制点结构体 `ctrl_point_t`

```c
typedef struct
{
    uint16_t RawValue;                  /* 原始寄存器值 */
    uint8_t  Type;                      /* 控制点类型 */
    uint8_t  AlmLimType;                /* 报警门限类型 */
    uint8_t  PyldType;                  /* 载荷数据类型 */
    uint8_t  Decimal;                   /* 小数位数 (模拟量) */
    int32_t  ScaleNumerator;            /* 缩放分子 */
    int32_t  ScaleDenominator;          /* 缩放分母 */
    int32_t  ScaledValue;               /* 缩放后的工程值 */
    uint8_t  AlarmFlag;                 /* 报警标志 */
} ctrl_point_t;
```

### 3.3 寄存器段定义 `reg_segment_def_t`

```c
typedef struct
{
    uint16_t StartAddr;        /* Modbus 寄存器起始地址 */
    uint16_t RegCount;         /* 本段读取的寄存器数量 */
    uint8_t  CtrlP_StartOff;   /* 本段对应控制点数组的起始偏移 */
    uint8_t  CtrlP_Count;      /* 本段包含的控制点个数 */
} reg_segment_def_t;
```

### 3.4 设备寄存器映射表 `device_reg_map_t`

```c
typedef struct
{
    uint8_t  SegCount;                  /* 段数(1=单段, 2+=多段) */
    const reg_segment_def_t *pSegments; /* 各段定义指针 */
} device_reg_map_t;
```

### 3.5 从站设备信息 `rtu_device_info_t`

```c
typedef struct
{
    uint8_t  RtuIdNum;    /* Modbus 地址 */
    uint8_t  RtuType;     /* 设备类型 */
    uint8_t  RtuPort;     /* 串口端口 */
    uint8_t  Enable;      /* 使能标志 */
} rtu_device_info_t;
```

### 3.6 设备信息包 `rtu_info_packet_t`

```c
typedef struct
{
    uint8_t           DeviceCount;      /* 设备数量 */
    rtu_device_info_t Devices[32];      /* 设备列表 */
} rtu_info_packet_t;
```

---

## 四、设备段定义与映射表

### 4.1 单段设备定义（collect.c）

所有现有设备均为单段（SegCount=1），寄存器段定义如下：

| 设备类型 | 起始地址 | 寄存器数 | 控制点数 | 段定义变量 |
|---|---|---|---|---|
| DES3 | 0x07D1 | 3 | 4 | `SegDef_DES3` |
| DES4 | 0x0106 | 4 | 4 | `SegDef_DES4` |
| DES9 | 0x0106 | 14 | 9 | `SegDef_DES9` |
| DESA | 0x0106 | 15 | 13 | `SegDef_DESA` |
| G1A3 | 0x0106 | 5 | 5 | `SegDef_G1A3` |
| M1AI | 0x0106 | 11 | 6 | `SegDef_M1AI` |
| L2AI | 0x0106 | 11 | 4 | `SegDef_L2AI` |
| LDA1 | 0x0106 | 14 | 1 | `SegDef_LDA1` |
| DEM2 | 0x0106 | 17 | 9 | `SegDef_DEM2` |
| DEM4 | 0x0106 | 17 | 17 | `SegDef_DEM4` |
| DEP4 | 0x0106 | 5 | 5 | `SegDef_DEP4` |
| ESM0 | 0x0106 | 5 | 5 | `SegDef_ESM0` |
| ESM3 | 0x0106 | 15 | 15 | `SegDef_ESM3` |
| G2A3 | 0x0021 | 5 | 5 | `SegDef_G2A3` |

代码示例：

```c
static const reg_segment_def_t SegDef_DES3[] = { {0x07D1, 0x03, 0, 4} };
static const reg_segment_def_t SegDef_DES4[] = { {0x0106, 0x04, 0, 4} };
// ... 以此类推
```

### 4.2 设备寄存器映射表

```c
static const device_reg_map_t s_DeviceRegMap[MAX_PRODEUCT_TYPE] =
{
    /* TYPE_DES3 */ {1, SegDef_DES3},
    /* TYPE_DES4 */ {1, SegDef_DES4},
    /* TYPE_DES9 */ {1, SegDef_DES9},
    /* ... 以此类推 14 种设备 ... */
    /* TYPE_G2A3 */ {1, SegDef_G2A3},
};
```

### 4.3 控制点数量表

```c
const uint8_t CtrlPNum_Table[MAX_PRODEUCT_TYPE] =
{
    CTRLP_NUM_DES3,   /* 4 */
    CTRLP_NUM_DES4,   /* 4 */
    CTRLP_NUM_DES9,   /* 9 */
    // ... 对应宏定义见 collect.h
};
```

---

## 五、控制点偏移表（响应解析优化）

### 5.1 三种偏移模式

| 模式 | 说明 | 适用设备 |
|---|---|---|
| **线性偏移（NULL）** | 第 i 个控制点 = `pBuf[3 + 2*i]` | DES4, G1A3, G2A3 |
| **固定偏移表** | 每个控制点有独立的字节偏移值 | DES3, LDA1, DEP4, ESM0, ESM3 |
| **分段偏移表** | 各控制点在响应帧中位置不规则 | DES9, DESA, M1AI, L2AI, DEM2, DEM4 |

### 5.2 各设备类型偏移表

```c
/* CTRLP_FIXED_ZERO(0xFFFF) 标记该控制点值恒为 0 */
static const uint16_t CtrlPOffset_DES3[] = {5, 7, CTRLP_FIXED_ZERO, 3};
static const uint16_t CtrlPOffset_DES9[] = {3, 5, 7, 9, 19, 21, 23, 25, 27};
static const uint16_t CtrlPOffset_DESA[] = {3, 5, 7, 9, 19, 21, 23, 25, 27, 13, 15, 17, 31};
static const uint16_t CtrlPOffset_M1AI[] = {3, 11, 13, 15, 19, 21};
static const uint16_t CtrlPOffset_L2AI[] = {3, 11, 13, 15};
static const uint16_t CtrlPOffset_LDA1[] = {25};
static const uint16_t CtrlPOffset_DEM2[] = {3, 9, 11, 13, 19, 21, 27, 29, 35};
static const uint16_t CtrlPOffset_DEM4[] = {3, 9, 11, 13, 19, 21, 27, 29, 35, 5, 7, 15, 17, 23, 25, 31, 33};
static const uint16_t CtrlPOffset_DEP4[] = {5, CTRLP_FIXED_ZERO, CTRLP_FIXED_ZERO, 9, 11};
static const uint16_t CtrlPOffset_ESM0[] = {3, 5, 7, 9, 11};
static const uint16_t CtrlPOffset_ESM3[] = {3, 5, 7, 9, CTRLP_FIXED_ZERO, 13, 15, 17, 19, 21, 23, 25, 27, CTRLP_FIXED_ZERO, 31};
```

### 5.3 主指针数组

```c
static const uint16_t* const s_CtrlPOffsetTable[MAX_PRODEUCT_TYPE] =
{
    CtrlPOffset_DES3,   /* TYPE_DES3 */
    NULL,               /* TYPE_DES4 - 线性偏移 */
    CtrlPOffset_DES9,   /* TYPE_DES9 */
    CtrlPOffset_DESA,   /* TYPE_DESA */
    NULL,               /* TYPE_G1A3 - 线性偏移 */
    CtrlPOffset_M1AI,   /* TYPE_M1AI */
    CtrlPOffset_L2AI,   /* TYPE_L2AI */
    CtrlPOffset_LDA1,   /* TYPE_LDA1 */
    CtrlPOffset_DEM2,   /* TYPE_DEM2 */
    CtrlPOffset_DEM4,   /* TYPE_DEM4 */
    CtrlPOffset_DEP4,   /* TYPE_DEP4 */
    CtrlPOffset_ESM0,   /* TYPE_ESM0 */
    CtrlPOffset_ESM3,   /* TYPE_ESM3 */
    NULL,               /* TYPE_G2A3 - 线性偏移 */
};
```

### 5.4 解析逻辑（Handle_MultiSeg_Response 核心）

```c
const uint16_t *pOffsets = s_CtrlPOffsetTable[RtuType];
if(pOffsets != NULL)
{
    uint16_t byteOff = pOffsets[pSeg->CtrlP_StartOff + i];
    if(byteOff == CTRLP_FIXED_ZERO)
        gCtrlPointBuf[destIdx].RawValue = 0;
    else
        gCtrlPointBuf[destIdx].RawValue = (uint16_t)pBuf[byteOff] << 8 | pBuf[byteOff + 1];
}
else
{
    /* 线性偏移默认处理 */
    gCtrlPointBuf[destIdx].RawValue = GET_BIGPU16_DATA(&pBuf[3 + i * 2]);
}
```

---

## 六、核心函数说明

### 6.1 发送相关

| 函数 | 说明 |
|---|---|
| `Modbus_CRC16()` | 查表法计算 Modbus CRC16 |
| `Modbus_CheckCRC()` | 校验帧尾 2 字节 CRC |
| `Modbus_SendFrame()` | 通过 USART3 发送数据帧（带互斥锁和 TC 等待） |
| `SendSegmentReadCmd()` | 构造并发送 Modbus 0x03 读指令（静态函数） |
| `Handle_RtuType_SegmentSend()` | 根据设备类型+段索引发送 |
| `Handle_RtuType_ToSend()` | 单段设备兼容接口 |

### 6.2 接收与解析

| 函数 | 说明 |
|---|---|
| `Handle_MultiSeg_Response()` | 核心响应解析：CRC校验 → 偏移查表 → 赋值控制点 |
| `is_RtuReturn_protocol()` | 响应入口：多段/单段设备分支 |
| `HandleUART3RecvData_Modbus()` | UART 回调入口，含发现模式/正常模式路由 |

### 6.3 采集循环

| 函数 | 说明 |
|---|---|
| `GetAllRtuValue()` | 遍历所有使能设备，段级轮询：发指令 → 等信号量 → 解析 |
| `collectTask()` | FreeRTOS 任务入口：初始化 → 无限采集循环（间隔 1s） |
| `RtuDevice_Init()` | 初始化设备列表（当前为空，需填充实际设备） |

### 6.4 设备发现

| 函数 | 说明 |
|---|---|
| `AutoFindRtu()` | 扫描地址 1~0x50，发送读 ID 指令 |
| `Handle_FindFlkRtu_RecvData()` | 从响应帧提取 22 字节 ID，匹配型号字符串 |

---

## 七、采集流程

### 7.1 正常运行流程

```
collectTask()
  → RtuDevice_Init()                    // 加载设备列表
  → for(;;)
      → GetAllRtuValue()                // 一轮采集
          → for 每个设备
              → for 每个段(Segment)
                  → Handle_RtuType_SegmentSend()  // 发 Modbus 指令
                  → xSemaphoreTake(200ms 超时)    // 等待响应
                  → 收到信号量 → 数据已在 gCtrlPointBuf 中
                  → 超时 → 跳过该设备剩余段
              → gCtrlP_Idx += CtrlPNum_Table[type]
      → osDelay(1000)                    // 采集间隔 1 秒
```

### 7.2 响应帧处理流程（UART 侧）

```
USART3 RXNE 中断 → 逐字节接收
  → 10ms 无新字节 → 超时回调
  → xQueueSend → vUartTask 接收队列
  → HandleUART3RecvData()
  → HandleUART3RecvData_Modbus()
      ├─ 发现模式: Handle_FindFlkRtu_RecvData() → 加入设备列表
      └─ 正常模式: is_RtuReturn_protocol() → Handle_MultiSeg_Response()
      → xSemaphoreGive(gGetOneOK_BinSem)  // 通知采集任务
```

---

## 八、Modbus 协议细节

### 8.1 发送指令格式（功能码 0x03）

```
[站号:1B] [0x03:1B] [起始地址高:1B] [起始地址低:1B] [寄存器数高:1B] [寄存器数低:1B] [CRC低:1B] [CRC高:1B]
```

### 8.2 响应帧格式

```
[0:站号] [1:0x03] [2:字节数] [3:4:数据0] [5:6:数据1] ... [末尾2B:CRC]
```

### 8.3 CRC16 算法

- 多项式：`0x8005`（反转后 `0xA001`）
- 初始值：`0xFFFF`
- 小端序：CRC 低字节在前，高字节在后
- 实现方式：256 元素查表法

---

## 九、设备自动发现

### 9.1 发现指令

发送：`[地址] 0x03 0x00 0x01 0x00 0x0B [CRC]`
读取从地址 `0x0001` 开始的 11 个寄存器（22 字节设备 ID）。

### 9.2 设备 ID 格式

响应帧中 `pBuf[3]~pBuf[24]` 为 22 字节设备 ID 字符串。
设备型号存储在最后 4 字节 `pBuf[21]~pBuf[24]`（对应 `RtuIdInfo[18..21]`）。

例如：`"KSDDES4190801001,/DES4"` → 型号 `"DES4"`

### 9.3 设备 ID 匹配表

```c
static const device_id_match_t s_DeviceIdTable[] =
{
    {"DES4", TYPE_DES4},  {"DES9", TYPE_DES9},  {"DESA", TYPE_DESA},
    {"G1A3", TYPE_G1A3},  {"G2A3", TYPE_G2A3},
    {"M1AI", TYPE_M1AI},  {"L2AI", TYPE_L2AI},  {"LDA1", TYPE_LDA1},
    {"DEM2", TYPE_DEM2},  {"DEA2", TYPE_DEM2},   /* 别名映射 */
    {"DEM4", TYPE_DEM4},  {"DEA4", TYPE_DEM4},   /* 别名映射 */
    {"DEP4", TYPE_DEP4},  {"ESM0", TYPE_ESM0},   {"ESM3", TYPE_ESM3},
};
```

---

## 十、全局变量

| 变量 | 类型 | 说明 |
|---|---|---|
| `gRtu_SegmentIdx` | `volatile uint8_t` | 当前段索引 |
| `gRtu_Idx` | `volatile uint8_t` | 当前设备索引 |
| `gCtrlP_Idx` | `volatile uint16_t` | 全局控制点游标 |
| `gRtu_Num` | `volatile uint8_t` | 设备总数 |
| `gRtu_InfoPacket` | `rtu_info_packet_t` | 设备列表 |
| `gCtrlPointBuf[]` | `ctrl_point_t[256]` | 控制点数据缓冲区 |
| `gModbusTxBuf[]` | `uint8_t[256]` | 发送缓冲区 |
| `gModbusRxBuf[]` | `uint8_t[256]` | 接收缓冲区（当前未使用） |
| `gGetOneOK_BinSem` | `SemaphoreHandle_t` | 采集完成信号量 |
| `gModbusTxMutex` | `SemaphoreHandle_t` | 串口发送互斥量 |

---

## 十一、FreeRTOS 任务配置

在 `freertos.c` 的 `MX_FREERTOS_Init()` 中添加：

```c
collectTaskHandle = osThreadNew(collectTask, NULL, &collectTask_attributes);
```

任务属性：

```c
const osThreadAttr_t collectTask_attributes =
{
    .name = "collectTask",
    .priority = (osPriority_t) osPriorityNormal,
    .stack_size = 512 * 4
};
```

---

## 十二、新建设备适配步骤

当需要接入一个新的从站设备时：

### 步骤 1：添加设备类型枚举

在 `collect.h` 中：

```c
#define TYPE_NEW      14
#define MAX_PRODEUCT_TYPE  15
```

添加控制点数量宏：

```c
#define CTRLP_NUM_NEW    5
```

更新 `CtrlPNum_Table[]`。

### 步骤 2：定义寄存器段表

```c
static const reg_segment_def_t SegDef_NewDevice[] = {
    {0x0010, 2, 0, 2},    /* 段0: 地址0x0010, 2个寄存器 → 控制点0~1 */
    {0x0100, 3, 2, 3},    /* 段1: 地址0x0100, 3个寄存器 → 控制点2~4 */
};
```

### 步骤 3：注册到设备映射表

```c
/* TYPE_NEW */ {2, SegDef_NewDevice},
```

### 步骤 4：添加控制点偏移表

```c
static const uint16_t CtrlPOffset_NEW[] = {3, 5, 9, 11, 13};
```

在 `s_CtrlPOffsetTable[]` 中添加指针。

### 步骤 5：添加设备 ID 匹配

```c
{"NEW4", TYPE_NEW},
```

### 步骤 6：实现控制点属性回调

使用 `__attribute__((weak))` 重写 `GetCtrlP_AlmLimType()`、`GetCtrlP_PyldType()`、`GetCtrlP_TypeStr()`。

---

## 十三、遗留问题和改进建议

### ?? 问题 1：信号量释放函数使用不当

`HandleUART3RecvData_Modbus()` 中调用了 `xSemaphoreGiveFromISR()`，但该函数是在 **UART 任务上下文**（非 ISR）中执行的。应改用 `xSemaphoreGive()`。

**修改方法**：将发现模式和正常模式中的 `xSemaphoreGiveFromISR` 替换为 `xSemaphoreGive`。

### ?? 问题 2：RtuDevice_Init() 为空

当前 `RtuDevice_Init()` 只是清空设备列表，未从 Flash/参数区加载实际设备配置，也未添加测试设备。**设备列表为空时 GetAllRtuValue() 直接跳过**，不会进行任何采集。

**需要实现**：从 Flash 参数区或通过外部配置接口填充 `gRtu_InfoPacket.Devices[]`。

### ?? 问题 3：总控制点数未单独统计

`gCtrlP_Idx` 在采集循环中作为游标使用，遍历结束后其值等于总控制点数，但没有单独的变量永久保存这个值。建议添加：

```c
uint16_t GetTotalCtrlPCount(void)
{
    uint16_t total = 0;
    for(uint8_t i = 0; i < gRtu_Num; i++)
        total += CtrlPNum_Table[gRtu_InfoPacket.Devices[i].RtuType];
    return total;
}
```

### ?? 问题 4：未处理 Modbus 异常响应

当从站返回异常响应（如 0x83）时，当前代码会因 `pBuf[FUNC_IDX] != 0x03` 校验失败而丢弃，但未记录错误原因。建议增加异常码解析。

### ?? 问题 5：gModbusRxBuf 未使用

声明了 `gModbusRxBuf[256]` 但从未在代码中引用。当前接收直接使用 UART 框架的 `gcv_Uart3RecvBuf[]`。可删除或保留供将来 DMA 模式使用。

### ?? 问题 6：缩放和报警功能未实现

`ctrl_point_t` 中的 `ScaleNumerator`、`ScaleDenominator`、`ScaledValue`、`AlarmFlag` 字段已定义，但没有任何代码对其进行计算或设置。需要实现控制点工程值缩放和报警判定逻辑。

### ?? 问题 7：采集任务在硬件初始化前启动

如果 `collectTask` 在 USART3 完全初始化之前启动，可能导致异常。确保 `MX_USART3_UART_Init()` 在 `osKernelStart()` 之前已调用（当前 `main.c` 中已满足）。

### 建议：增加采集统计

建议添加运行时统计变量：

```c
volatile uint32_t gCollect_CycleCount;     /* 采集轮次计数 */
volatile uint32_t gCollect_TimeoutCount;   /* 超时计数 */
volatile uint32_t gCollect_ErrorCount;     /* 错误计数 */
```

---

## 十四、编译说明

### 14.1 工具链

- 编译器：ARMCC V5.06 update 6 (build 750)
- IDE：Keil MDK uVision5
- 芯片：STM32F103xE

### 14.2 编译结果（当前）

```
Program Size: Code=43480 RO-data=2484 RW-data=340 ZI-data=60508
0 Error(s), 0 Warning(s)
```

### 14.3 编译命令

```bash
C:\Keil_v5\UV4\UV4.exe -r MDK-ARM.uvprojx -j0 -t "MDK-ARM"
```
