/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <sys/printk.h>
#include <sys/byteorder.h>
#include <zephyr.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/bas.h>
//#include <bluetooth/services/hrs.h>

#include "dclk_service.h"

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
		      BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
		      BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))
};

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err) {
		printk("Connection failed (err 0x%02x)\n", err);
	} else {
		printk("Connected\n");
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected (reason 0x%02x)\n", reason);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void bt_ready(void)
{
	int err;

	printk("Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.cancel = auth_cancel,
};

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level) {
		battery_level = 100U;
	}

	bt_bas_set_battery_level(battery_level);
}

// static void hrs_notify(void)
// {
// 	static uint16_t heartrate = 90U;

// 	/* Heartrate measurements simulation */
// 	heartrate++;
// 	if (heartrate == 160U) {
// 		heartrate = 90U;
// 	}

// 	bt_dclk_notify(heartrate);
// }


/*SHOT CLOCK*/

const uint32_t CLOCK_MAX_VALUE = 10000; // msec

static void dclk_expire_cb(void);
static void dclk_stop_cb(void);
K_TIMER_DEFINE(dclk_timer, &dclk_expire_cb, &dclk_stop_cb);

enum dclk_state_type
{
	RUN,
	PAUSE,
	STOP
};
// struct k_timer dclk_timer;
static enum dclk_state_type dclk_state = 0; // shot clock state
static uint32_t dclk_value = 0;

void dclk_start()
{
	k_timer_start(&dclk_timer, K_MSEC(CLOCK_MAX_VALUE), K_NO_WAIT);
	dclk_state = RUN;
}

static void dclk_pause()
{
	dclk_value = k_timer_remaining_get(&dclk_timer);
	k_timer_stop(&dclk_timer);
	dclk_state = PAUSE;
}

static void dclk_resume()
{
	if (dclk_state == PAUSE)
	{
		k_timer_start(&dclk_timer, K_MSEC(dclk_value), K_NO_WAIT);
	}
	else
	{
		// LOG_INF("Clock is not is pause state");
		k_timer_start(&dclk_timer, K_MSEC(CLOCK_MAX_VALUE), K_NO_WAIT);
	}
	dclk_state = RUN;
}

static void dclk_stop()
{
	k_timer_stop(&dclk_timer);
	dclk_state = STOP;
}

void dclk_notify()
{
	if (dclk_state != PAUSE)
	{
		dclk_value = k_timer_remaining_get(&dclk_timer);
	}

	bt_dclk_notify(dclk_value, dclk_state);

}


static void dclk_expire_cb(void)
{
	dclk_start();

}
static void dclk_stop_cb(void)
{

}







const uint16_t UPDATE_PERIOD = 1000;

void main(void)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	bt_ready();

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	/* Implement notification. At the moment there is no suitable way
	 * of starting delayed work so we do it here
	 */
	dclk_start();
	while (1) {
		k_sleep(K_MSEC(UPDATE_PERIOD));
		dclk_notify();
		/* Heartrate measurements simulation */
		//hrs_notify();

		/* Battery level simulation */
		//bas_notify();
	}
}
