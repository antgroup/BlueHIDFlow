// BlueHIDFlow Web 配置服务器
// 支持 AP 模式和 Captive Portal
#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "config_manager.h"

// ============================================
// Web 配置服务器
// ============================================
class ConfigWebServer {
public:
    ConfigWebServer();
    ~ConfigWebServer();

    // 生命周期
    void begin();
    void handleClient();
    void stop();

    // 状态
    bool isRunning() const { return _running; }
    bool isInAPMode() const { return _apMode; }

    // 模式切换
    void startAPMode();
    void stopAPMode();

    // 配置
    void setOnSaveCallback(void (*callback)()) { _onSaveCallback = callback; }
    void setOnRebootCallback(void (*callback)()) { _onRebootCallback = callback; }

    // AP 配置 (公开访问)
    static constexpr const char* AP_SSID = "BlueHIDFlow-config";
    static constexpr const char* AP_PASSWORD = "bluehidflow";  // AP 热点密码（可通过 Web 配置修改）
    static const IPAddress AP_IP;
    static const IPAddress AP_GATEWAY;
    static const IPAddress AP_SUBNET;

private:
    WebServer* _server;
    DNSServer* _dnsServer;
    bool _running;
    bool _apMode;
    int _lastClientCount;  // 上次客户端数量

    // 回调
    void (*_onSaveCallback)();
    void (*_onRebootCallback)();

    // 路由设置
    void setupRoutes();

    // 页面路由
    void handleRoot();
    void handleWifi();
    void handleMqtt();
    void handleCamera();
    void handleAI();
    void handleBluetooth();
    void handlePhone();
    void handleSystem();

    // API 路由
    void handleApiConfig();
    void handleApiConfigSave();
    void handleApiWifiScan();
    void handleApiWifiConnect();
    void handleApiMqttTest();
    void handleApiCameraCapture();
    void handleApiStatus();
    void handleApiReboot();
    void handleApiReset();

    // 工具方法
    void sendJSON(int code, const String& json);
    void sendError(int code, const String& message);
    void sendSuccess(const String& message = "OK");
    String getPostBody();
    String escapeJSON(const String& input);
};

// 全局实例
extern ConfigWebServer webServer;

#endif // WEB_SERVER_H