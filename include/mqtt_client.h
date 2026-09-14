#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "imqtt_publisher.h"

// ============================================================
// MQTT 默认配置（可通过 Web 配置页面或 config.h 覆盖）
// ============================================================
// 注意：以下为编译时默认值，实际运行时从 ConfigManager 读取用户配置
// TLS CA 证书由开发者在 config.h 中通过 MQTT_CA_CERT 配置，不内置厂商证书
#define MQTT_BROKER     ""          // MQTT Broker 地址
#define MQTT_PORT       8883        // MQTT Broker 端口（TLS 默认 8883）
#define MQTT_USE_TLS    1           // 是否使用 TLS 加密
#define MQTT_USERNAME   ""          // MQTT 用户名
#define MQTT_PASSWORD   ""          // MQTT 密码

// 设备 ID (由 WiFi MAC 地址初始化)
extern char g_deviceId[32];

// ============================================================
// MQTT 主题定义（默认值，可通过配置覆盖）
// 注意：实际主题前缀从 configManager.mqtt().topicPrefix 读取
// ============================================================
#define DEFAULT_TOPIC_PREFIX  "bluehidflow/"
#define CMD_TOPIC_SUFFIX      "command/"
#define RSP_TOPIC_SUFFIX      "response/"
#define STATUS_TOPIC_SUFFIX   "status/"
#define LOG_TOPIC_SUFFIX      "log/"
#define CAMERA_TOPIC_SUFFIX   "camera/"

// 主题缓冲区
extern char g_commandTopic[128];
extern char g_responseTopic[128];
extern char g_statusTopic[128];
extern char g_logTopic[128];

// ============================================================
// 日志系统配置
// ============================================================
#define MQTT_LOG_CACHE_SIZE       20       // 日志缓存最大条数
#define MQTT_LOG_UPLOAD_INTERVAL  30000    // 日志上报间隔 (30秒)
#define MQTT_LOG_MAX_LENGTH       128      // 单条日志最大长度

// 日志级别 (避免与 NimBLE 宏冲突)
typedef enum {
    MQTT_LOG_INFO = 0,
    MQTT_LOG_WARN = 1,
    MQTT_LOG_ERROR = 2
} MqttLogLevel;

// 日志事件类型（关键事件）
typedef enum {
    MQTT_LOG_EVENT_BOOT,           // 启动
    MQTT_LOG_EVENT_WIFI_CONNECT,   // WiFi 连接
    MQTT_LOG_EVENT_WIFI_DISCONNECT,// WiFi 断开
    MQTT_LOG_EVENT_MQTT_CONNECT,   // MQTT 连接
    MQTT_LOG_EVENT_MQTT_DISCONNECT,// MQTT 断开
    MQTT_LOG_EVENT_BLE_CONNECT,    // BLE 连接
    MQTT_LOG_EVENT_BLE_DISCONNECT, // BLE 断开
    MQTT_LOG_EVENT_ERROR,          // 异常错误
    MQTT_LOG_EVENT_RESTART,        // 重启
    MQTT_LOG_EVENT_CONFIG_SAVE,    // 配置保存
    MQTT_LOG_EVENT_NFC_DETECT,     // NFC 检测到标签
    MQTT_LOG_EVENT_NFC_REMOVE      // NFC 标签移除
} MqttLogEventType;

// ============================================================
// 设备状态枚举
// ============================================================
typedef enum {
    STATUS_OFFLINE = 0,
    STATUS_CONNECTING,
    STATUS_ONLINE,
    STATUS_PROCESSING
} DeviceStatus;

// 外部声明设备状态 (在 main.cpp 中定义)
extern DeviceStatus g_deviceStatus;
extern unsigned long g_lastStatusPublish;
extern const unsigned long STATUS_PUBLISH_INTERVAL;

// ============================================================
// MQTT 回调类型
// ============================================================
typedef void (*MQTTCommandCallback)(const char* action, const char* params);
typedef void (*MQTTStatusCallback)(const char* status);

// ============================================================
// 日志缓存结构
// ============================================================
struct MqttLogEntry {
    unsigned long timestamp;
    MqttLogLevel level;
    MqttLogEventType event;
    char message[MQTT_LOG_MAX_LENGTH];
};

// ============================================================
// 日志管理器类
// ============================================================
class LogManager {
public:
    LogManager();

    void begin();
    void addLog(MqttLogLevel level, MqttLogEventType event, const char* message);
    void loop();  // 在主循环中调用，检查是否需要上报

    // 强制立即上报（用于关键错误）
    void flush();

    // 获取缓存统计
    int getCount() const { return _count; }
    bool isEmpty() const { return _count == 0; }

private:
    MqttLogEntry _cache[MQTT_LOG_CACHE_SIZE];
    int _head;          // 写入位置
    int _count;         // 当前条数
    unsigned long _lastUpload;
    bool _initialized;

    bool uploadLogs();  // 上报日志到 MQTT
    void clear();       // 清空缓存
    const char* getLevelName(MqttLogLevel level);
    const char* getEventName(MqttLogEventType event);
};

// ============================================================
// MQTT 客户端类
// ============================================================
class MqttClient : public IMqttPublisher {
public:
    MqttClient();

    // 初始化
    bool begin();
    void setupTopics();

    // 设置服务器 IP（DNS 解析后）
    void setResolvedIP(const IPAddress& ip) { _resolvedIP = ip; _useResolvedIP = true; }

    // 连接管理
    bool connect();
    bool isConnected();
    void loop();
    void reconnect();

    // 发布
    bool publish(const char* topic, const char* payload, bool retain = false);
    bool publishCommand(const char* action, const char* message_id, const char* params);
    bool publishStatus(const char* status);
    bool publishResponse(const char* message_id, const char* status, const char* error = nullptr);
    bool publishResponseWithData(const char* message_id, const char* status, JsonDocument& data);
    bool publishLog(const char* payload);
    bool publishCameraFrame(const char* frame_base64, size_t size);  // 摄像头帧发布（已废弃）
    bool publishCameraUrl(const char* url);  // 摄像头帧 URL 发布

    // 订阅
    bool subscribe(const char* topic);
    void subscribeToCommands();

    // 回调
    void onMessage(char* topic, byte* payload, unsigned int length);
    void setCommandCallback(void (*callback)(const char* action, const char* params));
    void setStatusCallback(void (*callback)(const char* status));

private:
    WiFiClientSecure _wifiClient;
    PubSubClient _mqttClient;
    bool _initialized;
    MQTTCommandCallback _commandCallback = nullptr;
    MQTTStatusCallback _statusCallback = nullptr;
    IPAddress _resolvedIP;
    bool _useResolvedIP = false;

    void setupTLS();
    void callback(char* topic, byte* payload, unsigned int length);
};

// 外部实例
extern MqttClient mqtt;
extern LogManager logManager;

// ============================================================
// 便捷日志宏
// ============================================================
#define MQTT_LOG_INFO(event, msg)    logManager.addLog(MQTT_LOG_INFO, event, msg)
#define MQTT_LOG_WARN(event, msg)    logManager.addLog(MQTT_LOG_WARN, event, msg)
#define MQTT_LOG_ERROR(event, msg)   logManager.addLog(MQTT_LOG_ERROR, event, msg)

#endif // MQTT_CLIENT_H