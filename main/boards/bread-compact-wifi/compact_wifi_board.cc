#include "wifi_board.h"
#include "audio_codecs/no_audio_codec.h"
#include "display/oled_display.h"
#include "system_reset.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "iot/thing_manager.h"
#include "led/single_led.h"
#include "camera/camera_ne101.h"
#include "assets/lang_config.h"

#include <wifi_station.h>
#include <esp_log.h>
#include <driver/i2c_master.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include "driver/uart.h"
#include <esp_camera.h>
#include <driver/gpio.h>


#ifdef SH1106
#include <esp_lcd_panel_sh1106.h>
#endif

#define TAG "CompactWifiBoard"

LV_FONT_DECLARE(font_puhui_14_1);
LV_FONT_DECLARE(font_awesome_14_1);

class CompactWifiBoard : public WifiBoard {
private:
    i2c_master_bus_handle_t display_i2c_bus_;
    esp_lcd_panel_io_handle_t panel_io_ = nullptr;
    esp_lcd_panel_handle_t panel_ = nullptr;
    Display* display_ = nullptr;
    Button boot_button_;
    Button touch_button_;
    Button volume_up_button_;
    Button volume_down_button_;

    void InitializeDisplayI2c() {
        i2c_master_bus_config_t bus_config = {
            .i2c_port = (i2c_port_t)0,
            .sda_io_num = DISPLAY_SDA_PIN,
            .scl_io_num = DISPLAY_SCL_PIN,
            .clk_source = I2C_CLK_SRC_DEFAULT,
            .glitch_ignore_cnt = 7,
            .intr_priority = 0,
            .trans_queue_depth = 0,
            .flags = {
                .enable_internal_pullup = 1,
            },
        };
        ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &display_i2c_bus_));
    }

    void InitializeSsd1306Display() {
        // SSD1306 config
        esp_lcd_panel_io_i2c_config_t io_config = {
            .dev_addr = 0x3C,
            .on_color_trans_done = nullptr,
            .user_ctx = nullptr,
            .control_phase_bytes = 1,
            .dc_bit_offset = 6,
            .lcd_cmd_bits = 8,
            .lcd_param_bits = 8,
            .flags = {
                .dc_low_on_data = 0,
                .disable_control_phase = 0,
            },
            .scl_speed_hz = 400 * 1000,
        };

        ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c_v2(display_i2c_bus_, &io_config, &panel_io_));

        ESP_LOGI(TAG, "Install SSD1306 driver");
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = -1;
        panel_config.bits_per_pixel = 1;

        esp_lcd_panel_ssd1306_config_t ssd1306_config = {
            .height = static_cast<uint8_t>(DISPLAY_HEIGHT),
        };
        panel_config.vendor_config = &ssd1306_config;

#ifdef SH1106
        ESP_ERROR_CHECK(esp_lcd_new_panel_sh1106(panel_io_, &panel_config, &panel_));
#else
        ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(panel_io_, &panel_config, &panel_));
#endif
        ESP_LOGI(TAG, "SSD1306 driver installed");

        // Reset the display
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_));
        if (esp_lcd_panel_init(panel_) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize display");
            display_ = new NoDisplay();
            return;
        }

        // Set the display to on
        ESP_LOGI(TAG, "Turning display on");
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_, true));

        display_ = new OledDisplay(panel_io_, panel_, DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y,
            {&font_puhui_14_1, &font_awesome_14_1});
    }

    void InitializeButtons() {
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting && !WifiStation::GetInstance().IsConnected()) {
                ResetWifiConfiguration();
            }
            app.ToggleChatState();
        });
        touch_button_.OnClick([this]() {
            ESP_LOGW(TAG, "[ZB]Touch button clicked");
            //listenging mode
     
        });
        touch_button_.OnLongPress([this]() {
            ESP_LOGW(TAG, "[ZB]Touch button long pressed");
           
        });

        volume_up_button_.OnClick([this]() {
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() + 10;
            if (volume > 100) {
                volume = 100;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_up_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(100);
            GetDisplay()->ShowNotification(Lang::Strings::MAX_VOLUME);
        });

        volume_down_button_.OnClick([this]() {
            auto codec = GetAudioCodec();
            auto volume = codec->output_volume() - 10;
            if (volume < 0) {
                volume = 0;
            }
            codec->SetOutputVolume(volume);
            GetDisplay()->ShowNotification(Lang::Strings::VOLUME + std::to_string(volume));
        });

        volume_down_button_.OnLongPress([this]() {
            GetAudioCodec()->SetOutputVolume(0);
            GetDisplay()->ShowNotification(Lang::Strings::MUTED);
        });
    }

    // 摄像头初始化
    void InitializeCamera() {
         camera_config_t camera_config = {
             .pin_pwdn = CAMERA_PIN_PWDN,
             .pin_reset = CAMERA_PIN_RESET,
             .pin_xclk = CAMERA_PIN_XCLK,
             .pin_sccb_sda = CAMERA_PIN_SIOD,
             .pin_sccb_scl = CAMERA_PIN_SIOC,
 
             .pin_d7 = CAMERA_PIN_D7,
             .pin_d6 = CAMERA_PIN_D6,
             .pin_d5 = CAMERA_PIN_D5,
             .pin_d4 = CAMERA_PIN_D4,
             .pin_d3 = CAMERA_PIN_D3,
             .pin_d2 = CAMERA_PIN_D2,
             .pin_d1 = CAMERA_PIN_D1,
             .pin_d0 = CAMERA_PIN_D0,
             .pin_vsync = CAMERA_PIN_VSYNC,
             .pin_href = CAMERA_PIN_HREF,
             .pin_pclk = CAMERA_PIN_PCLK,
 
             //XCLK 20MHz or 10MHz for OV2640 double FPS (Experimental)
             .xclk_freq_hz = 5000000,
             .ledc_timer = LEDC_TIMER_0,
             .ledc_channel = LEDC_CHANNEL_0,
 
             .pixel_format = PIXFORMAT_JPEG, //YUV422,GRAYSCALE,RGB565,JPEG
             .frame_size = FRAMESIZE_FHD,    //QQVGA-UXGA, For ESP32, do not use sizes above QVGA when not JPEG. The performance of the ESP32-S series has improved a lot, but JPEG mode always gives better frame rates.
 
             .jpeg_quality = 36, //0-63, for OV series camera sensors, lower number means higher quality
             .fb_count = 2,       //When jpeg mode is used, if fb_count more than one, the driver will work in continuous mode.
             .fb_location = CAMERA_FB_IN_PSRAM,
             .grab_mode = CAMERA_GRAB_LATEST,
         };

         //print all camera config 
         ESP_LOGI(TAG, "--- Camera Configuration ---");

         // Pin Definitions
         ESP_LOGI(TAG, "pin_pwdn:    %d", camera_config.pin_pwdn);
         ESP_LOGI(TAG, "pin_reset:   %d", camera_config.pin_reset);
         ESP_LOGI(TAG, "pin_xclk:    %d", camera_config.pin_xclk);
         ESP_LOGI(TAG, "pin_sccb_sda:%d", camera_config.pin_sccb_sda);
         ESP_LOGI(TAG, "pin_sccb_scl:%d", camera_config.pin_sccb_scl);
         ESP_LOGI(TAG, "pin_d7:      %d", camera_config.pin_d7);
         ESP_LOGI(TAG, "pin_d6:      %d", camera_config.pin_d6);
         ESP_LOGI(TAG, "pin_d5:      %d", camera_config.pin_d5);
         ESP_LOGI(TAG, "pin_d4:      %d", camera_config.pin_d4);
         ESP_LOGI(TAG, "pin_d3:      %d", camera_config.pin_d3);
         ESP_LOGI(TAG, "pin_d2:      %d", camera_config.pin_d2);
         ESP_LOGI(TAG, "pin_d1:      %d", camera_config.pin_d1);
         ESP_LOGI(TAG, "pin_d0:      %d", camera_config.pin_d0);
         ESP_LOGI(TAG, "pin_vsync:   %d", camera_config.pin_vsync);
         ESP_LOGI(TAG, "pin_href:    %d", camera_config.pin_href);
         ESP_LOGI(TAG, "pin_pclk:    %d", camera_config.pin_pclk);

         // Clock Settings
         ESP_LOGI(TAG, "xclk_freq_hz:%d", camera_config.xclk_freq_hz);
         ESP_LOGI(TAG, "ledc_timer:  %d", camera_config.ledc_timer);
         ESP_LOGI(TAG, "ledc_channel:%d", camera_config.ledc_channel);

         // Image Settings
         // Note: These print integer values. Refer to esp_camera.h for enum definitions.
         ESP_LOGI(TAG, "pixel_format:%d", camera_config.pixel_format);
         ESP_LOGI(TAG, "frame_size:  %d", camera_config.frame_size);
         ESP_LOGI(TAG, "jpeg_quality:%d", camera_config.jpeg_quality);
         ESP_LOGI(TAG, "fb_count:    %d", camera_config.fb_count);
         ESP_LOGI(TAG, "fb_location: %d", camera_config.fb_location);
         ESP_LOGI(TAG, "grab_mode:   %d", camera_config.grab_mode);


         ESP_LOGI(TAG, "--- End Configuration ---");
         //initialize the camera
         esp_err_t err = esp_camera_init(&camera_config);
         if (err != ESP_OK)
         {
             ESP_LOGE(TAG, "Camera Init Failed");
             return;
         }
         ESP_LOGI(TAG, "Camera Init Success");

         //todo:get camera sensor and apply image settings

    }

    // 物联网初始化，添加对 AI 可见设备
    void InitializeIot() {
       auto& thing_manager = iot::ThingManager::GetInstance();
        thing_manager.AddThing(iot::CreateThing("Speaker"));
        thing_manager.AddThing(iot::CreateThing("Lamp"));
        thing_manager.AddThing(iot::CreateThing("Camera"));
    }

public:
    CompactWifiBoard()  :
        boot_button_(BOOT_BUTTON_GPIO),
        touch_button_(TOUCH_BUTTON_GPIO),
        volume_up_button_(VOLUME_UP_BUTTON_GPIO),
        volume_down_button_(VOLUME_DOWN_BUTTON_GPIO) {
        InitializeDisplayI2c();
        InitializeSsd1306Display();
        InitializeButtons();
        InitializeIot();
        InitializeCamera();
    }

    virtual Led* GetLed() override {
        static SingleLed led(BUILTIN_LED_GPIO);
        return &led;
    }

     virtual Camera* GetCamera() override {
         static CameraNe101 camera;
 
         return &camera;
     }

    virtual AudioCodec* GetAudioCodec() override {
#ifdef AUDIO_I2S_METHOD_SIMPLEX
        static NoAudioCodecSimplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK, AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_SCK, AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
#else
        static NoAudioCodecDuplex audio_codec(AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_GPIO_BCLK, AUDIO_I2S_GPIO_WS, AUDIO_I2S_GPIO_DOUT, AUDIO_I2S_GPIO_DIN);
#endif
       return &audio_codec;
    }

    virtual Display* GetDisplay() override {
        return display_;
    }
};

DECLARE_BOARD(CompactWifiBoard);
