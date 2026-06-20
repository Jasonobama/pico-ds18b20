# DS18B20 温度传感器 — Raspberry Pi Pico 2

[English](README.md)

基于 Raspberry Pi Pico 2 的 DS18B20 数字温度传感器读取方案，支持 LCD1602 I2C 屏幕实时显示和 USB 串口输出。

## 硬件连接

| Pico 2 引脚 | 连接设备 | 说明 |
|------------|---------|------|
| GPIO 7 | DS18B20 DATA (DQ) | 需外接 4.7kΩ 上拉电阻到 3.3V |
| GPIO 12 | LCD1602 SDA | I2C0 数据线 |
| GPIO 13 | LCD1602 SCL | I2C0 时钟线 |
| VBUS (5V) | LCD1602 VCC | 屏幕供电 |
| GND | DS18B20 GND, LCD1602 GND | 共地 |
| 3V3(OUT) | DS18B20 VDD | 传感器供电 (非寄生模式) |

## 软件结构

```
DS18B20/
├── DS18B20.c              # 主程序 (初始化、主循环、LCD 驱动修正)
├── README.md              # 中文 README
├── README-EN.md           # English README
└── lib/
    ├── onewire.h / onewire.c       # 1-Wire 协议驱动 (位操作, 支持 ROM 搜索)
    ├── ds18b20.h / ds18b20.c       # DS18B20 传感器高层 API
    └── lcd1602_i2c.h / lcd1602_i2c.c # LCD1602 I2C 屏幕驱动
```

### 各模块说明

| 文件 | 功能 |
|------|------|
| `DS18B20.c` | 主程序入口，I2C 总线扫描、LCD 初始化（标准 4 位协议）、传感器发现、温度读取与显示 |
| `lib/onewire.c` | Maxim 1-Wire 底层协议实现：总线复位、位读写、字节读写、ROM 搜索算法 (二叉树搜索) |
| `lib/ds18b20.c` | DS18B20 高层封装：设备发现、地址缓存、温度转换请求、scratchpad 读写、分辨率管理 |
| `lib/lcd1602_i2c.c` | PCF8574 I2C 背包驱动 LCD1602：4 位模式、光标控制、字符串/字符输出 |

## 构建方式

### 前提条件

- Raspberry Pi Pico SDK 2.2.0 (安装于 `~/.pico-sdk/sdk/2.2.0`)
- ARM 交叉编译工具链 `arm-none-eabi-gcc` 14.2
- CMake 3.31+、Ninja 1.12+

### 构建步骤

```bash
# 进入项目目录
cd DS18B20

# 创建构建目录
mkdir build && cd build

# CMake 配置 (需先确保 PICO_SDK_PATH 环境变量已设置)
cmake -G "Ninja" ..

# 编译
ninja

# 或使用 cmake --build
cmake --build .
```

构建产物位于 `build/` 目录，其中 `DS18B20.uf2` 可直接拖入 Pico 2 的 USB 大容量存储设备进行烧录。

### VS Code 开发

项目已包含 `.vscode/` 配置，使用 Raspberry Pi Pico VS Code 扩展可直接一键编译烧录。

## 使用方式

1. 按照上述硬件连接表接好 DS18B20 传感器和 LCD1602 屏幕
2. 将 `build/DS18B20.uf2` 拖入 Pico 2
3. 打开 USB 串口终端 (115200 波特率)，观察输出
4. LCD 第一行显示摄氏度、第二行显示华氏度，每 2 秒更新一次

### 串口输出示例

```
DS18B20 Temperature Sensor Demo
===============================
Scanning I2C bus (i2c0)...
  I2C device found at 0x27
I2C scan complete.

Found 1 DS18B20 device(s) on the bus.

Requesting temperatures...
Temperature (index 0): 25.50 C  |  77.90 F
---------------------------------------
```

## 关键特性

- **1-Wire 位操作** — 通过关中断 + 微秒级忙等待实现精确的 1-Wire 时序
- **ROM 搜索** — 支持总线挂载多个 DS18B20，自动搜索并缓存 ROM 地址
- **LCD 4 位初始化** — 使用标准 HD44780 协议完成 8 位→4 位模式切换
- **I2C 自动扫描** — 启动时扫描 I2C 总线检测屏幕地址
- **双温度单位** — 同时显示摄氏度和华氏度

## 许可证

MIT License