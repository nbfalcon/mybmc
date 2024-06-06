#include "nvs_config.h"
#include "wifi.h"
#include "creds.h"
#include <string.h>

int main() {
    init_nvs();
    init_wifi();

    struct my_config_wifi_entry wf_conf = {
        .psk = CRED_PSK,
        .ssid = CRED_SSID,
    };
    wf_conf.psk_len = strlen(wf_conf.psk);
    wf_conf.ssid_len = strlen(wf_conf.ssid);

    wifi_connect(&wf_conf);
}