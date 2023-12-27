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
//#include "d_UART_helper.c"
#include "d_BT_helper.h"

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

	bluetooth_init();

	return 0;
}
