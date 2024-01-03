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

int main(void)
{
	// 	const struct device *const dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
	// 	uint32_t dtr = 0;

	// #if defined(CONFIG_USB_DEVICE_STACK_NEXT)
	// 	if (enable_usb_device_next())
	// 	{
	// 		return 0;
	// 	}
	// #else
	// 	if (usb_enable(NULL))
	// 	{
	// 		return 0;
	// 	}
	// #endif

	dclk_client_init(&app_callbacks, 123456);

	while (1)
	{
		k_sleep(K_FOREVER);
	}

	return 0;
}
