#include "lcd1602_i2c.h"
#include "hardware/i2c.h"
#include <string.h>

#define LCD_CLEARDISPLAY   0x01
#define LCD_ENTRYMODESET   0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET    0x20
#define LCD_ENTRYLEFT      0x02
#define LCD_BLINKON        0x01
#define LCD_CURSORON       0x02
#define LCD_DISPLAYON      0x04
#define LCD_2LINE          0x08
#define LCD_BACKLIGHT      0x08
#define LCD_ENABLE_BIT     0x04
#define LCD_CHARACTER      1
#define LCD_COMMAND        0
#define DELAY_US           600

static inline void i2c_write_byte(uint8_t val) {
    i2c_write_blocking(LCD1602_I2C_PORT, LCD1602_I2C_ADDR, &val, 1, false);
}

static void lcd_toggle_enable(uint8_t val) {
    sleep_us(DELAY_US);
    i2c_write_byte(val | LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
    i2c_write_byte(val & ~LCD_ENABLE_BIT);
    sleep_us(DELAY_US);
}

static void lcd_send_byte(uint8_t val, int mode) {
    uint8_t high = mode | (val & 0xF0) | LCD_BACKLIGHT;
    uint8_t low  = mode | ((val << 4) & 0xF0) | LCD_BACKLIGHT;
    i2c_write_byte(high);
    lcd_toggle_enable(high);
    i2c_write_byte(low);
    lcd_toggle_enable(low);
}

void lcd1602_init(void) {
    i2c_init(LCD1602_I2C_PORT, LCD1602_I2C_BAUD);
    gpio_set_function(LCD1602_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(LCD1602_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(LCD1602_SDA_PIN);
    gpio_pull_up(LCD1602_SCL_PIN);
    sleep_ms(50);

    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x03, LCD_COMMAND);
    lcd_send_byte(0x02, LCD_COMMAND);
    lcd_send_byte(LCD_ENTRYMODESET | LCD_ENTRYLEFT, LCD_COMMAND);
    lcd_send_byte(LCD_FUNCTIONSET | LCD_2LINE, LCD_COMMAND);
    lcd_send_byte(LCD_DISPLAYCONTROL | LCD_DISPLAYON, LCD_COMMAND);
    lcd1602_clear();
}

void lcd1602_clear(void) {
    lcd_send_byte(LCD_CLEARDISPLAY, LCD_COMMAND);
}

void lcd1602_set_cursor(int line, int pos) {
    int val = (line == 0) ? 0x80 + pos : 0xC0 + pos;
    lcd_send_byte(val, LCD_COMMAND);
}

void lcd1602_char(char c) {
    lcd_send_byte(c, LCD_CHARACTER);
}

void lcd1602_string(const char *s) {
    while (*s) lcd1602_char(*s++);
}
