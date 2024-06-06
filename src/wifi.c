/*
 * Copyright (c) 2023 Craig Peacock.
 * Copyright (c) 2017 ARM Ltd.
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "wifi.h"
#include <zephyr/kernel.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/wifi_mgmt.h>

// For simplicity, assume only 1 WiFi net_if.
static K_SEM_DEFINE(sem_wifi_connected, 0, 1);
static K_SEM_DEFINE(sem_have_ip_address, 0, 1);
static bool wifi_connected, have_ip_address;

static void handle_wifi(struct net_mgmt_event_callback* cb, uint32_t mgmt_event, struct net_if* iface)
{
    const struct wifi_status* status = (const struct wifi_status*)cb->info;
    switch (mgmt_event) {
    case NET_EVENT_WIFI_CONNECT_RESULT:
        if (status->conn_status <= WIFI_STATUS_CONN_SUCCESS) {
            printk("NET: WiFi is now connected.");
            wifi_connected = true;
        } else {
            wifi_connected = false;
            printk("NET: Connecting to WiFi failed (%d)\n", status->conn_status);
        }
        k_sem_give(&sem_wifi_connected);
        break;
    case NET_EVENT_WIFI_DISCONNECT_RESULT:
        wifi_connected = false;
        printk("NET: WiFi disconnected\n");
        break;
    }
}

static void handle_net(struct net_mgmt_event_callback* cb, uint32_t mgmt_envt, struct net_if* iface)
{
    const struct wifi_status* status = (const struct wifi_status*)cb->info;
    switch (mgmt_envt) {
    case NET_EVENT_IPV4_ADDR_ADD:
    case NET_EVENT_IPV6_ADDR_ADD: {
        printk("NET: IP Address acquired. Find me at the following addresses:\n");

        for (int i = 0; i < NET_IF_MAX_IPV4_ADDR; i++) {
            char buf_addr[NET_IPV4_ADDR_LEN], buf_gw[NET_IPV4_ADDR_LEN];
            net_addr_ntop(AF_INET,
                &iface->config.ip.ipv4->unicast[i].ipv4.address.in_addr,
                buf_addr, sizeof(buf_addr));
            net_addr_ntop(AF_INET,
                &iface->config.ip.ipv4->gw,
                buf_gw, sizeof(buf_gw));
            printk("- IPv4 %s (via Gateway %s)\n", buf_addr, buf_gw);
        }

        have_ip_address = true;
        // FIXME: race condition, needs to be reset every time before we request a connect.
        // FIXME: something condvar.
        k_sem_give(&sem_have_ip_address);
    }
    }
}

int wifi_connect(struct my_config_wifi_entry* config)
{
    struct net_if* iface = net_if_get_first_wifi();

    struct wifi_connect_req_params wifi_params = { 0 };
    wifi_params.ssid = config->ssid;
    wifi_params.psk = config->psk;
    wifi_params.ssid_length = config->ssid_len;
    wifi_params.psk_length = config->psk_len;
    wifi_params.channel = WIFI_CHANNEL_ANY;
    wifi_params.security = WIFI_SECURITY_TYPE_PSK;
    wifi_params.band = WIFI_FREQ_BAND_2_4_GHZ;
    wifi_params.mfp = WIFI_MFP_OPTIONAL;

    printk("Connecting to SSID...: %s\n", wifi_params.ssid);

    if (net_mgmt(NET_REQUEST_WIFI_CONNECT, iface, &wifi_params, sizeof(struct wifi_connect_req_params))) {
        printk("WiFi Connection Request Failed\n");
        return 1;
    }
    k_sem_take(&sem_wifi_connected, K_FOREVER);
    if (!wifi_connected) {
        // We already got an error message
        return 1;
    }

    k_sem_take(&sem_have_ip_address, K_SECONDS(30));
    if (!have_ip_address) {
        printk("Obtaining an IP address took too long. Is the Router configured correctly?\n");
    }

    return 0;
}

int init_wifi(void)
{
    static struct net_mgmt_event_callback handle_wifi_cb, handle_net_cb;
    net_mgmt_init_event_callback(&handle_wifi_cb, handle_wifi,
        NET_EVENT_WIFI_CONNECT_RESULT | NET_EVENT_WIFI_DISCONNECT_RESULT);
    net_mgmt_init_event_callback(&handle_net_cb, handle_net,
        NET_EVENT_IPV4_ADDR_ADD);
    net_mgmt_add_event_callback(&handle_wifi_cb);
    net_mgmt_add_event_callback(&handle_net_cb);
    printk("Init Done: WiFi\n");

    return 0;
}