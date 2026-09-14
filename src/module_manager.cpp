// BlueHIDFlow 模块管理器实现
#include "module.h"

ModuleManager& ModuleManager::instance() {
    static ModuleManager mgr;
    return mgr;
}

void ModuleManager::registerModule(IModule* mod) {
    if (_count >= MAX_MODULES) {
        Serial.printf("[Module] 模块注册表已满，无法注册: %s\n", mod->name());
        return;
    }
    _modules[_count++] = mod;
    _sorted = false;
    Serial.printf("[Module] 注册模块: %s (优先级=%d)\n", mod->name(), mod->priority());
}

void ModuleManager::sortModules() {
    // 简单插入排序（模块数量很少）
    for (int i = 1; i < _count; i++) {
        IModule* key = _modules[i];
        int j = i - 1;
        while (j >= 0 && _modules[j]->priority() > key->priority()) {
            _modules[j + 1] = _modules[j];
            j--;
        }
        _modules[j + 1] = key;
    }
    _sorted = true;
}

void ModuleManager::setupAll() {
    if (!_sorted) sortModules();

    Serial.println("[Module] ===== 开始初始化所有模块 =====");
    for (int i = 0; i < _count; i++) {
        Serial.printf("[Module] 初始化: %s (优先级=%d)\n", _modules[i]->name(), _modules[i]->priority());
        _modules[i]->setup();
    }
    Serial.println("[Module] ===== 所有模块初始化完成 =====");
}

void ModuleManager::loopAll() {
    for (int i = 0; i < _count; i++) {
        _modules[i]->loop();
    }
}

void ModuleManager::endAll() {
    // 反序销毁
    Serial.println("[Module] ===== 开始销毁所有模块 =====");
    for (int i = _count - 1; i >= 0; i--) {
        Serial.printf("[Module] 销毁: %s\n", _modules[i]->name());
        _modules[i]->end();
    }
    Serial.println("[Module] ===== 所有模块已销毁 =====");
}

void ModuleManager::printModules() const {
    Serial.printf("[Module] 已注册 %d 个模块:\n", _count);
    for (int i = 0; i < _count; i++) {
        Serial.printf("  [%d] %s (优先级=%d)\n", i, _modules[i]->name(), _modules[i]->priority());
    }
}