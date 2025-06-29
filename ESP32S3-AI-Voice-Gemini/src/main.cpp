/* Using LVGL with Arduino requires some extra steps:
   Be sure to read the docs here: https://docs.lvgl.io/master/details/integration/framework/arduino.html
*/

#include "lv_conf.h"
#include <lvgl.h>
#include <Arduino.h>
#include <string.h>
#include "pincfg.h"
#include "dispcfg.h"
#include "AXS15231B_touch.h"
#include <Arduino_GFX_Library.h>
#include <base64.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "storage_manager.h"
#include "speech_to_text.h"
#include "text_to_speech.h"
#include "gemini_client.h"
#include "ui_manager.h"
#include "esp_log.h"
#include "wifi_manager.h"

#include "ui.h"

static const char *TAG = "MAIN";

Arduino_DataBus *bus = new Arduino_ESP32QSPI(TFT_CS, TFT_SCK, TFT_SDA0, TFT_SDA1, TFT_SDA2, TFT_SDA3);
Arduino_GFX *g = new Arduino_AXS15231B(bus, GFX_NOT_DEFINED, 0, false, TFT_res_W, TFT_res_H);
Arduino_Canvas *gfx = new Arduino_Canvas(TFT_res_W, TFT_res_H, g, 0, 0, TFT_rot);
AXS15231B_Touch touch(Touch_SCL, Touch_SDA, Touch_INT, Touch_ADDR, TFT_rot);

// Button state for voice recording
static bool is_recording = false;
static uint32_t last_button_press = 0;
static const uint32_t BUTTON_DEBOUNCE_MS = 200;

// ✅ Improved printf override with better filtering
static int vprintf_to_ui(const char *fmt, va_list args) {
    char buf[256];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    if (len > 0 && strlen(buf) > 0) {
        // Filter out audio debug spam and other noise
        String msg = String(buf);
        if (msg.indexOf("I2S") == -1 && 
            msg.indexOf("DMA") == -1 && 
            msg.indexOf("AUDIO") == -1 &&
            msg.indexOf("HTTP") == -1) {
            chat_screen_append_txt("ESP", "%s", buf);
        }
    }
    return vprintf(fmt, args);
}

// Callback so LVGL know the elapsed time
uint32_t millis_cb(void) {
    return millis();
}

// LVGL calls it when a rendered image needs to copied to the display
void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);

    gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_map, w, h);

    lv_disp_flush_ready(disp);
}

// Read the touchpad
void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    uint16_t x, y;
    if (touch.touched()) {
        touch.readData(&x, &y);
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
        
        // Handle voice recording button press
        uint32_t now = millis();
        if (now - last_button_press > BUTTON_DEBOUNCE_MS) {
            last_button_press = now;
            
            // Check if touch is on the talk button area (you need to adjust coordinates)
            if (x >= 100 && x <= 220 && y >= 350 && y <= 450) {
                if (!is_recording && !speech_to_text_is_recording() && !text_to_speech_is_playing()) {
                    // Start recording
                    is_recording = true;
                    chat_screen_append_txt(TAG, "🎤 Bắt đầu ghi âm...");
                    speech_to_text_start();
                    
                    // Create task to handle recording
                    xTaskCreatePinnedToCore(
                        speech_to_text_task,
                        "voice_record",
                        8192,
                        NULL,
                        3,
                        NULL,
                        1
                    );
                }
            }
        }
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void setup() {
    // ✅ Extended delay for proper initialization
    delay(3000);
    
    Serial.begin(115200);
    
    // ✅ Wait for Serial with timeout
    int wait_count = 0;
    while (!Serial && wait_count < 50) {
        delay(100);
        wait_count++;
    }
    
    Serial.println("=== ESP32-S3 AI Voice Assistant Starting ===");
    Serial.printf("Free heap at start: %d bytes\n", ESP.getFreeHeap());
    Serial.printf("PSRAM found: %s\n", psramFound() ? "YES" : "NO");

    // ✅ 1) Display initialization
    Serial.println("Initializing display...");
    if (!gfx->begin(40000000UL)) {
        Serial.println("Failed to init display!");
        return;
    }
    gfx->fillScreen(BLACK);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
    Serial.println("Display initialized");

    // ✅ 2) Touch initialization
    Serial.println("Initializing touch...");
    if (!touch.begin()) {
        Serial.println("Failed to init touch!");
        return;
    }
    touch.enOffsetCorrection(true);
    touch.setOffsets(
        Touch_X_min, Touch_X_max, TFT_res_W - 1,
        Touch_Y_min, Touch_Y_max, TFT_res_H - 1
    );
    Serial.println("Touch initialized");

    // ✅ 3) LVGL core initialization
    Serial.println("Initializing LVGL...");
    lv_init();
    lv_tick_set_cb(millis_cb);

    // ✅ 4) LVGL display driver with PSRAM buffer
    uint32_t w = gfx->width(), h = gfx->height();
    uint32_t bufSize = w * h / 10;
    
    lv_color_t *buf = NULL;
    if (psramFound()) {
        buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        Serial.println("Using PSRAM for LVGL buffer");
    }
    
    if (!buf) {
        buf = (lv_color_t *)malloc(bufSize * 2);
        Serial.println("Using regular RAM for LVGL buffer");
    }
    
    if (!buf) {
        Serial.println("LVGL buffer alloc failed!");
        return;
    }
    
    lv_display_t *disp = lv_display_create(w, h);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, buf, NULL, bufSize * 2, LV_DISPLAY_RENDER_MODE_PARTIAL);
    Serial.println("LVGL display driver initialized");

    // ✅ 5) LVGL input
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    Serial.println("LVGL input initialized");

    // ✅ 6) UI initialization
    Serial.println("Loading UI...");
    ui_init();
    lv_scr_load(ui_Main);

    // Process UI a few times
    for (int i = 0; i < 5; i++) {
        lv_task_handler();
        delay(20);
    }
    gfx->flush();
    Serial.println("UI loaded");

    // ✅ 7) Show initial messages
    chat_screen_append_txt(TAG, "🚀 AI Voice Assistant v2.0");
    chat_screen_append_txt(TAG, "📱 Hardware: ESP32-S3 JC3248W535C");
    chat_screen_append_txt(TAG, "🎨 LVGL v%d.%d.%d", 
                          lv_version_major(), lv_version_minor(), lv_version_patch());
    chat_screen_append_txt(TAG, "💾 Free heap: %d bytes", ESP.getFreeHeap());
    if (psramFound()) {
        chat_screen_append_txt(TAG, "🧠 PSRAM available");
    }
    
    // ✅ 8) Initialize subsystems with proper order and delays
    Serial.println("Initializing WiFi...");
    wifi_manager_init();
    delay(500);
    
    Serial.println("Initializing storage...");
    storage_manager_init();
    delay(200);
    
    Serial.println("Initializing speech-to-text...");
    speech_to_text_init();
    delay(200);
    
    Serial.println("Initializing Gemini client...");
    gemini_client_init();
    delay(200);
    
    // ✅ 9) Initialize TTS LAST to avoid I2S conflicts
    Serial.println("Initializing text-to-speech...");
    text_to_speech_init();
    delay(3000); // Extended delay for TTS to fully initialize
    
    // ✅ 10) Set printf override AFTER all audio initialization
    esp_log_set_vprintf(vprintf_to_ui);
    
    // ✅ 11) Connect WiFi
    chat_screen_append_txt(TAG, "📶 Connecting to WiFi...");
    lv_task_handler();
    gfx->flush();
    
    wifi_manager_connect("", "");
    
    Serial.println("Setup completed successfully!");
    Serial.printf("Final free heap: %d bytes\n", ESP.getFreeHeap());
}

void loop() {
    // ✅ Prioritize LVGL task handling
    lv_task_handler();
    
    // ✅ TTS loop is now handled by dedicated task, just call lightweight version
    text_to_speech_loop();
    
    // ✅ Flush display
    gfx->flush();
    
    // ✅ Small delay to prevent watchdog and allow other tasks
    delay(5);
}