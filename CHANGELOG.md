# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- MQTT TLS CA 证书可配置（`MqttConfig.caCert`），支持 Web 配置页面和 config.h 两种方式
- Web 配置页面新增 TLS CA 证书输入框
- `ICameraUploader` 接口，开发者可自定义图片上传实现

### Changed
- MQTT CA 证书不再内置厂商根证书，由开发者自行配置
- Web/API 不再返回明文 WiFi 密码和 AI API Key（返回掩码 `******`）
- Web 配置页面不再回显密码类字段（留空表示保留原值）
- 删除冗余的 `emqxsl-ca.crt` 文件

### Security
- 防止 `/api/config` GET 接口泄露 WiFi 密码和 AI API Key
- `WifiConfigProvider` / `AIConfigProvider` 的 `toJson` 对敏感字段返回掩码
- `fromJson` 中空密码/空 API Key 视为未修改，避免误覆盖
- 移除 `camera_server.cpp` 中的 `setInsecure()` 调用
- 移除硬编码 OSS Access Token

### Removed
- `emqxsl-ca.crt` 冗余证书文件
- `EMQX_CA_CERT` 内嵌证书常量
- `uploadFrameToBackend` 方法及其硬编码 OSS 凭据

## [0.9.0] - 2026-05-15

### Added
- Configurable MQTT topic prefix (default: `bluehidflow/`)
- Protocol versioning support for commands (v1.0)
- Support for new command format with `version`, `id`, `timestamp`, `metadata` fields
- Apache 2.0 License
- SECURITY.md security policy
- GitHub Issue/PR templates
- `config.example.h` as the configuration template (config.h now gitignored)
- `KEY_COMBO` command for key combinations
- `VOLUME_UP` / `VOLUME_DOWN` media key commands
- `BACK` key command
- `SWITCH_MODE` command for runtime HID mode switching
- `RECONNECT` command for forced WiFi/MQTT reconnect
- MQTT log topic (`{prefix}log/{device_id}`) for device log reporting

### Changed
- Project renamed from "AntSpark" to "BlueHIDFlow"
- Default device name changed to `BlueHIDFlow-HID`
- Default AP SSID changed to `BlueHIDFlow-config`, password changed to `bluehidflow`
- MQTT topic prefix now configurable via Web UI
- API documentation updated with new protocol format
- All hardcoded MQTT broker addresses and credentials removed from source code
- All internal/proprietary documentation removed from repository

### Removed
- `NFC_WRITE` command (Alipay-specific, use `NLaunch` for general NFC writing)
- `writeAlipayURL()` method from NFCCardWriter (use `writeLaunchCard()` instead)
- `ALIPAY_NFC_URL` and `ALIPAY_PACKAGE_NAME` config macros
- NFC (PN532) module support (NFC_READ, NFC_SCAN, NFC_STOP, NLaunch commands)
- 舵机 (SG90) 模块支持 (MOVE_SERVO, GET_SERVO commands)
- `pn532.cpp` / `pn532.h` NFC 读写模块源码
- `servo_control.cpp` / `servo_control.h` 舵机控制模块源码
- Hardcoded MQTT broker address, credentials, and backend URL
- `BACKEND_URL` macro (unused)
- Internal working documents (WORK_STATUS.md, MQTT_FIX_REPORT, work logs)
- `config.h` from version control (contains personal credentials)

### Security
- Removed hardcoded API keys and credentials from source code
- config.h added to .gitignore to prevent credential leaks
- MQTT broker address and credentials now require user configuration
- AP mode password changed from `88888888` to `bluehidflow`

### Backward Compatibility
- Old command format (using `message_id`) still supported
- Both `id` (new) and `message_id` (old) fields accepted

## [0.8.0] - 2026-04-21

### Added
- Unified BLE HID interface with Pimpl pattern
- Keyboard and mouse mode separation (requires restart to switch)
- NFC Launch card support for app deep linking
- Servo control for NFC/phone positioning
- Web configuration portal (AP mode with captive portal)

### Changed
- Improved HID mode switching mechanism
- Enhanced Android compatibility for mouse mode
- Better error handling and logging

### Fixed
- BLE advertising restart issues
- HID coordinate system calibration
- MQTT reconnection stability

## [0.7.0] - 2026-03-31

### Added
- Absolute coordinate system for mouse mode
- Configurable HID sensitivity and move delay
- Direction swipe commands (SWIPE_UP, SWIPE_DOWN, SWIPE_LEFT, SWIPE_RIGHT)
- NFC read/write/scan commands
- Servo control commands

### Changed
- Refactored coordinate handling for better accuracy
- Improved gesture recognition

### Fixed
- Mouse acceleration compensation for Android
- Memory leaks in BLE connection handling

## [0.6.0] - 2026-03-17

### Added
- Initial BLE HID mouse support
- Basic MQTT command handling
- Configuration persistence using Preferences
- WiFi connection management with auto-reconnect
- NFC PN532 module support

### Changed
- Migrated from Arduino IDE to PlatformIO
- Improved build configuration for ESP32-S3

## [0.5.0] - 2026-02-01

### Added
- Project initialization
- Basic ESP32-S3 Arduino framework setup
- WiFi and MQTT connectivity
- BLE HID keyboard prototype

---

## Version Naming Convention

- **MAJOR**: Incompatible API changes
- **MINOR**: New features, backward compatible
- **PATCH**: Bug fixes, backward compatible