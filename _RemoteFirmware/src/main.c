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
#include <settings/settings.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>
#include <bluetooth/uuid.h>
#include <bluetooth/gatt.h>
#include <bluetooth/services/bas.h>
#include <bluetooth/conn.h>

#include "dclk_service.h"

#include <logging/log.h>
#include <sys/printk.h>

#include <device.h>
#include <devicetree.h>
#include <drivers/gpio.h>

#define LED0_NODE DT_ALIAS(led0)
#define LED0 DT_GPIO_LABEL(LED0_NODE, gpios)
#define PIN DT_GPIO_PIN(LED0_NODE, gpios)
#define FLAGS DT_GPIO_FLAGS(LED0_NODE, gpios)

LOG_MODULE_REGISTER(main_app, LOG_LEVEL_INF);

// static void bt_stop_advertize();
static void bt_pairing_start();
void bt_stop_advertise();
void bt_start_advertise();

K_WORK_DEFINE(bt_start_adv_work, bt_start_advertise);
K_WORK_DEFINE(bt_pairing_work, bt_pairing_start);
K_WORK_DEFINE(bt_stop_adv_work, bt_stop_advertise);

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
				  BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
				  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
				  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))};

typedef struct bt_info
{
	/** number of active BLE connection */
	uint8_t num_paired;
	uint8_t num_conn;

	struct bt_conn *conn_list[CONFIG_BT_MAX_CONN];
	/** pairing status */
	bool pair_en;

} bt_info;

struct bt_info bt_dclk_info =
	{
		.num_conn = 0,
		.num_paired = 0,
		.pair_en = false};

static void bt_count_bonds(const struct bt_bond_info *info, void *user_data)
{
	int *bond_cnt = user_data;
	if ((*bond_cnt) < 0)
	{
		return;
	}
	(*bond_cnt)++;
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	LOG_INF("Pairing Complete: %d", bonded);

	bt_dclk_info.conn_list[bt_dclk_info.num_paired] = conn;
	bt_dclk_info.num_paired++;
}

static void setup_accept_list_cb(const struct bt_bond_info *info, void *user_data)
{
	int *bond_cnt = user_data;

	if ((*bond_cnt) < 0)
	{
		return;
	}

	int err = bt_le_whitelist_add(&info->addr);
	LOG_INF("Added following peer to accept list: %x %x\n", info->addr.a.val[0],
			info->addr.a.val[1]);
	if (err)
	{
		LOG_ERR("Cannot add peer to filter accept list (err: %d)\n", err);
		(*bond_cnt) = -EIO;
	}
	else
	{
		(*bond_cnt)++;
	}
}

// ADVERTISING
static int setup_accept_list(uint8_t local_id)
{
	int err = bt_le_whitelist_clear();

	if (err)
	{
		LOG_INF("Cannot clear BLE accept list (err: %d)\n", err);
		return err;
	}

	int bond_cnt = 0;

	bt_foreach_bond(local_id, setup_accept_list_cb, &bond_cnt);

	return bond_cnt;
}

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		LOG_INF("Connection failed (err %u)\n", err);
	//	k_work_submit(&bt_start_adv_work);
		return;
	}

	LOG_INF("BT Connected\n");
	bt_dclk_info.num_conn++;
	// k_work_submit(&bt_stop_adv_work);

	// bt_conn_set_security(conn, BT_SECURITY_L4);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("BT Disconnected (reason %u)\n", reason);
	bt_dclk_info.num_conn--;
	// advertize to try and reconnect
	// k_work_submit(&bt_start_adv_work);
}

static void on_security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	// char addr[BT_ADDR_LE_STR_LEN];

	// bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err)
	{
		LOG_INF("Security changed");
	}
	else
	{
		LOG_INF("Security failed");
	}
}

void on_le_param_updated(struct bt_conn *conn, uint16_t interval, uint16_t latency, uint16_t timeout)
{
    double connection_interval = interval*1.25;         // in ms
    uint16_t supervision_timeout = timeout*10;          // in ms
    LOG_INF("Connection parameters updated: interval %.2f ms, latency %d intervals, timeout %d ms", connection_interval, latency, supervision_timeout);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.security_changed = on_security_changed,
	.le_param_updated = on_le_param_updated
};

void bt_stop_advertise(void)
{

	int err = bt_le_adv_stop();
	if (err)
	{
		LOG_INF("Falied to terminate advertising");
	}
	LOG_INF("Advertising terminated");
	bt_dclk_info.pair_en = false;
}

void bt_start_advertise(struct k_work *work)
{
	int err = 0;
	bt_le_adv_stop();

	int allowed_cnt = setup_accept_list(BT_ID_DEFAULT);
	LOG_DBG("bond_count = %d", allowed_cnt);
	if (allowed_cnt < 0)
	{
		LOG_INF("Acceptlist setup failed (err:%d)\n", allowed_cnt);
	}
	else if (allowed_cnt > 0)
	{
		LOG_INF("Acceptlist setup number  = %d \n", allowed_cnt);
		// One shot advertizing due to bt_adv_param settings
		// stops upon connection
		err = bt_le_adv_start(BT_LE_ADV_OPT_FILTER_CONN, ad, ARRAY_SIZE(ad), NULL, 0);

		if (err)
		{
			LOG_INF("Advertising failed to start (err %d)\n", err);
			return;
		}
		LOG_INF("Advertising successfully started\n");
		return;
	}

	LOG_INF("No Bonds -- Please start pairing");
	return;
}

static void auth_pairing(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	int err = bt_conn_auth_pairing_confirm(conn);
	LOG_INF("Pairing Authorized %d: %s\n", err, addr);
}

static void passkey_entry(struct bt_conn *conn)
{
	LOG_INF("Sending entry passkey = %d", 123456);
	bt_conn_auth_passkey_entry(conn, 123456);
}

void passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	LOG_INF("Controller passkey = %d", passkey);
}

void passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	LOG_INF("Confirm Passkey = %d", passkey);
	bt_conn_auth_passkey_confirm(conn);
}
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = passkey_display,
	.passkey_confirm = passkey_confirm,
	.passkey_entry = passkey_entry,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing,
	//.pairing_complete = pairing_complete,
};

static void bt_pairing_start()
{
	LOG_INF("Pairing start--");
	int err = 0;

	bt_le_adv_stop();

	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	if (err)
	{
		LOG_INF("Cannot delete bond (err: %d)\n", err);
	}
	else
	{
		LOG_INF("Bond deleted succesfully \n");
	}

	LOG_INF("Advertising with no Accept list \n");
	// One shot advertizing due to bt_adv_param settings
	// stops upon connection
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);

	if (err)
	{
		LOG_INF("Advertising failed to start (err %d)\n", err);
		return;
	}
}

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level)
	{
		battery_level = 100U;
	}

	bt_bas_set_battery_level(bt_dclk_info.num_paired);
}

/*SHOT CLOCK*/

const uint32_t CLOCK_MAX_VALUE = 10000; // msec

static void dclk_expire_cb(struct k_timer *timer_id);
static void dclk_stop_cb(struct k_timer *timer_id);
K_TIMER_DEFINE(dclk_timer, &dclk_expire_cb, &dclk_stop_cb);

enum dclk_state_type
{
	RUN,
	PAUSE,
	STOP,
	EXPRIRE
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

static void dclk_expire_cb(struct k_timer *timer_id)
{
	// dclk_start();
	dclk_state = EXPRIRE;
	dclk_value = 0;
	bt_dclk_notify(dclk_value, dclk_state);
}

static void dclk_stop_cb(struct k_timer *timer_id)
{
}

const uint16_t UPDATE_PERIOD = 1000;


const struct device *dev;
void interface_init()
{
	int ret;

	dev = device_get_binding(LED0);
	if (dev == NULL)
	{
		return;
	}

	ret = gpio_pin_configure(dev, PIN, GPIO_OUTPUT_ACTIVE | FLAGS);
	if (ret < 0)
	{
		return;
	}

	gpio_pin_set(dev, PIN, 1);
}



void main(void)
{


	int err;
	LOG_INF("Hello! I'm a remote.");

	interface_init();	

	err = bt_enable(NULL);
	if (err)
	{
		LOG_INF("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		err = settings_load();
		if (err)
		{
			return;
		}
		LOG_INF("Settings Loaded");

		int bond_cnt = 0;

		bt_foreach_bond(BT_ID_DEFAULT, &bt_count_bonds, &bond_cnt);

		bt_dclk_info.num_paired = bond_cnt;
		LOG_INF("Existing BT Bonds = %d", bond_cnt);
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	bt_pairing_start();
	//  if (bt_dclk_info.num_paired < 2)
	//  {
	//  	LOG_INF("No current bonds.\n Start pairing...");
	//  	bt_pairing_start();
	//  }
	//  else
	//  {
	//  	LOG_INF("Advertising with accept list");
	//  	k_work_submit(&bt_start_advertise);
	//  }

	// dclk_start();

	LOG_INF("Starting DCLK notifications.");
	int led_val = 1;
	while (1)
	{

		k_sleep(K_MSEC(UPDATE_PERIOD));
		gpio_pin_set(dev, PIN, led_val);
		led_val ^= 1;
		// dclk_notify();
		// bas_notify();
	}
}
