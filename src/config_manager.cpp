// BlueHIDFlow 配置管理器实现
#include "config_manager.h"
#include <WiFi.h>

ConfigManager configManager;

ConfigManager::ConfigManager() : _initialized(false) {
}

void ConfigManager::begin() {
    if (_initialized) return;

    _prefs.begin(NAMESPACE, false);  // false = 读写模式
    _initialized = true;

    Serial.println("[Config] 配置管理器初始化完成");
}

void ConfigManager::end() {
    if (_initialized) {
        _prefs.end();
        _initialized = false;
    }
}

bool ConfigManager::load() {
    if (!_initialized) begin();

    Serial.println("[Config] 加载配置...");

    // Wi-Fi
    _config.wifi.ssid = readString(KEY_WIFI_SSID);
    _config.wifi.password = readString(KEY_WIFI_PASS);

    // 摄像头
    _config.camera.resolution = _prefs.getUChar(KEY_CAM_RES, _config.camera.resolution);
    _config.camera.jpegQuality = _prefs.getUChar(KEY_CAM_QUAL, _config.camera.jpegQuality);
    _config.camera.distanceCm = _prefs.getUChar(KEY_CAM_DIST, _config.camera.distanceCm);

    // MQTT
    _config.mqtt.broker = readString(KEY_MQTT_BROKER);
    _config.mqtt.port = _prefs.getUShort(KEY_MQTT_PORT, _config.mqtt.port);
    _config.mqtt.username = readString(KEY_MQTT_USER);
    _config.mqtt.password = readString(KEY_MQTT_PASS);
    _config.mqtt.caCert = readString(KEY_MQTT_CA);
    _config.mqtt.topicPrefix = readString(KEY_MQTT_PREFIX, _config.mqtt.topicPrefix);

    // 确保主题前缀以 "/" 结尾
    if (_config.mqtt.topicPrefix.length() > 0 && !_config.mqtt.topicPrefix.endsWith("/")) {
        _config.mqtt.topicPrefix += "/";
    }

    // 始终使用 MAC 地址生成设备 ID（忽略之前保存的值）
    _config.mqtt.deviceId = generateDeviceId();
    Serial.printf("[Config] 设备 ID: %s\n", _config.mqtt.deviceId.c_str());
    Serial.printf("[Config] 主题前缀: %s\n", _config.mqtt.topicPrefix.c_str());

    // AI
    _config.ai.apiKey = readString(KEY_AI_KEY);
    _config.ai.apiUrl = readString(KEY_AI_URL, _config.ai.apiUrl);
    _config.ai.model = readString(KEY_AI_MODEL, _config.ai.model);
    _config.ai.timeoutMs = _prefs.getULong(KEY_AI_TIMEOUT, _config.ai.timeoutMs);

    // 蓝牙
    _config.bluetooth.deviceName = readString(KEY_BT_NAME);
    {
        uint8_t rawMode = _prefs.getUChar(KEY_BT_MODE, static_cast<uint8_t>(_config.bluetooth.hidMode));
        // NVS 迁移：旧枚举值 0(AUTO)/1(DIGITIZER)/2(MOUSE)
        //           新枚举值 0(KEYBOARD)/1(MOUSE)
        if (rawMode == 2) {
            // 旧 MOUSE(2) → 新 MOUSE(1)
            _config.bluetooth.hidMode = HIDMode::MOUSE;
            _prefs.putUChar(KEY_BT_MODE, static_cast<uint8_t>(HIDMode::MOUSE));
            Serial.println("[Config] HID模式迁移: 旧MOUSE(2) → 新MOUSE(1)");
        } else if (rawMode == 1) {
            // 新 MOUSE(1) 直接使用
            _config.bluetooth.hidMode = HIDMode::MOUSE;
        } else if (rawMode == 0) {
            // KEYBOARD(0) 或旧 AUTO(0) 都映射为 KEYBOARD
            _config.bluetooth.hidMode = HIDMode::KEYBOARD;
        } else {
            // 其他旧值（如 DIGITIZER=1）映射为 KEYBOARD
            _config.bluetooth.hidMode = HIDMode::KEYBOARD;
            _prefs.putUChar(KEY_BT_MODE, static_cast<uint8_t>(HIDMode::KEYBOARD));
            Serial.printf("[Config] HID模式迁移: 未知值(%d) → KEYBOARD(0)\n", rawMode);
        }
    }

    // 手机
    _config.phone.model = readString(KEY_PHONE_MODEL);
    _config.phone.screenWidth = _prefs.getUShort(KEY_PHONE_W, _config.phone.screenWidth);
    _config.phone.screenHeight = _prefs.getUShort(KEY_PHONE_H, _config.phone.screenHeight);
    _config.phone.hidSensitivity = _prefs.getFloat(KEY_PHONE_SENS, _config.phone.hidSensitivity);
    _config.phone.moveDelayMs = _prefs.getUShort(KEY_PHONE_DELAY, _config.phone.moveDelayMs);

    // 系统
    _config.system.logLevel = static_cast<LogLevel>(_prefs.getUChar(KEY_SYS_LOG, static_cast<uint8_t>(_config.system.logLevel)));
    _config.system.debugEnabled = _prefs.getBool(KEY_SYS_DEBUG, _config.system.debugEnabled);

    Serial.println("[Config] 配置加载完成");
    return true;
}

bool ConfigManager::save() {
    if (!_initialized) begin();

    Serial.println("[Config] 保存配置...");

    // Wi-Fi
    writeString(KEY_WIFI_SSID, _config.wifi.ssid);
    writeString(KEY_WIFI_PASS, _config.wifi.password);

    // 摄像头
    _prefs.putUChar(KEY_CAM_RES, _config.camera.resolution);
    _prefs.putUChar(KEY_CAM_QUAL, _config.camera.jpegQuality);
    _prefs.putUChar(KEY_CAM_DIST, _config.camera.distanceCm);

    // MQTT
    writeString(KEY_MQTT_BROKER, _config.mqtt.broker);
    _prefs.putUShort(KEY_MQTT_PORT, _config.mqtt.port);
    writeString(KEY_MQTT_USER, _config.mqtt.username);
    writeString(KEY_MQTT_PASS, _config.mqtt.password);
    writeString(KEY_MQTT_CA, _config.mqtt.caCert);
    writeString(KEY_MQTT_DEVID, _config.mqtt.deviceId);
    writeString(KEY_MQTT_PREFIX, _config.mqtt.topicPrefix);

    // AI
    writeString(KEY_AI_KEY, _config.ai.apiKey);
    writeString(KEY_AI_URL, _config.ai.apiUrl);
    writeString(KEY_AI_MODEL, _config.ai.model);
    _prefs.putULong(KEY_AI_TIMEOUT, _config.ai.timeoutMs);

    // 蓝牙
    writeString(KEY_BT_NAME, _config.bluetooth.deviceName);
    _prefs.putUChar(KEY_BT_MODE, static_cast<uint8_t>(_config.bluetooth.hidMode));

    // 手机
    writeString(KEY_PHONE_MODEL, _config.phone.model);
    _prefs.putUShort(KEY_PHONE_W, _config.phone.screenWidth);
    _prefs.putUShort(KEY_PHONE_H, _config.phone.screenHeight);
    _prefs.putFloat(KEY_PHONE_SENS, _config.phone.hidSensitivity);
    _prefs.putUShort(KEY_PHONE_DELAY, _config.phone.moveDelayMs);

    // 系统
    _prefs.putUChar(KEY_SYS_LOG, static_cast<uint8_t>(_config.system.logLevel));
    _prefs.putBool(KEY_SYS_DEBUG, _config.system.debugEnabled);

    Serial.println("[Config] 配置保存完成");
    return true;
}

void ConfigManager::reset() {
    if (!_initialized) begin();

    Serial.println("[Config] 重置所有配置...");

    _prefs.clear();

    // 恢复默认值
    _config = AppConfig();
    _config.bluetooth.deviceName = generateDeviceId();
    _config.mqtt.deviceId = generateDeviceId();  // 使用 MAC 地址生成，避免冲突

    save();
}

void ConfigManager::resetSection(const String& section) {
    Serial.printf("[Config] 重置配置节: %s\n", section.c_str());

    if (section == "wifi") {
        _config.wifi = WifiConfig();
        _prefs.remove(KEY_WIFI_SSID);
        _prefs.remove(KEY_WIFI_PASS);
    } else if (section == "camera") {
        _config.camera = CameraConfig();
        _prefs.remove(KEY_CAM_RES);
        _prefs.remove(KEY_CAM_QUAL);
        _prefs.remove(KEY_CAM_DIST);
    } else if (section == "mqtt") {
        _config.mqtt = MqttConfig();
        _config.mqtt.deviceId = generateDeviceId();  // 使用 MAC 地址生成，避免冲突
        _prefs.remove(KEY_MQTT_BROKER);
        _prefs.remove(KEY_MQTT_PORT);
        _prefs.remove(KEY_MQTT_USER);
        _prefs.remove(KEY_MQTT_PASS);
        _prefs.remove(KEY_MQTT_CA);
        _prefs.remove(KEY_MQTT_DEVID);
        _prefs.remove(KEY_MQTT_PREFIX);
    } else if (section == "ai") {
        _config.ai = AIConfig();
        _prefs.remove(KEY_AI_KEY);
        _prefs.remove(KEY_AI_URL);
        _prefs.remove(KEY_AI_MODEL);
        _prefs.remove(KEY_AI_TIMEOUT);
    } else if (section == "bluetooth") {
        _config.bluetooth = BluetoothConfig();
        _config.bluetooth.deviceName = generateDeviceId();
        _prefs.remove(KEY_BT_NAME);
        _prefs.remove(KEY_BT_MODE);
    } else if (section == "phone") {
        _config.phone = PhoneConfig();
        _prefs.remove(KEY_PHONE_MODEL);
        _prefs.remove(KEY_PHONE_W);
        _prefs.remove(KEY_PHONE_H);
        _prefs.remove(KEY_PHONE_SENS);
        _prefs.remove(KEY_PHONE_DELAY);
    } else if (section == "system") {
        _config.system = SystemConfig();
        _prefs.remove(KEY_SYS_LOG);
        _prefs.remove(KEY_SYS_DEBUG);
    }
}

bool ConfigManager::isComplete() const {
    // 必要配置检查
    if (!_config.wifi.isValid()) {
        Serial.println("[Config] Wi-Fi 配置不完整");
        return false;
    }
    if (!_config.mqtt.isValid()) {
        Serial.println("[Config] MQTT 配置不完整");
        return false;
    }
    if (!_config.ai.isValid()) {
        Serial.println("[Config] AI 配置不完整");
        return false;
    }
    if (!_config.phone.isValid()) {
        Serial.println("[Config] 手机配置不完整");
        return false;
    }
    if (_config.bluetooth.deviceName.length() == 0) {
        Serial.println("[Config] 蓝牙设备名称未设置");
        return false;
    }

    return true;
}

bool ConfigManager::validate() const {
    String errors = getValidationErrors();
    return errors.length() == 0;
}

String ConfigManager::getValidationErrors() const {
    String errors = "";

    // Wi-Fi 校验
    if (_config.wifi.ssid.length() > 32) {
        errors += "Wi-Fi SSID 过长 (最大32字符)\n";
    }
    if (_config.wifi.password.length() > 63) {
        errors += "Wi-Fi 密码过长 (最大63字符)\n";
    }

    // 摄像头校验
    if (_config.camera.resolution > 4) {
        errors += "摄像头分辨率设置无效\n";
    }
    if (_config.camera.jpegQuality < 1 || _config.camera.jpegQuality > 63) {
        errors += "JPEG 质量应在 1-63 之间\n";
    }
    if (_config.camera.distanceCm < 5 || _config.camera.distanceCm > 50) {
        errors += "摄像头距离应在 5-50cm 之间\n";
    }

    // MQTT 校验
    if (_config.mqtt.broker.length() > 0 && _config.mqtt.broker.length() > 128) {
        errors += "MQTT Broker 地址过长\n";
    }
    if (_config.mqtt.port == 0) {
        errors += "MQTT 端口不能为 0\n";
    }

    // AI 校验
    if (_config.ai.apiKey.length() > 0 && _config.ai.apiKey.length() > 128) {
        errors += "API Key 过长\n";
    }

    // 手机校验
    if (_config.phone.screenWidth > 4096 || _config.phone.screenHeight > 4096) {
        errors += "手机屏幕分辨率超出合理范围\n";
    }

    return errors;
}

String ConfigManager::generateDeviceId() const {
    // 优先使用 WiFi MAC 地址生成设备 ID
    uint8_t mac[6];
    WiFi.macAddress(mac);

    // 检查 MAC 地址是否有效（全 0 表示获取失败）
    bool macValid = false;
    for (int i = 0; i < 6; i++) {
        if (mac[i] != 0) {
            macValid = true;
            break;
        }
    }

    char id[32];
    if (macValid) {
        // 使用完整 MAC 地址生成 ID（无前缀，纯 MAC 地址）
        snprintf(id, sizeof(id), "%02X%02X%02X%02X%02X%02X",
                 mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    } else {
        // MAC 地址获取失败，使用时间戳后 6 位
        unsigned long timestamp = millis();
        snprintf(id, sizeof(id), "udid-%06lu", timestamp % 1000000);
    }

    Serial.printf("[Config] 生成设备 ID: %s\n", id);
    return String(id);
}

void ConfigManager::printConfig() const {
    Serial.println("\n========== 配置信息 ==========");

    Serial.println("\n[Wi-Fi]");
    Serial.printf("  SSID: %s\n", _config.wifi.ssid.c_str());
    Serial.printf("  密码: %s\n", _config.wifi.password.length() > 0 ? "******" : "(未设置)");

    Serial.println("\n[摄像头]");
    Serial.printf("  分辨率: %s\n", _config.camera.getResolutionName().c_str());
    Serial.printf("  JPEG质量: %d\n", _config.camera.jpegQuality);
    Serial.printf("  距离: %dcm\n", _config.camera.distanceCm);

    Serial.println("\n[MQTT]");
    Serial.printf("  Broker: %s\n", _config.mqtt.broker.c_str());
    Serial.printf("  端口: %d\n", _config.mqtt.port);
    Serial.printf("  用户名: %s\n", _config.mqtt.username.c_str());
    Serial.printf("  CA 证书: %s\n", _config.mqtt.caCert.length() > 0 ? "已设置" : "(未设置)");
    Serial.printf("  设备ID: %s\n", _config.mqtt.deviceId.c_str());
    Serial.printf("  主题前缀: %s\n", _config.mqtt.topicPrefix.c_str());

    Serial.println("\n[大模型]");
    Serial.printf("  API URL: %s\n", _config.ai.apiUrl.c_str());
    Serial.printf("  模型: %s\n", _config.ai.model.c_str());
    Serial.printf("  API Key: %s\n", _config.ai.apiKey.length() > 0 ? "******" : "(未设置)");
    Serial.printf("  超时: %lums\n", _config.ai.timeoutMs);

    Serial.println("\n[蓝牙]");
    Serial.printf("  设备名称: %s\n", _config.bluetooth.deviceName.c_str());
    Serial.printf("  HID模式: %s\n", _config.bluetooth.getHidModeName().c_str());

    Serial.println("\n[手机]");
    Serial.printf("  型号: %s\n", _config.phone.model.c_str());
    Serial.printf("  屏幕分辨率: %d x %d\n", _config.phone.screenWidth, _config.phone.screenHeight);
    Serial.printf("  HID 灵敏度: %.2f 像素/HID单位\n", _config.phone.hidSensitivity);
    Serial.printf("  HID 移动延迟: %dms\n", _config.phone.moveDelayMs);

    Serial.println("\n[系统]");
    Serial.printf("  日志级别: %s\n", _config.system.getLogLevelName().c_str());
    Serial.printf("  调试模式: %s\n", _config.system.debugEnabled ? "开启" : "关闭");

    Serial.println("\n[状态]");
    Serial.printf("  配置完整: %s\n", isComplete() ? "是" : "否");
    Serial.printf("  校验通过: %s\n", validate() ? "是" : "否");

    String errors = getValidationErrors();
    if (errors.length() > 0) {
        Serial.println("\n[错误]");
        Serial.println(errors);
    }

    Serial.println("==============================\n");
}

// 私有方法：读取字符串
String ConfigManager::readString(const char* key, const String& defaultVal) {
    return _prefs.getString(key, defaultVal.c_str());
}

// 私有方法：写入字符串
void ConfigManager::writeString(const char* key, const String& value) {
    _prefs.putString(key, value);
}