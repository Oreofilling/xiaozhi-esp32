#include <esp_log.h>
#include <esp_err.h>
#include <string>
#include <cstdlib>
#include <cstring>

#include "camera.h"
#include "board.h"

#define TAG "Camera"

Camera::Camera() {
    ESP_LOGI(TAG,"New Camera");

}

Camera::~Camera() {

}

void* Camera::Capture(const char* format) { 
    ESP_LOGI(TAG,"capture image");   
    return nullptr;
}

int Camera::GetWidth() {
    return 0;
}

int Camera::GetHeight() {
    return 0;
}

int Camera::GetBufferSize() {
    return 0;
}

const char* Camera::GetFormat() {
    return "Unknown";
}