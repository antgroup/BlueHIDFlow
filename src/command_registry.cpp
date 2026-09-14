// BlueHIDFlow 命令注册表实现
#include "command_registry.h"

CommandRegistry& CommandRegistry::instance() {
    static CommandRegistry registry;
    return registry;
}

void CommandRegistry::registerCommand(ICommand* cmd) {
    if (_count >= MAX_COMMANDS) {
        Serial.printf("CMD: 命令注册表已满，无法注册: %s\n", cmd->name());
        return;
    }
    _commands[_count++] = cmd;
    Serial.printf("CMD: 注册命令: %s\n", cmd->name());
}

ICommand* CommandRegistry::find(const char* name) {
    for (int i = 0; i < _count; i++) {
        if (strcmp(_commands[i]->name(), name) == 0) {
            return _commands[i];
        }
    }
    return nullptr;
}

void CommandRegistry::forEach(void (*callback)(ICommand*)) {
    for (int i = 0; i < _count; i++) {
        callback(_commands[i]);
    }
}