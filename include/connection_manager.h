// BlueHIDFlow 连接管理器
// 处理 WiFi/MQTT 连接状态和异常恢复
#ifndef CONNECTION_MANAGER_H
#define CONNECTION_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include "config_manager.h"
#include "web_server.h"

// ============================================
// 连接状态
// ============================================
enum class ConnectionState {
    CONN_DISCONNECTED,
    CONN_CONNECTING_WIFI,
    CONN_WIFI_CONNECTED,
    CONN_CONNECTING_MQTT,
    CONN_MQTT_CONNECTED,
    CONN_AP_MODE,           // AP 模式（用户已连接或配置不完整）
    CONN_AP_WAITING,        // AP 模式等待用户连接（启动后60秒窗口）
    CONN_ERROR_STATE
};

// ============================================
// 连接管理器
// ============================================
class ConnectionManager {
public:
    ConnectionManager();

    // 初始化
    void begin();

    // 主循环
    void update();

    // 状态查询
    ConnectionState getState() const { return _state; }
    bool isWiFiConnected() const { return WiFi.isConnected(); }
    bool isMQTTConnected() const { return _mqttConnected; }
    bool isInAPMode() const {
        return _state == ConnectionState::CONN_AP_MODE ||
               _state == ConnectionState::CONN_AP_WAITING;
    }
    bool isAPWaiting() const { return _state == ConnectionState::CONN_AP_WAITING; }
    bool hasUserConnected() const { return _userConnected; }

    // 连接操作
    bool connectWiFi();
    bool connectMQTT();
    void enterAPMode(bool permanent = false);  // permanent=true 表示配置不完整，永久AP模式
    void enterAPWaitingMode();                  // 启动后等待用户连接
    void reconnect();
    void exitAPModeAndConnect();               // 退出AP模式并尝试连接

    // 用户连接通知
    void notifyUserConnected();
    void notifyUserDisconnected();

    // 配置
    void setWiFiTimeout(uint32_t ms) { _wifiTimeout = ms; }
    void setMQTTTimeout(uint32_t ms) { _mqttTimeout = ms; }
    void setMaxRetries(uint8_t count) { _maxRetries = count; }
    void setAPWaitTimeout(uint32_t ms) { _apWaitTimeout = ms; }

    // 回调
    void onWiFiConnected(void (*callback)()) { _onWiFiConnected = callback; }
    void onWiFiDisconnected(void (*callback)()) { _onWiFiDisconnected = callback; }
    void onMQTTConnected(void (*callback)()) { _onMQTTConnected = callback; }
    void onMQTTDisconnected(void (*callback)()) { _onMQTTDisconnected = callback; }

    // 调试
    void printStatus() const;

private:
    ConnectionState _state;
    bool _mqttConnected;
    bool _userConnected;
    uint8_t _retryCount;

    // 超时配置
    uint32_t _wifiTimeout;
    uint32_t _mqttTimeout;
    uint32_t _apWaitTimeout;  // AP 等待窗口时间（默认60秒）
    uint8_t _maxRetries;

    // 时间戳
    unsigned long _lastConnectAttempt;
    unsigned long _lastStateChange;
    unsigned long _apStartTime;  // AP 模式开始时间

    // 回调
    void (*_onWiFiConnected)();
    void (*_onWiFiDisconnected)();
    void (*_onMQTTConnected)();
    void (*_onMQTTDisconnected)();

    // 内部方法
    void setState(ConnectionState newState);
    bool tryConnectWiFi();
    bool tryConnectMQTT();
    void handleWiFiEvents();
};

// 全局实例
extern ConnectionManager connectionManager;

#endif // CONNECTION_MANAGER_H