// src/gemini_client.cpp - Fixed UTF-8 and Response Handling

#include "gemini_client.h"
#include "ui_manager.h"
#include "wifi_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <string.h>

static const char *TAG = "GEMINI";
static char s_api_key[80] = {0};
static const char *apiKey = "API-Key";

extern "C" {
  void gemini_client_init(void) {
    strncpy(s_api_key, apiKey, sizeof(s_api_key) - 1);
    s_api_key[sizeof(s_api_key)-1] = '\0';
    logi(TAG, "Gemini client initialized (Pure Arduino)");
  }

  esp_err_t gemini_client_test_key(const char *key) {
    const char *use_key = (key && strlen(key) > 0) ? key : apiKey;
    
    if (WiFi.status() != WL_CONNECTED) {
      loge(TAG, "WiFi not connected!");
      return ESP_FAIL;
    }
    
    HTTPClient http;
    
    String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + String(use_key);
    
    logi(TAG, "Testing API key...");
    
    if (!http.begin(url)) {
        loge(TAG, "HTTP begin failed");
        return ESP_FAIL;
    }
    
    http.setTimeout(15000);
    http.addHeader("Content-Type", "application/json; charset=utf-8");
    
    int httpCode = http.GET();
    
    logi(TAG, "API test response code: %d", httpCode);
    
    if (httpCode == HTTP_CODE_OK || httpCode == 200) {
        String response = http.getString();
        logi(TAG, "API key validation successful");
        
        http.end();
        strncpy(s_api_key, use_key, sizeof(s_api_key) - 1);
        return ESP_OK;
    } else if (httpCode > 0) {
        String response = http.getString();
        loge(TAG, "HTTP error %d: %s", httpCode, response.c_str());
    } else {
        loge(TAG, "HTTP request failed: %s", http.errorToString(httpCode).c_str());
    }
    
    http.end();
    return ESP_FAIL;
  }

  char *gemini_client_request(const char *input) {
    if (!input || strlen(input) == 0) {
      loge(TAG, "Empty input");
      return strdup("Empty input provided");
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      loge(TAG, "WiFi not connected!");
      return strdup("WiFi not connected");
    }

    const char *use_key = (strlen(s_api_key) > 0) ? s_api_key : apiKey;
    
    HTTPClient http;
    
    String url = "https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent?key=" + String(use_key);
    
    // ✅ FIX 1: Create proper JSON with UTF-8 support
    JsonDocument doc;
    JsonArray contents = doc["contents"].to<JsonArray>();
    JsonObject content = contents.add<JsonObject>();
    JsonArray parts = content["parts"].to<JsonArray>();
    JsonObject part = parts.add<JsonObject>();
    part["text"] = input;
    
    JsonObject genConfig = doc["generationConfig"].to<JsonObject>();
    genConfig["maxOutputTokens"] = 100;
    genConfig["temperature"] = 0.7;
    
    String payload;
    serializeJson(doc, payload);
    
    logi(TAG, "Sending request to Gemini...");
    logi(TAG, "Input text: %s", input);
    
    if (!http.begin(url)) {
        loge(TAG, "HTTP begin failed");
        return strdup("HTTP begin failed");
    }
    
    // ✅ FIX 2: Proper UTF-8 headers
    http.setTimeout(30000);
    http.addHeader("Content-Type", "application/json; charset=utf-8");
    http.addHeader("Accept", "application/json");
    http.addHeader("Accept-Charset", "utf-8");
    
    int httpCode = http.POST(payload);
    
    logi(TAG, "HTTP Response Code: %d", httpCode);
    
    if (httpCode != HTTP_CODE_OK && httpCode != 200) {
        if (httpCode > 0) {
            String error_response = http.getString();
            loge(TAG, "HTTP error %d: %s", httpCode, error_response.c_str());
        } else {
            loge(TAG, "HTTP POST failed: %s", http.errorToString(httpCode).c_str());
        }
        http.end();
        return strdup("HTTP request failed");
    }
    
    String response = http.getString();
    http.end();
    
    if (response.length() == 0) {
        loge(TAG, "No response data received");
        return strdup("No response data");
    }
    
    logi(TAG, "Raw response received (%d chars)", response.length());
    
    // ✅ FIX 3: Proper UTF-8 JSON parsing
    JsonDocument responseDoc;
    
    DeserializationError error = deserializeJson(responseDoc, response);
    if (error) {
        loge(TAG, "JSON parsing failed: %s", error.c_str());
        // Try to extract partial response for debugging
        int maxLen = response.length() > 200 ? 200 : response.length();
        String preview = response.substring(0, maxLen);
        loge(TAG, "Response preview: %s", preview.c_str());
        return strdup("Invalid JSON response");
    }
    
    // ✅ FIX 4: Better response extraction with UTF-8 handling
    if (responseDoc["candidates"][0]["content"]["parts"][0]["text"]) {
        String answer = responseDoc["candidates"][0]["content"]["parts"][0]["text"].as<String>();
        
        // ✅ Clean up the text but preserve UTF-8 characters
        answer.trim();
        
        // Remove any null bytes or invalid characters
        String cleanAnswer = "";
        for (int i = 0; i < answer.length(); i++) {
            char c = answer.charAt(i);
            if (c != 0 && c != '\r') {  // Keep newlines but remove carriage returns and null bytes
                cleanAnswer += c;
            }
        }
        
        char *result = strdup(cleanAnswer.c_str());
        logi(TAG, "✅ Gemini response parsed successfully");
        logi(TAG, "Answer length: %d characters", strlen(result));
        // logi(TAG, "Answer: %s", result);
        return result;
    } else {
        // Check for API error
        if (responseDoc["error"]["message"]) {
            String errorMsg = responseDoc["error"]["message"].as<String>();
            loge(TAG, "Gemini API error: %s", errorMsg.c_str());
            return strdup(errorMsg.c_str());
        }
        
        loge(TAG, "No valid response found in JSON");
        return strdup("No valid response found");
    }
  }
} // extern "C"