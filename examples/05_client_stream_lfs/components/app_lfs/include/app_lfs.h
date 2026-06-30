#pragma once

#include "esp_err.h"

esp_err_t app_lfs_mount(void);
void app_lfs_unmount(void);
esp_err_t app_lfs_read_file(const char *path);