#ifndef _BOARD_CONFIG_H_
#define _BOARD_CONFIG_H_

#include <driver/gpio.h>

#define AUDIO_INPUT_SAMPLE_RATE  16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000

// 如果使用 Duplex I2S 模式，请注释下面一行
#define AUDIO_I2S_METHOD_SIMPLEX

#ifdef AUDIO_I2S_METHOD_SIMPLEX

#define AUDIO_I2S_MIC_GPIO_WS   GPIO_NUM_1
#define AUDIO_I2S_MIC_GPIO_SCK  GPIO_NUM_2
#define AUDIO_I2S_MIC_GPIO_DIN  GPIO_NUM_3
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_19
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_41
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_42

#else

#define AUDIO_I2S_GPIO_WS GPIO_NUM_44
#define AUDIO_I2S_GPIO_BCLK GPIO_NUM_55
#define AUDIO_I2S_GPIO_DIN  GPIO_NUM_6
#define AUDIO_I2S_GPIO_DOUT GPIO_NUM_7

#endif


#define BUILTIN_LED_GPIO        GPIO_NUM_48
#define BOOT_BUTTON_GPIO        GPIO_NUM_0
#define TOUCH_BUTTON_GPIO       GPIO_NUM_21
#define VOLUME_UP_BUTTON_GPIO   GPIO_NUM_40
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_39

#define DISPLAY_SDA_PIN GPIO_NUM_41
#define DISPLAY_SCL_PIN GPIO_NUM_42
#define DISPLAY_WIDTH   128

// Camera pin configuration for ESP32-CAM AI Thinker
#define CAMERA_MODULE_NAME "ESP-S3-EYE"
#define CAMERA_PIN_PWDN -1  // Not used
#define CAMERA_PIN_RESET -1 // Not used

// Camera interface pins
#define CAMERA_PIN_VSYNC GPIO_NUM_6   // Vertical sync
#define CAMERA_PIN_HREF GPIO_NUM_7    // Horizontal reference
#define CAMERA_PIN_PCLK GPIO_NUM_13   // Pixel clock
#define CAMERA_PIN_XCLK GPIO_NUM_15   // System clock

// I2C pins for camera control
#define CAMERA_PIN_SIOD GPIO_NUM_4    // I2C data
#define CAMERA_PIN_SIOC GPIO_NUM_5    // I2C clock

// Camera data bus pins
#define CAMERA_PIN_D0 GPIO_NUM_11     // Data bit 0
#define CAMERA_PIN_D1 GPIO_NUM_9      // Data bit 1
#define CAMERA_PIN_D2 GPIO_NUM_8      // Data bit 2
#define CAMERA_PIN_D3 GPIO_NUM_10     // Data bit 3
#define CAMERA_PIN_D4 GPIO_NUM_12     // Data bit 4
#define CAMERA_PIN_D5 GPIO_NUM_18     // Data bit 5
#define CAMERA_PIN_D6 GPIO_NUM_17     // Data bit 6
#define CAMERA_PIN_D7 GPIO_NUM_16     // Data bit 7

#define SENSOR_POWER_IO (GPIO_NUM_3)
#define PWM_IO          (47)      /* PWM output pin */
#define PWM_FREQ        (20000)   /* PWM frequency in Hz */
#define PWM_MIN_DUTY    (5)       /* Minimum PWM duty cycle */

#if CONFIG_OLED_SSD1306_128X32
#define DISPLAY_HEIGHT  32
#elif CONFIG_OLED_SSD1306_128X64
#define DISPLAY_HEIGHT  64
#elif CONFIG_OLED_SH1106_128X64
#define DISPLAY_HEIGHT  64
#define SH1106
#else
#error "未选择 OLED 屏幕类型"
#endif

#define DISPLAY_MIRROR_X true
#define DISPLAY_MIRROR_Y true

#endif // _BOARD_CONFIG_H_
