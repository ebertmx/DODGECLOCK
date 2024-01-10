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
/*


*/
/*SHOT CLOCK*/
struct k_timer d_timer;
static uint8_t d_state = 0;		 // shot clock state
static uint32_t d_clock = 10000; // shot clock in ms

static void d_clock_expire(struct k_timer *timer_id)
{
	d_state = 2;
	d_clock = 0;
	d_clock = 10000;
	d_state = 0;
	k_timer_start(&d_timer, K_MSEC(d_clock), K_NO_WAIT);
}
/*


*/

/*DCLK Service and BLE*/
static uint32_t app_clock_cb(void)
{
	//return get_dclock();
	return 0;
}

static uint8_t app_state_cb(void)
{
	//return get_dstate();
	return 0;
}

static struct dclk_cb app_callbacks = {
	.clock_cb = app_clock_cb,
	.state_cb = app_state_cb,
};

/*


*/
/*Button Callbacks*/
static uint8_t pair_cb(uint8_t evt)
{
	LOG_INF("Allow pairing");
	dclk_pairing();
	return 0;
}
static uint8_t user_cb(uint8_t evt)
{
}


void update_dclock(void)
{
	//uint32_t temp_dclock = get_dclock();
	//uint8_t temp_dstate = get_dstate();
	while (1)
	{
		//temp_dclock = get_dclock();
		//temp_dstate = get_dstate();
		//dclk_send_clock_notify(&temp_dclock);
		//dclk_send_state_notify(&temp_dstate);

		k_sleep(K_MSEC(SYNC_INTERVAL));
	}
}

//K_TIMER_DEFINE(d_timer, d_clock_expire, NULL);

void main(void)
{
	int err;

	LOG_INF("Starting DCLK Controller \n");

	err = interface_init(NULL);
	if (err)
	{
		LOG_INF("Interface init failed (err %d)\n", err);
		return;
	}

	// err = dclk_init(&app_callbacks);
	// if (err)
	// {
	// 	LOG_INF("Failed to init LBS (err:%d)\n", err);
	// 	return;
	// }
	// LOG_INF("Bluetooth initialized\n");
	// LOG_INF("MAIN--sleep");

	// dclk_pairing();


	// /*Timer*/

	// d_clock = 10000;
	// d_state = 0;
	// k_timer_start(&d_timer, K_MSEC(d_clock), K_NO_WAIT);

	for (;;)
	{
		k_sleep(K_FOREVER);
	}
}

// Start timer thread

//K_THREAD_DEFINE(dclock_update, STACKSIZE, update_dclock, NULL, NULL, NULL, PRIORITY, 0, 0);
