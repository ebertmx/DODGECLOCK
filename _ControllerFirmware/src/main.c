/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/conn.h>

#include <zephyr/drivers/gpio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include <zephyr/settings/settings.h>

#include "DCLK.h"
#include "Interface.h"


LOG_MODULE_REGISTER(DCLK_app, LOG_LEVEL_INF);



#define STACKSIZE 1024
#define PRIORITY 7

#define RUN_LED_BLINK_INTERVAL 1000

#define SYNC_INTERVAL 300


/*DCLK Service and BLE*/
static uint32_t app_clock_cb(void)
{
	return get_dclock();
}

static uint8_t app_state_cb(void)
{
	return get_dstate();
}



static struct dclk_cb app_callbacks = {
	.clock_cb = app_clock_cb,
	.state_cb = app_state_cb,
};

static uint8_t pair_cb(void)
{
	start_pairing();
}
static uint8_t user_cb(void)
{

}

static struct interface_cb inter_callbacks = {
	.pair_cb = pair_cb,
	.user_cb = user_cb,
};


void update_dclock(void)
{
	while (1)
	{
		
		dclk_send_clock_notify(get_dclock());
		dclk_send_state_notify(get_dstate());

		k_sleep(K_MSEC(SYNC_INTERVAL));
	}
}

void main(void)
{
	int err;

	LOG_INF("Starting DCLK Controller \n");

	err = interface_init(&inter_callbacks);
	if (err)
	{
		printk("Interface init failed (err %d)\n", err);
		return;
	}
	err= bt_unpair(BT_ID_DEFAULT,BT_ADDR_LE_ANY);

	err = dclk_init(&app_callbacks);
	if (err)
	{
		printk("Failed to init LBS (err:%d)\n", err);
		return;
	}
	LOG_INF("Bluetooth initialized\n");

	start_advertising();

	for (;;)
	{
		//dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}

// Start timer thread

K_THREAD_DEFINE(dclock_update, STACKSIZE, update_dclock, NULL, NULL, NULL, PRIORITY, 0, 0);
