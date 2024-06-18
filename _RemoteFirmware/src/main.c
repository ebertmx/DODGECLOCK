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

LOG_MODULE_REGISTER(main_app, LOG_LEVEL_INF);

#define MY_STACK_SIZE 3000
#define MY_PRIORITY 5

void bt_connection_manager(void *, void *, void *);

K_THREAD_STACK_DEFINE(my_stack_area, MY_STACK_SIZE);
struct k_thread my_thread_data;

k_tid_t my_tid;

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
				  BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
				  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
				  BT_UUID_16_ENCODE(BT_UUID_DIS_VAL))};

typedef struct bt_dclk_dev
{
	uint8_t IS_BONDED;
	uint8_t IS_CONNECTED;
	struct bt_conn *conn;
	bt_addr_le_t addr;

} bt_dclk_dev;

struct k_msgq conn_msgq;
enum bt_message_type_t
{
	CONN,
	DISCONN,
	SECURITY,
	BOND,

};

struct bt_message_t
{
	enum bt_message_type_t type;
	struct bt_conn *conn;
};

static void bt_pairing_stop();
static void bt_pairing_start();
void bt_stop_advertise();
void bt_start_advertise();
bool bt_compair_addr(bt_addr_le_t addr1, bt_addr_le_t addr2);
int bt_check_dev_list(struct bt_dclk_dev dclk_dev[], int len, bt_addr_le_t addr);

static void bt_force_disconnect(struct bt_conn *conn)
{
	printk("Force disconnect\n");
	int err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err)
	{
		printk("Failed to disconnect: %d\n", err);
	}
	else
	{
		bt_conn_unref(conn);
	}
}

bool clear_dclk_dev(struct bt_dclk_dev devs[], int len)
{
	for (int i = 0; i < len; i++)
	{
		devs[i].conn = NULL;
		devs[i].IS_CONNECTED = 0;
		devs[i].IS_BONDED = 0;
	}
}

void bt_connection_manager(void *a, void *b, void *c)
{

	printk("bt_connection_manager starting\n");
	int err;
	struct bt_dclk_dev dclk_dev[CONFIG_BT_MAX_PAIRED];

	uint8_t NUM_DEVS = CONFIG_BT_MAX_PAIRED;
	clear_dclk_dev(dclk_dev, NUM_DEVS);
	bool pair_enabled = true;

	struct bt_message_t data;
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_pairing_start();

	while (1)
	{
		/* get a data item */
		k_msgq_get(&conn_msgq, &data, K_FOREVER);

		struct bt_conn *conn = data.conn;
		printk("\n!!Event = %d!!\n", data.type);
		bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

		printk("Device Addr: %s\n", addr_str);

		int dev_index = bt_check_dev_list(dclk_dev, NUM_DEVS, *(bt_conn_get_dst(conn)));
		printk("Device index = %d\n", dev_index);

		printk("Pairing status = %d\n", pair_enabled);

		switch (data.type)
		{
		case CONN:

			printk("Connected\n");

			////

			if (dev_index >= 0)
			{
				printk("Device in list. Allow connection\n");
				dclk_dev[dev_index].IS_CONNECTED = true;
			}
			else
			{
				if (pair_enabled)
				{
					printk("Allow connection for pairing\n");
				}
				else
				{
					bt_force_disconnect(conn);
				}
			}

			break;
		case DISCONN:

			printk("Disconnected\n");

			if (dev_index >= 0)
			{
				dclk_dev[dev_index].IS_CONNECTED = false;
				bt_start_advertise();
			}
			else
			{
				bt_conn_unref(conn);
			}

			break;

		case SECURITY:

			if (dev_index >= 0)
			{
				printk("Security change complete");
			}
			else
			{
				if (pair_enabled)
				{
					printk("Pairing enabled. Security change complete\n");
				}
				else
				{
					printk("Pairing NOT enabled. Security change canceled. \n");
					bt_force_disconnect(conn);
				}
			}

			break;

		case BOND:

			if (dev_index >= 0)
			{
				printk("Device already bonded\n");
				break;
			}

			if (pair_enabled)
			{

				printk("Bonding complete\n");

				for (int i = 0; i < NUM_DEVS; i++)
				{
					if (dclk_dev[i].IS_BONDED == false)
					{
						dclk_dev[i].conn = conn;
						dclk_dev[i].addr = *(bt_conn_get_dst(conn));
						dclk_dev[i].IS_BONDED = 1;
						dclk_dev[i].IS_CONNECTED = 1;
						printk("Device added to list\n");
						break;
					}
				}

				bt_stop_advertise();
			}
			else
			{
				printk("Bonding canceled. Pairing NOT enabled\n");
				bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
				bt_conn_unref(conn);
			}
			break;
		default:
			break;
		}
	}
}

// static void add_bt_dclk_dev(dclk_dev *devs, size_t len)

int bt_check_dev_list(struct bt_dclk_dev dclk_dev[], int len, bt_addr_le_t addr)
{
	// printk("Compare %d\n", len);

	if (dclk_dev[0].conn != NULL)
	{

		for (int i = 0; i < len; i++)
		{
			if (bt_compair_addr(dclk_dev[i].addr, addr))
			{
				//	printk("addr match\n");
				return i;
			}
		}
	}

	return -1;
}
bool bt_compair_addr(bt_addr_le_t addr1, bt_addr_le_t addr2)
{
	// printk("add_size = %d\n",sizeof(addr1.a.val));
	for (int i = 0; i < sizeof(addr1.a.val); i++)
	{
		// printk("%d     %d\n", addr1.a.val[i], addr2.a.val[i]);

		if (addr1.a.val[i] != addr2.a.val[i])
		{
			return false;
		}
	}
	return true;
}

void bt_stop_advertise(void)
{
	int err = bt_le_adv_stop();
	if (err)
	{
		printk("Falied to terminate advertising\n");
	}
	printk("Advertising terminated\n");
}

void bt_start_advertise(void)
{
	int err;
	printk("Starting BT advertising\n");
	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err)
	{
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
}

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		printk("Connection failed (err 0x%02x)\n", err);
	}
	else
	{
		bt_conn_ref(conn);
		// printk("Connecting\n");

		struct bt_message_t data;

		data.type = CONN;
		data.conn = conn;

		if (k_msgq_put(&conn_msgq, &data, K_NO_WAIT))
		{
			// bt_conn_unref(conn);
		}
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr_str[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));

	// printk("Disconnecting: %s (reason %u)\n", addr_str, reason);
	struct bt_message_t data;
	data.type = DISCONN;
	data.conn = conn;

	if (k_msgq_put(&conn_msgq, &data, K_NO_WAIT))
	{
		bt_conn_unref(conn);
	}
}

void security_change(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	if (err)
	{
		printk("Failed to change security\n");
	}
	else
	{

		// printk("Security set. Level = %d\n", level);

		struct bt_message_t data;
		data.type = SECURITY;
		data.conn = conn;

		if (k_msgq_put(&conn_msgq, &data, K_NO_WAIT))
		{
			bt_conn_unref(conn);
		}
	}
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_change,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr_str[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr_str, sizeof(addr_str));
	// printk("Pairing; bond = %d\n", bonded);
	if (bonded)
	{
		// printk("Pairing in progress %s\n", addr_str);
		struct bt_message_t data;
		data.type = BOND;
		data.conn = conn;

		if (k_msgq_put(&conn_msgq, &data, K_NO_WAIT))
		{
		}
	}
	else
	{
		printk("Pairing failed %s\n", addr_str);
	}
}

static void auth_pairing(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];
	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
	int err = bt_conn_auth_pairing_confirm(conn);
	printk("Pairing Authorized %d: %s\n", err, addr);
}

static void passkey_entry(struct bt_conn *conn)
{
	printk("Sending entry passkey = %d\n", 123456);
	bt_conn_auth_passkey_entry(conn, 123456);
}

void passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	printk("Controller passkey = %d\n", passkey);
}

void passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
	printk("Confirm Passkey = %d\n", passkey);

	bt_conn_auth_passkey_confirm(conn);
}
static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = passkey_display,
	.passkey_confirm = passkey_confirm,
	.passkey_entry = passkey_entry,
	.cancel = auth_cancel,
	.pairing_confirm = auth_pairing,
	.pairing_complete = pairing_complete,
};

static void bt_pairing_start()
{
	printk("Pairing enabled\n");
	bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	bt_start_advertise();
}

static void bt_pairing_stop()
{
	printk("Pairing disabled\n");
	bt_stop_advertise();
}

static void bas_notify(void)
{
	uint8_t battery_level = bt_bas_get_battery_level();

	battery_level--;

	if (!battery_level)
	{
		battery_level = 100U;
	}

	bt_bas_set_battery_level(69);
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
		// printk("Clock is not is pause state");
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

void main(void)
{
	int err;
	printk("hello");
	err = bt_enable(NULL);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return;
	}

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		err = settings_load();
		if (err)
		{
			return;
		}
	}

	bt_conn_cb_register(&conn_callbacks);
	bt_conn_auth_cb_register(&auth_cb_display);

	char my_msgq_buffer[10 * sizeof(struct bt_message_t)];
	k_msgq_init(&conn_msgq, my_msgq_buffer, sizeof(struct bt_message_t), 10);

	k_tid_t my_tid =
		k_thread_create(&my_thread_data, my_stack_area,
						K_THREAD_STACK_SIZEOF(my_stack_area),
						bt_connection_manager,
						NULL, NULL, NULL,
						MY_PRIORITY, 0, K_NO_WAIT);

	// dclk_start();
	while (1)
	{
		k_sleep(K_MSEC(UPDATE_PERIOD));
		// dclk_notify();
		// bas_notify();
	}
}
