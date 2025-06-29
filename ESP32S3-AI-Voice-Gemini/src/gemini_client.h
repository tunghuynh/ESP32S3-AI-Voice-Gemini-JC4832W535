#ifndef GEMINI_CLIENT_H
#define GEMINI_CLIENT_H

#include "esp_err.h"
#ifdef __cplusplus
extern "C" {
#endif
void gemini_client_init(void);
esp_err_t gemini_client_test_key(const char *key);
char* gemini_client_request(const char *input);

#ifdef __cplusplus
}
#endif
#endif // GEMINI_CLIENT_H
