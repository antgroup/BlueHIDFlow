// BlueHIDFlow 摄像头图片上传器 - 空实现
// 未配置上传逻辑时使用，不执行任何网络请求，仅返回空 URL
#ifndef NOOP_CAMERA_UPLOADER_H
#define NOOP_CAMERA_UPLOADER_H

#include "icamera_uploader.h"

// ============================================================
// 空实现：不执行上传，仅返回空 URL
// ============================================================
// 开发者应将此处替换为实际的图片上传实现（如接入自有 OSS/CDN 等）。
// 上传端点和鉴权凭据应从配置文件或运行环境读取，禁止硬编码密钥到源码。
class NoopCameraUploader : public ICameraUploader {
public:
    String uploadImage(const uint8_t* /*frameData*/, size_t /*frameSize*/) override {
        return "";
    }
};

#endif // NOOP_CAMERA_UPLOADER_H