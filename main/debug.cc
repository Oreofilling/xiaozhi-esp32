/* Console example

 This example code is in the Public Domain (or CC0 licensed, at your option.)

 Unless required by applicable law or agreed to in writing, this
 software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 CONDITIONS OF ANY KIND, either express or implied.
 */

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "linenoise/linenoise.h"
#include "esp_vfs_fat.h"
#include "debug.h"
#include "cJSON.h"
#include "application.h"
#include <esp_camera.h>

#define TAG "-->DEBUG"
/* Prompt to be printed before each line.
 * This can be customized, made dynamic, etc.
 */
#define PROMPT_STR "debug"

/**
 * Debug module state structure
 */
typedef struct mdDebug {
    bool isInit;    ///< Initialization flag
} mdDebug_t;


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


static mdDebug_t g_debug = {0};
/* Console command history can be stored to and loaded from a file.
 * The easiest way to do this is to use FATFS filesystem on top of
 * wear_levelling library.
 */
/**
 * Initialize console subsystem
 * Configures UART, VFS, and linenoise settings
 */
static void initialize_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
    /* Move the caret to the beginning of the next line on '\n' */
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
        .source_clk = UART_SCLK_REF_TICK,
#else
        .source_clk = UART_SCLK_XTAL,
#endif
    };
    /* Install UART driver for interrupt-driven reads and writes */
    const uart_port_t uart_num = UART_NUM_0; 
    ESP_ERROR_CHECK(uart_driver_install(uart_num,
                                        1024, 0, 0, NULL, 0));  // Increased buffer size for UTF-8
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(uart_num);

    /* Initialize the console */
    esp_console_config_t console_config = {
        .max_cmdline_length = 1024,  // Increased for UTF-8 support
        .max_cmdline_args = 8,
#if CONFIG_LOG_COLORS
        .hint_color = atoi(LOG_COLOR_CYAN)
#endif
    };
    ESP_ERROR_CHECK(esp_console_init(&console_config));

    /* Configure linenoise line completion library */
    /* Enable multiline editing. If not set, long commands will scroll within
     * single line.
     */
    linenoiseSetMultiLine(1);

    /* Tell linenoise where to get command completions and hints */
    linenoiseSetCompletionCallback(&esp_console_get_completion);
    linenoiseSetHintsCallback((linenoiseHintsCallback *) &esp_console_get_hint);

    /* Set command history size */
    linenoiseHistorySetMaxLen(100);

    /* Set command maximum length */
    linenoiseSetMaxLineLen(console_config.max_cmdline_length);

    /* Don't return empty lines */
    linenoiseAllowEmpty(false);
}

/**
 * Console task function
 * Handles command line input/output
 * @param argv Task arguments (unused)
 */
static void taskfunc(void *argv)
{
    const char *prompt = LOG_COLOR_I PROMPT_STR "> " LOG_RESET_COLOR;
    int probe_status = linenoiseProbe();
    if (probe_status) {
        linenoiseSetDumbMode(1);
#if CONFIG_LOG_COLORS
        prompt = PROMPT_STR "> ";
#endif
    }

    while (true) {
        char *line = linenoise(prompt);
        if (line == NULL) {

            continue;
        }

        if (strlen(line) > 0) {
            linenoiseHistoryAdd(line);
        }

        int ret;
        esp_err_t err = esp_console_run(line, &ret);
        if (err == ESP_ERR_NOT_FOUND) {
            printf("Unrecognized command\n");
        } else if (err == ESP_ERR_INVALID_ARG) {
            // command was empty
        } else if (err == ESP_OK && ret != ESP_OK) {
            printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
        } else if (err != ESP_OK) {
            printf("Internal error: %s\n", esp_err_to_name(err));
        }

        linenoiseFree(line);
    }

    ESP_LOGE(TAG, "Error or end-of-input, terminating console");
    esp_console_deinit();
}



static int send_json(cJSON* root)
{
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return -1;
    }

    char* json_string = cJSON_PrintUnformatted(root);
    if (json_string) {
        ESP_LOGW(TAG, "Constructed JSON: %s", json_string);

        auto& app = Application::GetInstance();
        app.TestSendText(json_string);

        free(json_string);
    }

    cJSON_Delete(root);
    return 0;
}


/*--------- ---------------------------------TEST------------------------------------------*/
static std::vector<uint8_t> opus_test_data = {
    0x00, 0x00, 0x00, 0x9d, 0x58, 0xe0, 0xe7, 0x03, 0xd1, 0x03, 0xbc, 0x5b, 0x10, 0x00, 0x00, 0x2a, 0xf7, 0x65, 0xc1, 0x5f, 0xb7, 0x34, 0x0d, 0xdf, 0xb2, 0x11, 0xf9, 0x87, 0x46, 0xd2, 0x74, 0x0d, 0xc9, 0xda, 0xa5, 0x5c, 0x5d, 0x8c, 0x1b, 0x25, 0x02, 0xed, 0x28, 0x32, 0x69, 0x47, 0x25, 0xbd, 0x70, 0xe8, 0xd6, 0xe1, 0x8b, 0xaa, 0x44, 0x27, 0x43, 0xd3, 0xb6, 0x4f, 0x76, 0x1d, 0xea, 0xae, 0x8f, 0xc1, 0xc1, 0x89, 0x31, 0xe0, 0xe6, 0x33, 0x10, 0xf2, 0xcd, 0xee, 0x80, 0x25, 0x49, 0x81, 0xfe, 0x88, 0x89, 0xdb, 0x29, 0xf2, 0x86, 0x84, 0xfe, 0xec, 0x19, 0x1a, 0x36, 0x56, 0xf9, 0x41, 0x6a, 0x40, 0x23, 0x41, 0x9a, 0x1a, 0xe6, 0x71, 0x09, 0x19, 0x45, 0xe8, 0xb6, 0x5f, 0x39, 0x14, 0x2e, 0x62, 0x75, 0xef, 0x67, 0x62, 0xd4, 0x7c, 0xbb, 0x5d, 0x8b, 0x0e, 0xc3, 0x49, 0x0f, 0xd8, 0xb9, 0x94, 0x70, 0x23, 0x48, 0x07, 0x0d, 0x05, 0x90, 0x36, 0x32, 0xd2, 0xc7, 0xdc, 0x4a, 0x40, 0x00, 0x49, 0x5e, 0x70, 0xc1, 0x65, 0xc3, 0x11, 0x1a, 0x3e, 0x27, 0x57, 0xbf, 0x1b, 0xb0, 0x3c, 0x66,
};

static int send_hello_request(int argc, char **argv)
{
    ESP_LOGW(TAG, "send hello request");
    auto& app = Application::GetInstance();
    app.TestOpenAudioChannel();
    return 0;
}

static int send_listen_start_request(int argc, char **argv)
{
    ESP_LOGW(TAG, "send listen start request");
    auto& app = Application::GetInstance();
    app.TestSendStartListening();
    return 0;
}

static int send_listen_stop_request(int argc, char **argv)
{
    ESP_LOGW(TAG, "send listen stop request");
    auto& app = Application::GetInstance();
    app.TestSendStopListening();
    return 0;
}

static int send_text_request(int argc, char **argv)
{
    ESP_LOGW(TAG, "send text request,argc=%d", argc);

    if (argc < 2) {
        ESP_LOGE(TAG, "No text provided");
        return -1;
    }   
    const char* text = argv[1];
    size_t text_len = strlen(text);
    ESP_LOGW(TAG, "text length: %d, text: %s", text_len, text);
    
    // 构建JSON 
    cJSON* root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return -1;
    }

    cJSON_AddStringToObject(root, "type", "listen");
    cJSON_AddStringToObject(root, "mode", "manual");
    cJSON_AddStringToObject(root, "state", "detect");
    cJSON_AddStringToObject(root, "text", text); 
    
    send_json(root);
    return 0;
}

static int send_audio_request(int argc, char **argv)
{
    ESP_LOGW(TAG, "send audio request");
    auto& app = Application::GetInstance();
    app.TestSendAudio(opus_test_data);
    return 0;
}

static int capture_image_request(int argc, char **argv)
{
    ESP_LOGW(TAG, "capture image request");
    auto& app = Application::GetInstance();
    auto  ret = app.TestCaptureImage();
    if (ret) {
        ESP_LOGW(TAG, "image captured");
    }
    return 0;
}
static int send_image_request(int argc, char **argv)
{
    ESP_LOGW(TAG, "send image request");
    //auto& app = Application::GetInstance();
    //app.TestSendText("image");
    return 0;
}

static int send_video_request(int argc, char **argv)
{
    ESP_LOGW(TAG, "send video request");
    //auto& app = Application::GetInstance();
    //app.TestSendText("video");
    return 0;
}

static int send_file_request(int argc, char **argv)
{
    ESP_LOGW(TAG, "send file request");
    return 0;
}

static int init_camera_request(int argc, char **argv)
{
    ESP_LOGW(TAG, "init camera request");
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

            .jpeg_quality = 12, //0-63, for OV series camera sensors, lower number means higher quality
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
        return 0;
    }
    ESP_LOGI(TAG, "Camera Init Success");
    return 0;
}
/* ---------------------------------TEST------------------------------------------*/


/**
 * Add commands to debug console
 * @param cmd Array of console commands to register
 * @param count Number of commands in array
 */
void debug_cmd_add(esp_console_cmd_t *cmd, uint32_t count)
{
    // return ;
    if (g_debug.isInit) {
        uint32_t i = 0;
        for (i = 0; i < count; i++) {
            esp_console_cmd_register(&cmd[i]);
        }
    }
}

/* debug cmd */
static esp_console_cmd_t g_cmd[] = {
    {"hello", "send hello request", NULL, send_hello_request, NULL},
    {"listen_start", "listen start", NULL, send_listen_start_request, NULL},
    {"listen_stop", "listen stop", NULL, send_listen_stop_request, NULL},
    {"send_text", "send text", NULL, send_text_request, NULL},
    {"send_audio", "send audio", NULL, send_audio_request, NULL},
    {"init_camera", "init camera", NULL, init_camera_request, NULL},
    {"capture_image", "capture image", NULL, capture_image_request, NULL},
    {"send_image", "send image", NULL, send_image_request, NULL},
    {"send_video", "send video", NULL, send_video_request, NULL},
    {"send_file", "send file", NULL, send_file_request, NULL},
};

/**
 * Initialize debug console
 * Sets up console and starts command processing task
 */
void debug_open(void)
{
    // return ;
    memset(&g_debug, 0, sizeof(g_debug));
    initialize_console();
    /* Register commands */
    esp_console_register_help_command();
    xTaskCreatePinnedToCore((TaskFunction_t)taskfunc, TAG, 3 * 1024, NULL, 4, NULL, 1);
    g_debug.isInit = true;

    /*Register cmd*/
    debug_cmd_add(g_cmd, sizeof(g_cmd) / sizeof(esp_console_cmd_t));
}

/**
 * Clean up debug console resources
 * Currently empty but reserved for future use
 */
void debug_close()
{

}


