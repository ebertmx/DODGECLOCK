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

LOG_MODULE_REGISTER(Controller_app, LOG_LEVEL_INF);

#define STACKSIZE 512
#define PRIORITY 7

#define CLOCK_RESET_VALUE 10000

#define SYNC_INTERVAL 500
/*


*/
/*SHOT CLOCK*/
struct k_timer d_timer;
static uint8_t clock_state = 0; // shot clock state
static uint32_t clock_value = 0;

static void d_clock_expire(struct k_timer *timer_id)
{
	clock_state = 2;
	// k_timer_start(&d_timer, K_MSEC(CLOCK_RESET_VALUE), K_NO_WAIT);
}
/*


*/

/*DCLK Service and BLE*/
static uint32_t DCLK_clock_cb(void)
{
	// d_clock = k_timer_remaining_get(&d_timer);
	return k_timer_remaining_get(&d_timer);
	;
}

static uint8_t DCKL_state_cb(void)
{
	return clock_state;
}

static struct dclk_cb DCLK_callbacks = {
	.clock_cb = DCLK_clock_cb,
	.state_cb = DCKL_state_cb,
};

/*


*/
/*Button Callbacks*/
static uint8_t pair_cb(uint8_t evt)
{
	LOG_INF("pairing : %d", evt);
	dclk_pairing();

	return 0;
}
static uint8_t user_cb(uint8_t evt)
{
	LOG_INF("user : %d", evt);
	return 0;
}

static uint8_t start_cb(uint8_t evt)
{
	LOG_INF("start : %d", evt);
	clock_state = 0;

	k_timer_start(&d_timer, K_MSEC(CLOCK_RESET_VALUE), K_NO_WAIT);

	return 0;
}

static uint8_t stop_cb(uint8_t evt)
{

	LOG_INF("stop : %d", evt);
	return 0;
}

static struct interface_cb interface_callbacks =
	{
		.pair = pair_cb,
		.user = user_cb,
		.start = start_cb,
		.stop = stop_cb,
};

void main(void)
{
	int err;

	LOG_INF("Starting DCLK Controller \n");
	k_timer_init(&d_timer, d_clock_expire, NULL);

	err = interface_init(&interface_callbacks);
	if (err)
	{
		LOG_ERR("Interface init failed (err %d)\n", err);
		return;
	}

	err = dclk_init(&DCLK_callbacks);
	if (err)
	{
		LOG_ERR("Failed to init LBS (err:%d)\n", err);
		return;
	}

	LOG_INF("Initialized \n");


	static uint32_t d_clock = 0;
	static uint8_t d_state = 0;
	static uint8_t num_conn = 1;

	for (;;)
	{

		k_sched_lock();

		d_clock = ROUND_UP(k_timer_remaining_get(&d_timer),1000)/1000;
		d_state = clock_state;

		k_sched_unlock();

		dclk_send_clock_notify(&d_clock);
		dclk_send_state_notify(&d_state);
		err = interface_update(&d_clock, &d_state, &num_conn);
		if(err)
		{
			LOG_ERR("DISPLAY NOT UPDATED");
		}
		//	LOG_INF("RUNNING\n");
		k_sleep(K_MSEC(SYNC_INTERVAL));
	}
	return;
}

// Start timer thread

// K_THREAD_DEFINE(dclock_update, STACKSIZE, update_dclock, NULL, NULL, NULL, PRIORITY, 0, 0);
