#ifndef CAMERA_H
#define CAMERA_H

#include <esp_timer.h>
#include <esp_log.h>
#include <esp_camera.h>
#include <string>


class Camera {
public:
    Camera();
    virtual ~Camera();

    virtual void* Capture(const char* format);
    virtual int GetWidth();
    virtual int GetHeight();
    virtual int GetBufferSize();
    virtual const char* GetFormat();

protected:
    friend class CameraLockGuard;
    virtual bool Lock(int timeout_ms = 0) = 0;
    virtual void Unlock() = 0;

private:

};

class CameraLockGuard {
public:
    CameraLockGuard(Camera *camera) : camera_(camera) {
        if (!camera_->Lock(30000)) {
            ESP_LOGE("Camera", "Failed to lock camera");
        }
    }
    ~CameraLockGuard() {
        camera_->Unlock();
    }

private:
    Camera *camera_;
};

class NoCamera : public Camera {
private:
    virtual bool Lock(int timeout_ms = 0) override {
        return true;
    }
    virtual void Unlock() override {}
};


#endif