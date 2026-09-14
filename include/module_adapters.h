// BlueHIDFlow 模块适配器
// 将现有模块包装为 IModule 接口，不修改原始类
#ifndef MODULE_ADAPTERS_H
#define MODULE_ADAPTERS_H

#include "module.h"
#include "config_manager.h"
#include "mqtt_client.h"
#include "ble_hid.h"
#include "command_handler.h"
#include "connection_manager.h"
#include "web_server.h"
#include "led_control.h"

// ============================================================
// ConfigManager 适配器
// ============================================================
class ConfigManagerModule : public IModule {
public:
    const char* name() const override { return "ConfigManager"; }
    int priority() const override { return 0; }
    void setup() override {
        configManager.begin();
        configManager.load();
        configManager.printConfig();
    }
    void loop() override {}
    void end() override { configManager.end(); }
};

// ============================================================
// LogManager 适配器
// ============================================================
class LogManagerModule : public IModule {
public:
    const char* name() const override { return "LogManager"; }
    int priority() const override { return 10; }
    void setup() override {
        logManager.begin();
        MQTT_LOG_INFO(MQTT_LOG_EVENT_BOOT, "Device boot complete");
    }
    void loop() override {}
    void end() override {}
};

// ============================================================
// LEDController 适配器
// ============================================================
class LEDControllerModule : public IModule {
public:
    const char* name() const override { return "LEDController"; }
    int priority() const override { return 20; }
    void setup() override {
        g_ledController.begin();
        g_ledController.onMQTTDisconnected();
    }
    void loop() override {
        g_ledController.loop();
    }
    void end() override {}
};

// ============================================================
// ConnectionManager 适配器
// ============================================================
class ConnectionManagerModule : public IModule {
public:
    const char* name() const override { return "ConnectionManager"; }
    int priority() const override { return 30; }
    void setup() override {
        // 回调注册在 main.cpp 中完成（因为回调函数在那里定义）
        connectionManager.begin();
    }
    void loop() override {
        connectionManager.update();
    }
    void end() override {}
};

// ============================================================
// BLEHIDDevice 适配器
// ============================================================
class BLEHIDModule : public IModule {
public:
    const char* name() const override { return "BLEHID"; }
    int priority() const override { return 40; }
    void setup() override {
        // BLE 初始化在 WiFi 连接回调中执行
    }
    void loop() override {
        bleHid.handleAdvertisingRestart();
    }
    void end() override {
        bleHid.end();
    }
};

// ============================================================
// MqttClient 适配器
// ============================================================
class MqttClientModule : public IModule {
public:
    const char* name() const override { return "MqttClient"; }
    int priority() const override { return 50; }
    void setup() override {
        // MQTT 初始化在 ConnectionManager 中触发
    }
    void loop() override {
        mqtt.loop();
    }
    void end() override {}
};

// ============================================================
// CommandHandler 适配器
// ============================================================
class CommandHandlerModule : public IModule {
public:
    const char* name() const override { return "CommandHandler"; }
    int priority() const override { return 60; }
    void setup() override {
        // CommandHandler 初始化在 MQTT 连接回调中执行
    }
    void loop() override {
        cmdHandler.loop();
    }
    void end() override {}
};

// ============================================================
// ConfigWebServer 适配器
// ============================================================
class WebServerModule : public IModule {
public:
    const char* name() const override { return "WebServer"; }
    int priority() const override { return 70; }
    void setup() override {
        // Web 服务器由 ConnectionManager 管理
    }
    void loop() override {
        // 仅 AP 模式下处理请求（由 main.cpp 判断后调用）
    }
    void end() override {
        webServer.stop();
    }
};

#endif // MODULE_ADAPTERS_H