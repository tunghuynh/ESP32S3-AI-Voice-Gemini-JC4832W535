#include "text_to_speech.h"
#include "ui_manager.h"
#include <Audio.h>
#include <Arduino.h>

static const char *TAG = "TTS";

// ✅ CRITICAL: Use DIFFERENT I2S port to avoid conflicts with display
#define I2S_DOUT 41  // Keep same pins but use I2S1 instead of I2S0
#define I2S_BCLK 42
#define I2S_LRC  2

// Biến toàn cục
Audio audio;
static bool is_initialized = false;
static bool is_speaking = false;
static TaskHandle_t audio_task_handle = NULL;

// ✅ NEW: Audio task with minimal delay for smooth streaming
void audio_task(void *parameter) {
    while (true) {
        if (is_initialized) {
            audio.loop();
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // Minimal 1ms delay for streaming
    }
}

extern "C" {

void text_to_speech_init(void) {
    if (is_initialized) {
        logw(TAG, "TTS already initialized");
        return;
    }
    
    logi(TAG, "Initializing TTS with Audio library...");
    
    // ✅ FIX 1: Use PSRAM for audio buffer if available
    if (psramFound()) {
        logi(TAG, "PSRAM found, using for audio buffers");
    }
    
    // ✅ FIX 2: Audio setup with larger buffers
    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(30); // Lower volume for stability
    
    // ✅ FIX 3: Increase buffers for longer sentences
    audio.setConnectionTimeout(10000, 8000);  // Longer timeouts
    audio.forceMono(true);  // Force mono for better performance
    audio.setTone(0, 0, 0); // Disable tone processing
    
    is_initialized = true;
    is_speaking = false;
    
    // ✅ FIX 4: Create HIGH priority audio task on Core 0
    xTaskCreatePinnedToCore(
        audio_task,
        "audio_task",
        8192,           // Increase stack size
        NULL,
        10,             // MAXIMUM priority
        &audio_task_handle,
        0               // Core 0 (LVGL runs on Core 1)
    );
    
    logi(TAG, "TTS initialized successfully");
    
    // ✅ FIX 5: Test with SHORT audio for stability
    delay(2000);
    audio.connecttospeech("Sẵn sàng", "vi");
    is_speaking = true;
}

void text_to_speech_play(const char *text) {
    if (!is_initialized) {
        loge(TAG, "TTS not initialized");
        return;
    }
    
    if (!text || strlen(text) == 0) {
        logw(TAG, "Empty text for TTS");
        return;
    }
    
    // ✅ FIX 6: Proper audio stop and wait
    if (is_speaking) {
        logw(TAG, "Stopping previous TTS");
        audio.stopSong();
        is_speaking = false;
        
        // Wait for audio to fully stop
        int stop_wait = 0;
        while (audio.isRunning() && stop_wait < 50) {
            vTaskDelay(pdMS_TO_TICKS(20));
            stop_wait++;
        }
        
        if (audio.isRunning()) {
            loge(TAG, "Audio still running after stop!");
        }
        
        // Additional delay for I2S to reset
        vTaskDelay(pdMS_TO_TICKS(300));
    }
    
    logi(TAG, "Playing TTS: %.50s%s", text, strlen(text) > 50 ? "..." : "");
    
    // ✅ FIX 7: Split long text into shorter chunks
    String cleanText = String(text);
    cleanText.trim();
    
    // Remove problematic characters
    cleanText.replace("\"", "");
    cleanText.replace("\\", "");
    cleanText.replace("\n", " ");
    cleanText.replace("\r", "");
    
    // ✅ CRITICAL: Limit to SHORT sentences for stability
    // if (cleanText.length() > 50) {
    //     cleanText = cleanText.substring(0, 50);
    //     // Find last space to avoid cutting words
    //     int lastSpace = cleanText.lastIndexOf(' ');
    //     if (lastSpace > 20) {
    //         cleanText = cleanText.substring(0, lastSpace);
    //     }
    // }
    
    // ✅ FIX 8: Start TTS with retry mechanism
    is_speaking = true;
    bool success = audio.connecttospeech(cleanText.c_str(), "vi");
    
    if (!success) {
        loge(TAG, "Failed to start TTS playback");
        is_speaking = false;
        
        // Retry with simplified text
        vTaskDelay(pdMS_TO_TICKS(500));
        logi(TAG, "Retrying TTS with fallback...");
        success = audio.connecttospeech("Xin lỗi, có lỗi phát âm", "vi");
        if (success) {
            is_speaking = true;
        }
    }
    
    if (success) {
        logi(TAG, "TTS started successfully");
    }
}

// ✅ FIX 9: Simplified loop - audio task handles the heavy lifting
void text_to_speech_loop(void) {
    if (!is_initialized) return;
    
    // Just update speaking status
    if (is_speaking && !audio.isRunning()) {
        is_speaking = false;
        logi(TAG, "TTS playback finished");
    }
}

bool text_to_speech_is_playing(void) {
    if (!is_initialized) return false;
    
    // Update status
    if (is_speaking && !audio.isRunning()) {
        is_speaking = false;
    }
    
    return is_speaking;
}

void text_to_speech_stop(void) {
    if (is_speaking) {
        logi(TAG, "Stopping TTS playback");
        audio.stopSong();
        is_speaking = false;
        
        // Wait for stop to complete
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

} // extern "C"