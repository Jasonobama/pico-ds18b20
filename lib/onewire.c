/**
 * OneWire 库 — Raspberry Pi Pico C/C++ SDK
 *
 * 基于 Pico SDK GPIO 函数实现 Maxim 1-Wire 协议。
 * 改编自 Arduino OneWire 库（Jim Studt、Paul Stoffregen 等）。
 *
 * 时序关键代码段通过关中断确保微秒级精确波形，
 * 以满足 1-Wire 协议的时序要求。
 *
 * @license MIT
 */

#include "onewire.h"
#include "hardware/gpio.h"
#include "hardware/sync.h"

// ---------------------------------------------------------------------------
// 内部辅助函数
// ---------------------------------------------------------------------------

// 写入一个位到总线（关中断保证时序）
static inline void ow_write_bit_internal(OneWire *ow, uint8_t bit) {
    if (bit) {
        // 写 1：拉低 1-15us，释放，等待 60us
        uint32_t irq = save_and_disable_interrupts();
        gpio_set_dir(ow->pin, GPIO_OUT);
        gpio_put(ow->pin, 0);
        busy_wait_us_32(2);
        restore_interrupts(irq);
        gpio_set_dir(ow->pin, GPIO_IN);
        busy_wait_us_32(3);
        sleep_us(57);
    } else {
        // 写 0：拉低 60-120us，释放，等待
        uint32_t irq = save_and_disable_interrupts();
        gpio_set_dir(ow->pin, GPIO_OUT);
        gpio_put(ow->pin, 0);
        busy_wait_us_32(65);
        restore_interrupts(irq);
        gpio_set_dir(ow->pin, GPIO_IN);
        busy_wait_us_32(3);
        sleep_us(2);
    }
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

void onewire_init(OneWire *ow, uint pin) {
    ow->pin = pin;
    ow->bitmask = 1;
#if ONEWIRE_SEARCH
    onewire_reset_search(ow);
#endif
    gpio_init(pin);
    // 初始为输入模式（高阻态 / 释放总线）
    gpio_set_dir(pin, GPIO_IN);
    gpio_pull_up(pin);
}

uint8_t onewire_reset(OneWire *ow) {
    uint8_t presence;

    // 拉低总线至少 480us
    gpio_set_dir(ow->pin, GPIO_OUT);
    gpio_put(ow->pin, 0);
    sleep_us(500);

    // 释放总线
    gpio_set_dir(ow->pin, GPIO_IN);

    // 等待设备拉低总线（存在脉冲）
    sleep_us(70);

    uint32_t irq = save_and_disable_interrupts();
    presence = (gpio_get(ow->pin) == 0) ? 1 : 0;
    restore_interrupts(irq);

    // 等待存在脉冲窗口剩余时间
    sleep_us(410);

    return presence;
}

void onewire_select(OneWire *ow, const uint8_t rom[8]) {
    onewire_write(ow, 0x55, 0); // MATCH ROM 命令
    for (int i = 0; i < 8; i++) {
        onewire_write(ow, rom[i], 0);
    }
}

void onewire_skip(OneWire *ow) {
    onewire_write(ow, 0xCC, 0); // SKIP ROM 命令
}

void onewire_write(OneWire *ow, uint8_t v, uint8_t power) {
    for (uint8_t bit_mask = 1; bit_mask; bit_mask <<= 1) {
        ow_write_bit_internal(ow, (bit_mask & v) ? 1 : 0);
    }
    if (!power) {
        onewire_depower(ow);
    }
}

void onewire_write_bytes(OneWire *ow, const uint8_t *buf, uint16_t count, uint8_t power) {
    for (uint16_t i = 0; i < count; i++) {
        onewire_write(ow, buf[i], 0);
    }
    if (!power) {
        onewire_depower(ow);
    }
}

uint8_t onewire_read(OneWire *ow) {
    uint8_t result = 0;
    for (uint8_t bit_mask = 1; bit_mask; bit_mask <<= 1) {
        if (onewire_read_bit(ow)) {
            result |= bit_mask;
        }
    }
    return result;
}

void onewire_read_bytes(OneWire *ow, uint8_t *buf, uint16_t count) {
    for (uint16_t i = 0; i < count; i++) {
        buf[i] = onewire_read(ow);
    }
}

void onewire_write_bit(OneWire *ow, uint8_t v) {
    ow_write_bit_internal(ow, v);
}

uint8_t onewire_read_bit(OneWire *ow) {
    uint8_t result;

    // 拉低 1-15us 以启动读时隙
    uint32_t irq = save_and_disable_interrupts();
    gpio_set_dir(ow->pin, GPIO_OUT);
    gpio_put(ow->pin, 0);
    busy_wait_us_32(1);
    gpio_set_dir(ow->pin, GPIO_IN);
    // 短暂等待让设备驱动总线
    busy_wait_us_32(8);
    result = (gpio_get(ow->pin) != 0) ? 1 : 0;
    restore_interrupts(irq);

    // 等待读时隙剩余时间（总计约 55us）
    sleep_us(50);

    return result;
}

void onewire_depower(OneWire *ow) {
    gpio_set_dir(ow->pin, GPIO_IN);
}

// ---------------------------------------------------------------------------
// ROM 搜索
// ---------------------------------------------------------------------------

#if ONEWIRE_SEARCH

void onewire_reset_search(OneWire *ow) {
    ow->LastDiscrepancy = 0;
    ow->LastDeviceFlag = 0;
    ow->LastFamilyDiscrepancy = 0;
    for (int i = 0; i < 8; i++) {
        ow->ROM_NO[i] = 0;
    }
}

void onewire_target_search(OneWire *ow, uint8_t family_code) {
    ow->ROM_NO[0] = family_code;
    for (int i = 1; i < 8; i++) {
        ow->ROM_NO[i] = 0;
    }
    ow->LastDiscrepancy = 64;
    ow->LastFamilyDiscrepancy = 0;
    ow->LastDeviceFlag = 0;
}

// 二叉树搜索算法，参考 Maxim AN187：
// https://www.analog.com/en/app-notes/1wire-search-algorithm.html
uint8_t onewire_search(OneWire *ow, uint8_t newAddr[8]) {
    uint8_t id_bit_number = 1;
    uint8_t last_zero = 0;
    uint8_t rom_byte_number = 0;
    uint8_t rom_byte_mask = 1;
    uint8_t search_result = 0;
    uint8_t search_direction;

    // 上次调用未标记为最后一个设备
    if (!ow->LastDeviceFlag) {
        // 1-Wire 复位
        if (!onewire_reset(ow)) {
            // 无设备响应，重置搜索状态
            ow->LastDiscrepancy = 0;
            ow->LastDeviceFlag = 0;
            ow->LastFamilyDiscrepancy = 0;
            return 0;
        }

        // 发送搜索命令
        onewire_write(ow, 0xF0, 0); // SEARCH ROM

        // 执行搜索循环
        do {
            uint8_t id_bit, cmp_id_bit;

            // 读取一位及其补码
            id_bit = onewire_read_bit(ow);
            cmp_id_bit = onewire_read_bit(ow);

            // 两者均为 1 表示总线上无设备
            if (id_bit && cmp_id_bit) {
                break;
            }

            if (id_bit != cmp_id_bit) {
                // 所有设备在此位值相同，直接采用
                search_direction = id_bit;
            } else {
                // 差异位：不同设备在此位值不同
                if (id_bit_number < ow->LastDiscrepancy) {
                    search_direction = (ow->ROM_NO[rom_byte_number] & rom_byte_mask) ? 1 : 0;
                } else {
                    // 相等时选择 0
                    search_direction = (id_bit_number == ow->LastDiscrepancy) ? 1 : 0;
                }

                // 选择了 0，记录此位置
                if (search_direction == 0) {
                    last_zero = id_bit_number;
                    // 检查是否为家族差异
                    if (last_zero < 9) {
                        ow->LastFamilyDiscrepancy = last_zero;
                    }
                }
            }

            // 在 ROM 字节中置位或清零
            if (search_direction == 1) {
                ow->ROM_NO[rom_byte_number] |= rom_byte_mask;
            } else {
                ow->ROM_NO[rom_byte_number] &= ~rom_byte_mask;
            }

            // 发送搜索方向位
            onewire_write_bit(ow, search_direction);

            // 递增计数器
            id_bit_number++;
            rom_byte_mask <<= 1;

            // 掩码为 0 时进位到下一字节
            if (rom_byte_mask == 0) {
                rom_byte_number++;
                rom_byte_mask = 1;
            }
        } while (rom_byte_number < 8);

        // 搜索成功（至少处理了 64 位，id_bit_number 从 1 开始所以 >= 65）
        if (id_bit_number >= 65) {
            ow->LastDiscrepancy = last_zero;

            // 检查是否为最后一个设备
            if (ow->LastDiscrepancy == 0) {
                ow->LastDeviceFlag = 1;
            }
            search_result = 1;
        }
    }

    // 未找到设备时重置计数器
    if (!search_result || !ow->ROM_NO[0]) {
        ow->LastDiscrepancy = 0;
        ow->LastDeviceFlag = 0;
        ow->LastFamilyDiscrepancy = 0;
        search_result = 0;
    }

    for (int i = 0; i < 8; i++) {
        newAddr[i] = ow->ROM_NO[i];
    }
    return search_result;
}

#endif // ONEWIRE_SEARCH

// ---------------------------------------------------------------------------
// CRC
// ---------------------------------------------------------------------------

#if ONEWIRE_CRC

// 计算 Dallas Semiconductor 8 位 CRC（紧凑软件实现）
uint8_t onewire_crc8(const uint8_t *addr, uint8_t len) {
    uint8_t crc = 0;
    while (len--) {
        uint8_t inbyte = *addr++;
        for (uint8_t i = 8; i; i--) {
            uint8_t mix = (crc ^ inbyte) & 0x01;
            crc >>= 1;
            if (mix) crc ^= 0x8C;
            inbyte >>= 1;
        }
    }
    return crc;
}

#endif // ONEWIRE_CRC
