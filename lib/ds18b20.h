/**
 * DS18B20 温度传感器库 — Raspberry Pi Pico C/C++ SDK
 *
 * DS18B20、DS18S20 及 DS1822 温度传感器的高层 API，
 * 基于 OneWire 库构建。改编自 Arduino DallasTemperature 库。
 *
 * @license MIT
 */

#ifndef DS18B20_H
#define DS18B20_H

#include "onewire.h"

#ifdef __cplusplus
extern "C" {
#endif

// ---------------------------------------------------------------------------
// 型号 ID
// ---------------------------------------------------------------------------
#define DS18S20_MODEL  0x10
#define DS18B20_MODEL  0x28
#define DS1822_MODEL   0x22

// ---------------------------------------------------------------------------
// 1-Wire 命令
// ---------------------------------------------------------------------------
#define DS18B20_CMD_CONVERT_TEMP     0x44
#define DS18B20_CMD_COPY_SCRATCHPAD  0x48
#define DS18B20_CMD_READ_SCRATCHPAD  0xBE
#define DS18B20_CMD_WRITE_SCRATCHPAD 0x4E
#define DS18B20_CMD_RECALL_EEPROM    0xB8
#define DS18B20_CMD_READ_POWER_SUPPLY 0xB4
#define DS18B20_CMD_ALARM_SEARCH     0xEC

// ---------------------------------------------------------------------------
// Scratchpad 寄存器偏移
// ---------------------------------------------------------------------------
#define DS18B20_SP_TEMP_LSB       0
#define DS18B20_SP_TEMP_MSB       1
#define DS18B20_SP_HIGH_ALARM     2
#define DS18B20_SP_LOW_ALARM      3
#define DS18B20_SP_CONFIG         4
#define DS18B20_SP_RESERVED       5
#define DS18B20_SP_COUNT_REMAIN   6
#define DS18B20_SP_COUNT_PER_C    7
#define DS18B20_SP_CRC            8

// ---------------------------------------------------------------------------
// 分辨率配置值
// ---------------------------------------------------------------------------
#define DS18B20_RES_9_BIT   0x1F
#define DS18B20_RES_10_BIT  0x3F
#define DS18B20_RES_11_BIT  0x5F
#define DS18B20_RES_12_BIT  0x7F

// ---------------------------------------------------------------------------
// 错误值
// ---------------------------------------------------------------------------
#define DS18B20_ERROR_VALUE  -127.0f

// ---------------------------------------------------------------------------
// 设备地址类型（8 字节 ROM 码）
// ---------------------------------------------------------------------------
typedef uint8_t DeviceAddress[8];

// ---------------------------------------------------------------------------
// DS18B20 传感器上下文结构体
// ---------------------------------------------------------------------------
#define DS18B20_MAX_DEVICES 4

typedef struct {
    OneWire *ow;              // 指向 OneWire 实例的指针
    uint8_t devices;          // 总线上发现的设备数量
    uint8_t bit_resolution;   // 全局分辨率（9-12 位）
    uint8_t parasite;         // 寄生供电模式标志
    uint8_t wait_for_conversion; // 是否等待温度转换完成
    DeviceAddress addr[DS18B20_MAX_DEVICES]; // 缓存的 ROM 地址
} DS18B20;

// ---------------------------------------------------------------------------
// DS18B20 API
// ---------------------------------------------------------------------------

// 初始化 DS18B20 上下文，绑定 OneWire 实例
void ds18b20_init(DS18B20 *sensor, OneWire *ow);

// 开始：扫描总线上的设备，读取分辨率
void ds18b20_begin(DS18B20 *sensor);

// 根据索引获取 ROM 地址（成功返回 1，失败返回 0）
uint8_t ds18b20_get_address(DS18B20 *sensor, uint8_t *addr, uint8_t index);

// 获取总线上发现的设备数量
uint8_t ds18b20_get_device_count(DS18B20 *sensor);

// 校验 ROM 地址的 CRC
uint8_t ds18b20_valid_address(const uint8_t *addr);

// 检查设备是否在线（读取 scratchpad 并校验 CRC）
uint8_t ds18b20_is_connected(DS18B20 *sensor, const uint8_t *addr);

// 从设备读取 scratchpad（9 字节）
void ds18b20_read_scratchpad(DS18B20 *sensor, const uint8_t *addr, uint8_t *scratchpad);

// 向设备写入 scratchpad
void ds18b20_write_scratchpad(DS18B20 *sensor, const uint8_t *addr, const uint8_t *scratchpad);

// 获取全局分辨率（9-12）
uint8_t ds18b20_get_resolution(DS18B20 *sensor);

// 设置全局分辨率（9-12）
void ds18b20_set_resolution(DS18B20 *sensor, uint8_t resolution);

// 获取指定设备的分辨率
uint8_t ds18b20_get_device_resolution(DS18B20 *sensor, const uint8_t *addr);

// 设置指定设备的分辨率
uint8_t ds18b20_set_device_resolution(DS18B20 *sensor, const uint8_t *addr, uint8_t resolution);

// 设置/获取是否等待温度转换完成
void ds18b20_set_wait_for_conversion(DS18B20 *sensor, uint8_t flag);
uint8_t ds18b20_get_wait_for_conversion(DS18B20 *sensor);

// 向总线所有设备发送温度转换命令
void ds18b20_request_temperatures(DS18B20 *sensor);

// 向指定设备（按地址）发送温度转换命令
uint8_t ds18b20_request_temp_by_address(DS18B20 *sensor, const uint8_t *addr);

// 向指定设备（按索引）发送温度转换命令
uint8_t ds18b20_request_temp_by_index(DS18B20 *sensor, uint8_t index);

// 读取指定设备的摄氏温度
float ds18b20_get_temp_c(DS18B20 *sensor, const uint8_t *addr);

// 读取指定设备的华氏温度
float ds18b20_get_temp_f(DS18B20 *sensor, const uint8_t *addr);

// 按设备索引读取摄氏温度
float ds18b20_get_temp_c_by_index(DS18B20 *sensor, uint8_t index);

// 按设备索引读取华氏温度
float ds18b20_get_temp_f_by_index(DS18B20 *sensor, uint8_t index);

// 检查总线是否处于寄生供电模式
uint8_t ds18b20_is_parasite_power_mode(DS18B20 *sensor);

// 温度单位转换工具
float ds18b20_to_fahrenheit(float celsius);
float ds18b20_to_celsius(float fahrenheit);

// 读取设备的供电模式
uint8_t ds18b20_read_power_supply(DS18B20 *sensor, const uint8_t *addr);

#ifdef __cplusplus
}
#endif

#endif // DS18B20_H
