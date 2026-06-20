/**
 * DS18B20 温度传感器库 — Raspberry Pi Pico C/C++ SDK
 *
 * DS18B20、DS18S20 及 DS1822 温度传感器的高层 API，
 * 基于 OneWire 库构建。改编自 Arduino DallasTemperature 库。
 *
 * @license MIT
 */

#include "ds18b20.h"
#include <string.h>

// ---------------------------------------------------------------------------
// 内部辅助函数
// ---------------------------------------------------------------------------

// 根据分辨率计算温度转换所需的最大等待时间（毫秒）
// 9 位: 93.75ms, 10 位: 187.5ms, 11 位: 375ms, 12 位: 750ms
static inline uint16_t ds18b20_millis_to_wait_for_conversion(uint8_t resolution) {
    switch (resolution) {
        case 9:  return 94;
        case 10: return 188;
        case 11: return 375;
        default: return 750;
    }
}

// 从设备 scratchpad 读取原始温度值，并校验 CRC
// 成功返回 1，失败返回 0
static uint8_t ds18b20_raw_temperature(DS18B20 *sensor, const uint8_t *addr, int16_t *raw) {
    uint8_t sp[9];
    ds18b20_read_scratchpad(sensor, addr, sp);

    if (onewire_crc8(sp, 8) != sp[DS18B20_SP_CRC]) {
        return 0;
    }

    *raw = ((int16_t)sp[DS18B20_SP_TEMP_MSB] << 8) | sp[DS18B20_SP_TEMP_LSB];
    return 1;
}

// 将原始温度值转换为摄氏度
// DS18B20 默认 12 位，0.0625°C/bit
static float ds18b20_raw_to_celsius(int16_t raw) {
    return (float)raw * 0.0625f;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

void ds18b20_init(DS18B20 *sensor, OneWire *ow) {
    sensor->ow = ow;
    sensor->devices = 0;
    sensor->parasite = 0;
    sensor->bit_resolution = 12;
    sensor->wait_for_conversion = 1;
}

void ds18b20_begin(DS18B20 *sensor) {
    DeviceAddress addr;

    onewire_reset_search(sensor->ow);
    sensor->devices = 0;

    while (onewire_search(sensor->ow, addr)) {
        if (sensor->devices >= DS18B20_MAX_DEVICES) break;
        if (!ds18b20_valid_address(addr)) continue;

        memcpy(sensor->addr[sensor->devices], addr, 8);

        if (!sensor->parasite && ds18b20_read_power_supply(sensor, addr)) {
            sensor->parasite = 1;
        }

        uint8_t sp[9];
        ds18b20_read_scratchpad(sensor, addr, sp);

        uint8_t res = 12;
        if (addr[0] != DS18S20_MODEL) {
            switch (sp[DS18B20_SP_CONFIG]) {
                case DS18B20_RES_9_BIT:  res = 9;  break;
                case DS18B20_RES_10_BIT: res = 10; break;
                case DS18B20_RES_11_BIT: res = 11; break;
            }
        }
        if (res > sensor->bit_resolution) {
            sensor->bit_resolution = res;
        }

        sensor->devices++;
    }
}

uint8_t ds18b20_get_device_count(DS18B20 *sensor) {
    return sensor->devices;
}

uint8_t ds18b20_valid_address(const uint8_t *addr) {
    return (onewire_crc8(addr, 7) == addr[7]);
}

uint8_t ds18b20_is_connected(DS18B20 *sensor, const uint8_t *addr) {
    uint8_t scratchpad[9];
    ds18b20_read_scratchpad(sensor, addr, scratchpad);
    return (onewire_crc8(scratchpad, 8) == scratchpad[DS18B20_SP_CRC]);
}

void ds18b20_read_scratchpad(DS18B20 *sensor, const uint8_t *addr, uint8_t *scratchpad) {
    onewire_reset(sensor->ow);
    onewire_select(sensor->ow, addr);
    onewire_write(sensor->ow, DS18B20_CMD_READ_SCRATCHPAD, 0);

    scratchpad[DS18B20_SP_TEMP_LSB]     = onewire_read(sensor->ow);
    scratchpad[DS18B20_SP_TEMP_MSB]     = onewire_read(sensor->ow);
    scratchpad[DS18B20_SP_HIGH_ALARM]   = onewire_read(sensor->ow);
    scratchpad[DS18B20_SP_LOW_ALARM]    = onewire_read(sensor->ow);
    scratchpad[DS18B20_SP_CONFIG]       = onewire_read(sensor->ow);
    scratchpad[DS18B20_SP_RESERVED]     = onewire_read(sensor->ow);
    scratchpad[DS18B20_SP_COUNT_REMAIN] = onewire_read(sensor->ow);
    scratchpad[DS18B20_SP_COUNT_PER_C]  = onewire_read(sensor->ow);
    scratchpad[DS18B20_SP_CRC]          = onewire_read(sensor->ow);

    onewire_reset(sensor->ow);
}

void ds18b20_write_scratchpad(DS18B20 *sensor, const uint8_t *addr, const uint8_t *scratchpad) {
    onewire_reset(sensor->ow);
    onewire_select(sensor->ow, addr);
    onewire_write(sensor->ow, DS18B20_CMD_WRITE_SCRATCHPAD, 0);
    onewire_write(sensor->ow, scratchpad[DS18B20_SP_HIGH_ALARM], 0);
    onewire_write(sensor->ow, scratchpad[DS18B20_SP_LOW_ALARM], 0);

    // DS18S20 不使用配置寄存器
    if (addr[0] != DS18S20_MODEL) {
        onewire_write(sensor->ow, scratchpad[DS18B20_SP_CONFIG], 0);
    }

    onewire_reset(sensor->ow);

    // 保存到 EEPROM
    onewire_write(sensor->ow, DS18B20_CMD_COPY_SCRATCHPAD, sensor->parasite);
    if (sensor->parasite) {
        sleep_ms(10);  // 寄生供电模式下需要 10ms 延迟
    }
    onewire_reset(sensor->ow);
}

uint8_t ds18b20_read_power_supply(DS18B20 *sensor, const uint8_t *addr) {
    uint8_t ret = 0;
    onewire_reset(sensor->ow);
    onewire_select(sensor->ow, addr);
    onewire_write(sensor->ow, DS18B20_CMD_READ_POWER_SUPPLY, 0);
    ret = onewire_read_bit(sensor->ow);
    onewire_reset(sensor->ow);
    return ret;
}

uint8_t ds18b20_get_resolution(DS18B20 *sensor) {
    return sensor->bit_resolution;
}

void ds18b20_set_resolution(DS18B20 *sensor, uint8_t resolution) {
    sensor->bit_resolution = resolution;
}

uint8_t ds18b20_get_device_resolution(DS18B20 *sensor, const uint8_t *addr) {
    // DS18S20 固定为 9 位分辨率
    if (addr[0] == DS18S20_MODEL) {
        return 9;
    }

    uint8_t scratchpad[9];
    ds18b20_read_scratchpad(sensor, addr, scratchpad);

    switch (scratchpad[DS18B20_SP_CONFIG]) {
        case DS18B20_RES_9_BIT:  return 9;
        case DS18B20_RES_10_BIT: return 10;
        case DS18B20_RES_11_BIT: return 11;
        case DS18B20_RES_12_BIT: return 12;
        default:                 return 12;
    }
}

uint8_t ds18b20_set_device_resolution(DS18B20 *sensor, const uint8_t *addr, uint8_t resolution) {
    // DS18S20 不支持分辨率修改
    if (addr[0] == DS18S20_MODEL) {
        return 0;
    }

    uint8_t scratchpad[9];
    ds18b20_read_scratchpad(sensor, addr, scratchpad);

    switch (resolution) {
        case 9:  scratchpad[DS18B20_SP_CONFIG] = DS18B20_RES_9_BIT;  break;
        case 10: scratchpad[DS18B20_SP_CONFIG] = DS18B20_RES_10_BIT; break;
        case 11: scratchpad[DS18B20_SP_CONFIG] = DS18B20_RES_11_BIT; break;
        default: scratchpad[DS18B20_SP_CONFIG] = DS18B20_RES_12_BIT; break;
    }

    ds18b20_write_scratchpad(sensor, addr, scratchpad);
    return 1;
}

void ds18b20_set_wait_for_conversion(DS18B20 *sensor, uint8_t flag) {
    sensor->wait_for_conversion = flag;
}

uint8_t ds18b20_get_wait_for_conversion(DS18B20 *sensor) {
    return sensor->wait_for_conversion;
}

void ds18b20_request_temperatures(DS18B20 *sensor) {
    onewire_reset(sensor->ow);
    onewire_skip(sensor->ow);
    onewire_write(sensor->ow, DS18B20_CMD_CONVERT_TEMP, sensor->parasite);

    if (sensor->wait_for_conversion) {
        uint16_t ms = ds18b20_millis_to_wait_for_conversion(sensor->bit_resolution);
        sleep_ms(ms);
    }
}

uint8_t ds18b20_request_temp_by_address(DS18B20 *sensor, const uint8_t *addr) {
    onewire_reset(sensor->ow);
    onewire_select(sensor->ow, addr);
    onewire_write(sensor->ow, DS18B20_CMD_CONVERT_TEMP, sensor->parasite);

    if (sensor->wait_for_conversion) {
        uint16_t ms = ds18b20_millis_to_wait_for_conversion(sensor->bit_resolution);
        sleep_ms(ms);
    }

    return 1;
}

uint8_t ds18b20_request_temp_by_index(DS18B20 *sensor, uint8_t index) {
    if (index >= sensor->devices) return 0;
    return ds18b20_request_temp_by_address(sensor, sensor->addr[index]);
}

float ds18b20_get_temp_c(DS18B20 *sensor, const uint8_t *addr) {
    int16_t raw;
    if (!ds18b20_raw_temperature(sensor, addr, &raw)) {
        return DS18B20_ERROR_VALUE;
    }
    return ds18b20_raw_to_celsius(raw);
}

float ds18b20_get_temp_f(DS18B20 *sensor, const uint8_t *addr) {
    float c = ds18b20_get_temp_c(sensor, addr);
    if (c <= DS18B20_ERROR_VALUE + 1.0f) {
        return DS18B20_ERROR_VALUE;
    }
    return ds18b20_to_fahrenheit(c);
}

uint8_t ds18b20_get_address(DS18B20 *sensor, uint8_t *addr, uint8_t index) {
    if (index >= sensor->devices) return 0;
    memcpy(addr, sensor->addr[index], 8);
    return 1;
}

float ds18b20_get_temp_c_by_index(DS18B20 *sensor, uint8_t index) {
    if (index >= sensor->devices) return DS18B20_ERROR_VALUE;
    return ds18b20_get_temp_c(sensor, sensor->addr[index]);
}

float ds18b20_get_temp_f_by_index(DS18B20 *sensor, uint8_t index) {
    if (index >= sensor->devices) return DS18B20_ERROR_VALUE;
    return ds18b20_get_temp_f(sensor, sensor->addr[index]);
}

uint8_t ds18b20_is_parasite_power_mode(DS18B20 *sensor) {
    return sensor->parasite;
}

float ds18b20_to_fahrenheit(float celsius) {
    return (celsius * 1.8f) + 32.0f;
}

float ds18b20_to_celsius(float fahrenheit) {
    return (fahrenheit - 32.0f) / 1.8f;
}
