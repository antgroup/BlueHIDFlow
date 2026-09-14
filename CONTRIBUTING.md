# Contributing to BlueHIDFlow

感谢你考虑为 BlueHIDFlow 做出贡献！这是一个开源的移动端自动化测试工具，我们欢迎任何形式的贡献。

## 📋 目录

- [行为准则](#行为准则)
- [如何贡献](#如何贡献)
- [开发指南](#开发指南)
- [提交规范](#提交规范)
- [代码风格](#代码风格)
- [Pull Request 流程](#pull-request-流程)

## 行为准则

请参阅我们的行为准则，确保所有参与者都能在友好、包容的环境中进行协作。

## 如何贡献

### 报告 Bug

如果你发现了 Bug，请通过 GitHub Issues 提交报告。在提交之前：

1. 搜索现有的 Issues，确认该 Bug 未被报告
2. 使用 Bug 报告模板填写详细信息
3. 包含以下信息：
   - 固件版本
   - 硬件型号（ESP32-S3 开发板类型）
   - 复现步骤
   - 预期行为 vs 实际行为
   - 串口日志（如有）

### 提交功能请求

如果你有新功能的想法：

1. 先在 Issues 中讨论你的想法
2. 等待维护者确认后再开始开发
3. 使用功能请求模板描述功能需求和用例

### 提交代码

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'feat: add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 提交 Pull Request

## 开发指南

### 环境准备

```bash
# 安装 PlatformIO
pip install platformio

# 克隆仓库
git clone git@github.com:antgroup/BlueHIDFlow.git
cd BlueHIDFlow

# 安装依赖
pio pkg install
```

### 编译固件

```bash
# 编译
pio run

# 编译并上传
pio run --target upload

# 串口监视器
pio device monitor -b 115200
```

### 项目结构

```
BlueHIDFlow/
├── src/               # 源代码
├── include/           # 头文件
├── lib/               # 外部库
├── docs/              # 文档
├── platformio.ini     # PlatformIO 配置
└── config.example.h   # 设备配置模板（复制为 include/config.h 后修改）
```

### 模块说明

- `ble_hid.*` - BLE HID 核心功能（键盘/鼠标）
- `command_handler.*` - MQTT 命令解析与执行
- `config_manager.*` - 配置持久化（NVS）
- `connection_manager.*` - WiFi/MQTT 连接管理
- `mqtt_client.*` - MQTT 客户端封装
- `web_server.*` - AP 模式 Web 配置界面

## 提交规范

我们使用 [Conventional Commits](https://www.conventionalcommits.org/) 规范：

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Type 类型

- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式（不影响功能）
- `refactor`: 代码重构
- `perf`: 性能优化
- `test`: 测试相关
- `chore`: 构建/工具相关

### 示例

```
feat(ble): add multi-touch gesture support

- Add pinch-to-zoom gesture recognition
- Implement two-finger swipe detection
- Update gesture configuration in web UI

Closes #123
```

## 代码风格

### C++ 代码风格

- 使用 Clang-Format 格式化代码
- 遵循 Google C++ 风格指南
- 函数和变量使用 `snake_case` 命名
- 常量使用 `UPPER_SNAKE_CASE` 命名
- 类和结构体使用 `PascalCase` 命名

### 注释规范

```cpp
// ============================================
// 区块标题
// ============================================

// 单行注释说明

/**
 * @brief 函数简要说明
 * @param param1 参数1说明
 * @return 返回值说明
 */
```

### 文件头注释

```cpp
// BlueHIDFlow 模块名称
// 功能简述
// 
// Copyright 2024-2026 BlueHIDFlow Contributors
// SPDX-License-Identifier: Apache-2.0
```

## Pull Request 流程

### 提交前检查

- [ ] 代码已通过编译
- [ ] 代码风格符合规范
- [ ] 已添加必要的注释
- [ ] 已更新相关文档
- [ ] 已测试基本功能
- [ ] Commit 信息符合规范

### PR 标题格式

```
<type>(<scope>): <description>
```

示例：
- `feat(ble): add multi-touch gesture support`
- `fix(mqtt): fix reconnection timeout issue`
- `docs(api): update protocol documentation`

### Review 流程

1. 提交 PR 后，维护者会进行代码审查
2. 根据审查意见进行修改
3. 通过审查后，维护者会合并代码

## 🙏 感谢你的贡献！

BlueHIDFlow 的发展离不开每一位贡献者的支持。无论你是修复 Bug、添加功能，还是改进文档，你的贡献都会让这个项目变得更好。