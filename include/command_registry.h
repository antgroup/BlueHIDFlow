// BlueHIDFlow 命令注册表
// 提供自注册命令模式，新增命令只需添加一个 .cpp 文件
#ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H

#include <Arduino.h>

// 命令结果枚举
typedef enum {
    SUCCESS,
    FAILED,
    TIMEOUT,
    INVALID_PARAMS,
    NOT_IMPLEMENTED,
    RESPONSE_SENT    // 命令已自行发送 MQTT 响应，execute() 末尾无需再发
} CommandResult;

class CommandContext;

// ============================================================
// 命令接口
// ============================================================
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual const char* name() const = 0;
    virtual CommandResult execute(CommandContext& ctx) = 0;
};

// ============================================================
// 命令注册表（单例）
// ============================================================
class CommandRegistry {
public:
    static CommandRegistry& instance();

    void registerCommand(ICommand* cmd);
    ICommand* find(const char* name);
    void forEach(void (*callback)(ICommand*));
    int count() const { return _count; }

private:
    CommandRegistry() = default;
    static constexpr int MAX_COMMANDS = 32;
    ICommand* _commands[MAX_COMMANDS];
    int _count = 0;
};

// ============================================================
// 自注册宏
// 用法: REGISTER_COMMAND(MyCommandClass)
// ============================================================
#define REGISTER_COMMAND(cls) \
    static cls _cmd_instance_##cls; \
    static bool _cmd_registered_##cls = [](){ \
        CommandRegistry::instance().registerCommand(&_cmd_instance_##cls); \
        return true; }();

#endif // COMMAND_REGISTRY_H