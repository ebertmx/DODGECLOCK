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

// #include "d_UART_helper.c"
#include "DCLK_client.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/types.h>

#define SW0_NODE DT_NODELABEL(button0)
static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static struct gpio_callback button0_cb_data;

LOG_MODULE_REGISTER(Display_app, CONFIG_LOG_DEFAULT_LEVEL);

/*DCLK Client Service and BLE*/

static uint8_t app_clock_cb(uint32_t data)
{
	LOG_INF("DCLK - clock = %d ms", data);
	return BT_GATT_ITER_CONTINUE;
}
static uint8_t app_state_cb(uint8_t data)
{
	LOG_INF("DCLK - state = %d", data);
	return BT_GATT_ITER_CONTINUE;
}

static void unsubscribed(struct bt_gatt_subscribe_params *params)
{
	LOG_INF("unsub cb");
	return; // BT_GATT_ITER_CONTINUE;
}

static struct dclk_client_cb app_callbacks = {
	.received_clock = app_clock_cb,
	.received_state = app_state_cb,
	.unsubscribed = unsubscribed,
};

static void pair_work_cb(void)
{
	dclk_set_adv(true);
}
K_WORK_DEFINE(initiate_pairing, pair_work_cb);
static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{

	if (pins == BIT(button0.pin))
	{
		k_work_submit(&initiate_pairing);
	}
}

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

	gpio_pin_configure_dt(&button0, GPIO_INPUT);
	gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_EDGE_RISING);
	gpio_init_callback(&button0_cb_data, button_pressed, BIT(button0.pin));
	gpio_add_callback(button0.port, &button0_cb_data);

	dclk_client_init(&app_callbacks, 123456);

	//dclk_set_adv(true);

	while (1)
	{
		k_sleep(K_FOREVER);
	}

	return 0;
}
