// BlueHIDFlow 配置提供者实现
// 消除配置 section 逻辑的三重复制
#include "config_provider.h"
#include "config_manager.h"

// ============================================================
// Wi-Fi 配置提供者
// ============================================================
class WifiConfigProvider : public IConfigProvider {
public:
    const char* sectionName() const override { return "wifi"; }

    void load(Preferences& prefs) override {
        // 由 ConfigManager 统一加载
    }
    void save(Preferences& prefs) override {
        // 由 ConfigManager 统一保存
    }
    void reset() override {
        configManager.wifi() = WifiConfig();
    }

    JsonObject toJson(JsonDocument& doc) override {
        JsonObject obj = doc["wifi"].to<JsonObject>();
        obj["ssid"] = configManager.wifi().ssid;
        // 不在 API/页面中暴露明文 WiFi 密码
        obj["password"] = configManager.wifi().password.length() > 0 ? "******" : "";
        return obj;
    }

    bool fromJson(JsonObject obj) override {
        if (obj.containsKey("ssid")) configManager.wifi().ssid = obj["ssid"].as<String>();
        // 空密码视为未修改，避免前端未填写时覆盖已保存的密码
        if (obj.containsKey("password")) {
            String pwd = obj["password"].as<String>();
            if (pwd.length() > 0) {
                configManager.wifi().password = pwd;
            }
        }
        return true;
    }

    bool isValid() const override {
        return configManager.wifi().isValid();
    }
};

// ============================================================
// 摄像头配置提供者
// ============================================================
class CameraConfigProvider : public IConfigProvider {
public:
    const char* sectionName() const override { return "camera"; }

    void load(Preferences& prefs) override {}
    void save(Preferences& prefs) override {}
    void reset() override { configManager.camera() = CameraConfig(); }

    JsonObject toJson(JsonDocument& doc) override {
        JsonObject obj = doc["camera"].to<JsonObject>();
        obj["resolution"] = configManager.camera().resolution;
        obj["jpegQuality"] = configManager.camera().jpegQuality;
        obj["distanceCm"] = configManager.camera().distanceCm;
        return obj;
    }

    bool fromJson(JsonObject obj) override {
        if (obj.containsKey("resolution")) configManager.camera().resolution = obj["resolution"].as<uint8_t>();
        if (obj.containsKey("jpegQuality")) configManager.camera().jpegQuality = obj["jpegQuality"].as<uint8_t>();
        if (obj.containsKey("distanceCm")) configManager.camera().distanceCm = obj["distanceCm"].as<uint8_t>();
        return true;
    }

    bool isValid() const override { return true; }
};

// ============================================================
// MQTT 配置提供者
// ============================================================
class MqttConfigProvider : public IConfigProvider {
public:
    const char* sectionName() const override { return "mqtt"; }

    void load(Preferences& prefs) override {}
    void save(Preferences& prefs) override {}
    void reset() override {
        configManager.mqtt() = MqttConfig();
        configManager.mqtt().deviceId = configManager.generateDeviceId();
    }

    JsonObject toJson(JsonDocument& doc) override {
        JsonObject obj = doc["mqtt"].to<JsonObject>();
        obj["broker"] = configManager.mqtt().broker;
        obj["port"] = configManager.mqtt().port;
        obj["username"] = configManager.mqtt().username;
        obj["caCert"] = configManager.mqtt().caCert;
        obj["deviceId"] = configManager.mqtt().deviceId;
        return obj;
    }

    bool fromJson(JsonObject obj) override {
        if (obj.containsKey("broker")) configManager.mqtt().broker = obj["broker"].as<String>();
        if (obj.containsKey("port")) configManager.mqtt().port = obj["port"].as<uint16_t>();
        if (obj.containsKey("username")) configManager.mqtt().username = obj["username"].as<String>();
        // 空密码视为未修改，避免前端未填写时覆盖已保存的密码
        if (obj.containsKey("password")) {
            String pwd = obj["password"].as<String>();
            if (pwd.length() > 0) {
                configManager.mqtt().password = pwd;
            }
        }
        if (obj.containsKey("caCert")) configManager.mqtt().caCert = obj["caCert"].as<String>();
        if (obj.containsKey("deviceId")) configManager.mqtt().deviceId = obj["deviceId"].as<String>();
        return true;
    }

    bool isValid() const override {
        return configManager.mqtt().isValid();
    }
};

// ============================================================
// AI 配置提供者
// ============================================================
class AIConfigProvider : public IConfigProvider {
public:
    const char* sectionName() const override { return "ai"; }

    void load(Preferences& prefs) override {}
    void save(Preferences& prefs) override {}
    void reset() override { configManager.ai() = AIConfig(); }

    JsonObject toJson(JsonDocument& doc) override {
        JsonObject obj = doc["ai"].to<JsonObject>();
        // 不在 API/页面中暴露明文 API Key
        obj["apiKey"] = configManager.ai().apiKey.length() > 0 ? "******" : "";
        obj["apiUrl"] = configManager.ai().apiUrl;
        obj["model"] = configManager.ai().model;
        obj["timeoutMs"] = configManager.ai().timeoutMs;
        return obj;
    }

    bool fromJson(JsonObject obj) override {
        // 空 API Key 视为未修改，避免前端未填写时覆盖已保存的值
        if (obj.containsKey("apiKey")) {
            String key = obj["apiKey"].as<String>();
            if (key.length() > 0) {
                configManager.ai().apiKey = key;
            }
        }
        if (obj.containsKey("apiUrl")) configManager.ai().apiUrl = obj["apiUrl"].as<String>();
        if (obj.containsKey("model")) configManager.ai().model = obj["model"].as<String>();
        if (obj.containsKey("timeoutMs")) configManager.ai().timeoutMs = obj["timeoutMs"].as<uint32_t>();
        return true;
    }

    bool isValid() const override {
        return configManager.ai().isValid();
    }
};

// ============================================================
// 蓝牙配置提供者
// ============================================================
class BluetoothConfigProvider : public IConfigProvider {
public:
    const char* sectionName() const override { return "bluetooth"; }

    void load(Preferences& prefs) override {}
    void save(Preferences& prefs) override {}
    void reset() override {
        configManager.bluetooth() = BluetoothConfig();
        configManager.bluetooth().deviceName = configManager.generateDeviceId();
    }

    JsonObject toJson(JsonDocument& doc) override {
        JsonObject obj = doc["bluetooth"].to<JsonObject>();
        obj["deviceName"] = configManager.bluetooth().deviceName;
        obj["hidMode"] = static_cast<uint8_t>(configManager.bluetooth().hidMode);
        return obj;
    }

    bool fromJson(JsonObject obj) override {
        if (obj.containsKey("deviceName")) configManager.bluetooth().deviceName = obj["deviceName"].as<String>();
        if (obj.containsKey("hidMode")) configManager.bluetooth().hidMode = static_cast<HIDMode>(obj["hidMode"].as<uint8_t>());
        return true;
    }

    bool isValid() const override { return true; }
};

// ============================================================
// 手机配置提供者
// ============================================================
class PhoneConfigProvider : public IConfigProvider {
public:
    const char* sectionName() const override { return "phone"; }

    void load(Preferences& prefs) override {}
    void save(Preferences& prefs) override {}
    void reset() override { configManager.phone() = PhoneConfig(); }

    JsonObject toJson(JsonDocument& doc) override {
        JsonObject obj = doc["phone"].to<JsonObject>();
        obj["model"] = configManager.phone().model;
        obj["screenWidth"] = configManager.phone().screenWidth;
        obj["screenHeight"] = configManager.phone().screenHeight;
        obj["hidSensitivity"] = configManager.phone().hidSensitivity;
        obj["moveDelayMs"] = configManager.phone().moveDelayMs;
        return obj;
    }

    bool fromJson(JsonObject obj) override {
        if (obj.containsKey("model")) configManager.phone().model = obj["model"].as<String>();
        if (obj.containsKey("screenWidth")) {
            uint32_t w = obj["screenWidth"].as<uint32_t>();
            if (w >= 100 && w <= 4096) configManager.phone().screenWidth = (uint16_t)w;
            else return false;
        }
        if (obj.containsKey("screenHeight")) {
            uint32_t h = obj["screenHeight"].as<uint32_t>();
            if (h >= 100 && h <= 4096) configManager.phone().screenHeight = (uint16_t)h;
            else return false;
        }
        if (obj.containsKey("hidSensitivity")) {
            float s = obj["hidSensitivity"].as<float>();
            if (s >= 0.1 && s <= 100.0) configManager.phone().hidSensitivity = s;
            else return false;
        }
        if (obj.containsKey("moveDelayMs")) configManager.phone().moveDelayMs = obj["moveDelayMs"].as<uint16_t>();
        return true;
    }

    bool isValid() const override {
        return configManager.phone().isValid();
    }
};

// ============================================================
// 系统配置提供者
// ============================================================
class SystemConfigProvider : public IConfigProvider {
public:
    const char* sectionName() const override { return "system"; }

    void load(Preferences& prefs) override {}
    void save(Preferences& prefs) override {}
    void reset() override { configManager.system() = SystemConfig(); }

    JsonObject toJson(JsonDocument& doc) override {
        JsonObject obj = doc["system"].to<JsonObject>();
        obj["logLevel"] = static_cast<uint8_t>(configManager.system().logLevel);
        obj["debugEnabled"] = configManager.system().debugEnabled;
        return obj;
    }

    bool fromJson(JsonObject obj) override {
        if (obj.containsKey("logLevel")) configManager.system().logLevel = static_cast<LogLevel>(obj["logLevel"].as<uint8_t>());
        if (obj.containsKey("debugEnabled")) configManager.system().debugEnabled = obj["debugEnabled"].as<bool>();
        return true;
    }

    bool isValid() const override { return true; }
};

// ============================================================
// ConfigProviderRegistry 实现
// ============================================================
ConfigProviderRegistry& ConfigProviderRegistry::instance() {
    static ConfigProviderRegistry registry;
    return registry;
}

void ConfigProviderRegistry::registerProvider(IConfigProvider* p) {
    if (_count >= MAX_PROVIDERS) {
        Serial.printf("[ConfigProvider] 注册表已满，无法注册: %s\n", p->sectionName());
        return;
    }
    _providers[_count++] = p;
    Serial.printf("[ConfigProvider] 注册: %s\n", p->sectionName());
}

IConfigProvider* ConfigProviderRegistry::find(const char* section) {
    for (int i = 0; i < _count; i++) {
        if (strcmp(_providers[i]->sectionName(), section) == 0) {
            return _providers[i];
        }
    }
    return nullptr;
}

void ConfigProviderRegistry::loadAll(Preferences& prefs) {
    for (int i = 0; i < _count; i++) {
        _providers[i]->load(prefs);
    }
}

void ConfigProviderRegistry::saveAll(Preferences& prefs) {
    for (int i = 0; i < _count; i++) {
        _providers[i]->save(prefs);
    }
}

void ConfigProviderRegistry::resetAll() {
    for (int i = 0; i < _count; i++) {
        _providers[i]->reset();
    }
}

String ConfigProviderRegistry::sectionToJson(const char* section) {
    IConfigProvider* p = find(section);
    if (!p) return "{}";

    JsonDocument doc;
    p->toJson(doc);
    String result;
    serializeJson(doc, result);
    return result;
}

String ConfigProviderRegistry::allToJson() {
    JsonDocument doc;
    for (int i = 0; i < _count; i++) {
        _providers[i]->toJson(doc);
    }
    String result;
    serializeJson(doc, result);
    return result;
}

bool ConfigProviderRegistry::applySection(const char* section, JsonObject obj) {
    IConfigProvider* p = find(section);
    if (!p) return false;
    return p->fromJson(obj);
}

// ============================================================
// 静态注册所有配置提供者
// ============================================================
static WifiConfigProvider       _provWifi;
static CameraConfigProvider     _provCamera;
static MqttConfigProvider       _provMqtt;
static AIConfigProvider         _provAI;
static BluetoothConfigProvider  _provBT;
static PhoneConfigProvider      _provPhone;
static SystemConfigProvider     _provSystem;

static bool _providersRegistered = [](){
    auto& reg = ConfigProviderRegistry::instance();
    reg.registerProvider(&_provWifi);
    reg.registerProvider(&_provCamera);
    reg.registerProvider(&_provMqtt);
    reg.registerProvider(&_provAI);
    reg.registerProvider(&_provBT);
    reg.registerProvider(&_provPhone);
    reg.registerProvider(&_provSystem);
    return true;
}();