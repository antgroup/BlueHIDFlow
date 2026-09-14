// BlueHIDFlow 配置提供者接口
// 消除配置 section 逻辑的三重复制（command_handler, web_server, config_manager）
#ifndef CONFIG_PROVIDER_H
#define CONFIG_PROVIDER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// ============================================================
// 配置提供者接口
// 每个 section 实现此接口，自描述其字段映射
// ============================================================
class IConfigProvider {
public:
    virtual ~IConfigProvider() = default;

    // section 名称（如 "wifi", "mqtt"）
    virtual const char* sectionName() const = 0;

    // 从 NVS 加载
    virtual void load(Preferences& prefs) = 0;

    // 保存到 NVS
    virtual void save(Preferences& prefs) = 0;

    // 重置为默认值
    virtual void reset() = 0;

    // 序列化为 JSON
    virtual JsonObject toJson(JsonDocument& doc) = 0;

    // 从 JSON 反序列化并应用
    // 返回 true 表示成功，false 表示验证失败
    virtual bool fromJson(JsonObject obj) = 0;

    // 校验
    virtual bool isValid() const = 0;
};

// ============================================================
// 配置提供者注册表
// ============================================================
class ConfigProviderRegistry {
public:
    static ConfigProviderRegistry& instance();

    void registerProvider(IConfigProvider* p);
    IConfigProvider* find(const char* section);

    // 批量操作
    void loadAll(Preferences& prefs);
    void saveAll(Preferences& prefs);
    void resetAll();

    // 便捷方法
    String sectionToJson(const char* section);
    String allToJson();
    bool applySection(const char* section, JsonObject obj);

    int count() const { return _count; }

private:
    ConfigProviderRegistry() = default;
    static constexpr int MAX_PROVIDERS = 8;
    IConfigProvider* _providers[MAX_PROVIDERS];
    int _count = 0;
};

#endif // CONFIG_PROVIDER_H