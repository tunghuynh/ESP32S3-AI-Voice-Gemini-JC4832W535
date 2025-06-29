#include "speech_to_text.h"
#include "esp_log.h"
#include "ui_manager.h"
#include "gemini_client.h"
#include "wifi_manager.h"
#include "pincfg.h"
#include <driver/i2s.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <string.h>
#include <base64.h>
#include <math.h>

static const char *TAG = "STT";

// I2S Configuration
#define I2S_NUM                 I2S_NUM_1
#define I2S_SAMPLE_RATE         16000
#define I2S_BITS_PER_SAMPLE     I2S_BITS_PER_SAMPLE_32BIT
#define I2S_CHANNELS            1
#define I2S_DMA_BUF_COUNT       8
#define I2S_DMA_BUF_LEN         1024

// Recording Configuration
#define RECORD_TIME_SECONDS     5
#define RECORD_BUFFER_SIZE      (I2S_SAMPLE_RATE * RECORD_TIME_SECONDS * sizeof(int16_t))
#define WAV_HEADER_SIZE         44

// Global variables
static bool s_is_recording = false;
static uint8_t *s_record_buffer = NULL;
static size_t s_record_buffer_pos = 0;
static TaskHandle_t s_recording_task_handle = NULL;

// WAV file header structure
typedef struct {
    char riff[4];           // "RIFF"
    uint32_t file_size;     // File size - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    uint32_t fmt_size;      // Format chunk size (16)
    uint16_t audio_format;  // Audio format (1 = PCM)
    uint16_t num_channels;  // Number of channels
    uint32_t sample_rate;   // Sample rate
    uint32_t byte_rate;     // Byte rate
    uint16_t block_align;   // Block align
    uint16_t bits_per_sample; // Bits per sample
    char data[4];           // "data"
    uint32_t data_size;     // Data size
} wav_header_t;

// Create WAV header
static void create_wav_header(wav_header_t *header, uint32_t data_size) {
    memcpy(header->riff, "RIFF", 4);
    header->file_size = data_size + WAV_HEADER_SIZE - 8;
    memcpy(header->wave, "WAVE", 4);
    memcpy(header->fmt, "fmt ", 4);
    header->fmt_size = 16;
    header->audio_format = 1; // PCM
    header->num_channels = 1;
    header->sample_rate = I2S_SAMPLE_RATE;
    header->byte_rate = I2S_SAMPLE_RATE * 2; // 16-bit mono
    header->block_align = 2;
    header->bits_per_sample = 16;
    memcpy(header->data, "data", 4);
    header->data_size = data_size;
}

void speech_to_text_init(void) {
    ESP_LOGI(TAG, "Initializing I2S for INMP441 microphone...");
    
    // I2S configuration for INMP441
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = I2S_SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = I2S_DMA_BUF_COUNT,
        .dma_buf_len = I2S_DMA_BUF_LEN,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };
    
    // I2S pin configuration for INMP441
    i2s_pin_config_t pin_config = {
        .bck_io_num = MIC_I2S_SCK,    // Serial Clock (BCLK)
        .ws_io_num = MIC_I2S_WS,      // Word Select (LRC)
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = MIC_I2S_SD     // Serial Data (DOUT)
    };
    
    // Install and start I2S driver
    esp_err_t ret = i2s_driver_install(I2S_NUM, &i2s_config, 0, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install I2S driver: %s", esp_err_to_name(ret));
        return;
    }
    
    ret = i2s_set_pin(I2S_NUM, &pin_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set I2S pins: %s", esp_err_to_name(ret));
        return;
    }
    
    // Clear I2S buffers
    i2s_zero_dma_buffer(I2S_NUM);
    
    ESP_LOGI(TAG, "I2S initialized successfully");
    ESP_LOGI(TAG, "Sample rate: %d Hz, Channels: %d, Bits: %d", 
             I2S_SAMPLE_RATE, I2S_CHANNELS, I2S_BITS_PER_SAMPLE);
}

void speech_to_text_start(void) {
    if (s_is_recording) {
        ESP_LOGW(TAG, "Already recording!");
        return;
    }
    
    ESP_LOGI(TAG, "Starting speech recording...");
    
    // Allocate recording buffer
    if (s_record_buffer) {
        free(s_record_buffer);
    }
    
    s_record_buffer = (uint8_t *)malloc(RECORD_BUFFER_SIZE + WAV_HEADER_SIZE);
    if (!s_record_buffer) {
        ESP_LOGE(TAG, "Failed to allocate recording buffer");
        return;
    }
    
    s_record_buffer_pos = WAV_HEADER_SIZE; // Skip header space
    s_is_recording = true;
    
    // Start I2S
    i2s_start(I2S_NUM);
    i2s_zero_dma_buffer(I2S_NUM);
    
    ESP_LOGI(TAG, "Recording started for %d seconds", RECORD_TIME_SECONDS);
}

void speech_to_text_stop(uint8_t **out_buf, size_t *out_len) {
    if (!s_is_recording) {
        ESP_LOGW(TAG, "Not recording!");
        if (out_buf) *out_buf = NULL;
        if (out_len) *out_len = 0;
        return;
    }
    
    s_is_recording = false;
    
    // Stop I2S
    i2s_stop(I2S_NUM);
    
    ESP_LOGI(TAG, "Recording stopped. Recorded %d bytes", s_record_buffer_pos - WAV_HEADER_SIZE);
    
    if (s_record_buffer && s_record_buffer_pos > WAV_HEADER_SIZE) {
        // Create WAV header
        wav_header_t wav_header;
        create_wav_header(&wav_header, s_record_buffer_pos - WAV_HEADER_SIZE);
        memcpy(s_record_buffer, &wav_header, WAV_HEADER_SIZE);
        
        if (out_buf) {
            *out_buf = s_record_buffer;
            s_record_buffer = NULL; // Transfer ownership
        }
        if (out_len) {
            *out_len = s_record_buffer_pos;
        }
    } else {
        ESP_LOGW(TAG, "No audio data recorded");
        if (out_buf) *out_buf = NULL;
        if (out_len) *out_len = 0;
        
        if (s_record_buffer) {
            free(s_record_buffer);
            s_record_buffer = NULL;
        }
    }
    
    s_record_buffer_pos = 0;
}

char* speech_to_text_process(const uint8_t *buf, size_t len) {
    if (!buf || len == 0) {
        ESP_LOGE(TAG, "Invalid audio buffer");
        return strdup("Error: Invalid audio data");
    }
    
    if (!wifi_manager_is_connected()) {
        ESP_LOGE(TAG, "WiFi not connected for speech-to-text");
        return strdup("Error: WiFi not connected");
    }
    
    ESP_LOGI(TAG, "Processing audio buffer (%d bytes) with speech-to-text...", len);
    
    // For now, return a placeholder response
    // In a real implementation, you would:
    // 1. Encode audio to base64
    // 2. Send to Google Speech-to-Text API or similar
    // 3. Parse the response and return the text
    
    // Simple simulation - analyze audio level to determine if speech was detected
    if (len > WAV_HEADER_SIZE) {
        const int16_t *audio_data = (const int16_t*)(buf + WAV_HEADER_SIZE);
        size_t sample_count = (len - WAV_HEADER_SIZE) / sizeof(int16_t);
        
        // Calculate average amplitude
        uint32_t total_amplitude = 0;
        for (size_t i = 0; i < sample_count; i++) {
            total_amplitude += abs(audio_data[i]);
        }
        
        uint32_t avg_amplitude = total_amplitude / sample_count;
        ESP_LOGI(TAG, "Average audio amplitude: %d", avg_amplitude);
        
        if (avg_amplitude > 100) { // Threshold for speech detection
            return strdup("Xin chào, tôi đã nghe thấy giọng nói của bạn!");
        } else {
            return strdup("Không phát hiện được giọng nói rõ ràng.");
        }
    }
    
    return strdup("Error: Processing audio failed");
}

bool speech_to_text_is_recording(void) {
    return s_is_recording;
}

void speech_to_text_task(void *pvParameters) {
    ESP_LOGI(TAG, "Speech-to-text recording task started");
    
    const TickType_t record_duration = pdMS_TO_TICKS(RECORD_TIME_SECONDS * 1000);
    TickType_t start_time = xTaskGetTickCount();
    
    uint8_t *i2s_read_buffer = (uint8_t *)malloc(I2S_DMA_BUF_LEN * 4);
    if (!i2s_read_buffer) {
        ESP_LOGE(TAG, "Failed to allocate I2S read buffer");
        goto cleanup;
    }
    
    while (s_is_recording && (xTaskGetTickCount() - start_time) < record_duration) {
        size_t bytes_read = 0;
        esp_err_t ret = i2s_read(I2S_NUM, i2s_read_buffer, I2S_DMA_BUF_LEN * 4, &bytes_read, pdMS_TO_TICKS(100));
        
        if (ret == ESP_OK && bytes_read > 0) {
            // Convert 32-bit samples to 16-bit
            int32_t *samples_32 = (int32_t *)i2s_read_buffer;
            int16_t *samples_16 = (int16_t *)(s_record_buffer + s_record_buffer_pos);
            
            size_t samples_count = bytes_read / 4;
            for (size_t i = 0; i < samples_count && s_record_buffer_pos < RECORD_BUFFER_SIZE + WAV_HEADER_SIZE - 2; i++) {
                // Convert 32-bit to 16-bit by taking upper 16 bits and scaling
                samples_16[i] = (int16_t)(samples_32[i] >> 16);
                s_record_buffer_pos += 2;
            }
        }
        
        // Small delay to prevent watchdog timeout
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
cleanup:
    if (i2s_read_buffer) {
        free(i2s_read_buffer);
    }
    
    // Stop recording and process audio
    uint8_t *audio_buffer = NULL;
    size_t audio_len = 0;
    
    speech_to_text_stop(&audio_buffer, &audio_len);
    
    if (audio_buffer && audio_len > 0) {
        chat_screen_append_txt(TAG, "🎤 Processing audio...");
        
        char *transcribed_text = speech_to_text_process(audio_buffer, audio_len);
        if (transcribed_text) {
            chat_screen_append_txt("You", "%s", transcribed_text);
            
            // Send to Gemini for response  
            char *gemini_response = gemini_client_request(transcribed_text);
            if (gemini_response) {
                chat_screen_append_txt("Gemini", "%s", gemini_response);
                free(gemini_response);
            }
            
            free(transcribed_text);
        }
        
        free(audio_buffer);
    } else {
        chat_screen_append_txt(TAG, "❌ No audio recorded");
    }
    
    s_recording_task_handle = NULL;
    ESP_LOGI(TAG, "Recording task completed");
    
    vTaskDelete(NULL);
}