// BlueHIDFlow 配置命令实现
// GET_CONFIG, SET_CONFIG, CONFIG
// 使用 ConfigProviderRegistry 消除配置逻辑重复
#include "command_registry.h"
#include "command_context.h"
#include "config_provider.h"
#include "ble_hid.h"
#include "config_manager.h"
#include "mqtt_client.h"

// ============================================================
// GET_CONFIG 命令 - 获取配置
// ============================================================
class GetConfigCommand : public ICommand {
public:
    const char* name() const override { return "GET_CONFIG"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: ===== 执行 GET_CONFIG 命令 =====");

        String section = "";
        if (ctx.params && strlen(ctx.params) > 0) {
            JsonDocument doc;
            if (ctx.parseParams(doc)) {
                JsonObject paramsObj = doc.as<JsonObject>();
                if (paramsObj.containsKey("section")) {
                    section = paramsObj["section"].as<String>();
                }
            }
        }

        Serial.printf("CMD: 获取配置节：%s\n", section.length() > 0 ? section.c_str() : "(全部)");

        JsonDocument dataDoc;
        auto& registry = ConfigProviderRegistry::instance();

        if (section.length() == 0) {
            // 获取所有配置节
            for (int i = 0; i < registry.count(); i++) {
                // 遍历所有 provider 并序列化
                // 使用 allToJson 然后反序列化到 dataDoc
                String allJson = registry.allToJson();
                deserializeJson(dataDoc, allJson);
                break;
            }
        } else {
            IConfigProvider* provider = registry.find(section.c_str());
            if (provider) {
                provider->toJson(dataDoc);
            }
        }

        if (ctx.mqtt.publishResponseWithData(ctx.messageId, "SUCCESS", dataDoc)) {
            Serial.println("CMD: 配置查询成功，响应已发布");
            return RESPONSE_SENT;
        } else {
            Serial.println("CMD: 配置查询成功，但发布失败");
            return FAILED;
        }
    }
};
REGISTER_COMMAND(GetConfigCommand)

// ============================================================
// SET_CONFIG 命令 - 设置配置
// ============================================================
class SetConfigCommand : public ICommand {
public:
    const char* name() const override { return "SET_CONFIG"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: ===== 执行 SET_CONFIG 命令 =====");

        if (!ctx.params || strlen(ctx.params) == 0) {
            Serial.println("CMD: 错误 - params 为空");
            return FAILED;
        }

        Serial.printf("CMD: 原始参数：%s\n", ctx.params);

        JsonDocument doc;
        if (!ctx.parseParams(doc)) return FAILED;

        JsonObject root = doc.as<JsonObject>();
        if (root.isNull()) {
            Serial.println("CMD: 错误 - 空 JSON");
            return FAILED;
        }

        bool anySectionApplied = false;
        String lastSection = "";
        auto& registry = ConfigProviderRegistry::instance();

        // 遍历所有注册的 provider，检查 JSON 中是否有对应的 section
        for (int i = 0; i < registry.count(); i++) {
            // 这里无法直接遍历，用 section 名称检查
        }

        // 使用已知 section 列表遍历（与 provider 注册顺序无关）
        const char* sections[] = {"wifi", "camera", "mqtt", "ai", "bluetooth", "phone", "system"};
        for (const char* sec : sections) {
            if (root.containsKey(sec)) {
                IConfigProvider* provider = registry.find(sec);
                if (provider) {
                    JsonObject sectionConfig = root[sec].as<JsonObject>();
                    if (provider->fromJson(sectionConfig)) {
                        anySectionApplied = true;
                        lastSection = sec;
                    }
                }
            }
        }

        if (!anySectionApplied) {
            JsonDocument errorDoc;
            errorDoc["error"] = "没有有效的配置节或配置验证失败";
            ctx.mqtt.publishResponseWithData(ctx.messageId, "FAILED", errorDoc);
            return RESPONSE_SENT;
        }

        if (!ctx.config.validate()) {
            String errors = ctx.config.getValidationErrors();
            Serial.printf("CMD: 配置验证失败：%s\n", errors.c_str());
            JsonDocument errorDoc;
            errorDoc["error"] = errors.c_str();
            ctx.mqtt.publishResponseWithData(ctx.messageId, "FAILED", errorDoc);
            return RESPONSE_SENT;
        }

        if (ctx.config.save()) {
            Serial.println("CMD: 配置已保存");
            ctx.log.addLog(MQTT_LOG_INFO, MQTT_LOG_EVENT_CONFIG_SAVE, "Config updated via MQTT");

            JsonDocument dataDoc;
            dataDoc["saved"] = true;
            dataDoc["section"] = lastSection;
            bool restartRequired = (lastSection == "wifi" || lastSection == "mqtt" || lastSection == "bluetooth");
            dataDoc["restartRequired"] = restartRequired;
            if (restartRequired) {
                Serial.println("CMD: 警告 - 配置修改需要重启相关服务才能生效");
            }

            if (ctx.mqtt.publishResponseWithData(ctx.messageId, "SUCCESS", dataDoc)) {
                Serial.println("CMD: 配置保存成功，响应已发布");
                return RESPONSE_SENT;
            }
        } else {
            Serial.println("CMD: 错误 - 配置保存失败");
        }
        return FAILED;
    }
};
REGISTER_COMMAND(SetConfigCommand)

// ============================================================
// CONFIG 命令 - 配置更新（未实现）
// ============================================================
class ConfigCommand : public ICommand {
public:
    const char* name() const override { return "CONFIG"; }
    CommandResult execute(CommandContext& ctx) override {
        Serial.println("CMD: 执行 CONFIG 命令");
        if (!ctx.params || strlen(ctx.params) == 0) {
            Serial.println("CMD: 错误 - params 为空");
            return FAILED;
        }
        Serial.println("CMD: 配置更新已接收");
        return NOT_IMPLEMENTED;
    }
};
REGISTER_COMMAND(ConfigCommand)