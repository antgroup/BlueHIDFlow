// BlueHIDFlow 摄像头图片上传接口
// 将图片上传逻辑抽象为接口，交给开发者自行实现，开源不暴露私有实现
// 参考 imqtt_publisher.h 的解耦风格
#ifndef ICAMERA_UPLOADER_H
#define ICAMERA_UPLOADER_H

#include <Arduino.h>

// ============================================================
// 摄像头图片上传接口
// ============================================================
// 实现者需将图片数据上传到自有后端/对象存储，并返回可访问的图片 URL。
// 上传端点、鉴权凭据等由实现者自行管理，禁止硬编码密钥到源码。
class ICameraUploader {
public:
    virtual ~ICameraUploader() = default;

    // 上传 JPEG 图片数据，返回图片 URL；失败返回空字符串
    // frameData: 图片数据指针
    // frameSize: 图片数据长度（字节）
    virtual String uploadImage(const uint8_t* frameData, size_t frameSize) = 0;
};

#endif // ICAMERA_UPLOADER_H