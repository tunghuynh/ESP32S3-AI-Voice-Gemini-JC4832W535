#ifndef SPEECH_TO_TEXT_H
#define SPEECH_TO_TEXT_H

#include "esp_err.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void speech_to_text_init(void);
void speech_to_text_start(void);
void speech_to_text_stop(uint8_t **out_buf, size_t *out_len);
char* speech_to_text_process(const uint8_t *buf, size_t len);
bool speech_to_text_is_recording(void);
void speech_to_text_task(void *pvParameters);
#ifdef __cplusplus
}
#endif
#endif // SPEECH_TO_TEXT_H
