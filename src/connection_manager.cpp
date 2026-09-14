// BlueHIDFlow 连接管理器实现
#include "connection_manager.h"
#include "mqtt_client.h"

ConnectionManager connectionManager;

ConnectionManager::ConnectionManager() :
    _state(ConnectionState::CONN_DISCONNECTED),
    _mqttConnected(false),
    _userConnected(false),
    _retryCount(0),
    _wifiTimeout(30000),
    _mqttTimeout(60000),
    _apWaitTimeout(10000),  // 默认10秒等待窗口（调试用）
    _maxRetries(3),
    _lastConnectAttempt(0),
    _lastStateChange(0),
    _apStartTime(0),
    _onWiFiConnected(nullptr),
    _onWiFiDisconnected(nullptr),
    _onMQTTConnected(nullptr),
    _onMQTTDisconnected(nullptr) {
}

void ConnectionManager::begin() {
    Serial.println("[Conn] 连接管理器初始化");

    // 检查配置完整性
    if (!configManager.isComplete()) {
        Serial.println("[Conn] 配置不完整，进入 AP 模式");
        enterAPMode(true);  // 永久AP模式
        return;
    }

    // 配置完整，进入AP等待模式（给用户60秒时间窗口）
    enterAPWaitingMode();
}

void ConnectionManager::update() {
    unsigned long now = millis();

    switch (_state) {
        case ConnectionState::CONN_AP_WAITING:
            // AP 等待模式：检查是否有用户连接或超时
            if (_userConnected) {
                // 用户已连接，切换到正常AP模式
                Serial.println("[Conn] 用户已连接，进入配置模式");
                setState(ConnectionState::CONN_AP_MODE);
            } else if (now - _apStartTime > _apWaitTimeout) {
                // 超时，退出AP模式并尝试连接
                Serial.println("[Conn] AP 等待超时，退出AP模式");
                exitAPModeAndConnect();
            }
            break;

        case ConnectionState::CONN_AP_MODE:
            // AP 模式：处理用户配置
            // 如果用户断开连接且配置完整，可以提示重启
            break;

        case ConnectionState::CONN_CONNECTING_WIFI:
            // 检查 WiFi 是否连接成功
            if (WiFi.isConnected()) {
                Serial.println("[Conn] WiFi 连接成功");
                setState(ConnectionState::CONN_WIFI_CONNECTED);
                _retryCount = 0;

                if (_onWiFiConnected) _onWiFiConnected();
            }
            // 检查 WiFi 连接超时
            else if (now - _lastConnectAttempt > _wifiTimeout) {
                Serial.println("[Conn] WiFi 连接超时");
                _retryCount++;

                if (_retryCount >= _maxRetries) {
                    Serial.println("[Conn] 达到最大重试次数，进入 AP 模式");
                    enterAPMode(true);
                } else {
                    Serial.printf("[Conn] 重试 WiFi 连接 (%d/%d)\n", _retryCount, _maxRetries);
                    tryConnectWiFi();
                }
            }
            break;

        case ConnectionState::CONN_WIFI_CONNECTED:
            // WiFi 已连接，尝试 MQTT
            if (!_mqttConnected && configManager.mqtt().isValid()) {
                setState(ConnectionState::CONN_CONNECTING_MQTT);
                _lastConnectAttempt = now;
                tryConnectMQTT();
            }
            break;

        case ConnectionState::CONN_CONNECTING_MQTT:
            // 检查 MQTT 是否连接成功
            if (::mqtt.isConnected()) {
                Serial.println("[Conn] MQTT 连接成功");
                _mqttConnected = true;
                setState(ConnectionState::CONN_MQTT_CONNECTED);
                _retryCount = 0;

                if (_onMQTTConnected) _onMQTTConnected();
            }
            // 检查 MQTT 连接超时
            else if (now - _lastConnectAttempt > _mqttTimeout) {
                Serial.println("[Conn] MQTT 连接超时");
                _retryCount++;

                if (_retryCount >= _maxRetries) {
                    Serial.println("[Conn] 达到最大重试次数，进入 AP 模式");
                    enterAPMode(true);
                } else {
                    Serial.printf("[Conn] 重试 MQTT 连接 (%d/%d)\n", _retryCount, _maxRetries);
                    tryConnectMQTT();
                }
            }
            break;

        case ConnectionState::CONN_MQTT_CONNECTED:
            // 检查连接状态
            if (!WiFi.isConnected()) {
                Serial.println("[Conn] WiFi 断开");
                if (_onWiFiDisconnected) _onWiFiDisconnected();
                _mqttConnected = false;
                setState(ConnectionState::CONN_DISCONNECTED);
                _retryCount = 0;
                reconnect();
            }
            break;

        case ConnectionState::CONN_DISCONNECTED:
            // 尝试重新连接
            if (now - _lastConnectAttempt > 5000) {
                reconnect();
            }
            break;

        default:
            break;
    }
}

bool ConnectionManager::connectWiFi() {
    if (!configManager.wifi().isValid()) {
        Serial.println("[Conn] WiFi 配置无效");
        return false;
    }

    _retryCount = 0;
    return tryConnectWiFi();
}

bool ConnectionManager::connectMQTT() {
    if (!configManager.mqtt().isValid()) {
        Serial.println("[Conn] MQTT 配置无效");
        return false;
    }

    _retryCount = 0;
    return tryConnectMQTT();
}

void ConnectionManager::enterAPMode(bool permanent) {
    setState(ConnectionState::CONN_AP_MODE);

    // 断开现有连接
    WiFi.disconnect(true, true);
    _mqttConnected = false;

    // 启动 AP 模式 (webServer.begin() 会在 startAPMode 后自动调用)
    webServer.startAPMode();
    webServer.begin();

    Serial.println("[Conn] 已进入 AP 配置模式");
    Serial.printf("[Conn] 请连接热点: %s (密码: %s)\n",
                  ConfigWebServer::AP_SSID, ConfigWebServer::AP_PASSWORD);
    Serial.println("[Conn] 然后访问: http://192.168.8.1");

    if (permanent) {
        Serial.println("[Conn] 配置不完整，需要用户配置");
    }
}

void ConnectionManager::enterAPWaitingMode() {
    setState(ConnectionState::CONN_AP_WAITING);
    _apStartTime = millis();
    _userConnected = false;

    // 启动 AP 模式
    webServer.startAPMode();
    webServer.begin();

    Serial.println("[Conn] 进入 AP 等待模式");
    Serial.printf("[Conn] 热点: %s (密码: %s)\n",
                  ConfigWebServer::AP_SSID, ConfigWebServer::AP_PASSWORD);
    Serial.printf("[Conn] 等待用户连接，超时时间: %lu 秒\n", _apWaitTimeout / 1000);
    Serial.println("[Conn] 10秒内无连接将自动进入正常模式");
}

void ConnectionManager::exitAPModeAndConnect() {
    Serial.println("[Conn] 退出 AP 模式，尝试正常连接...");

    // 先改变状态，避免 reconnect() 检测到 AP 模式直接返回
    setState(ConnectionState::CONN_DISCONNECTED);

    // 关闭 AP 模式
    webServer.stopAPMode();
    webServer.stop();

    // 尝试连接
    _retryCount = 0;
    _mqttConnected = false;
    _userConnected = false;
    reconnect();
}

void ConnectionManager::notifyUserConnected() {
    _userConnected = true;
    Serial.println("[Conn] 用户已连接到热点");

    // 如果在等待模式，切换到正常AP模式
    if (_state == ConnectionState::CONN_AP_WAITING) {
        setState(ConnectionState::CONN_AP_MODE);
        Serial.println("[Conn] 切换到配置模式");
    }
}

void ConnectionManager::notifyUserDisconnected() {
    _userConnected = false;
    Serial.println("[Conn] 用户已断开热点连接");
}

void ConnectionManager::reconnect() {
    if (_state == ConnectionState::CONN_AP_MODE ||
        _state == ConnectionState::CONN_AP_WAITING) {
        return;  // AP 模式下不自动重连
    }

    Serial.println("[Conn] 开始连接...");

    if (!configManager.isComplete()) {
        Serial.println("[Conn] 配置不完整");
        enterAPMode(true);
        return;
    }

    _retryCount = 0;

    if (!WiFi.isConnected()) {
        setState(ConnectionState::CONN_CONNECTING_WIFI);
        _lastConnectAttempt = millis();
        tryConnectWiFi();
    } else if (!_mqttConnected && configManager.mqtt().isValid()) {
        setState(ConnectionState::CONN_CONNECTING_MQTT);
        _lastConnectAttempt = millis();
        tryConnectMQTT();
    }
}

bool ConnectionManager::tryConnectWiFi() {
    const WifiConfig& wifi = configManager.wifi();

    Serial.printf("[Conn] 连接 WiFi: %s\n", wifi.ssid.c_str());

    WiFi.disconnect(true, true);
    delay(100);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi.ssid.c_str(), wifi.password.c_str());

    // 等待连接（非阻塞，由 update() 处理超时）
    return true;
}

bool ConnectionManager::tryConnectMQTT() {
    const MqttConfig& mqttConfig = configManager.mqtt();

    // 使用配置中的 broker 地址，而不是硬编码的宏
    String brokerHost = mqttConfig.broker.length() > 0 ? mqttConfig.broker : String(MQTT_BROKER);
    uint16_t brokerPort = mqttConfig.port > 0 ? mqttConfig.port : MQTT_PORT;

    Serial.printf("[Conn] 连接 MQTT: %s:%d\n", brokerHost.c_str(), brokerPort);

    // 等待 WiFi 完全连接和 DNS 配置完成
    Serial.println("[Conn] 等待 WiFi 和 DNS 配置完成...");
    unsigned long startWait = millis();
    while (millis() - startWait < 2000) {
        if (WiFi.dnsIP(0) != IPAddress(0, 0, 0, 0)) {
            Serial.printf("[Conn] DNS 服务器已配置：%s\n", WiFi.dnsIP(0).toString().c_str());
            break;
        }
        delay(300);
    }

    // 尝试 DNS 解析
    Serial.println("[Conn] 开始 DNS 解析...");
    IPAddress resolvedIP;
    int dnsRetries = 5;
    while (dnsRetries > 0) {
        Serial.printf("[Conn] DNS 解析尝试：%s (剩余%d次)\n", brokerHost.c_str(), dnsRetries);
        if (WiFi.hostByName(brokerHost.c_str(), resolvedIP)) {
            Serial.printf("[Conn] DNS 解析成功：%s -> %s\n",
                          brokerHost.c_str(), resolvedIP.toString().c_str());
            break;
        }
        Serial.printf("[Conn] DNS 解析失败，重试... (%d)\n", dnsRetries);
        delay(1000);
        dnsRetries--;
    }

    if (dnsRetries == 0) {
        Serial.println("[Conn] DNS 解析失败，进入 AP 模式");
        // DNS 失败不进入 AP 模式，返回 false 让上层重试
        return false;
    }

    // 初始化 MQTT 客户端
    if (!::mqtt.begin()) {
        Serial.println("[Conn] MQTT 初始化失败");
        return false;
    }

    // 设置解析后的 IP（重要！）
    mqtt.setResolvedIP(resolvedIP);
    Serial.printf("[Conn] MQTT 服务器 IP 已设置：%s\n", resolvedIP.toString().c_str());

    // 发起连接（非阻塞，结果由 update() 检查）
    ::mqtt.connect();
    return true;
}

void ConnectionManager::setState(ConnectionState newState) {
    if (_state != newState) {
        ConnectionState oldState = _state;
        _state = newState;
        _lastStateChange = millis();

        Serial.printf("[Conn] 状态变更: %d -> %d\n", oldState, newState);
    }
}

void ConnectionManager::printStatus() const {
    Serial.println("\n========== 连接状态 ==========");
    Serial.printf("状态: ");
    switch (_state) {
        case ConnectionState::CONN_DISCONNECTED: Serial.println("已断开"); break;
        case ConnectionState::CONN_CONNECTING_WIFI: Serial.println("连接 WiFi 中..."); break;
        case ConnectionState::CONN_WIFI_CONNECTED: Serial.println("WiFi 已连接"); break;
        case ConnectionState::CONN_CONNECTING_MQTT: Serial.println("连接 MQTT 中..."); break;
        case ConnectionState::CONN_MQTT_CONNECTED: Serial.println("MQTT 已连接"); break;
        case ConnectionState::CONN_AP_MODE: Serial.println("AP 配置模式"); break;
        case ConnectionState::CONN_AP_WAITING: Serial.println("AP 等待模式"); break;
        case ConnectionState::CONN_ERROR_STATE: Serial.println("错误状态"); break;
    }

    Serial.printf("WiFi: %s\n", WiFi.isConnected() ? "已连接" : "未连接");
    if (WiFi.isConnected()) {
        Serial.printf("  SSID: %s\n", WiFi.SSID().c_str());
        Serial.printf("  IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("  RSSI: %d dBm\n", WiFi.RSSI());
    }

    Serial.printf("MQTT: %s\n", _mqttConnected ? "已连接" : "未连接");
    Serial.printf("用户连接: %s\n", _userConnected ? "是" : "否");
    Serial.printf("重试次数: %d/%d\n", _retryCount, _maxRetries);
    Serial.println("==============================\n");
}