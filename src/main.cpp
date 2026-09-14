// BlueHIDFlow Main Application - v0.9.0
//
// 集成配置管理、Web 配置、BLE HID、MQTT 客户端、日志上报
//
// 2026-05-22

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// 核心模块
#include "config_manager.h"
#include "connection_manager.h"
#include "web_server.h"
#include "config.h"

// 功能模块（摄像头暂时禁用）
// #include "camera_server.h"
#include "ble_hid.h"
#include "mqtt_client.h"
#include "command_handler.h"
#include "led_control.h"

// 模块管理
#include "module.h"
#include "module_adapters.h"

// ============================================================
// 全局实例声明
// ============================================================
extern ConfigManager configManager;
extern ConnectionManager connectionManager;
extern ConfigWebServer webServer;
// extern CameraServer cameraServer;  // 摄像头暂时禁用
extern BLEHIDDevice bleHid;
extern MqttClient mqtt;
extern LogManager logManager;
extern CommandHandler cmdHandler;
extern LEDController g_ledController;

// ============================================================
// 模块适配器实例
// ============================================================
static ConfigManagerModule   modConfig;
static LogManagerModule      modLog;
static LEDControllerModule   modLED;
static ConnectionManagerModule modConn;
static BLEHIDModule          modBLE;
static MqttClientModule      modMQTT;
static CommandHandlerModule  modCmd;
static WebServerModule       modWeb;

// ============================================================
// 设备状态
// ============================================================
DeviceStatus g_deviceStatus = STATUS_OFFLINE;
unsigned long g_lastStatusPublish = 0;
const unsigned long STATUS_PUBLISH_INTERVAL = 30000;

// ============================================================
// 前向声明
// ============================================================
void setupNTP();
void onWiFiConnected();
void onWiFiDisconnected();
void onMQTTConnected();
void onMQTTDisconnected();
void onReceiveCommand(const char* action, const char* params);
void publishDeviceStatus();


// ============================================================
// Setup
// ============================================================
void setup() {
    // 串口初始化
    delay(1000);
    Serial.begin(115200);

    // 等待 USB 串口连接 (ESP32-S3 USB CDC)
    unsigned long startWait = millis();
    while (!Serial && (millis() - startWait < 3000)) {
        delay(10);
    }
    delay(200);

    Serial.println("\r\n========================================");
    Serial.println("BlueHIDFlow Firmware v0.9.0");
    Serial.println("========================================\r\n");

    delay(1000);

    // 注册模块（按优先级排序）
    ModuleManager& mgr = ModuleManager::instance();
    mgr.registerModule(&modConfig);   // 优先级 0
    mgr.registerModule(&modLog);      // 优先级 10
    mgr.registerModule(&modLED);      // 优先级 20
    mgr.registerModule(&modConn);     // 优先级 30
    mgr.registerModule(&modBLE);      // 优先级 40
    mgr.registerModule(&modMQTT);     // 优先级 50
    mgr.registerModule(&modCmd);      // 优先级 60
    mgr.registerModule(&modWeb);      // 优先级 70

    // 设置连接管理器回调（在模块初始化前）
    connectionManager.onWiFiConnected(onWiFiConnected);
    connectionManager.onWiFiDisconnected(onWiFiDisconnected);
    connectionManager.onMQTTConnected(onMQTTConnected);
    connectionManager.onMQTTDisconnected(onMQTTDisconnected);

    // 按优先级初始化所有模块
    mgr.setupAll();

    // AP 模式信息
    if (connectionManager.isInAPMode()) {
        Serial.println("[Main] AP 配置模式已启动");
        Serial.println("[Main] 请连接热点: BlueHIDFlow-config");
        Serial.println("[Main] 密码: bluehidflow");
        Serial.println("[Main] 配置页面: http://192.168.8.1");
        if (connectionManager.isAPWaiting()) {
            Serial.println("[Main] 10秒内无连接将自动进入正常模式");
        }
    }
}

// ============================================================
// WiFi 连接回调
// ============================================================
void onWiFiConnected() {
    Serial.println("[Main] WiFi 已连接");
    Serial.printf("[Main] IP: %s\n", WiFi.localIP().toString().c_str());

    // 记录日志
    char logMsg[64];
    snprintf(logMsg, sizeof(logMsg), "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
    MQTT_LOG_INFO(MQTT_LOG_EVENT_WIFI_CONNECT, logMsg);

    // 等待网络稳定
    delay(1000);

    // 配置 NTP
    setupNTP();

    // 初始化 BLE HID
    String bleName = configManager.bluetooth().deviceName;
    if (bleName.length() == 0) {
        bleName = configManager.generateDeviceId();
    }
    bleHid.begin(bleName.c_str(), configManager.bluetooth().hidMode);
    Serial.printf("[Main] BLE HID 已启动: %s, 模式: %s\n",
                  bleName.c_str(), bleHid.getModeName());
}

void onWiFiDisconnected() {
    Serial.println("[Main] WiFi 已断开");
    g_deviceStatus = STATUS_OFFLINE;
    MQTT_LOG_WARN(MQTT_LOG_EVENT_WIFI_DISCONNECT, "WiFi disconnected");
}

// ============================================================
// MQTT 连接回调
// ============================================================
void onMQTTConnected() {
    Serial.println("[Main] MQTT 已连接");
    g_deviceStatus = STATUS_ONLINE;

    g_ledController.onMQTTConnected();

    // 记录日志 (会在 MQTT 连接后上报)
    MQTT_LOG_INFO(MQTT_LOG_EVENT_MQTT_CONNECT, "MQTT broker connected");

    // 设置命令回调
    mqtt.setCommandCallback(onReceiveCommand);

    // 订阅命令主题
    mqtt.subscribeToCommands();

    // 发布初始状态
    publishDeviceStatus();

    // 初始化命令处理器
    cmdHandler.begin();
}

void onMQTTDisconnected() {
    Serial.println("[Main] MQTT 已断开");
    g_deviceStatus = STATUS_OFFLINE;

    g_ledController.onMQTTDisconnected();

    MQTT_LOG_WARN(MQTT_LOG_EVENT_MQTT_DISCONNECT, "MQTT broker disconnected");
}

// ============================================================
// NTP 配置
// ============================================================
void setupNTP() {
    configTime(8 * 3600, 0, "pool.ntp.org", "time.google.com", "time.cloudflare.com");
    Serial.println("[Main] NTP 已配置");
}

// ============================================================
// 命令回调
// ============================================================
void onReceiveCommand(const char* action, const char* params) {
    Serial.printf("[Main] 收到命令: %s\n", action);
    g_deviceStatus = STATUS_PROCESSING;

    cmdHandler.execute(action, params);

    g_deviceStatus = STATUS_ONLINE;
}

// ============================================================
// 发布设备状态
// ============================================================
void publishDeviceStatus() {
    const char* status;
    switch (g_deviceStatus) {
        case STATUS_OFFLINE:    status = "offline"; break;
        case STATUS_CONNECTING: status = "connecting"; break;
        case STATUS_ONLINE:     status = "connected"; break;
        case STATUS_PROCESSING: status = "busy"; break;
        default:                status = "unknown"; break;
    }

    if (mqtt.publishStatus(status)) {
        Serial.printf("[Main] 状态已发布: %s\n", status);
    }
}

// ============================================================
// Main Loop
// ============================================================
void loop() {
    unsigned long now = millis();

    // 通过模块管理器执行所有模块的 loop()
    ModuleManager::instance().loopAll();

    // AP 模式下仅处理 Web 和 LED
    if (connectionManager.isInAPMode()) {
        webServer.handleClient();
        return;
    }

    // 定期发布设备状态
    if (mqtt.isConnected() && (now - g_lastStatusPublish >= STATUS_PUBLISH_INTERVAL)) {
        g_lastStatusPublish = now;
        publishDeviceStatus();
    }

    // 心跳日志 (每 30 秒)
    static unsigned long lastHeartbeat = 0;
    if (now - lastHeartbeat >= 30000) {
        lastHeartbeat = now;
        Serial.printf("[HEARTBEAT] 运行 %lu 秒, WiFi=%d, BLE=%d(配对=%d), MQTT=%d, 模式=%s\n",
            now / 1000,
            WiFi.isConnected(),
            bleHid.isConnected(),
            bleHid.isPaired(),
            mqtt.isConnected(),
            bleHid.getModeName());
    }
}