/**
 * OneWire 库 — Raspberry Pi Pico C/C++ SDK
 *
 * 基于 Pico SDK GPIO 函数实现 Maxim 1-Wire 协议。
 * 改编自 Arduino OneWire 库（Jim Studt、Paul Stoffregen 等）。
 *
 * @license MIT
 */

#ifndef ONEWIRE_H
#define ONEWIRE_H

#include "pico/stdlib.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ONEWIRE_SEARCH   1
#define ONEWIRE_CRC      1

typedef struct {
    uint pin;           // GPIO 引脚编号
    uint8_t bitmask;    // 预计算的位掩码（用于快速访问）
#if ONEWIRE_SEARCH
    uint8_t ROM_NO[8];
    uint8_t LastDiscrepancy;
    uint8_t LastFamilyDiscrepancy;
    uint8_t LastDeviceFlag;
#endif
} OneWire;

// 在指定 GPIO 引脚上初始化 OneWire 实例
void onewire_init(OneWire *ow, uint pin);

// 执行 1-Wire 总线复位。返回 1 表示检测到设备，0 表示无设备。
uint8_t onewire_reset(OneWire *ow);

// 通过 8 字节 ROM 地址选中指定设备
void onewire_select(OneWire *ow, const uint8_t rom[8]);

// 跳过 ROM 选择（广播到总线所有设备）
void onewire_skip(OneWire *ow);

// 向总线写入一个字节。power=1 时保持总线供电。
void onewire_write(OneWire *ow, uint8_t v, uint8_t power);

// 写入多个字节
void onewire_write_bytes(OneWire *ow, const uint8_t *buf, uint16_t count, uint8_t power);

// 从总线读取一个字节
uint8_t onewire_read(OneWire *ow);

// 读取多个字节
void onewire_read_bytes(OneWire *ow, uint8_t *buf, uint16_t count);

// 写入单个位
void onewire_write_bit(OneWire *ow, uint8_t v);

// 读取单个位
uint8_t onewire_read_bit(OneWire *ow);

// 停止强制对总线供电
void onewire_depower(OneWire *ow);

#if ONEWIRE_SEARCH
// 重置搜索状态
void onewire_reset_search(OneWire *ow);

// 搜索下一个设备。找到返回 1，否则返回 0。
uint8_t onewire_search(OneWire *ow, uint8_t newAddr[8]);

// 设置搜索目标为指定家族码
void onewire_target_search(OneWire *ow, uint8_t family_code);
#endif

#if ONEWIRE_CRC
// 计算 Dallas Semiconductor 8 位 CRC
uint8_t onewire_crc8(const uint8_t *addr, uint8_t len);
#endif

#ifdef __cplusplus
}
#endif

#endif // ONEWIRE_H
