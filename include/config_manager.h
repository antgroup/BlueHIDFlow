// BlueHIDFlow 配置管理器
// 支持持久化存储和 Web 配置
#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>

// ============================================
// Wi-Fi 配置
// ============================================
struct WifiConfig {
    String ssid;
    String password;

    WifiConfig() : ssid(""), password("") {}

    bool isValid() const {
        return ssid.length() > 0;
    }
};

// ============================================
// 摄像头配置
// ============================================
struct CameraConfig {
    uint8_t resolution;      // 0=QVGA, 1=VGA, 2=SVGA, 3=XGA, 4=SXGA
    uint8_t jpegQuality;     // 1-63, 越小质量越高
    uint8_t distanceCm;      // 摄像头距离 (cm)

    CameraConfig() : resolution(2), jpegQuality(10), distanceCm(18) {}

    // 获取实际分辨率
    void getResolution(uint16_t& width, uint16_t& height) const {
        switch(resolution) {
            case 0: width = 320; height = 240; break;  // QVGA
            case 1: width = 640; height = 480; break;  // VGA
            case 2: width = 800; height = 600; break;  // SVGA (默认)
            case 3: width = 1024; height = 768; break; // XGA
            case 4: width = 1280; height = 960; break; // SXGA
            default: width = 800; height = 600; break;
        }
    }

    String getResolutionName() const {
        switch(resolution) {
            case 0: return "QVGA (320x240)";
            case 1: return "VGA (640x480)";
            case 2: return "SVGA (800x600)";
            case 3: return "XGA (1024x768)";
            case 4: return "SXGA (1280x960)";
            default: return "Unknown";
        }
    }
};

// ============================================
// MQTT 配置
// ============================================
struct MqttConfig {
    String broker;
    uint16_t port;
    String username;
    String password;
    String caCert;       // MQTT TLS CA 证书 (PEM 格式)，由开发者配置
    String deviceId;
    String topicPrefix;  // MQTT 主题前缀，默认 "bluehidflow/"

    MqttConfig() : port(8883), deviceId(""), topicPrefix("bluehidflow/") {}

    bool isValid() const {
        return broker.length() > 0 && deviceId.length() > 0;
    }
};

// ============================================
// 大模型 API 配置
// ============================================
// 默认采用 OpenAI 兼容协议（/v1/chat/completions），用户可在 Web 配置页面填写：
//   - OpenAI:        https://api.openai.com/v1/chat/completions
//   - Anthropic:     需使用兼容代理或修改 vision provider
//   - 阿里通义千问:  https://dashscope.aliyuncs.com/compatible-mode/v1/chat/completions
//   - 本地 Ollama:   http://<host>:11434/v1/chat/completions
struct AIConfig {
    String apiKey;
    String apiUrl;
    String model;
    uint32_t timeoutMs;

    AIConfig() : apiUrl(""),
                 model(""),
                 timeoutMs(30000) {}

    bool isValid() const {
        return apiKey.length() > 0;
    }
};

// ============================================
// 蓝牙配置
// ============================================
enum class HIDMode : uint8_t {
    KEYBOARD = 0,   // 键盘模式：文字输入、系统键、媒体键
    MOUSE = 1       // 鼠标模式：精确定位点击
};

struct BluetoothConfig {
    String deviceName;
    HIDMode hidMode;

    BluetoothConfig() : deviceName(""), hidMode(HIDMode::KEYBOARD) {}

    String getHidModeName() const {
        switch(hidMode) {
            case HIDMode::KEYBOARD: return "键盘模式";
            case HIDMode::MOUSE: return "鼠标模式";
            default: return "未知";
        }
    }
};

// ============================================
// 手机配置
// ============================================
struct PhoneConfig {
    String model;
    uint16_t screenWidth;
    uint16_t screenHeight;
    float hidSensitivity;   // HID 灵敏度：像素/HID单位
    uint16_t moveDelayMs;   // HID 移动延迟（毫秒），用于避免 Android 鼠标加速

    PhoneConfig() : screenWidth(1080), screenHeight(2400), hidSensitivity(2.5f), moveDelayMs(100) {}

    bool isValid() const {
        return screenWidth > 0 && screenHeight > 0;
    }
};

// ============================================
// 系统配置
// ============================================
enum class LogLevel : uint8_t {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

struct SystemConfig {
    LogLevel logLevel;
    bool debugEnabled;

    SystemConfig() : logLevel(LogLevel::INFO), debugEnabled(false) {}

    String getLogLevelName() const {
        switch(logLevel) {
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            default: return "Unknown";
        }
    }
};

// ============================================
// 完整配置
// ============================================
struct AppConfig {
    WifiConfig wifi;
    CameraConfig camera;
    MqttConfig mqtt;
    AIConfig ai;
    BluetoothConfig bluetooth;
    PhoneConfig phone;
    SystemConfig system;
};

// ============================================
// 配置管理器
// ============================================
class ConfigManager {
public:
    ConfigManager();

    // 初始化
    void begin();
    void end();

    // 加载/保存
    bool load();
    bool save();
    void reset();
    void resetSection(const String& section);

    // 校验
    bool isComplete() const;      // 检查必要配置是否完整
    bool validate() const;        // 验证配置格式
    String getValidationErrors() const;

    // 获取配置
    AppConfig& getConfig() { return _config; }
    const AppConfig& getConfig() const { return _config; }

    // 快捷访问
    WifiConfig& wifi() { return _config.wifi; }
    CameraConfig& camera() { return _config.camera; }
    MqttConfig& mqtt() { return _config.mqtt; }
    AIConfig& ai() { return _config.ai; }
    BluetoothConfig& bluetooth() { return _config.bluetooth; }
    PhoneConfig& phone() { return _config.phone; }
    SystemConfig& system() { return _config.system; }

    const WifiConfig& wifi() const { return _config.wifi; }
    const CameraConfig& camera() const { return _config.camera; }
    const MqttConfig& mqtt() const { return _config.mqtt; }
    const AIConfig& ai() const { return _config.ai; }
    const BluetoothConfig& bluetooth() const { return _config.bluetooth; }
    const PhoneConfig& phone() const { return _config.phone; }
    const SystemConfig& system() const { return _config.system; }

    // 设备 ID 生成
    String generateDeviceId() const;

    // 调试
    void printConfig() const;

private:
    Preferences _prefs;
    AppConfig _config;
    bool _initialized;

    // 存储键名
    static constexpr const char* NAMESPACE = "bluehidflow";

    // Wi-Fi 键
    static constexpr const char* KEY_WIFI_SSID = "wifi_ssid";
    static constexpr const char* KEY_WIFI_PASS = "wifi_pass";

    // 摄像头键
    static constexpr const char* KEY_CAM_RES = "cam_res";
    static constexpr const char* KEY_CAM_QUAL = "cam_qual";
    static constexpr const char* KEY_CAM_DIST = "cam_dist";

    // MQTT 键
    static constexpr const char* KEY_MQTT_BROKER = "mqtt_host";
    static constexpr const char* KEY_MQTT_PORT = "mqtt_port";
    static constexpr const char* KEY_MQTT_USER = "mqtt_user";
    static constexpr const char* KEY_MQTT_PASS = "mqtt_pass";
    static constexpr const char* KEY_MQTT_CA = "mqtt_ca";  // TLS CA 证书
    static constexpr const char* KEY_MQTT_DEVID = "mqtt_devid";
    static constexpr const char* KEY_MQTT_PREFIX = "mqtt_prefix";  // 主题前缀

    // AI 键
    static constexpr const char* KEY_AI_KEY = "ai_key";
    static constexpr const char* KEY_AI_URL = "ai_url";
    static constexpr const char* KEY_AI_MODEL = "ai_model";
    static constexpr const char* KEY_AI_TIMEOUT = "ai_timeout";

    // 蓝牙键
    static constexpr const char* KEY_BT_NAME = "bt_name";
    static constexpr const char* KEY_BT_MODE = "bt_mode";

    // 手机键
    static constexpr const char* KEY_PHONE_MODEL = "phone_model";
    static constexpr const char* KEY_PHONE_W = "phone_w";
    static constexpr const char* KEY_PHONE_H = "phone_h";
    static constexpr const char* KEY_PHONE_SENS = "phone_sens";  // HID 灵敏度
    static constexpr const char* KEY_PHONE_DELAY = "phone_delay";  // HID 移动延迟

    // 系统键
    static constexpr const char* KEY_SYS_LOG = "sys_log";
    static constexpr const char* KEY_SYS_DEBUG = "sys_debug";

    // 辅助方法
    String readString(const char* key, const String& defaultVal = "");
    void writeString(const char* key, const String& value);
};

// 全局实例
extern ConfigManager configManager;

#endif // CONFIG_MANAGER_H