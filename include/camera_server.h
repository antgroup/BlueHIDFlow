// BlueHIDFlow 摄像头服务器
// 支持 OV2640 摄像头，集成配置管理器
#ifndef CAMERA_SERVER_H
#define CAMERA_SERVER_H

#include <Arduino.h>
#include <WebServer.h>
#include <esp_camera.h>
#include "config_manager.h"
#include "gpio_config.h"
#include "icamera_uploader.h"

// ============================================================
// 摄像头配置常量
// ============================================================
#define CAMERA_MODEL              "OV2640"
#define MAX_JPEG_BUFFER_SIZE      (1024 * 150)  // 150KB，适应更高分辨率

// OV2640 引脚配置 (ESP32-S3)
// 引脚定义统一在 gpio_config.h 中，此处引用以保持与 esp-camera 库兼容
#define PWDN_GPIO_NUM     CAM_PWDN
#define RESET_GPIO_NUM    CAM_RESET
#define XCLK_GPIO_NUM     CAM_XCLK
#define SIOD_GPIO_NUM     CAM_SIOD
#define SIOC_GPIO_NUM     CAM_SIOC
#define Y9_GPIO_NUM       CAM_Y9
#define Y8_GPIO_NUM       CAM_Y8
#define Y7_GPIO_NUM       CAM_Y7
#define Y6_GPIO_NUM       CAM_Y6
#define Y5_GPIO_NUM       CAM_Y5
#define Y4_GPIO_NUM       CAM_Y4
#define Y3_GPIO_NUM       CAM_Y3
#define Y2_GPIO_NUM       CAM_Y2
#define VSYNC_GPIO_NUM    CAM_VSYNC
#define HREF_GPIO_NUM     CAM_HREF
#define PCLK_GPIO_NUM     CAM_PCLK

// ============================================================
// 坐标映射器
// ============================================================
class CoordinateMapper {
public:
    CoordinateMapper() : _screenWidth(1080), _screenHeight(2400) {}

    void setScreenSize(uint16_t width, uint16_t height) {
        _screenWidth = width;
        _screenHeight = height;
    }

    // 屏幕坐标 → HID 坐标 (0-32767)
    uint16_t screenToHIDX(uint16_t screenX) const {
        return (screenX * 32767) / _screenWidth;
    }

    uint16_t screenToHIDY(uint16_t screenY) const {
        return (screenY * 32767) / _screenHeight;
    }

    // HID 坐标 → 屏幕坐标
    uint16_t hidToScreenX(uint16_t hidX) const {
        return (hidX * _screenWidth) / 32767;
    }

    uint16_t hidToScreenY(uint16_t hidY) const {
        return (hidY * _screenHeight) / 32767;
    }

    uint16_t getScreenWidth() const { return _screenWidth; }
    uint16_t getScreenHeight() const { return _screenHeight; }

private:
    uint16_t _screenWidth;
    uint16_t _screenHeight;
};

// ============================================================
// 摄像头服务器类
// ============================================================
class CameraServer {
public:
    CameraServer();

    // 初始化
    bool begin();
    bool begin(const CameraConfig& config);
    void end();

    // 主循环
    void handleClient();
    void loop();

    // 拍照
    bool captureFrame();
    uint8_t* getFrameBuffer() { return _frameBuffer; }
    size_t getFrameSize() { return _frameSize; }

    // 拍照并上传（通过注入的上传器），返回图片URL
    String captureAndUpload();

    // 设置图片上传器（开发者自行实现上传逻辑，避免暴露私有实现）
    void setUploader(ICameraUploader* uploader) { _uploader = uploader; }

    // 定时拍照控制
    void startAutoCapture(uint32_t intervalMs = 3000);  // 开始定时拍照
    void stopAutoCapture();                               // 停止定时拍照
    bool isAutoCaptureEnabled() const { return _autoCaptureEnabled; }
    uint32_t getAutoCaptureInterval() const { return _autoCaptureIntervalMs; }

    // 配置
    void setConfig(const CameraConfig& config);
    CameraConfig getConfig() const { return _config; }

    // 分辨率
    void getFrameSize(int* width, int* height);
    void setResolution(int width, int height);

    // 状态
    bool isInitialized() const { return _frameBuffer != nullptr; }  // 返回帧缓冲区是否已分配
    bool isCameraReady() const { return _cameraInitialized; }  // 摄像头硬件是否已初始化

    // 坐标映射器
    CoordinateMapper& mapper() { return _mapper; }

private:
    WebServer _server;
    camera_config_t _cameraConfig;
    CameraConfig _config;

    uint8_t* _frameBuffer;
    size_t _frameSize;
    size_t _frameBufferCapacity;

    bool _cameraInitialized;

    // 定时拍照相关
    bool _autoCaptureEnabled;
    uint32_t _autoCaptureIntervalMs;
    unsigned long _lastCaptureTime;

    CoordinateMapper _mapper;

    // 图片上传器（开发者注入）
    ICameraUploader* _uploader;

    // 内部方法
    bool initCamera();
    void applyConfig();
    framesize_t resolutionToFrameSize(uint8_t resolution) const;

    // HTTP 处理
    void handleCapture();
    void handleStream();
    void handleStatus();
    void sendJPEG(WiFiClient& client);
};

// 全局实例
extern CameraServer cameraServer;

#endif // CAMERA_SERVER_H