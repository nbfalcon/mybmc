#pragma once

#include <zephyr/fs/nvs.h>

extern struct nvs_fs my_config_nvs;
int init_nvs(void);