
#ifndef AP_MANAGER_H
#define AP_MANAGER_H

#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
/**
 * @brief Initialize the AP manager.
 *        Call once at startup.
 */
void ap_manager_init(void);

/**
 * @brief Start the HTTP captive portal AP for config.
 *        Will block until portal is running.
 */
void ap_manager_start(void);
#ifdef __cplusplus
}
#endif
#endif // AP_MANAGER_H

