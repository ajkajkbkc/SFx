# Modbus 从站协议开发说明书（新平台）

> **版本**: 1.0  
> **目标**: 在新平台（STM32F103 + FreeRTOS）上实现一套**完全合理、易读、可维护**的 Modbus RTU/TCP 从站协议栈  
> **原则**: 不向后兼容旧设备，所有设计从零开始，按 Modbus 标准规范实现

---

## 一、总体架构

```
┌──────────────────────────────────────────────────────┐
│                  上层应用 (app_main)                   │
├──────────────────────────────────────────────────────┤
│   mb_slave_msg_handler()       协议帧分发             │
│   ├─ mb_slave_read_holding_register()   0x03         │
│   ├─ mb_slave_write_register()          0x06         │
│   ├─ mb_slave_write_multiple_registers() 0x10        │
│   └─ ...其他功能码                                  │
├──────────────────────────────────────────────────────┤
│   handler_table[]       函数指针映射表                │
│   mb_find_entry()       地址 → handler 查找           │
├──────────────────────────────────────────────────────┤
│   通用数据 handler       特殊业务 handler              │
│   read_flash_word        mb_handle_read_rtc           │
│   write_ram_halfword     mb_handle_write_value        │
│   ...                   ...                          │
├──────────────────────────────────────────────────────┤
│   传输层适配                                            │
│   UART1/2/3  │  TCP 端口502  │  MQTT                 │
└──────────────────────────────────────────────────────┘
```

---

## 二、地址空间设计（关键改进）

### 2.1 改进要点

旧设备最大的设计缺陷是**地址空间不统一**——前段（0x0001~0x001C）是"入口选择码"，后段是真正的连续寄存器。新平台必须**全部采用标准 Modbus 连续地址空间**。

### 2.2 地址分配表

| 起始地址 | 结束地址 | 寄存器数 | 用途 | 存储位置 | 读写属性 |
|---------|---------|---------|------|---------|---------|
| 0x0001 | 0x000B | 11 | 模块ID信息 | Flash | 读/写 |
| 0x000C | 0x0015 | 10 | MQTT用户名 | Flash | 读/写 |
| 0x0016 | 0x001F | 10 | MQTT密码 | Flash | 读/写 |
| 0x0020 | 0x0051 | 50 | MQTT推送主题 | Flash | 读/写 |
| 0x0052 | 0x0083 | 50 | MQTT订阅主题 | Flash | 读/写 |
| 0x0084 | 0x00B5 | 50 | MQTT报警推送主题 | Flash | 读/写 |
| 0x00B6 | 0x00B7 | 2 | 网关IP | Flash | 读/写 |
| 0x00B8 | 0x00B9 | 2 | 本机IP | Flash | 读/写 |
| 0x00BA | 0x00BB | 2 | S0通道目标IP | Flash | 读/写 |
| 0x00BC | 0x00BC | 1 | S0通道本机端口 | Flash | 读/写 |
| 0x00BD | 0x00BD | 1 | S0通道目标端口 | Flash | 读/写 |
| 0x00BE | 0x00BF | 2 | S1通道目标IP | Flash | 读/写 |
| 0x00C0 | 0x00C0 | 1 | S1通道本机端口 | Flash | 读/写 |
| 0x00C1 | 0x00C1 | 1 | S1通道目标端口 | Flash | 读/写 |
| 0x00C2 | 0x00C2 | 1 | 网关轮询终端间隔 | Flash | 读/写 |
| 0x00C3 | 0x00C3 | 1 | MQTT上传间隔(分钟) | Flash | 读/写 |
| 0x00C4 | 0x00C6 | 3 | MAC地址 | Flash | 读/写 |
| 0x00C7 | 0x00C8 | 2 | 子网掩码 | Flash | 读/写 |
| 0x0081 | 0x0083 | 3 | RTC时间 | 实时 | 读/写 |
| 0x0100 | 0x0100 | 1 | 清除雷击计数 | — | 只写 |
| 0x0101 | 0x0101 | 1 | 模块站号 | Flash | 读/写 |
| 0x0103 | 0x0103 | 1 | 雷电流内存清除 | — | 只写 |
| 0x0104 | 0x0105 | 2 | 接地电阻采集频率 | Flash | 读/写 |
| 0x0106 | 0x0133 | 46 | **模块当前实时值** | 实时 | 读/选择性写 |
| 0x0134 | 0x0137 | 4 | SPD门限 | Flash | 读/写 |
| 0x0180 | 0x0181 | 2 | 接地电阻修正系数 | Flash | 读/写 |
| 0x0182 | 0x0182 | 1 | Zigbee恢复出厂 | — | 只写 |
| 0x0401 | 0x04C8 | 200 | Flash持久化参数(32bit) | Flash | 读/写 |
| 0x04C9 | 0x0590 | 200 | Flash持久化参数(16bit) | Flash | 读/写 |
| 0x0591 | 0x0800 | 624 | Flash持久化参数(8bit) | Flash | 读/写 |
| 0x0801 | 0x081E | 30 | 运行时参数(32bit) | RAM | 读/写 |
| 0x081F | 0x0904 | 230 | 运行时参数(16bit) | RAM | 读/写 |
| 0x0905 | 0x092C | 40 | 运行时参数(8bit) | RAM | 读/写 |
| 0x092D | 0x0D2C | — | 雷电流数据表 | Flash/RAM | 只读 |
| 0x1000 | 0x10FF | 256 | 计量寄存器(仅FSS) | 实时 | 只读 |
| 0x1100 | 0x1100 | 1 | 校表指令(仅FSS) | — | 读/写 |

### 2.3 地址设计原则

1. **连续排列**：每个数据块结束后，下一个紧接开始，不留空洞
2. **分区清晰**：系统配置区(0x0001~0x00C8) / 功能控制区(0x0100~0x0182) / 参数存储区(0x0401~0x0800) / 运行时参数区(0x0801~0x092C) / 特殊功能区(0x092D+)
3. **每个地址 = 一个 16bit 寄存器**：地址连续即寄存器连续，符合标准 Modbus 语义
4. **`maxNum` = 实际寄存器数量**：不再用 `maxNum` 模拟数据块大小，而是真实的连续地址数

---

## 三、核心数据结构

### 3.1 映射表条目

```c
typedef struct {
    uint16_t    startAddr;      // Modbus 协议起始地址
    uint16_t    regCount;       // 连续寄存器数量（不再同时使用 endAddr + maxNum）
    uint16_t    accessFlags;    // 访问属性: kReadable, kWritable, kBroadcast
    mb_read_handler_t  readHandler;   // 读 handler (NULL=不支持读)
    mb_write_handler_t writeHandler;  // 写 handler (NULL=不支持写)
} mb_map_entry_t;
```

**改进说明**：原设计中 `startAddr`/`endAddr`/`maxNum` 三个字段存在冗余和矛盾（`endAddr - startAddr + 1 != maxNum`），现在简化为 `startAddr` + `regCount`，两者必然一致。

### 3.2 访问属性宏

```c
#define MB_ACCESS_READ      (1 << 0)
#define MB_ACCESS_WRITE     (1 << 1)
#define MB_ACCESS_BROADCAST (1 << 2)
```

---

## 四、Handler 函数设计

### 4.1 函数指针类型

```c
// 读 handler: 读取 elementAddr 开始的 elementCnt 个寄存器，写入 outBuf (大端序)
// 返回 true=成功, false=失败
typedef bool (*mb_read_handler_t)(
    uint16_t elementAddr,
    uint16_t elementCnt,
    uint8_t *outBuf,
    uint16_t *outLen);

// 写 handler: 将 data (大端序 uint16_t 数组) 写入 elementAddr
typedef bool (*mb_write_handler_t)(
    uint16_t elementAddr,
    uint16_t elementCnt,
    const uint16_t *data);
```

### 4.2 Handler 分类

| 类别 | 函数 | 说明 |
|------|------|------|
| 通用存储 | `read_flash_word` / `write_flash_word` | Flash 32-bit 参数 |
| 通用存储 | `read_flash_halfword` / `write_flash_halfword` | Flash 16-bit 参数 |
| 通用存储 | `read_ram_word` / `write_ram_word` | RAM 32-bit 参数 |
| 通用存储 | `read_ram_halfword` / `write_ram_halfword` | RAM 16-bit 参数 |
| 通用存储 | `read_ltlist` | 雷电流数据（只读） |
| 字符串 | `read_flash_string` / `write_flash_string` | 字符串型 Flash 参数 |
| IP/Port | `read_ip_address` / `write_ip_address` | IP 地址（需反转字节序） |
| 特殊 | `handle_read_rtc` / `handle_write_rtc` | RTC 时间 |
| 特殊 | `handle_read_value` / `handle_write_value` | 模块当前值（产品相关） |
| 特殊 | `handle_write_des4lj_clear` | 清除雷击计数（只写） |

### 4.3 改进要点

1. **消除重复**：所有 IP 地址 handler 合并为 `read_ip_address` + 参数指定操作对象，不再每个 IP 写一个 wrapper
2. **消除 #if PROD_TYPE**：产品差异通过 handler 注册表实现，而非预处理器宏
3. **只写寄存器明确返回 `false`**：不再静默忽略非法写入操作

---

## 五、协议帧处理

### 5.1 帧格式校验

```c
// 单一入口，统一校验
bool mb_validate_frame(const uint8_t *frame, uint16_t len)
{
    // 1. 最小长度检查 (len >= 5)
    // 2. 从站地址匹配 (支持广播 0x00)
    // 3. CRC16 校验 (仅 RTU 模式, TCP 模式跳过)
    // 4. 功能码合法性检查
    // 5. 根据功能码校验特定长度约束
    return true;
}
```

### 5.2 主分发函数

```c
void mb_slave_msg_handler(md_slave_msg_pack *pMsg)
{
    // 1. 校验帧 (CRC + 格式)
    // 2. 根据功能码分发:
    //    0x03 → mb_handle_read_holding_registers()
    //    0x06 → mb_handle_write_single_register()
    //    0x10 → mb_handle_write_multiple_registers()
    //    other → mb_send_error(kIllegalFunction)
    // 3. 所有分支共用错误处理路径
}
```

### 5.3 读寄存器处理流程 (0x03)

```
mb_handle_read_holding_registers(pMsg):
  ├─ 解析 mbAddr, elementCnt
  ├─ elementCnt > MB_MAX_R_WORD_NUM → 错误
  ├─ mb_find_entry(mbAddr, elementCnt, &entry)
  │    └─ 失败 → mb_send_error(kIllegalAddress)
  ├─ 广播检查 (entry->accessFlags & kBroadcast)
  ├─ entry->readHandler == NULL → 返回空响应
  ├─ entry->readHandler(elementAddr, cnt, buf, &len)
  │    └─ 失败 → mb_send_error(kIllegalData)
  └─ 构建响应帧, mb_send_response()
```

### 5.4 写寄存器处理流程 (0x06 / 0x10)

```
mb_handle_write_register(pMsg):
  ├─ 解析 mbAddr, value
  ├─ mb_find_entry(mbAddr, 1, &entry)
  │    └─ 失败 → mb_send_error(kIllegalAddress)
  ├─ 广播检查
  ├─ entry->writeHandler == NULL → 返回错误 (不再静默忽略)
  ├─ entry->writeHandler(elementAddr, 1, &data)
  │    └─ 失败 → mb_send_error(kIllegalData)
  └─ 构建回显响应帧
```

---

## 六、响应处理（关键修复）

### 6.1 统一错误响应（修复旧代码的 Bug）

```c
void mb_send_error_response(md_slave_msg_pack *pMsg)
{
    uint8_t funcCode = pMsg->mcp_ReceiveBuff[1] | 0x80;
    uint8_t errCode  = pMsg->mcv_ErrorCode;

    // 构造异常响应帧
    pMsg->mcp_RespBuff[0] = pMsg->mcp_ReceiveBuff[0];  // 从站地址
    pMsg->mcp_RespBuff[1] = funcCode;                    // 功能码 | 0x80
    pMsg->mcp_RespBuff[2] = errCode;                     // 异常码
    pMsg->msv_RespLen = 3;

    // UART: 追加 CRC 后发送
    if (is_uart_sender(pMsg->mcv_Sender)) {
        uint16_t crc = calc_crc16(pMsg->mcp_RespBuff, 3);
        pMsg->mcp_RespBuff[3] = crc & 0xFF;
        pMsg->mcp_RespBuff[4] = (crc >> 8) & 0xFF;
        pMsg->msv_RespLen = 5;

        if (pMsg->uart_resp_func != NULL)
            pMsg->uart_resp_func(pMsg->mcp_RespBuff, pMsg->msv_RespLen);
    }
    // TCP/MQTT: 直接发送
    else if (is_tcp_sender(pMsg->mcv_Sender) && pMsg->tcp_resp_func != NULL)
        pMsg->tcp_resp_func(pMsg->mcp_RespBuff, pMsg->msv_RespLen, pMsg->clientID);
    else if (is_mqtt_sender(pMsg->mcv_Sender) && pMsg->mqtt_resp_func != NULL)
        pMsg->mqtt_resp_func(pMsg->mcp_RespBuff);
}
```

**修复内容**：旧代码中所有发送调用被注释掉（`//pMsg->uart_resp_func(...)`），导致从站从未发送异常响应，这是**严重违反 Modbus 协议规范的 bug**。新平台必须修复。

### 6.2 避免两处 CRC 校验

旧代码中 `is_mb_protocol()` 和 `mb_slave_msg_handler()` 重复校验 CRC。新平台统一在 `mb_validate_frame()` 中只做一次。

---

## 七、传输层设计

### 7.1 接收缓冲区

```c
// 为每个 UART 端口预分配静态缓冲区，避免每次请求动态分配
#define MB_RX_BUF_SIZE    256
#define MB_TX_BUF_SIZE    256

typedef struct {
    uint8_t rxBuffer[MB_RX_BUF_SIZE];
    uint8_t txBuffer[MB_TX_BUF_SIZE];
    uint16_t rxLength;
    uint16_t txLength;
    uint8_t senderType;  // MB_SENDER_UART1 / TCP / MQTT
    // 回调函数指针
    void (*sendFunc)(uint8_t *data, uint16_t len);
    // TCP/MQTT 特有字段
    uint8_t clientId;
} mb_channel_t;
```

**改进说明**：旧代码中每个 Modbus 请求都 `pvPortMalloc(2048)` 分配响应缓冲区，频繁的堆分配/释放导致内存碎片。新平台使用**预分配的静态缓冲区**，消除堆碎片风险。

### 7.2 CRC 策略

| 传输方式 | 是否需要 CRC | 说明 |
|---------|-------------|------|
| UART RTU | 是 | 每帧末尾 2 字节 CRC16 |
| TCP | 否 | TCP/IP 已有校验，Modbus TCP 标准不包含 CRC |
| MQTT | 否 | MQTT 协议层已有校验 |

旧代码在 TCP 路径中先添加 CRC 再校验一遍（`MBtcp2rtuFrame` → `is_mb_protocol`），这是不必要的开销。

---

## 八、支持的 Modbus 功能码

| 功能码 | 名称 | 是否实现 | 说明 |
|-------|------|---------|------|
| 0x01 | 读线圈 | 可选择 | 如果产品需要开关量输出 |
| 0x02 | 读离散输入 | 可选择 | 如果产品需要开关量输入 |
| 0x03 | **读保持寄存器** | **必须** | 核心功能 |
| 0x04 | 读输入寄存器 | 可选择 | 如需要分离只读/读写寄存器 |
| 0x05 | 写单线圈 | 可选择 | 配合 0x01 |
| 0x06 | **写单寄存器** | **必须** | 核心功能 |
| 0x0F | 写多线圈 | 可选择 | 配合 0x01 |
| 0x10 | **写多寄存器** | **必须** | 核心功能 |

---

## 九、错误码定义

```c
typedef enum {
    MB_ERR_ILLEGAL_FUNCTION     = 0x01,  // 不支持的功能码
    MB_ERR_ILLEGAL_ADDRESS      = 0x02,  // 非法寄存器地址
    MB_ERR_ILLEGAL_DATA_VALUE   = 0x03,  // 非法数据值
    MB_ERR_SLAVE_DEVICE_FAILURE = 0x04,  // 从站设备故障
    MB_ERR_ACKNOWLEDGE          = 0x05,  // 确认（长处理时用）
    MB_ERR_SLAVE_BUSY           = 0x06,  // 从站忙
} mb_error_code_t;
```

---

## 十、代码文件组织

```
PallasApp/ModBus/
├── mb.h                    // 公共类型、API 声明
├── mb.c                    // 协议帧解析、分发、错误响应
├── mb_maptable.h           // 映射表条目结构、地址宏、handler 类型
├── mb_maptable.c           // 映射表定义、通用 handler、查表函数
├── mb_crc.h / mb_crc.c     // CRC16 计算
└── Modbus开发说明书.md      // 本文档

PallasApp/Internet/
└── mbtcp.c                 // Modbus TCP 服务器 (仅协议适配层)
```

---

## 十一、新平台 vs 旧平台 改进对照表

| 项目 | 旧平台（不合理） | 新平台（改进后） |
|------|----------------|----------------|
| **地址空间** | 入口选择码 + 连续地址混用，违反 Modbus 语义 | 全部连续地址，符合标准 |
| **地址分配** | 存在空洞和矛盾 (endAddr-startAddr+1 ≠ maxNum) | startAddr + regCount 严格一致 |
| **响应缓冲区** | 每帧 pvPortMalloc(2048)，堆碎片风险 | 预分配静态缓冲区，零堆分配 |
| **CRC 校验** | is_mb_protocol + mb_slave_msg_handler 各做一次 | 统一入口，只校验一次 |
| **错误响应** | 所有发送回调被注释掉，从站从不回复异常 | 规范发送异常响应帧 |
| **非法写入** | "can not use 0x06" 空 case，静默忽略 | 返回 kIllegalFunction 错误 |
| **产品差异** | 散落在 handler 中的 #if PROD_TYPE | 通过 handler 注册表分离 |
| **代码重复** | handleUART1/2/3RecvData 大量复制粘贴 | 统一通道结构 + 分发 |
| **功能码覆盖** | 仅支持 0x03/0x06/0x10/0x6A | 标准功能码 + 可扩展 |
| **调试信息** | 入口地址含义模糊的参数注释 | 清晰的地址段说明 + 数据流向 |

---

## 十二、实现注意事项

### 12.1 静态缓冲区大小

响应缓冲区的最大需求出现在读多寄存器场景：
- 最大读寄存器数 `MB_MAX_R_WORD_NUM = 0x025A = 602`
- 响应帧最大长度 = 3（帧头）+ 602 × 2（数据）+ 2（CRC）= 1209 字节
- 建议 `MB_TX_BUF_SIZE = 1280`，留有余量

### 12.2 数据字节序

- Modbus 协议规定：**大端序（Big-Endian）** 传输
- 16-bit 值：高字节在前，低字节在后
- 32-bit 值：最高字节在前，最低字节在后
- 字符串：按顺序逐字节存放，每个寄存器高字节在前

### 12.3 广播消息处理

- 从站地址为 `0x00` 时视为广播
- 广播消息**不需要回复**
- 只允许 `accessFlags & kBroadcast` 的地址段响应广播
- 写操作型广播应该执行但不回复

### 12.4 从站地址过滤

- 非广播帧的从站地址必须与设备站号匹配，否则丢弃
- 支持动态修改站号（通过写地址 0x0101）
- TCP 通道建议也进行站号过滤（旧平台 TCP 跳过了此检查，属于安全漏洞）

---

## 十三、扩展建议

如需支持多个从站协议实例（例如 UART 和 TCP 使用不同的地址映射表），可将映射表指针作为 `mb_slave_msg_handler` 的参数传入：

```c
void mb_slave_msg_handler(md_slave_msg_pack *pMsg, const mb_map_entry_t *mapTable, uint16_t mapSize);
```

这样 UART、TCP、MQTT 通道可以使用独立的地址映射，互相隔离。
