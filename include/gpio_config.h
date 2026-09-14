#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H

#include <Arduino.h>

// ============================================================
// BlueHIDFlow ESP32-S3 GPIO 统一管脚分配表
// 硬件: ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM)
// ============================================================
//
// 模块接线概览:
// ┌──────────────┬──────────────────────────────────────┐
// │    模块       │              GPIO                     │
// ├──────────────┼──────────────────────────────────────┤
// │  OV2640      │ 详见 Camera 分区                    │
// │  蜂鸣器       │ GPIO8 (PWM)                        │
// │  LED 状态灯  │ R=GPIO3, G=GPIO4, B=GPIO5          │
// └──────────────┴──────────────────────────────────────┘
//
// 注意事项:
// - GPIO6~11 用于内置 Flash/PSRAM，不可占用
// - GPIO12/13/14 可用，但部分引脚有上下拉要求
// - Strapping 引脚( GPIO0/46/47/48 )在某些组合下可能影响启动
// ============================================================

// ============================================================
// OV2640 摄像头模块 (DVP 接口) - 18针版本
// 接线说明:
//   - PWDN接GND，摄像头一直工作
//   - RESET不连接
//   - XCLK需要ESP32提供外部时钟 (10-20MHz)
// 注意: 摄像头功能当前在固件中已禁用，引脚定义保留供开发者参考
// ============================================================
#define CAM_PWDN    -1      // 掉电控制（接GND，一直工作）
#define CAM_RESET   -1      // 复位（-1=不连接）
#define CAM_XCLK    10      // 系统时钟输出 (ESP32→Camera)
#define CAM_SIOD    40      // SCCB SDA (摄像头专用 I2C)
#define CAM_SIOC    39      // SCCB SCL (摄像头专用 I2C)
#define CAM_Y9      48      // D7
#define CAM_Y8      11      // D6
#define CAM_Y7      12      // D5
#define CAM_Y6      14      // D4
#define CAM_Y5      16      // D3
#define CAM_Y4      18      // D2
#define CAM_Y3      17      // D1
#define CAM_Y2      15      // D0
#define CAM_VSYNC   38      // 垂直同步
#define CAM_HREF    47      // 水平参考
#define CAM_PCLK    13      // 像素时钟

// ============================================================
// 蜂鸣器 (无源压电蜂鸣器，PWM 控制)
// ============================================================
#define BUZZER_PIN       8       // GPIO8 (支持 PWM/LEDC)

// ============================================================
// LED 状态指示灯 (共阴 RGB LED，低电平点亮)
// ============================================================
#define LED_R_PIN        3       // 红色 LED (错误/异常/MQTT断开)
#define LED_G_PIN        4       // 绿色 LED (正常运行/MQTT已连接)
#define LED_B_PIN        5       // 蓝色 LED (设备忙)

// ============================================================
// 辅助宏：初始化所有 GPIO
// ============================================================
#define GPIO_INIT_BUZZER()   pinMode(BUZZER_PIN, OUTPUT)
#define GPIO_INIT_LED_R()    pinMode(LED_R_PIN, OUTPUT)
#define GPIO_INIT_LED_G()    pinMode(LED_G_PIN, OUTPUT)
#define GPIO_INIT_LED_B()    pinMode(LED_B_PIN, OUTPUT)

#define GPIO_LED_R_ON()      digitalWrite(LED_R_PIN, LOW)
#define GPIO_LED_R_OFF()     digitalWrite(LED_R_PIN, HIGH)
#define GPIO_LED_G_ON()      digitalWrite(LED_G_PIN, LOW)
#define GPIO_LED_G_OFF()     digitalWrite(LED_G_PIN, HIGH)
#define GPIO_LED_B_ON()      digitalWrite(LED_B_PIN, LOW)
#define GPIO_LED_B_OFF()     digitalWrite(LED_B_PIN, HIGH)

#define GPIO_LED_ALL_OFF()   do { GPIO_LED_R_OFF(); GPIO_LED_G_OFF(); GPIO_LED_B_OFF(); } while(0)

#endif // GPIO_CONFIG_H
