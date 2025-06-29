#include "ap_manager.h"
#include "ui_manager.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>
static const char *TAG = "AP_MGR";
static httpd_handle_t portal_server = NULL;

// HTML form for Wi-Fi + API Key
static const char *portal_form =
"<html><head><title>Config WiFi & Gemini</title></head><body>"
"<h2>Configure Wi-Fi & Gemini Key</h2>"
"<form method='POST' action='/save'>"
"SSID:<br><input type='text' name='ssid'><br>"
"Password:<br><input type='password' name='pass'><br>"
"Gemini Key:<br><input type='text' name='key'><br><br>"
"<input type='submit' value='Save'>"
"</form>"
"</body></html>";

// Handler for GET /
static esp_err_t root_get_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, portal_form, strlen(portal_form));
    return ESP_OK;
}

// Simple URL form decoder (assumes no URL encoding)
static void parse_form(const char *body, char *ssid, size_t ssid_len,
                       char *pass, size_t pass_len,
                       char *key, size_t key_len) {
    // body format: ssid=...&pass=...&key=...
    char *p = strdup(body);
    char *token = strtok(p, "&");
    while (token) {
        if (strncmp(token, "ssid=", 5) == 0) {
            strncpy(ssid, token + 5, ssid_len - 1);
        } else if (strncmp(token, "pass=", 5) == 0) {
            strncpy(pass, token + 5, pass_len - 1);
        } else if (strncmp(token, "key=", 4) == 0) {
            strncpy(key, token + 4, key_len - 1);
        }
        token = strtok(NULL, "&");
    }
    free(p);
}

// Handler for POST /save
static esp_err_t save_post_handler(httpd_req_t *req) {
    int len = req->content_len;
    char *buf = malloc(len + 1);
    if (!buf) return ESP_ERR_NO_MEM;
    httpd_req_recv(req, buf, len);
    buf[len] = '\0';

    char ssid[64] = {0}, pass[64] = {0}, key[80] = {0};
    parse_form(buf, ssid, sizeof(ssid), pass, sizeof(pass), key, sizeof(key));
    free(buf);

    // Store to NVS
    nvs_handle_t h;
    if (nvs_open("config", NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_str(h, "ssid", ssid);
        nvs_set_str(h, "password", pass);
        nvs_set_str(h, "gemini_key", key);
        nvs_commit(h);
        nvs_close(h);
        logi(TAG, "Config saved: SSID=%s key=[hidden]", ssid);
    }

    const char *resp = "<html><body><h3>Saved. Rebooting...</h3></body></html>";
    httpd_resp_send(req, resp, strlen(resp));

    // Delay then restart
    vTaskDelay(pdMS_TO_TICKS(2000));
    esp_restart();
    return ESP_OK;
}

void ap_manager_init(void) {
    // Nothing here; start on demand
}

void ap_manager_start(void) {
    // Stop any STA
    esp_wifi_stop();

    // Configure as softAP
    wifi_config_t ap_cfg = {
        .ap = {
            .ssid = "ESP32-S3-Gemini",
            .ssid_len = 0,
            .channel = 1,
            .password = "123456",
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    esp_netif_create_default_wifi_ap();
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_cfg));
    ESP_ERROR_CHECK(esp_wifi_start());
    logi(TAG, "Started AP 'ESP32-S3-Gemini' with password '123456'");

    // Start HTTP server
    httpd_config_t http_cfg = HTTPD_DEFAULT_CONFIG();
    if (httpd_start(&portal_server, &http_cfg) == ESP_OK) {
        httpd_uri_t root = {
            .uri       = "/",
            .method    = HTTP_GET,
            .handler   = root_get_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(portal_server, &root);
        httpd_uri_t save = {
            .uri       = "/save",
            .method    = HTTP_POST,
            .handler   = save_post_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(portal_server, &save);
    }
}