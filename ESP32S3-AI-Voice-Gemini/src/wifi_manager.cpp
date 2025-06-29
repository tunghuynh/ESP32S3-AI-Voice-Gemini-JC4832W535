// wifi_manager.cpp - Arduino WiFi version

#include "wifi_manager.h"
#include "ui_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>

static const char *TAG = "WIFI_MGR";
static bool s_wifi_connected = false;

extern "C" {

void wifi_manager_init(void) {
    logi(TAG, "Wi-Fi manager initializing (Arduino)...");
    
    // ✅ Arduino WiFi initialization
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    logi(TAG, "Wi-Fi manager initialized (Arduino)");
}

esp_err_t wifi_manager_connect(const char *ssid, const char *password) {
    logi(TAG, "Setting Wi-Fi %s - %s", ssid ? ssid : "NULL", password ? "***" : "NULL");
    
    const char *use_ssid = (ssid && strlen(ssid) > 0) ? ssid : "tunghuynh.net";
    const char *use_pass = (password && strlen(password) > 0) ? password : "pwd";

    logi(TAG, "Using Wi-Fi SSID: %s", use_ssid);

    // ✅ Arduino WiFi connection
    WiFi.begin(use_ssid, use_pass);
    logi(TAG, "Wi-Fi connect initiated (Arduino)");
    
    // ✅ Wait for connection với timeout
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        logi(TAG, "Connecting... attempt %d/30", attempts + 1);
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        s_wifi_connected = true;
        logi(TAG, "Connected with IP: %s", WiFi.localIP().toString().c_str());
        
        // ✅ Test internet connectivity
        delay(3000);
        HTTPClient test_http;
        if (test_http.begin("http://clients3.google.com/generate_204")) {
            test_http.setTimeout(15000);
            int httpCode = test_http.GET();
            if (httpCode == 204 || httpCode == 200) {
                logi(TAG, "Internet connectivity OK (HTTP %d)", httpCode);
            } else {
                logw(TAG, "Internet test failed (HTTP %d)", httpCode);
            }
            test_http.end();
        }
        
        return ESP_OK;
    } else {
        s_wifi_connected = false;
        loge(TAG, "WiFi connection failed after 30 attempts");
        return ESP_FAIL;
    }
}

bool wifi_manager_is_connected(void) {
    s_wifi_connected = (WiFi.status() == WL_CONNECTED);
    return s_wifi_connected;
}

bool wifi_manager_wait_connected(uint32_t timeout_ms) {
    uint32_t start_time = millis();
    while ((millis() - start_time) < timeout_ms) {
        if (WiFi.status() == WL_CONNECTED) {
            s_wifi_connected = true;
            return true;
        }
        delay(100);
    }
    return false;
}

} // extern "C"