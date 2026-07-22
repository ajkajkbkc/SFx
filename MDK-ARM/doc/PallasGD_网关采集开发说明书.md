# PallasGD 网关采集与从站数据上传开发说明书

## 一、概述

本说明书描述基于 STM32F103RE + FreeRTOS + Modbus RTU 的网关设备开发需求，实现从站设备扫描、多段寄存器采集、数据格式化、MQTT 上传等功能。

### 硬件平台
- MCU: STM32F103RE (ARM Cortex-M3), 64KB RAM, 512KB Flash
- 通信: W5500 以太网 或 EC20 4G模块
- 显示: OLED 128x64, SSD1306 I2C
- 串口: USART1/2/3 (Modbus RTU 主机)
- 编译器: Keil ARMCC V5.06 update 6 (build 750)

### 软件平台
- RTOS: FreeRTOS V10.0.1 (CMSIS-RTOS v2 wrapper via cmsis_os.h)
- 内存管理: heap_4, configTOTAL_HEAP_SIZE = 22KB
- 协议: Modbus RTU (功能码 0x03 读保持寄存器)
- JSON: cJSON 库 (挂接 pvPortMalloc/vPortFree)
- MQTT: MQTTPacket 库

---

## 二、启动模式与工作模式

### 2.1 模式选择界面
开机首次启动时，OLED 显示 "Select Mode" 选择界面：
- **Device 模式** (独立模式)：仅采集自身数据并上传
- **Gateway 模式** (网关模式)：扫描从站设备、采集所有设备数据并上传

选择结果存入 Flash：`gFlashParam.st.Reserved_Byte[1]`
- `WORK_MODE_DEVICE = 1`
- `WORK_MODE_GATEWAY = 2`

仅首次开机或恢复出厂设置后显示选择界面（`Reserved_Byte[1] == 0` 时出现）。

### 2.2 网口/4G 选择
类似已有 WAN/4G 选择界面（KEY_5/KEY_1 切换，KEY_3 确认）。

---

## 三、从站设备扫描 (AutoFindRtu)

### 3.1 扫描流程
1. 上电后自动扫描 Modbus 地址 1~50 (`MB_RTU_ADDR_MAX = 50`) 的从站
2. 对每个地址发送 `0x03` 功能码读取设备ID寄存器
3. 根据返回的 ID 字符串识别设备类型
4. 网关自身注册为 RTU#0
5. 扫描完成后设置 `EVENT_SCAN_DONE` 事件标志

### 3.2 设备类型识别
通过设备 ID 字符串第 3~6 字符识别产品类型：

| ID前缀 | 设备类型 | 控制点数 | 说明 |
|--------|---------|---------|------|
| DES3 | TYPE_DES3 (0) | 4 | 3要素 SPD |
| DES4 | TYPE_DES4 (1) | 4 | 4要素 SPD |
| DES9 | TYPE_DES9 (2) | 9 | 9要素 SPD |
| DESA | TYPE_DESA (3) | 13 | A型(13要素) SPD |
| G1A3 | TYPE_G1A3 (4) | 5 | 接地电阻监测 |
| M1AI | TYPE_M1AI (5) | 8 | 雷电流监测 |
| L2AI | TYPE_L2AI (6) | 8 | 雷电流监测 |
| LDA1 | TYPE_LDA1 (7) | 1 | 漏电流监测 |
| DEM2 | TYPE_DEM2 (8) | 8 | 智能SPD(8要素) |
| DEM4 | TYPE_DEM4 (9) | 14 | 智能SPD(14要素) |
| DEP4 | TYPE_DEP4 (10) | 3 | SPD(3要素) |
| ESM0 | TYPE_ESM0 (11) | 5 | SPD(5要素) |
| ESM3 | TYPE_ESM3 (12) | 13 | SPD(13要素) |
| G2A3 | TYPE_G2A3 (13) | 5 | 接地电阻监测 |
| DEA2 | TYPE_DEA2 (14) | 9 | 智能SPD(9要素) |
| DEA4 | TYPE_DEA4 (15) | 17 | 智能SPD(17要素) |
| E1P1 | TYPE_E1P1 (16) | 4 | 智能配电(4要素) |
| E1T2 | TYPE_E1T2 (17) | 9 | 温度监测(9要素) |
| E1I2 | TYPE_E1I2 (18) | 9 | 温度监测(9要素) |
| ESF1 | TYPE_ESF1 (19) | 8 | 智能SPD(8要素) |
| ESC3 | TYPE_ESC3 (20) | 6 | 电流监测(6要素) |
| E1A2 | TYPE_E1A2 (21) | 45 | 智能配电(45要素) |
| E1B2 | TYPE_E1B2 (22) | 68 | 智能配电(68要素) |
| E1E2 | TYPE_E1E2 (23) | 74 | 智能配电(74要素) |
| ESML | TYPE_ESML (24) | 18 | 智能SPD(18要素) |
| DESL | TYPE_DESL (25) | 18 | SPD(18要素) |

### 3.3 配置参数
```c
#define MAX_RTU_NUM      30   // 最大从站数（含自身）
#define MAX_CTRLP_NUM    1000 // 最大控制点总数
#define MB_RTU_ADDR_MAX  50   // 扫描地址范围 1~50
```

---

## 四、寄存器采集

### 4.1 单段寄存器采集 (非FG模式)
使用 `Handle_RtuType_ToSend()` 发送 Modbus 读请求，通过 `is_RtuReturn_protocol()` 解析响应。

所有设备使用 UART3 (`huart3`) 进行通信。

### 4.2 多段不连续寄存器采集
对于复杂设备 (E1A2/E1B2/E1E2)，使用段定义方式：

```c
typedef struct {
    uint16_t StartAddr;      // 起始寄存器地址
    uint16_t RegCount;       // 寄存器数量
    uint8_t  CtrlP_StartOff; // 控制点起始偏移
    uint8_t  CtrlP_Count;    // 本段控制点数
} reg_segment_def_t;
```

段定义示例：
```c
static const reg_segment_def_t SegDef_E1A2[] = { {0x0106, 0x58, 0, 45} };
static const reg_segment_def_t SegDef_E1B2[] = {
    {0x0106, 0x58, 0, 45},
    {0x0200, 0x2E, 45, 23}
};
```

多段设备通过 `Handle_MultiSeg_Response()` 分段解析，处理 32 位电流值。

### 4.3 各设备类型寄存器偏移定义

#### DES3 (4点)
| i | 名称 | 寄存器偏移 |
|---|------|-----------|
| 0 | 遥信(YL) | pBuf[5,6] |
| 1 | 空开(KL) | pBuf[7,8] |
| 2 | 空 | 0 |
| 3 | 雷击计数(LtNum) | pBuf[3,4] |

#### DES4 (4点) / DES9 (9点) / DESA (13点)
连续读取：`pBuf[3+2*i]` 前4个；漏流在 pBuf[19+...]；温度在 pBuf[25+...]；电压在 pBuf[13+...]；寿命在 pBuf[31+...]

#### DEM2 (8点) / DEM4 (14点)
| i | 名称 | 寄存器 |
|---|------|--------|
| 0 | VS1 (电压状态1) | pBuf[3,4] |
| 1~3 | PE, SW1, SW2 | pBuf[9+2*(i-1)] |
| 4 | LtNum (计数) | pBuf[19,20] |
| 5~6 | Temp1, Temp2 | pBuf[27+2*(i-5)] |
| 7 | Life (寿命) | pBuf[35,36] |
| 8~9 | VS2, VS3 | pBuf[5+2*(i-8)] |
| 10~11 | SW3, SW4 | pBuf[15+2*(i-11)] |
| 12~13 | Temp3, Temp4 | pBuf[31+2*(i-12)] |

> **注意**：DEM2/DEM4 没有电流值（区别于早期版本）。

#### DEA2 (9点) / DEA4 (17点)
| i | 名称 | 寄存器 |
|---|------|--------|
| 0 | VS1 | pBuf[3,4] |
| 1~3 | PE, SW1, SW2 | pBuf[9+2*(i-1)] |
| 4 | LtNum | pBuf[19,20] |
| 5(仅DEA4) | CurA | pBuf[21,22] |
| 6~7 | Temp1, Temp2 | pBuf[27+2*(i-6)] |
| 8 | Life | pBuf[35,36] |
| 9~10 | VS2, VS3 | pBuf[5+2*(i-9)] |
| 11~12 | SW3, SW4 | pBuf[15+2*(i-11)] |
| 13~14 | CurB, CurC | pBuf[23+2*(i-13)] |
| 15~16 | Temp3, Temp4 | pBuf[31+2*(i-15)] |

#### M1AI / L2AI (8点)
| i | 名称 | 寄存器 |
|---|------|--------|
| 0 | LJ (次数) | pBuf[3,4] |
| 1~7 | LF/JX/LT/... | pBuf[11+2*(i-1)] |

#### DEP4 (3点)
| i | 名称 | 寄存器 |
|---|------|--------|
| 0 | YL (遥信) | pBuf[5,6] |
| 1 | LtNum (雷击计数) | pBuf[9,10] |
| 2 | Temp (温度) | pBuf[11,12] |

#### E1P1 (4点)
| i | 名称 | 寄存器 |
|---|------|--------|
| 0 | DO1 | pBuf[3,4] |
| 1 | DI1 | pBuf[5,6] |
| 2 | DI2 | pBuf[7,8] |
| 3 | UG (零地电压, 32位) | pBuf[11..14] 小端序重组 |

#### E1T2 / E1I2 (9点)
| i | 名称 | 寄存器 |
|---|------|--------|
| 0 | DO1 | pBuf[3,4] |
| 1~8 | TEMP1~TEMP8 | pBuf[5+2*(i-1)] |

#### ESF1 (8点)
| i | 名称 | 寄存器 |
|---|------|--------|
| 0 | DO1 | pBuf[3,4] |
| 1 | DI1 | pBuf[5,6] |
| 2 | DI2 | pBuf[7,8] |
| 3 | CurI | pBuf[9,10] |
| 4~7 | TEMP1~TEMP4 | pBuf[11+2*(i-4)] |

#### ESC3 (6点)
| i | 名称 | 寄存器 |
|---|------|--------|
| 0 | DO1 | pBuf[3,4] |
| 1 | DI1 | pBuf[5,6] |
| 2 | DI2 | pBuf[7,8] |
| 3 | CurA | pBuf[9,10] |
| 4 | CurB | pBuf[11,12] |
| 5 | CurC | pBuf[13,14] |

#### DESL (18点) / ESML (18点)
- i<=13: 连续 `pBuf[3+2*i]` (14个uint16)
- i=14: pBuf[37,38] (特殊项)
- i=15(DESL): pBuf[39,40] / ESML: pBuf[37,38]
- i=16(DESL): pBuf[45,46] / ESML: pBuf[39,40]
- i=17(DESL): pBuf[47,48]

#### E1A2 (45点)
- i=0~2: DO1/DI1/DI2 (uint16, pBuf[3+2*i])
- i=3~44: 32位电流值 (小端序重组)

#### E1B2 (68点) / E1E2 (74点)
- 第1段: 同 E1A2 (45点)
- 第2段: 32位电流值线性映射

---

## 五、控制点数据类型定义

### 5.1 值格式化类型

```c
#define PYLD_IO_TYPE            0  // 数字量（遥信/空开/接地等）
#define PYLD_X1_ANALOG_TYPE     1  // 值直接显示（雷击计数/次数等）
#define PYLD_X10_ANALOG_TYPE    2  // 值÷10（漏流/电压等）
#define PYLD_X10_N_ANALOG_TYPE  3  // 值÷10，允许负值（温度）
#define PYLD_X100_ANALOG_TYPE   4  // 值÷100（接地电阻等）
#define PYLD_XD1_ANALOG_TYPE    5  // 值÷100，2位小数（零地电压/电流）
#define PYLD_X10000_ANALOG_TYPE 6  // 值÷10000，4位小数（电荷量）
```

### 5.2 各设备类型的payload类型分配

参见 `Handle_RtuCtrlP_PyldType()` 函数。每个设备类型的每个控制点都分配了对应的 payload 类型，决定了值的格式化方式。

### 5.3 重要注意事项
- **int16_t 符号扩展问题**：`gCtrlP_Info[].NewValue` 是 `int16_t`，当值作为 `uint32_t` 传入格式化函数时会发生符号扩展。修复方法：使用 `(uint16_t)` 强制转换阻断符号扩展。
- **sizeof 溢出问题**：32位值通过 `memcpy` 存储时，必须使用 `sizeof(gCtrlP_Info[...].NewValue)`（2字节）而非 `sizeof(uint32_t)`（4字节），否则会覆盖相邻的 `AlmState` 字段。

---

## 六、控制点Key字符串映射

Key由原来单字母改为描述性多字符名称：

| Key | 含义 | 适用设备 |
|-----|------|---------|
| YL | 遥信 | DES3/4/9/A, ESM0/3, DESL/ESML, DEP4 |
| KL | 空开 | 同上 |
| PE | 接地 | 同上, DEM2/4, DEA2/4 |
| LtNum | 雷击计数 | 同上, DEM2/4(计数), DEA2/4 |
| CurA/B/C | A/B/C相电流/漏流 | DES9/A, ESM3, DESL/ESML, DEA2/4, ESC3 |
| Temp1/2 | 温度 | DES9/A, ESM0/3, DEM2/4, DEA2/4, E1T2, ESF1 |
| VolA/B/C | A/B/C相电压 | DESA, ESM3, DESL/ESML |
| Life | 寿命 | DESA, ESM3, DEM2/4, DEA2/4, ESML, DESL |
| VS1/2/3 | 电压状态 | DEM2/4, DEA2/4, E1P1(DO1) |
| SW1~4 | 脱扣 | DEM2/4, DEA2/4 |
| DO1 | 数字输出 | E1P1, E1T2, E1I2, ESF1, ESC3, E1A2/B2/E2 |
| DI1~8 | 数字输入 | E1P1, E1T2, E1I2, ESF1, ESC3, E1A2/B2/E2 |
| UG | 零地电压 | E1P1 |
| DJ | 采集次数 | G1A3, G2A3 |
| MIN | 采集频率 | G1A3, G2A3 |
| DRV | 地网电压 | G1A3, G2A3 |
| DR | 地网电阻 | G1A3, G2A3 |
| DRP | 电阻小数位 | G1A3, G2A3 |
| LJ | 雷击次数 | M1AI, L2AI |
| LF/F | 峰值 | M1AI, L2AI |
| JX | 极性 | M1AI, L2AI |
| LT | 持续时间 | M1AI, L2AI |
| Q | 电荷量 | M1AI, L2AI |
| WR | 单位能量 | M1AI, L2AI |
| Z | 波阻抗 | M1AI, L2AI |
| VP | 峰值电压 | M1AI, L2AI |
| LA | 漏电流 | LDA1 |
| U1/U2/U3 | 相电压 | E1A2, E1B2, E1E2 |
| U12/U23/U31 | 线电压 | E1A2, E1B2, E1E2 |
| I1/I2/I3 | 相电流 | E1A2, E1B2, E1E2 |
| FREQ | 频率 | E1A2, E1B2, E1E2 |
| POWP1~3/0 | 有功功率 | E1A2, E1B2, E1E2 |
| POWQ1~3/0 | 无功功率 | E1A2, E1B2, E1E2 |
| POWS1~3/0 | 视在功率 | E1A2, E1B2, E1E2 |
| PF1~3/0 | 功率因素 | E1A2, E1B2, E1E2 |
| EPI1~3/0 | 正向有功电能 | E1A2, E1B2, E1E2 |
| EPE1~3/0 | 反向有功电能 | E1A2, E1B2, E1E2 |
| EQL1~3/0 | 正向无功电能 | E1A2, E1B2, E1E2 |
| EQC1~3/0 | 反向无功电能 | E1A2, E1B2, E1E2 |
| THDUa/b/c | 电压谐波畸变率 | E1E2 |
| THDIa/b/c | 电流谐波畸变率 | E1E2 |
| Hum | 湿度 | DESL, ESML |

---

## 七、数据上传格式

### 7.1 从站数据上传 (kalyke_cycle_post_master_SLAVE)

#### 上传格式
```json
{
  "header": {
    "time": "2026/7/22 16:6:28",
    "client": "KSDSFE1250515001",
    "salveid": "KSDDES9210701001"
  },
  "data": {
    "YL": "0",
    "KL": "0",
    "PE": "0",
    "LtNum": "7",
    "CurA": "0.0A",
    "CurB": "0.0A",
    "CurC": "0.0A",
    "Temp1": "25.6°C",
    "Temp2": "28.4°C"
  }
}
```

#### 格式规则
1. **client** 和 **salveid**：取设备ID前16位，去掉类型后缀（如 `,/DES4`）
2. **单位**：值后追加对应单位（温度 `°C`、电流 `A`、电压 `V`、电阻 `Ω`、功率 `W/Var/VA`、电能 `kWh/kVarh` 等）
3. **°C 编码**：使用 UTF-8 双字节 `0xC2 0xB0`（代码中写为 `"\xC2\xB0" "C"`）
4. **Ω 编码**：使用 UTF-8 双字节 `0xCE 0xA9`（代码中写为 `"\xCE\xA9"`）
5. 每个从站单独一条 MQTT 消息，发布到 `gFlashParam.st.mqttPub` 主题

### 7.2 单位对照表

| 类型Str | 单位 | 说明 |
|--------|------|------|
| Temp1~Temp8 | °C | 温度 |
| CurA/B/C, CurI | A | 电流 |
| I1/I2/I3 | A | 相电流 |
| VolA/B/C | V | 相电压 |
| U1~U31, UG, DRV | V | 电压 |
| DR, ZR1/ZR2, Z | Ω | 电阻 |
| LA | mA | 漏电流 |
| POWP | W | 有功功率 |
| POWQ | Var | 无功功率 |
| POWS | VA | 视在功率 |
| EPI/EPE | kWh | 有功电能 |
| EQL/EQC | kVarh | 无功电能 |
| FREQ | Hz | 频率 |
| LF, F | kA | 雷电流峰值 |
| Q | C | 电荷量 |
| WR | J | 单位能量 |
| Life, Hum | % | 寿命/湿度 |
| YL/KL/PE/DO/DI等 | 无 | 数字量 |

### 7.3 自身设备温度上传
`kalyke_cycle_post_master_DIDOTEMP` 函数上传 DI/DO/温度数据：
```json
{
  "header": {"time": "...", "client": "KSDSFE1250515001", "salveid": "KSDSFE1250515001"},
  "data": {
    "DI1": "0", "DI2": "0", "DI3": "0", "DI4": "0",
    "DO1": "0", "DO2": "0",
    "T1": "25.5°C", "T2": "26.0°C", "T3": "26.5°C", "T4": "27.0°C"
  }
}
```

---

## 八、任务与同步机制

### 8.1 FreeRTOS 任务

| 任务名 | 栈大小 | 优先级 | 说明 |
|-------|-------|--------|------|
| mqttTask | 1024 | - | MQTT 通信 |
| uartTask | 1024 | - | UART 数据处理 |
| keyOledTask | 1024 | - | 按键与 OLED 显示 |
| att7022Task | 1024 | - | 电能计量 |
| collectTask | 2048 | - | 从站数据采集 |
| TmrSvc | 128w | - | 定时器服务 |

### 8.2 同步机制

| 对象 | 类型 | 用途 |
|-----|------|------|
| gGetOneOK_BinSem | Binary Semaphore | 单次采集完成信号 |
| gCtrlPMutexId | Mutex | 控制点数据互斥访问 |
| xCollectEventGroup | Event Group | 扫描完成事件(`EVENT_SCAN_DONE`) |

### 8.3 采集流程
```
AutoFindRtu (FreeRTOS任务启动时)
  → 扫描从站 1~50
  → 设置 EVENT_SCAN_DONE
  → CollectTask 等待 EVENT_SCAN_DONE
    → GetAllRtuValue() 循环采集所有设备
      → 对每个设备发送 Modbus 请求
      → 等待 gGetOneOK_BinSem 信号量
      → is_RtuReturn_protocol() 解析数据
  → MQTT 任务周期调用 kalyke_cycle_post_master_SLAVE()
    → 构建 cJSON → MQTT 发布
```

---

## 九、关键代码文件说明

| 文件 | 说明 |
|------|------|
| `PallasApp/app_collect.h` | 类型定义、宏定义、结构体 |
| `PallasApp/app_collect.c` | 扫描、采集、数据解析 |
| `PallasApp/app_payload.h` | payload类型定义、函数声明 |
| `PallasApp/app_payload.c` | key映射、值格式化、单位映射 |
| `PallasApp/app_main.c` | 主任务、上传函数 |
| `PallasApp/app_oled.c` | 模式选择UI |
| `PallasApp/app_uart.c` | UART接收处理 |
| `PallasApp/Internet/mqtt.c` | MQTT任务 |
| `PallasApp/app_tool.c` | 值转换工具函数 |

---

## 十、编译与部署

### 10.1 编译
- 项目文件：`MDK-ARM/Pallas_GD.uvprojx`
- 编译器：ARMCC V5.06 update 6
- 编译命令：`UV4.exe -r Pallas_GD.uvprojx -j0`
- 编译输出：`Code=~138KB, RO-data=~12KB, RW-data=~3.4KB, ZI-data=~57KB`

### 10.2 内存布局
```
RAM: 0x20000000 - 0x2000FFFF (64KB)
  RW-data: ~3.4KB (已初始化变量)
  ZI-data: ~57KB (零初始化变量 + 堆栈)
  Heap: configTOTAL_HEAP_SIZE = 22KB
  空闲: ~3.6KB
```

### 10.3 Flash 参数存储
- 从站信息：`gRtu_InfoPacket` → Flash
- 报警阈值：`gCtrlP_AlmLim` → Flash

---

## 十一、注意事项与已知问题

1. **UTF-8 编码**：源文件中的非ASCII字符（如 `°`）应使用转义序列 `\xC2\xB0` 而非直接写入字符，避免 Keil ARMCC 按 GBK 解析导致错误。
2. **符号扩展**：`int16_t` 转 `uint32_t` 时使用 `(uint16_t)` 强制转换阻止符号扩展。
3. **32位值存储**：`memcpy` 大小使用 `sizeof(int16_t)` 而非 `sizeof(uint32_t)`，防止覆盖相邻字段。
4. **信号量残留**：每次发送采集请求前调用 `xSemaphoreTake(gGetOneOK_BinSem, 0)` 清除残留状态。
5. **UART 缓冲区**：RX_LEN_UART1 = 512, RX_LEN_UART3 = 512，足够容纳最大 Modbus 响应帧。
