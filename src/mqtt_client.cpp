// BlueHIDFlow MQTT Client Implementation
// 基于 PubSubClient 的 MQTT 客户端封装
// 支持 TLS 加密连接和可配置主题前缀
// 2026-04-02

#include "mqtt_client.h"
#include "config.h"
#include "config_manager.h"

#ifndef MQTT_CA_CERT
#define MQTT_CA_CERT ""
#endif
#include "command_handler.h"
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include <esp_heap_caps.h>
#include <new>  // for std::nothrow

// ============================================================
// 全局主题变量和设备 ID
// ============================================================
char g_deviceId[32] = "BHF-0000";
char g_commandTopic[128];
char g_responseTopic[128];
char g_statusTopic[128];
char g_logTopic[128];

// ============================================================
// MQTT 客户端回调指针
// ============================================================
static void (*g_commandCallback)(const char* action, const char* params) = nullptr;
static void (*g_statusCallback)(const char* status) = nullptr;

// ============================================================
// WiFiClientSecure 和 PubSubClient 实例
// ============================================================
static WiFiClientSecure esp_client;
static PubSubClient mqtt_client(esp_client);

// 运行时 MQTT CA 证书，必须保持 WiFiClientSecure 使用期间有效
// 注：setCACert() 只保存指针，不复制内容，故使用全局 String 持有
static String g_mqttCaCert;

// ============================================================
// LogManager 实现
// ============================================================
LogManager::LogManager() :
    _head(0), _count(0), _lastUpload(0), _initialized(false) {
    memset(_cache, 0, sizeof(_cache));
}

void LogManager::begin() {
    _head = 0;
    _count = 0;
    _lastUpload = millis();
    _initialized = true;
    Serial.println("[LogManager] 日志管理器初始化完成");
}

void LogManager::addLog(MqttLogLevel level, MqttLogEventType event, const char* message) {
    if (!_initialized) begin();

    MqttLogEntry& entry = _cache[_head];
    entry.timestamp = millis();
    entry.level = level;
    entry.event = event;

    strncpy(entry.message, message, MQTT_LOG_MAX_LENGTH - 1);
    entry.message[MQTT_LOG_MAX_LENGTH - 1] = '\0';

    _head = (_head + 1) % MQTT_LOG_CACHE_SIZE;
    if (_count < MQTT_LOG_CACHE_SIZE) {
        _count++;
    }

    Serial.printf("[LOG] [%s] %s: %s\n",
                  getLevelName(level), getEventName(event), message);

    if (level == MQTT_LOG_ERROR) {
        flush();
    }
}

void LogManager::loop() {
    if (!_initialized || _count == 0) return;

    unsigned long now = millis();
    if (now - _lastUpload >= MQTT_LOG_UPLOAD_INTERVAL) {
        uploadLogs();
        _lastUpload = now;
    }
}

void LogManager::flush() {
    if (_count > 0) {
        uploadLogs();
        _lastUpload = millis();
    }
}

bool LogManager::uploadLogs() {
    if (_count == 0) return true;
    if (!mqtt_client.connected()) {
        Serial.println("[LogManager] MQTT 未连接，跳过日志上报");
        return false;
    }

    JsonDocument doc;
    JsonArray logs = doc.to<JsonArray>();

    int startIdx = (_head - _count + MQTT_LOG_CACHE_SIZE) % MQTT_LOG_CACHE_SIZE;

    for (int i = 0; i < _count; i++) {
        int idx = (startIdx + i) % MQTT_LOG_CACHE_SIZE;
        MqttLogEntry& entry = _cache[idx];

        JsonObject logObj = logs.add<JsonObject>();
        logObj["ts"] = entry.timestamp;
        logObj["level"] = getLevelName(entry.level);
        logObj["event"] = getEventName(entry.event);
        logObj["msg"] = entry.message;
    }

    char payload[2048];
    size_t len = serializeJson(doc, payload, sizeof(payload));

    Serial.printf("[LogManager] 上报 %d 条日志，payload 大小：%d bytes\n", _count, len);

    bool result = mqtt_client.publish(g_logTopic, payload, false);

    if (result) {
        clear();
    }

    return result;
}

void LogManager::clear() {
    _head = 0;
    _count = 0;
    memset(_cache, 0, sizeof(_cache));
}

const char* LogManager::getLevelName(MqttLogLevel level) {
    switch (level) {
        case MQTT_LOG_INFO:  return "INFO";
        case MQTT_LOG_WARN:  return "WARN";
        case MQTT_LOG_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

const char* LogManager::getEventName(MqttLogEventType event) {
    switch (event) {
        case MQTT_LOG_EVENT_BOOT:            return "BOOT";
        case MQTT_LOG_EVENT_WIFI_CONNECT:    return "WIFI_CONNECT";
        case MQTT_LOG_EVENT_WIFI_DISCONNECT: return "WIFI_DISCONNECT";
        case MQTT_LOG_EVENT_MQTT_CONNECT:    return "MQTT_CONNECT";
        case MQTT_LOG_EVENT_MQTT_DISCONNECT: return "MQTT_DISCONNECT";
        case MQTT_LOG_EVENT_BLE_CONNECT:     return "BLE_CONNECT";
        case MQTT_LOG_EVENT_BLE_DISCONNECT:  return "BLE_DISCONNECT";
        case MQTT_LOG_EVENT_ERROR:           return "ERROR";
        case MQTT_LOG_EVENT_RESTART:         return "RESTART";
        case MQTT_LOG_EVENT_CONFIG_SAVE:     return "CONFIG_SAVE";
        case MQTT_LOG_EVENT_NFC_DETECT:      return "NFC_DETECT";
        case MQTT_LOG_EVENT_NFC_REMOVE:     return "NFC_REMOVE";
        default: return "UNKNOWN";
    }
}

// ============================================================
// MqttClient 实现 - 基于官方示例
// ============================================================

MqttClient::MqttClient() {
    _initialized = false;
}

bool MqttClient::begin() {
    Serial.println("========================================");
    Serial.println("MQTT Client 初始化 (TLS + CA 证书)");
    Serial.println("========================================");

    // 1. 设置 CA 证书（关键！必须在连接前设置）
    // CA 证书由 config.h / Web 配置提供，不在源码中内置任何厂商根证书。
    // 优先使用运行时 Web 配置，其次使用 config.h 中的 MQTT_CA_CERT 编译期默认值。
    g_mqttCaCert = configManager.mqtt().caCert;
    if (g_mqttCaCert.length() == 0) {
        g_mqttCaCert = MQTT_CA_CERT;
    }

    if (g_mqttCaCert.length() > 0) {
        Serial.println("MQTT: 设置 CA 证书...");
        esp_client.setCACert(g_mqttCaCert.c_str());
        Serial.println("MQTT: CA 证书已设置");
    } else {
        Serial.println("MQTT: 警告：未配置 CA 证书，TLS 连接可能失败");
    }

    // 2. 设置 MQTT 服务器
    Serial.printf("MQTT: 设置服务器：%s:%d\n", MQTT_BROKER, MQTT_PORT);
    mqtt_client.setServer(MQTT_BROKER, MQTT_PORT);

    // 3. 设置 MQTT 缓冲区大小（支持摄像头帧大消息）
    mqtt_client.setBufferSize(MQTT_MAX_PACKET_SIZE);
    Serial.printf("MQTT: 缓冲区大小：%d bytes\n", MQTT_MAX_PACKET_SIZE);

    // 4. 设置 KeepAlive
    mqtt_client.setKeepAlive(60);

    // 4. 设置回调
    mqtt_client.setCallback([this](char* topic, byte* payload, unsigned int length) {
        this->callback(topic, payload, length);
    });

    // 5. 设置主题
    setupTopics();

    _initialized = true;
    Serial.println("MQTT Client 初始化完成");
    Serial.println("========================================");
    return true;
}

void MqttClient::setupTopics() {
    // 优先使用 MQTT 配置中的 deviceId（用户设置）
    String deviceId = configManager.mqtt().deviceId;

    // 如果未设置，自动生成（基于 MAC 地址或时间戳）
    if (deviceId.length() == 0) {
        deviceId = configManager.generateDeviceId();
    }

    strncpy(g_deviceId, deviceId.c_str(), sizeof(g_deviceId) - 1);
    g_deviceId[sizeof(g_deviceId) - 1] = '\0';

    // 使用配置中的主题前缀
    String topicPrefix = configManager.mqtt().topicPrefix;
    if (topicPrefix.length() == 0) {
        topicPrefix = DEFAULT_TOPIC_PREFIX;
    }

    snprintf(g_commandTopic, sizeof(g_commandTopic), "%s%s%s", topicPrefix.c_str(), CMD_TOPIC_SUFFIX, g_deviceId);
    snprintf(g_responseTopic, sizeof(g_responseTopic), "%s%s%s", topicPrefix.c_str(), RSP_TOPIC_SUFFIX, g_deviceId);
    snprintf(g_statusTopic, sizeof(g_statusTopic), "%s%s%s", topicPrefix.c_str(), STATUS_TOPIC_SUFFIX, g_deviceId);
    snprintf(g_logTopic, sizeof(g_logTopic), "%s%s%s", topicPrefix.c_str(), LOG_TOPIC_SUFFIX, g_deviceId);

    Serial.printf("MQTT 主题配置:\n");
    Serial.printf("  主题前缀: %s\n", topicPrefix.c_str());
    Serial.printf("  Device ID: %s\n", g_deviceId);
    Serial.printf("  Command: %s\n", g_commandTopic);
    Serial.printf("  Response: %s\n", g_responseTopic);
    Serial.printf("  Status: %s\n", g_statusTopic);
    Serial.printf("  Log: %s\n", g_logTopic);
}

bool MqttClient::connect() {
    if (mqtt_client.connected()) {
        Serial.println("MQTT 已连接");
        return true;
    }

    // 检查 WiFi 连接
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi 未连接，无法连接 MQTT");
        return false;
    }

    // 生成客户端 ID（使用 MAC 地址）
    String clientId = "ESP32-S3-" + String(WiFi.macAddress());

    Serial.println("========================================");
    Serial.printf("连接到 MQTT Broker: %s:%d\n", MQTT_BROKER, MQTT_PORT);
    Serial.printf("客户端 ID: %s\n", clientId.c_str());
    Serial.printf("用户名：%s\n", MQTT_USERNAME);
    Serial.println("========================================");

    // 尝试连接（官方示例方式 - 阻塞式）
    bool connected = mqtt_client.connect(
        clientId.c_str(),
        MQTT_USERNAME,
        MQTT_PASSWORD,
        g_statusTopic,
        1,  // QoS 1
        true,  // retain
        "{\"status\":\"offline\"}"
    );

    if (connected) {
        Serial.println("✅ 已成功连接到 MQTT Broker!");
        MQTT_LOG_INFO(MQTT_LOG_EVENT_MQTT_CONNECT, "MQTT broker connected");

        // 更新状态为 online
        publishStatus("online");

        return true;
    } else {
        int rc = mqtt_client.state();
        Serial.printf("❌ MQTT 连接失败，rc=%d\n", rc);
        Serial.println("");
        Serial.println("错误码说明:");
        Serial.println("  -4 = MQTT_CONNECTION_TIMEOUT (连接超时)");
        Serial.println("  -3 = MQTT_CONNECTION_LOST (连接丢失)");
        Serial.println("  -2 = MQTT_CONNECT_FAILED (连接失败)");
        Serial.println("  -1 = MQTT_DISCONNECTED (已断开)");
        Serial.println("  1 = MQTT_CONNECT_BAD_PROTOCOL (协议错误)");
        Serial.println("  2 = MQTT_CONNECT_BAD_CLIENT_ID (客户端 ID 错误)");
        Serial.println("  3 = MQTT_CONNECT_UNAVAILABLE (服务不可用)");
        Serial.println("  4 = MQTT_CONNECT_BAD_CREDENTIALS (用户名/密码错误)");
        Serial.println("  5 = MQTT_CONNECT_UNAUTHORIZED (未授权)");
        Serial.println("");

        if (rc == 4) {
            Serial.println("建议：检查 MQTT_USERNAME 和 MQTT_PASSWORD 是否正确");
        } else if (rc == -4 || rc == -2) {
            Serial.println("建议：检查网络防火墙或尝试重启设备");
        }

        MQTT_LOG_ERROR(MQTT_LOG_EVENT_ERROR, "MQTT connection failed");
        return false;
    }
}

bool MqttClient::isConnected() {
    return mqtt_client.connected();
}

void MqttClient::loop() {
    mqtt_client.loop();
    logManager.loop();
}

void MqttClient::reconnect() {
    Serial.println("MQTT: 尝试重新连接...");

    // 先断开（如果有连接）
    if (mqtt_client.connected()) {
        mqtt_client.disconnect();
        delay(100);
    }

    // 重新连接
    connect();
}

bool MqttClient::publish(const char* topic, const char* payload, bool retain) {
    if (!mqtt_client.connected()) {
        Serial.printf("无法发布到 %s - MQTT 未连接\n", topic);
        return false;
    }

    bool result = mqtt_client.publish(topic, payload, retain);
    if (result) {
        Serial.printf("已发布到 %s (%d bytes)\n", topic, strlen(payload));
    } else {
        Serial.printf("发布失败到 %s\n", topic);
    }
    return result;
}

bool MqttClient::publishCommand(const char* action, const char* message_id, const char* params) {
    JsonDocument doc;
    doc["action"] = action;
    doc["message_id"] = message_id;
    doc["timestamp"] = millis();

    if (params && strlen(params) > 0) {
        JsonDocument paramsDoc;
        DeserializationError parseError = deserializeJson(paramsDoc, params);
        if (parseError == DeserializationError::Ok) {
            doc["params"] = paramsDoc;
        }
    }

    char payload[256];
    serializeJson(doc, payload, sizeof(payload));

    return publish(g_commandTopic, payload, false);
}

bool MqttClient::publishStatus(const char* status) {
    JsonDocument doc;
    doc["device_id"] = g_deviceId;
    doc["status"] = status;
    doc["battery"] = 85;
    doc["wifi_rssi"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;
    doc["ip"] = WiFi.localIP().toString();

    // 获取 MAC 地址
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    doc["mac"] = macStr;

    char payload[512];
    serializeJson(doc, payload, sizeof(payload));

    return publish(g_statusTopic, payload, true);
}

bool MqttClient::publishResponse(const char* message_id, const char* status, const char* error) {
    if (!_initialized || !mqtt_client.connected()) return false;
    
    // 尽量减少在回调中的内存分配和复杂逻辑
    JsonDocument doc;
    doc["message_id"] = message_id ? message_id : "unknown";
    doc["status"] = status ? status : "unknown";
    doc["timestamp"] = millis();

    if (error) {
        doc["error"] = error;
    }

    String output;
    serializeJson(doc, output);
    return mqtt_client.publish(g_responseTopic, output.c_str(), false);
}

bool MqttClient::publishResponseWithData(const char* message_id, const char* status, JsonDocument& data) {
    // 使用动态分配 JsonDocument（堆内存）
    JsonDocument* doc = new (std::nothrow) JsonDocument();
    if (!doc) {
        Serial.println("无法分配 JsonDocument 内存");
        return false;
    }

    (*doc)["message_id"] = message_id;
    (*doc)["status"] = status;
    (*doc)["timestamp"] = millis();

    // 复制 data 中的所有字段到响应
    for (auto kv : data.as<JsonObject>()) {
        (*doc)[kv.key()] = kv.value();
    }

    // 使用堆内存分配 payload
    char* payload = (char*)heap_caps_malloc(4096, MALLOC_CAP_SPIRAM);
    if (!payload) {
        payload = (char*)malloc(4096);
    }
    if (!payload) {
        delete doc;
        return false;
    }

    serializeJson(*doc, payload, 4096);
    bool result = publish(g_responseTopic, payload, false);

    free(payload);
    delete doc;

    return result;
}

bool MqttClient::publishLog(const char* payload) {
    return publish(g_logTopic, payload, false);
}

// ============================================================
// 摄像头帧发布（使用 PSRAM 分配内存）
// ============================================================
bool MqttClient::publishCameraFrame(const char* frame_base64, size_t size) {
    if (!mqtt_client.connected()) {
        Serial.println("无法发布摄像头帧 - MQTT 未连接");
        return false;
    }

    // 构建 camera topic（使用配置中的主题前缀）
    String topicPrefix = configManager.mqtt().topicPrefix;
    if (topicPrefix.length() == 0) {
        topicPrefix = DEFAULT_TOPIC_PREFIX;
    }
    char cameraTopic[128];
    snprintf(cameraTopic, sizeof(cameraTopic), "%s%s%s", topicPrefix.c_str(), CAMERA_TOPIC_SUFFIX, g_deviceId);

    // 使用动态分配 JsonDocument（PSRAM）
    JsonDocument* doc = new (std::nothrow) JsonDocument();
    if (!doc) {
        Serial.println("无法分配 JsonDocument 内存");
        return false;
    }

    (*doc)["frame_base64"] = frame_base64;
    (*doc)["timestamp"] = millis();
    (*doc)["size"] = size;

    // 估算 payload 大小：Base64 数据 + JSON 开销
    size_t payloadSize = strlen(frame_base64) + 128;

    // 使用 PSRAM 分配 payload
    char* payload = (char*)heap_caps_malloc(payloadSize, MALLOC_CAP_SPIRAM);
    if (!payload) {
        // 尝试普通堆内存
        payload = (char*)malloc(payloadSize);
    }
    if (!payload) {
        Serial.println("无法分配 payload 内存");
        delete doc;
        return false;
    }

    size_t len = serializeJson(*doc, payload, payloadSize);

    Serial.printf("[MQTT] 发布摄像头帧：%d bytes 到 %s\n", len, cameraTopic);

    bool result = publish(cameraTopic, payload, false);

    free(payload);
    delete doc;

    if (result) {
        Serial.println("[MQTT] 摄像头帧发布成功");
    } else {
        Serial.println("[MQTT] 摄像头帧发布失败");
    }

    return result;
}

// ============================================================
// 摄像头帧 URL 发布（轻量级，只发送 URL）
// ============================================================
bool MqttClient::publishCameraUrl(const char* url) {
    if (!mqtt_client.connected()) {
        Serial.println("无法发布摄像头 URL - MQTT 未连接");
        return false;
    }

    // 构建 camera topic（使用配置中的主题前缀）
    String topicPrefix = configManager.mqtt().topicPrefix;
    if (topicPrefix.length() == 0) {
        topicPrefix = DEFAULT_TOPIC_PREFIX;
    }
    char cameraTopic[128];
    snprintf(cameraTopic, sizeof(cameraTopic), "%s%s%s", topicPrefix.c_str(), CAMERA_TOPIC_SUFFIX, g_deviceId);

    // 使用动态分配 JsonDocument（PSRAM）
    JsonDocument* doc = new (std::nothrow) JsonDocument();
    if (!doc) {
        Serial.println("无法分配 JsonDocument 内存");
        return false;
    }

    (*doc)["img_url"] = url;
    (*doc)["timestamp"] = millis();

    // 估算 payload 大小：URL + JSON 开销
    size_t payloadSize = strlen(url) + 64;

    // 使用 PSRAM 分配 payload
    char* payload = (char*)heap_caps_malloc(payloadSize, MALLOC_CAP_SPIRAM);
    if (!payload) {
        payload = (char*)malloc(payloadSize);
    }
    if (!payload) {
        Serial.println("无法分配 payload 内存");
        delete doc;
        return false;
    }

    size_t len = serializeJson(*doc, payload, payloadSize);

    Serial.printf("[MQTT] 发布摄像头 URL：%d bytes 到 %s\n", len, cameraTopic);

    bool result = publish(cameraTopic, payload, false);

    free(payload);
    delete doc;

    if (result) {
        Serial.println("[MQTT] 摄像头 URL 发布成功");
    } else {
        Serial.println("[MQTT] 摄像头 URL 发布失败");
    }

    return result;
}

bool MqttClient::subscribe(const char* topic) {
    if (!mqtt_client.connected()) {
        Serial.println("无法订阅 - MQTT 未连接");
        return false;
    }

    bool result = mqtt_client.subscribe(topic, 1);
    if (result) {
        Serial.printf("已订阅：%s\n", topic);
    } else {
        Serial.printf("订阅失败：%s\n", topic);
    }
    return result;
}

void MqttClient::subscribeToCommands() {
    Serial.println("订阅命令主题...");
    subscribe(g_commandTopic);
}

void MqttClient::callback(char* topic, byte* payload, unsigned int length) {
    Serial.printf("收到消息，主题：%s\n", topic);

    if (length == 0 || length > 1024) {
        Serial.printf("无效的消息长度：%d\n", length);
        return;
    }

    char* payloadStr = (char*)malloc(length + 1);
    if (payloadStr == nullptr) {
        Serial.println("malloc 失败");
        return;
    }

    memcpy(payloadStr, payload, length);
    payloadStr[length] = '\0';

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payloadStr);
    free(payloadStr);

    if (error != DeserializationError::Ok) {
        Serial.printf("JSON 解析错误：%s\n", error.c_str());
        return;
    }

    JsonObject root = doc.as<JsonObject>();

    if (strcmp(topic, g_commandTopic) == 0) {
        if (root.containsKey("action")) {
            const char* action = root["action"];
            char messageIdStr[64];  // 增大缓冲区
            if (root.containsKey("message_id")) {
                // 使用更通用的方法获取 message_id
                JsonVariant mid = root["message_id"];
                if (mid.is<const char*>()) {
                    const char* incoming = mid.as<const char*>();
                    snprintf(messageIdStr, sizeof(messageIdStr), "%s", incoming ? incoming : "");
                } else if (mid.is<unsigned long>()) {
                    snprintf(messageIdStr, sizeof(messageIdStr), "%lu", mid.as<unsigned long>());
                } else if (mid.is<long>()) {
                    snprintf(messageIdStr, sizeof(messageIdStr), "%ld", mid.as<long>());
                } else if (mid.is<int>()) {
                    snprintf(messageIdStr, sizeof(messageIdStr), "%d", mid.as<int>());
                } else {
                    // 尝试转换为字符串
                    String midStr = mid.as<String>();
                    if (midStr.length() > 0) {
                        snprintf(messageIdStr, sizeof(messageIdStr), "%s", midStr.c_str());
                    } else {
                        snprintf(messageIdStr, sizeof(messageIdStr), "%lu", millis());
                    }
                }
            } else {
                snprintf(messageIdStr, sizeof(messageIdStr), "%lu", millis());
            }
            Serial.printf("[MQTT] 收到命令: action=%s, message_id=%s\n", action, messageIdStr);

            String paramsStr = "{}";
            if (root.containsKey("params")) {
                serializeJson(root["params"], paramsStr);
            }

            // 设置 message_id 并发送 PROCESSING 响应
            setLastMessageId(messageIdStr);
            publishResponse(messageIdStr, "PROCESSING", nullptr);

            if (g_commandCallback) {
                g_commandCallback(action, paramsStr.c_str());
            }
        }
    }
}

void MqttClient::onMessage(char* topic, byte* payload, unsigned int length) {
    callback(topic, payload, length);
}

void MqttClient::setCommandCallback(void (*callback)(const char* action, const char* params)) {
    g_commandCallback = callback;
}

void MqttClient::setStatusCallback(void (*callback)(const char* status)) {
    g_statusCallback = callback;
}

// ============================================================
// 外部实例
// ============================================================
MqttClient mqtt;
LogManager logManager;
