#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H
#ifdef __cplusplus
extern "C" {
#endif
void storage_manager_init(void);
void storage_manager_log(const char *user, const char *bot);
#ifdef __cplusplus
}
#endif
#endif // STORAGE_MANAGER_H
