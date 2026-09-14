// BlueHIDFlow 摄像头服务器实现
#include "camera_server.h"
#include <WiFi.h>
#include "mqtt_client.h"
#include <esp_heap_caps.h>
#include "camera_uploaders/noop_camera_uploader.h"

// 默认使用空上传器（不执行上传）。开发者可调用 cameraServer.setUploader() 注入自定义实现。
static NoopCameraUploader s_defaultUploader;

CameraServer cameraServer;

// JPEG 标记
static const uint8_t JPEG_SOI[] = {0xFF, 0xD8};
static const uint8_t JPEG_EOI[] = {0xFF, 0xD9};

CameraServer::CameraServer() :
    _frameBuffer(nullptr),
    _frameSize(0),
    _frameBufferCapacity(0),
    _cameraInitialized(false),
    _autoCaptureEnabled(false),
    _autoCaptureIntervalMs(3000),
    _lastCaptureTime(0),
    _uploader(&s_defaultUploader) {
}

bool CameraServer::begin() {
    return begin(configManager.camera());
}

bool CameraServer::begin(const CameraConfig& config) {
    Serial.println("[Camera] 初始化摄像头（仅 MQTT 发布，HTTP 服务已禁用）...");
    Serial.printf("[Camera] 当前帧缓冲区状态：_frameBuffer=%p\n", _frameBuffer);

    _config = config;

    // 分配帧缓冲区 (仅在未分配时)
    if (!_frameBuffer) {
        _frameBufferCapacity = MAX_JPEG_BUFFER_SIZE;
        Serial.printf("[Camera] 尝试分配 %d bytes 帧缓冲区...\n", _frameBufferCapacity);
        _frameBuffer = (uint8_t*)ps_malloc(_frameBufferCapacity);
        if (!_frameBuffer) {
            Serial.println("[Camera] 帧缓冲区分配失败 (PSRAM)，尝试堆内存...");
            // 尝试普通堆内存
            _frameBuffer = (uint8_t*)malloc(_frameBufferCapacity);
            if (!_frameBuffer) {
                Serial.println("[Camera] 帧缓冲区分配失败 (堆内存)");
                return false;
            }
            Serial.printf("[Camera] 帧缓冲区分配成功 (堆内存): %d bytes\n", _frameBufferCapacity);
        } else {
            Serial.printf("[Camera] 帧缓冲区分配成功 (PSRAM): %d bytes\n", _frameBufferCapacity);
        }
        Serial.printf("[Camera] 分配后帧缓冲区状态：_frameBuffer=%p\n", _frameBuffer);
    } else {
        Serial.println("[Camera] 帧缓冲区已存在，跳过分配");
    }

    // 强制重新初始化摄像头（解决帧捕获问题）
    if (_cameraInitialized) {
        Serial.println("[Camera] 摄像头已初始化，强制重新初始化...");
        esp_camera_deinit();
        _cameraInitialized = false;
        delay(100);  // 等待摄像头完全断电
    }

    // 初始化摄像头
    if (!initCamera()) {
        Serial.println("[Camera] 摄像头初始化失败（可能未连接硬件）");
        // 不返回 false，允许继续运行（MQTT 发布功能在拍照时会检查）
    } else {
        Serial.println("[Camera] 摄像头初始化成功");
    }

    // 设置坐标映射器
    _mapper.setScreenSize(configManager.phone().screenWidth, configManager.phone().screenHeight);

    // HTTP 服务器已禁用 - 仅使用 MQTT 发布
    // _server.on("/capture", HTTP_GET, std::bind(&CameraServer::handleCapture, this));
    // _server.on("/stream", HTTP_GET, std::bind(&CameraServer::handleStream, this));
    // _server.on("/status", HTTP_GET, std::bind(&CameraServer::handleStatus, this));
    // _server.begin(81);

    Serial.println("[Camera] 摄像头初始化完成（HTTP 服务已禁用）");
    return true;  // 即使摄像头硬件未连接也返回 true
}

void CameraServer::end() {
    Serial.println("[Camera] 停止摄像头服务器...");

    // 先停止 HTTP 服务器
    _server.stop();
    Serial.println("[Camera] HTTP 服务器已停止");

    if (_cameraInitialized) {
        esp_camera_deinit();
        _cameraInitialized = false;
        Serial.println("[Camera] 摄像头已释放");
    }

    // 保留帧缓冲区，避免重新分配
    // if (_frameBuffer) {
    //     free(_frameBuffer);
    //     _frameBuffer = nullptr;
    // }
}

bool CameraServer::initCamera() {
    Serial.println("[Camera] 初始化 OV2640...");

    // 摄像头配置
    memset(&_cameraConfig, 0, sizeof(_cameraConfig));

    // 引脚配置 (ESP32-S3)
    _cameraConfig.pin_pwdn = PWDN_GPIO_NUM;
    _cameraConfig.pin_reset = RESET_GPIO_NUM;
    _cameraConfig.pin_xclk = XCLK_GPIO_NUM;
    _cameraConfig.pin_sccb_sda = SIOD_GPIO_NUM;  // 使用新名称
    _cameraConfig.pin_sccb_scl = SIOC_GPIO_NUM;  // 使用新名称
    _cameraConfig.pin_d7 = Y9_GPIO_NUM;
    _cameraConfig.pin_d6 = Y8_GPIO_NUM;
    _cameraConfig.pin_d5 = Y7_GPIO_NUM;
    _cameraConfig.pin_d4 = Y6_GPIO_NUM;
    _cameraConfig.pin_d3 = Y5_GPIO_NUM;
    _cameraConfig.pin_d2 = Y4_GPIO_NUM;
    _cameraConfig.pin_d1 = Y3_GPIO_NUM;
    _cameraConfig.pin_d0 = Y2_GPIO_NUM;
    _cameraConfig.pin_vsync = VSYNC_GPIO_NUM;
    _cameraConfig.pin_href = HREF_GPIO_NUM;
    _cameraConfig.pin_pclk = PCLK_GPIO_NUM;

    // 基础配置
    _cameraConfig.ledc_timer = LEDC_TIMER_0;
    _cameraConfig.ledc_channel = LEDC_CHANNEL_0;
    _cameraConfig.pixel_format = PIXFORMAT_JPEG;
    _cameraConfig.frame_size = resolutionToFrameSize(_config.resolution);
    _cameraConfig.jpeg_quality = _config.jpegQuality;
    _cameraConfig.fb_count = 1;
    _cameraConfig.fb_location = CAMERA_FB_IN_PSRAM;
    _cameraConfig.grab_mode = CAMERA_GRAB_LATEST;

    // XCLK 频率
    // 注意：摄像头模块使用内部晶振，XCLK引脚未连接
    // 设置较低频率避免LEDC配置问题，实际不会使用
    _cameraConfig.xclk_freq_hz = 10000000;  // 10MHz

    // 初始化
    Serial.println("[Camera] 调用 esp_camera_init...");
    esp_err_t err = esp_camera_init(&_cameraConfig);
    if (err != ESP_OK) {
        Serial.printf("[Camera] 初始化失败: 0x%x\n", err);
        return false;
    }

    Serial.println("[Camera] esp_camera_init 成功，等待摄像头稳定...");
    delay(500);  // 等待摄像头稳定

    // 获取传感器并设置参数
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        Serial.printf("[Camera] 检测到传感器: PID=0x%02X\n", s->id.PID);
        // 设置亮度 (-2 to 2)
        s->set_brightness(s, 0);
        // 设置对比度 (-2 to 2)
        s->set_contrast(s, 0);
        // 设置饱和度 (-2 to 2)
        s->set_saturation(s, 0);
        // 自动曝光
        s->set_exposure_ctrl(s, 1);
        // 自动白平衡
        s->set_whitebal(s, 1);
        // 自动增益
        s->set_gain_ctrl(s, 1);
        // 水平翻转 (根据实际安装调整)
        s->set_hmirror(s, 0);
        // 垂直翻转 (根据实际安装调整)
        s->set_vflip(s, 0);
    }

    uint16_t w, h;
    _config.getResolution(w, h);
    Serial.printf("[Camera] OV2640 初始化成功: %dx%d, 质量=%d\n",
                  w, h, _config.jpegQuality);

    _cameraInitialized = true;
    return true;
}

void CameraServer::applyConfig() {
    if (_cameraInitialized) {
        esp_camera_deinit();
        _cameraInitialized = false;
    }

    initCamera();
}

framesize_t CameraServer::resolutionToFrameSize(uint8_t resolution) const {
    switch (resolution) {
        case 0: return FRAMESIZE_QVGA;   // 320x240
        case 1: return FRAMESIZE_VGA;    // 640x480
        case 2: return FRAMESIZE_SVGA;   // 800x600
        case 3: return FRAMESIZE_XGA;    // 1024x768
        case 4: return FRAMESIZE_SXGA;   // 1280x960
        default: return FRAMESIZE_SVGA;
    }
}

void CameraServer::setConfig(const CameraConfig& config) {
    _config = config;
    applyConfig();
}

void CameraServer::handleClient() {
    _server.handleClient();
}

void CameraServer::loop() {
    // 定时拍照上传逻辑
    if (_autoCaptureEnabled && _cameraInitialized && mqtt.isConnected()) {
        unsigned long now = millis();
        if (now - _lastCaptureTime >= _autoCaptureIntervalMs) {
            _lastCaptureTime = now;

            // 拍照并上传
            String url = captureAndUpload();
            if (url.length() > 0) {
                // MQTT上报
                mqtt.publishCameraUrl(url.c_str());
                Serial.printf("[Camera] 定时拍照上报成功\n");
            }
        }
    }
}

bool CameraServer::captureFrame() {
    if (!_cameraInitialized || !_frameBuffer) {
        Serial.println("[Camera] 摄像头未初始化或帧缓冲区未分配");
        return false;
    }

    // 获取摄像头传感器状态
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        Serial.printf("[Camera] 传感器 PID: 0x%02X\n", s->id.PID);
    } else {
        Serial.println("[Camera] 无法获取传感器状态");
    }

    // 获取帧 (带超时重试)
    camera_fb_t* fb = nullptr;
    int retries = 3;

    for (int i = 0; i < retries; i++) {
        Serial.printf("[Camera] 尝试获取帧 %d/%d...\n", i + 1, retries);
        fb = esp_camera_fb_get();
        if (fb) {
            Serial.printf("[Camera] 获取帧成功: %d bytes, 格式=%d\n", fb->len, fb->format);
            break;
        }

        Serial.printf("[Camera] 获取帧失败，等待 500ms 后重试\n");
        delay(500);  // 增加重试延迟
    }

    if (!fb) {
        Serial.println("[Camera] 获取帧失败 (所有重试)");
        // 尝试重新初始化摄像头
        Serial.println("[Camera] 尝试重新初始化摄像头...");
        if (initCamera()) {
            Serial.println("[Camera] 重新初始化成功");
        } else {
            Serial.println("[Camera] 重新初始化失败");
        }
        return false;
    }

    // 检查帧数据有效性
    if (!fb->buf || fb->len == 0) {
        Serial.println("[Camera] 帧数据无效");
        esp_camera_fb_return(fb);
        return false;
    }

    // 检查缓冲区大小
    if (fb->len > _frameBufferCapacity) {
        Serial.printf("[Camera] 帧过大: %d > %d\n", fb->len, _frameBufferCapacity);
        esp_camera_fb_return(fb);
        return false;
    }

    // 复制帧数据
    memcpy(_frameBuffer, fb->buf, fb->len);
    _frameSize = fb->len;

    // 释放帧缓冲
    esp_camera_fb_return(fb);

    Serial.printf("[Camera] 拍照成功: %d bytes\n", _frameSize);
    return true;
}

void CameraServer::getFrameSize(int* width, int* height) {
    uint16_t w, h;
    _config.getResolution(w, h);
    if (width) *width = w;
    if (height) *height = h;
}

void CameraServer::setResolution(int width, int height) {
    if (width == 320 && height == 240) {
        _config.resolution = 0;
    } else if (width == 640 && height == 480) {
        _config.resolution = 1;
    } else if (width == 800 && height == 600) {
        _config.resolution = 2;
    } else if (width == 1024 && height == 768) {
        _config.resolution = 3;
    } else if (width == 1280 && height == 960) {
        _config.resolution = 4;
    } else {
        _config.resolution = 2;  // 默认 SVGA
    }

    applyConfig();
}

// ============================================
// HTTP 处理
// ============================================
void CameraServer::handleCapture() {
    Serial.println("[Camera] 收到 capture 请求");

    if (!_cameraInitialized) {
        Serial.println("[Camera] 摄像头未初始化");
        _server.send(500, "text/plain", "Camera not initialized");
        return;
    }

    Serial.println("[Camera] 开始拍照...");
    unsigned long startTime = millis();

    if (!captureFrame()) {
        Serial.println("[Camera] 拍照失败");
        _server.send(500, "text/plain", "Capture failed");
        return;
    }

    Serial.printf("[Camera] 拍照耗时: %lu ms, 大小: %d bytes\n",
                  millis() - startTime, _frameSize);

    // 使用 WiFiClient 直接发送二进制数据
    WiFiClient client = _server.client();

    String header = "HTTP/1.1 200 OK\r\n";
    header += "Content-Type: image/jpeg\r\n";
    header += "Content-Length: " + String(_frameSize) + "\r\n";
    header += "Connection: close\r\n";
    header += "Access-Control-Allow-Origin: *\r\n\r\n";

    client.print(header);
    client.write(_frameBuffer, _frameSize);
    client.flush();
    client.stop();

    Serial.println("[Camera] capture 响应已发送");
}

void CameraServer::handleStream() {
    Serial.println("[Camera] 收到 stream 请求");

    if (!_cameraInitialized) {
        Serial.println("[Camera] 摄像头未初始化");
        _server.send(500, "text/plain", "Camera not initialized");
        return;
    }

    WiFiClient client = _server.client();

    // MJPEG 流响应头
    String header =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=frame\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: close\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n";

    client.print(header);
    client.flush();

    Serial.println("[Camera] MJPEG 流开始");

    int frameCount = 0;
    unsigned long streamStart = millis();
    const unsigned long STREAM_TIMEOUT = 60000;  // 60秒超时

    // 持续发送帧
    while (client.connected() && (millis() - streamStart < STREAM_TIMEOUT)) {
        if (captureFrame()) {
            // 发送帧边界
            client.print("--frame\r\n");
            client.print("Content-Type: image/jpeg\r\n");
            client.print("Content-Length: " + String(_frameSize) + "\r\n\r\n");

            // 发送 JPEG 数据
            size_t written = client.write(_frameBuffer, _frameSize);
            client.print("\r\n");
            client.flush();

            frameCount++;

            if (frameCount % 10 == 0) {
                Serial.printf("[Camera] 流帧数: %d, 大小: %d bytes\n", frameCount, _frameSize);
            }

            if (written != _frameSize) {
                Serial.println("[Camera] 写入不完整，断开连接");
                break;
            }
        } else {
            Serial.println("[Camera] 获取帧失败");
            delay(100);
        }

        delay(50);  // 约 20 FPS
    }

    client.stop();
    Serial.printf("[Camera] MJPEG 流结束，共 %d 帧，耗时 %lu 秒\n",
                  frameCount, (millis() - streamStart) / 1000);
}

void CameraServer::handleStatus() {
    uint16_t w, h;
    _config.getResolution(w, h);

    String json = "{";
    json += "\"model\":\"" + String(CAMERA_MODEL) + "\",";
    json += "\"width\":" + String(w) + ",";
    json += "\"height\":" + String(h) + ",";
    json += "\"quality\":" + String(_config.jpegQuality) + ",";
    json += "\"distanceCm\":" + String(_config.distanceCm) + ",";
    json += "\"initialized\":" + String(_cameraInitialized ? "true" : "false") + ",";
    json += "\"screenWidth\":" + String(_mapper.getScreenWidth()) + ",";
    json += "\"screenHeight\":" + String(_mapper.getScreenHeight());
    json += "}";

    _server.send(200, "application/json", json);
}

void CameraServer::sendJPEG(WiFiClient& client) {
    if (!_frameBuffer || _frameSize == 0) return;

    client.write(JPEG_SOI, 2);
    client.write(_frameBuffer, _frameSize);
    client.write(JPEG_EOI, 2);
}


// 拍照并上传（通过注入的上传器）
String CameraServer::captureAndUpload() {
    Serial.println("[Camera] ===== captureAndUpload 开始 =====");
    Serial.printf("[Camera] _cameraInitialized=%d, _frameBuffer=%p\n", _cameraInitialized, _frameBuffer);

    if (!_cameraInitialized) {
        Serial.println("[Camera] 摄像头未初始化，无法拍照");
        return "";
    }

    if (!_frameBuffer) {
        Serial.println("[Camera] 帧缓冲区未分配");
        return "";
    }

    // 拍照
    Serial.println("[Camera] 开始拍照...");
    if (!captureFrame()) {
        Serial.println("[Camera] 拍照失败");
        return "";
    }

    Serial.printf("[Camera] 拍照成功: %d bytes\n", _frameSize);

    // 通过注入的上传器上传图片，返回图片 URL
    if (!_uploader) {
        Serial.println("[Camera] 未配置图片上传器，跳过上传");
        return "";
    }

    Serial.println("[Camera] 开始上传图片...");
    String url = _uploader->uploadImage(_frameBuffer, _frameSize);

    if (url.length() == 0) {
        Serial.println("[Camera] 图片上传返回空URL");
    } else {
        Serial.printf("[Camera] 图片上传成功: %s\n", url.c_str());
    }

    return url;
}

// 开始定时拍照
void CameraServer::startAutoCapture(uint32_t intervalMs) {
    _autoCaptureEnabled = true;
    _autoCaptureIntervalMs = intervalMs;
    _lastCaptureTime = 0;  // 立即开始第一次拍照
    Serial.printf("[Camera] 开始定时拍照，间隔: %lu ms\n", intervalMs);
}

// 停止定时拍照
void CameraServer::stopAutoCapture() {
    _autoCaptureEnabled = false;
    Serial.println("[Camera] 停止定时拍照");
}