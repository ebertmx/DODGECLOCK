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
// #include "d_UART_helper.c"
#include "DCLK_client.h"

/*DCLK Client Service and BLE*/
static uint32_t app_clock_cb(void)
{
	return 0;
}

static uint8_t app_state_cb(void)
{
	return 0;
}

static struct dclk_client_cb app_callbacks = {
	.clock_cb = app_clock_cb,
	.state_cb = app_state_cb,
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

	dclk_client_init_2(&app_callbacks, 123456);

	while (1)
	{
		k_sleep(K_FOREVER);
	}

	return 0;
}
