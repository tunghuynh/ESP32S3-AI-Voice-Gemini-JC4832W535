#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>
#ifdef __cplusplus
#include <WiFi.h>
#endif
#ifdef __cplusplus
extern "C" {
#endif

void wifi_manager_init(void);
esp_err_t wifi_manager_connect(const char *ssid, const char *password);

// ✅ THÊM: New functions
bool wifi_manager_is_connected(void);
bool wifi_manager_wait_connected(uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif
#endif // WIFI_MANAGER_H