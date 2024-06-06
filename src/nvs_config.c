/*
 * NVS Sample for Zephyr using high level API, the sample illustrates the usage
 * of NVS for storing data of different kind (strings, binary blobs, unsigned
 * 32 bit integer) and also how to read them back from flash. The reading of
 * data is illustrated for both a basic read (latest added value) as well as
 * reading back the history of data (previously added values). Next to reading
 * and writing data it also shows how data can be deleted from flash.
 *
 * The sample stores the following items:
 * 1. A string representing an IP-address: stored at id=1, data="192.168.1.1"
 * 2. A binary blob representing a key: stored at id=2, data=FF FE FD FC FB FA
 *    F9 F8
 * 3. A reboot counter (32bit): stored at id=3, data=reboot_counter
 * 4. A string: stored at id=4, data="DATA" (used to illustrate deletion of
 * items)
 *
 * At first boot the sample checks if the data is available in flash and adds
 * the items if they are not in flash.
 *
 * Every reboot increases the values of the reboot_counter and updates it in
 * flash.
 *
 * At the 10th reboot the string item with id=4 is deleted (or marked for
 * deletion).
 *
 * At the 11th reboot the string item with id=4 can no longer be read with the
 * basic nvs_read() function as it has been deleted. It is possible to read the
 * value with nvs_read_hist()
 *
 * At the 78th reboot the first sector is full and a new sector is taken into
 * use. The data with id=1, id=2 and id=3 is copied to the new sector. As a
 * result of this the history of the reboot_counter will be removed but the
 * latest values of address, key and reboot_counter is kept.
 *
 * Copyright (c) 2018 Laczen
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>
#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/storage/flash_map.h>
#include <zephyr/fs/nvs.h>
#include <zephyr/settings/settings.h>

struct nvs_fs my_config_nvs;

#define NVS_PARTITION		storage_partition
#define NVS_PARTITION_DEVICE	FIXED_PARTITION_DEVICE(NVS_PARTITION)
#define NVS_PARTITION_OFFSET	FIXED_PARTITION_OFFSET(NVS_PARTITION)

int init_nvs(void)
{
	int rc = 0;
	struct flash_pages_info info;

	my_config_nvs.flash_device = NVS_PARTITION_DEVICE;
	if (!device_is_ready(my_config_nvs.flash_device)) {
		printk("Flash device %s is not ready\n", my_config_nvs.flash_device->name);
		return 1;
	}
	my_config_nvs.offset = NVS_PARTITION_OFFSET;
	rc = flash_get_page_info_by_offs(my_config_nvs.flash_device, my_config_nvs.offset, &info);
	if (rc) {
		printk("Unable to get page info\n");
		return 1;
	}
	my_config_nvs.sector_size = info.size;
	my_config_nvs.sector_count = 16U;

	rc = nvs_mount(&my_config_nvs);
	if (rc) {
		printk("Flash Init failed\n");
        return 1;
	}

    settings_subsys_init();
    settings_load();

    printk("NVS initialized\n");
    return 0;
}