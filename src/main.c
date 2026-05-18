/* main.c - Application main entry point */

/*
 * Copyright (c) 2019 Andrei Stoica
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/types.h>
#include <stddef.h>

#include "ble.h"

int main(void)
{
	BLE_Init();

	while (true) {
		printk("Advertising for %d ms at %d ms intervals...\n",
		       CONFIG_APP_ADV_DURATION_MS,
		       CONFIG_APP_ADV_INTERVAL_MS);
		BLE_AdvertisingStart();
		k_sleep(K_MSEC(CONFIG_APP_ADV_DURATION_MS));

		BLE_AdvertisingStop();
		printk("Sleeping for %d ms...\n", CONFIG_APP_SLEEP_DURATION_MS);
		k_sleep(K_MSEC(CONFIG_APP_SLEEP_DURATION_MS));
	}

	return 0;
}
