#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "config.h"
#include <WiFi.h>

enum WiFiStatus {
    WIFI_DISCONNECTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_CONNECTED_PENDING,
    WIFI_NO_SSID_AVAIL,
    WIFI_CONNECT_FAILED,
    WIFI_CONNECTION_LOST
};

class WiFiManager {
private:
    WiFiStatus status;
    unsigned long lastConnectAttempt;
    bool autoReconnect;

    // 强制重连
    bool forceReconnect();

public:
    WiFiManager();

    // 初始化 WiFi
    void init();

    // 连接到 WiFi
    bool connect(const String& ssid, const String& password);

    // 断开连接
    void disconnect();

    // 更新状态（应该在 loop 中调用）
    void update();

    // 获取当前状态
    WiFiStatus getStatus() const { return status; }

    // 获取状态字符串
    String getStatusString() const;

    // 检查是否已连接
    bool isConnected() const { return status == WIFI_CONNECTED; }

    // 设置自动重连
    void setAutoReconnect(bool enable) { autoReconnect = enable; }

    // 获取 RSSI
    int getRSSI() const;

    // 等待连接（带超时）
    bool waitForConnection(unsigned long timeoutMs);
};

#endif // WIFI_MANAGER_H