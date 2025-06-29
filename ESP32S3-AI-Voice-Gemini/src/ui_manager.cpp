#include "ui_manager.h"
#include <Arduino.h>
#include <lvgl.h>
#include "ui.h"
#include <cstdlib>
#include <cstring>
#include <stdarg.h>
#include <stdio.h>

#define CHAT_BUF_SIZE 128

struct async_msg_t {
    char *txt;
};

static void _async_append_cb(void *param) {
    async_msg_t *m = static_cast<async_msg_t*>(param);
    lv_textarea_add_text(ui_tachat, m->txt);
    free(m->txt);
    free(m);
}

void chat_screen_append_txt(const char *tag, const char *format, ...) {
    char local[CHAT_BUF_SIZE];
    va_list args;
    va_start(args, format);
    vsnprintf(local, sizeof(local), format, args);
    va_end(args);

    size_t tag_len   = tag ? strlen(tag) : 0;
    size_t text_len  = strlen(local);
    size_t total_len = 1 + (tag_len ? tag_len + 2 : 0) + text_len + 1;

    async_msg_t *m = static_cast<async_msg_t*>(malloc(sizeof(*m)));
    m->txt = static_cast<char*>(malloc(total_len));
    char *p = m->txt;
    *p++ = '\n';
    if (tag_len) {
        memcpy(p, tag, tag_len);
        p += tag_len;
        *p++ = ':'; *p++ = ' ';
    }
    memcpy(p, local, text_len + 1);

    lv_async_call(_async_append_cb, m);
}

void loge(const char *tag, const char *fmt, ...) {
    char buf[CHAT_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Serial.println(buf);
    chat_screen_append_txt(tag, buf);
}

void logw(const char *tag, const char *fmt, ...) {
    char buf[CHAT_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Serial.println(buf);
    chat_screen_append_txt(tag, buf);
}

void logi(const char *tag, const char *fmt, ...) {
    char buf[CHAT_BUF_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    Serial.println(buf);
    chat_screen_append_txt(tag, buf);
}
