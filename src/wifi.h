#pragma once

#include <stdint.h>

struct my_config_wifi_entry {
    uint8_t sec_type;
    uint8_t psk_len, ssid_len;
    uint8_t ssid[32];
    uint8_t psk[64];
};

int wifi_connect(struct my_config_wifi_entry *config);
int init_wifi(void);

// Tries to connect to the WiFi. If we succeed, save it.
int wifi_add(struct my_config_wifi_entry* config);