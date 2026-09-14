// BlueHIDFlow 模块生命周期接口
// 统一模块的初始化/循环/销毁流程
#ifndef MODULE_H
#define MODULE_H

#include <Arduino.h>

// ============================================================
// 模块接口
// ============================================================
class IModule {
public:
    virtual ~IModule() = default;

    // 模块名称（调试用）
    virtual const char* name() const = 0;

    // 初始化（替代 begin()）
    virtual void setup() = 0;

    // 主循环（在 loop() 中调用）
    virtual void loop() = 0;

    // 清理（关机/重启前调用）
    virtual void end() = 0;

    // 优先级：越小越先初始化，loop 也按此顺序调用
    virtual int priority() const = 0;
};

// ============================================================
// 模块管理器（单例）
// ============================================================
class ModuleManager {
public:
    static ModuleManager& instance();

    // 注册模块（应在 setup() 前完成）
    void registerModule(IModule* mod);

    // 按 priority 排序后依次 setup()
    void setupAll();

    // 依次 loop()
    void loopAll();

    // 反序 end()
    void endAll();

    // 调试：打印所有模块
    void printModules() const;

    int count() const { return _count; }

private:
    ModuleManager() = default;
    void sortModules();

    static constexpr int MAX_MODULES = 16;
    IModule* _modules[MAX_MODULES];
    int _count = 0;
    bool _sorted = false;
};

#endif // MODULE_H