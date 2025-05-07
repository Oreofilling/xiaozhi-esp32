#include "iot/thing.h"
 #include "board.h"
 #include "config.h"
 #include "camera_ne101.h"
 #include "audio_codec.h"
 
 #include <driver/gpio.h>
 #include <esp_log.h>
 
 #include "lwip/err.h"
 #include "lwip/sys.h"
 #include <cJSON.h>
 #include "application.h"
 
 #define TAG "Camera"
 
 namespace iot {
 
 // 这里仅定义 Camera 的属性和方法，不包含具体的实现
 class Camera : public Thing {
 private:
     bool status_;
     
     void InitializeGpio() {
         ESP_LOGI(TAG, "initialize camera gpio");
         gpio_config_t config = {
            .pin_bit_mask = BIT64(SENSOR_POWER_IO),
            .mode = 0 ? GPIO_MODE_INPUT : GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_ENABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
         gpio_config(&config);
         gpio_set_level(SENSOR_POWER_IO, 1);
     }
 
 public:
     Camera() : Thing("Camera", "这是摄像头，也是你的眼睛，可以看到这个真实世界") {
         InitializeGpio();
         // 定义设备的属性
         properties_.AddBooleanProperty("power", "摄像头是否打开", [this]() -> bool {
             ESP_LOGI(TAG, "check camera status");
             return status_;
         });
 
 
         // 定义设备可以被远程执行的指令
         methods_.AddMethod("TurnOn", "打开摄像头", ParameterList(), [this](const ParameterList& parameters) {
             ESP_LOGI(TAG, "turn on camera");
             gpio_set_level(SENSOR_POWER_IO, 1);
             status_ = true;
         });
 
         methods_.AddMethod("TurnOff", "关闭摄像头", ParameterList(), [this](const ParameterList& parameters) {
             ESP_LOGI(TAG, "turn off camera");
             gpio_set_level(SENSOR_POWER_IO, 0);
             status_ = false;
         });
 
         methods_.AddMethod("Capture", "拍照", ParameterList(), [this](const ParameterList& parameters) {
             ESP_LOGI(TAG, "capture image");
             auto& app = Application::GetInstance();
             app.TestCaptureImage();    
         });
     }
 
     ~Camera() {
     }
 };
 
 } // namespace iot
 
 DECLARE_THING(Camera);