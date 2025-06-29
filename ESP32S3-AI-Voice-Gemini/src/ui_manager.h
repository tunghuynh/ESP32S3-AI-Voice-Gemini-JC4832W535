
#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Append text to the chat screen.
 * 
 * This function adds a new line of text to the chat area,
 * typically used to display system messages or responses.
 * 
 * @param txt The text to append to the chat screen.
 */
void chat_screen_append_txt(const char *tag, const char *format, ...);
void loge(const char *tag, const char *format, ...);
void logw(const char *tag, const char *format, ...);
void logi(const char *tag, const char *format, ...);

#ifdef __cplusplus
}
#endif
#endif // UI_MANAGER_H

