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

LOG_MODULE_REGISTER(Controller_app, LOG_LEVEL_ERR);

#define STACKSIZE 512
#define PRIORITY 7

#define SYNC_INTERVAL 500

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

void update_dclock(void)
{
	uint32_t temp_dclock = get_dclock();
	uint8_t temp_dstate = get_dstate();
	while (1)
	{
		temp_dclock = get_dclock();
		temp_dstate = get_dstate();
		dclk_send_clock_notify(&temp_dclock);
		dclk_send_state_notify(&temp_dstate);

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
		LOG_INF("Interface init failed (err %d)\n", err);
		return;
	}

	err = dclk_init(&app_callbacks, 123456);
	if (err)
	{
		LOG_INF("Failed to init LBS (err:%d)\n", err);
		return;
	}
	LOG_INF("Bluetooth initialized\n");
	LOG_INF("MAIN--sleep");

	dclk_pairing(true);


	for (;;)
	{
		k_sleep(K_FOREVER);
	}
}

// Start timer thread

K_THREAD_DEFINE(dclock_update, STACKSIZE, update_dclock, NULL, NULL, NULL, PRIORITY, 0, 0);
