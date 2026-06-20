/**
 * DS18B20 Temperature Sensor Demo for Raspberry Pi Pico
 *
 * Reads DS18B20 temperature via 1-Wire and displays on LCD1602 I2C
 * and USB serial output.
 *
 * Pin connections:
 *   DS18B20 DATA  -> GPIO 7  (with 4.7k pull-up to 3.3V)
 *   LCD1602 SDA   -> GPIO 12
 *   LCD1602 SCL   -> GPIO 13
 *   LCD1602 VCC   -> 5V (Pico VBUS)
 *   LCD1602 GND   -> GND
 */

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Override LCD1602 pin defaults before including the header
#define LCD1602_SDA_PIN  12
#define LCD1602_SCL_PIN  13
#define LCD1602_I2C_ADDR 0x27
#define LCD1602_I2C_BAUD 100000

#include "lib/lcd1602_i2c.h"
#include "lib/onewire.h"
#include "lib/ds18b20.h"

// DS18B20 1-Wire pin
#define ONE_WIRE_PIN  7

// I2C port for LCD
#define LCD_I2C_PORT  i2c0

// Send 4 bits with enable pulse (used for init when LCD is in 8-bit mode)
static void lcd_pulse_enable(uint8_t data) {
    i2c_write_blocking(i2c0, LCD1602_I2C_ADDR, &data, 1, false);
    sleep_us(1);
    uint8_t en = data | 0x04;
    i2c_write_blocking(i2c0, LCD1602_I2C_ADDR, &en, 1, false);
    sleep_us(1);
    i2c_write_blocking(i2c0, LCD1602_I2C_ADDR, &data, 1, false);
    sleep_us(50);
}

// Write a byte to LCD in 4-bit mode (rs: 0=command, 1=data)
static void lcd_write_byte(uint8_t rs, uint8_t val) {
    uint8_t high = (val & 0xF0) | rs | 0x08;
    uint8_t low  = ((val << 4) & 0xF0) | rs | 0x08;
    lcd_pulse_enable(high);
    lcd_pulse_enable(low);
}

static void lcd_init_correct(void) {
    // Wait for LCD power-up (>40ms after VCC stable)
    sleep_ms(50);

    // Step 1-3: Send Function Set 8-bit three times
    lcd_pulse_enable(0x38);  // RS=0, D7-D4=0011, BL=1
    sleep_ms(5);
    lcd_pulse_enable(0x38);
    sleep_us(150);
    lcd_pulse_enable(0x38);
    sleep_us(150);

    // Step 4: Switch to 4-bit mode
    lcd_pulse_enable(0x28);  // RS=0, D7-D4=0010, BL=1
    sleep_us(150);

    // Now in 4-bit mode
    lcd_write_byte(0, 0x28);  // Function Set: 4-bit, 2 lines, 5x8 dots
    lcd_write_byte(0, 0x08);  // Display Off
    lcd_write_byte(0, 0x01);  // Clear Display
    sleep_ms(2);
    lcd_write_byte(0, 0x06);  // Entry Mode: left to right, no shift
    lcd_write_byte(0, 0x0C);  // Display On, cursor off, blink off
}

int main() {
    // Initialize USB serial output
    stdio_init_all();

    // Wait a moment for USB serial to connect
    sleep_ms(2000);

    printf("DS18B20 Temperature Sensor Demo\n");
    printf("===============================\n");

    // Init I2C for LCD1602
    i2c_init(i2c0, LCD1602_I2C_BAUD);
    gpio_set_function(LCD1602_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(LCD1602_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(LCD1602_SDA_PIN);
    gpio_pull_up(LCD1602_SCL_PIN);

    // Scan I2C bus for devices
    printf("Scanning I2C bus (i2c0)...\n");
    for (int addr = 0; addr < 128; addr++) {
        uint8_t dummy = 0;
        int ret = i2c_write_blocking(i2c0, addr, &dummy, 1, false);
        if (ret >= 0) {
            printf("  I2C device found at 0x%02X\n", addr);
        }
    }
    printf("I2C scan complete.\n\n");

    // Initialize LCD1602 with correct 4-bit init sequence
    lcd_init_correct();

    // Initialize OneWire on GPIO 7
    OneWire ow;
    onewire_init(&ow, ONE_WIRE_PIN);

    // Initialize DS18B20 sensor
    DS18B20 sensors;
    ds18b20_init(&sensors, &ow);
    ds18b20_begin(&sensors);

    uint8_t device_count = ds18b20_get_device_count(&sensors);
    printf("Found %d DS18B20 device(s) on the bus.\n\n", device_count);

    if (device_count == 0) {
        printf("ERROR: No DS18B20 devices found!\n");
        printf("Check wiring and 4.7k pull-up resistor on DATA pin.\n");

        lcd1602_set_cursor(0, 0);
        lcd1602_string("No DS18B20 found");
        lcd1602_set_cursor(1, 0);
        lcd1602_string("Check wiring!");

        while (1) {
            sleep_ms(1000);
        }
    }

    // Main loop
    while (1) {
        printf("Requesting temperatures...\n");
        ds18b20_request_temperatures(&sensors);

        float temp_c = ds18b20_get_temp_c_by_index(&sensors, 0);
        float temp_f = ds18b20_to_fahrenheit(temp_c);

        // Display on LCD line 0: Celsius
        lcd1602_set_cursor(0, 0);
        char buf[17];
        snprintf(buf, sizeof(buf), "TemC: %.1f", temp_c);
        // Convert to a format that shows degree symbol placeholder
        // The degree symbol (0xDF) may need the LCD's CGRAM;
        // using 'o' as a fallback for the degree sign
        lcd1602_string(buf);
        lcd1602_char(' '); // space before unit
        lcd1602_char('C');

        // Display on LCD line 1: Fahrenheit
        lcd1602_set_cursor(1, 0);
        snprintf(buf, sizeof(buf), "TemF: %.1f", temp_f);
        lcd1602_string(buf);
        lcd1602_char(' ');
        lcd1602_char('F');

        // Print to USB serial
        printf("Temperature (index 0): %.2f C  |  %.2f F\n", temp_c, temp_f);
        printf("---------------------------------------\n");

        sleep_ms(2000);
    }

    return 0;
}
