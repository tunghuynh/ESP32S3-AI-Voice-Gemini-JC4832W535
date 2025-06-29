#include "storage_manager.h"
// #include "esp_vfs_fat.h"
// #include "sdmmc_cmd.h"
// #include "esp_log.h"
#include <time.h>

static const char *TAG = "storage";

void storage_manager_init(void) {
    // // mount TF-card qua SPI
    // esp_vfs_fat_mount_config_t mount_config = {
    //     .format_if_mount_failed = false,
    // };
    // sdmmc_card_t *card;
    // esp_vfs_fat_spiflash_mount("/spiflash", "storage", &mount_config, &card);
}

void storage_manager_log(const char *user, const char *bot) {
    // FILE *f = fopen("/spiflash/chat_log.txt", "a");
    // if (!f) return;
    // time_t t = time(NULL);
    // fprintf(f, "%ld,USER,%s\n", t, user);
    // fprintf(f, "%ld,BOT,%s\n", t, bot);
    // fclose(f);
}
