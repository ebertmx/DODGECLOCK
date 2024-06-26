/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include "DCLK_client.h"

#include "Interface_display.h"

LOG_MODULE_REGISTER(Display_app, CONFIG_LOG_DEFAULT_LEVEL);

/*DCLK Client Service and BLE*/

static uint8_t app_clock_cb(uint32_t data)
{
	LOG_INF("DCLK - Clock = %d ms", data);
	return BT_GATT_ITER_CONTINUE;
}
static uint8_t app_state_cb(uint8_t data)
{
	LOG_INF("DCLK - State = %d", data);
	return BT_GATT_ITER_CONTINUE;
}

static void unsubscribed(struct bt_gatt_subscribe_params *params)
{
	LOG_INF("unsub cb");
	return BT_GATT_ITER_CONTINUE;
}

static struct dclk_client_cb app_callbacks = {
	.received_clock = app_clock_cb,
	.received_state = app_state_cb,
	.unsubscribed = unsubscribed,
};

static uint8_t pair_cb(void)
{
	LOG_INF("Allow pairing");
	dclk_pairing(true);
}
static uint8_t user_cb(void)
{
}

static struct interface_cb inter_callbacks = {
	.pair_cb = pair_cb,
	.user_cb = user_cb,
};

int main(void)
{
	LOG_INF("DCLK_Display initialized\n");

	int err = interface_init(&inter_callbacks);
	if (err)
	{
		printk("Interface init failed (err %d)\n", err);
		return;
	}

	err = dclk_client_init(&app_callbacks, 123456);
	if (err)
	{
		printk("Failed to init LBS (err:%d)\n", err);
		return;
	}
	LOG_INF("Bluetooth initialized\n");

	//dclk_pairing(true);

	while (1)
	{
		k_sleep(K_FOREVER);
	}

	return 0;
}
