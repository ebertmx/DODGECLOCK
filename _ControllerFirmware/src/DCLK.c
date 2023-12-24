/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

/** @file
 *  @brief LED Button Service (LBS) sample
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/settings/settings.h>

#include "DCLK.h"

LOG_MODULE_DECLARE(DCLK_app);

static bool notify_state_enabled;
static bool notify_clock_enabled;
static uint8_t state;
static int32_t clock;
static struct dclk_cb dclk_cb;

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

/* STEP 3.2.1 - Define advertising parameter for no Accept List */
#define BT_LE_ADV_CONN_NO_ACCEPT_LIST                                   \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_ONE_TIME, \
					BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL)
/* STEP 3.2.2 - Define advertising parameter for when Accept List is used */
#define BT_LE_ADV_CONN_ACCEPT_LIST                                          \
	BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_FILTER_CONN | \
						BT_LE_ADV_OPT_ONE_TIME,                             \
					BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL)

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA(BT_DATA_NAME_COMPLETE, DEVICE_NAME, DEVICE_NAME_LEN),

};

static const struct bt_data sd[] = {
	BT_DATA_BYTES(BT_DATA_UUID128_ALL, BT_UUID_DCLK_VAL),
};
static void setup_accept_list_cb(const struct bt_bond_info *info, void *user_data)
{
	int *bond_cnt = user_data;

	if ((*bond_cnt) < 0)
	{
		return;
	}

	int err = bt_le_filter_accept_list_add(&info->addr);
	LOG_INF("Added following peer to accept list: %x %x\n", info->addr.a.val[0],
			info->addr.a.val[1]);
	if (err)
	{
		LOG_INF("Cannot add peer to filter accept list (err: %d)\n", err);
		(*bond_cnt) = -EIO;
	}
	else
	{
		(*bond_cnt)++;
	}
}

/* STEP 3.3.2 - Define the function to loop through the bond list */
static int setup_accept_list(uint8_t local_id)
{
	int err = bt_le_filter_accept_list_clear();

	if (err)
	{
		LOG_INF("Cannot clear accept list (err: %d)\n", err);
		return err;
	}

	int bond_cnt = 0;

	bt_foreach_bond(local_id, setup_accept_list_cb, &bond_cnt);

	return bond_cnt;
}

void initiate_pair(struct k_work *work)
{

	int err_code = bt_le_adv_stop();
	if (err_code)
	{
		printk("Cannot stop advertising err= %d \n", err_code);
		return;
	}
	err_code = bt_le_filter_accept_list_clear();
	if (err_code)
	{
		printk("Cannot clear accept list (err: %d)\n", err_code);
	}
	else
	{
		printk("Filter Accept List cleared succesfully");
	}
	err_code = bt_le_adv_start(BT_LE_ADV_CONN_NO_ACCEPT_LIST, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	if (err_code)
	{
		printk("Cannot start open advertising (err: %d)\n", err_code);
	}
	else
	{
		printk("Advertising in pairing mode started");
	}
}

/* STEP 3.4.1 - Define the function to advertise with the Accept List */
void advertise_with_acceptlist(struct k_work *work)
{
	int err = 0;
	int allowed_cnt = setup_accept_list(BT_ID_DEFAULT);
	if (allowed_cnt < 0)
	{
		LOG_INF("Acceptlist setup failed (err:%d)\n", allowed_cnt);
	}
	else
	{
		if (allowed_cnt == 0)
		{
			LOG_INF("Advertising with no Accept list \n");
			err = bt_le_adv_start(BT_LE_ADV_CONN_NO_ACCEPT_LIST, ad, ARRAY_SIZE(ad), sd,
								  ARRAY_SIZE(sd));
		}
		else
		{
			LOG_INF("Acceptlist setup number  = %d \n", allowed_cnt);
			err = bt_le_adv_start(BT_LE_ADV_CONN_ACCEPT_LIST, ad, ARRAY_SIZE(ad), sd,
								  ARRAY_SIZE(sd));
		}
		if (err)
		{
			LOG_INF("Advertising failed to start (err %d)\n", err);
			return;
		}
		LOG_INF("Advertising successfully started\n");
	}
}

K_WORK_DEFINE(advertise_acceptlist_work, advertise_with_acceptlist);


static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		LOG_INF("Connection failed (err %u)\n", err);
		return;
	}

	LOG_INF("Connected\n");
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)\n", reason);

	k_work_submit(&advertise_acceptlist_work);
}

static void on_security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err)
	{
		LOG_INF("Security changed: %s level %u\n", addr, level);
	}
	else
	{
		LOG_INF("Security failed: %s level %u err %d\n", addr, level, err);
	}
}
struct bt_conn_cb connection_callbacks = {
	.connected = on_connected,
	.disconnected = on_disconnected,
	.security_changed = on_security_changed,
};

static void auth_passkey_display(struct bt_conn *conn, unsigned int passkey)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Passkey for %s: %06u\n", addr, passkey);
}

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s\n", addr);
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
	.passkey_display = auth_passkey_display,
	.cancel = auth_cancel,
};

/* state configuration change callback function */
static void dclk_ccc_state_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_state_enabled = (value == BT_GATT_CCC_NOTIFY);
}

/* clock configuration change callback function */
static void dclk_ccc_clock_cfg_changed(const struct bt_gatt_attr *attr, uint16_t value)
{
	notify_clock_enabled = (value == BT_GATT_CCC_NOTIFY);
}

static ssize_t read_state(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
						  uint16_t len, uint16_t offset)
{
	// get a pointer to state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	const char *value = attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (dclk_cb.state_cb)
	{
		// Call the application callback function to update the get the current value of the button
		state = dclk_cb.state_cb();
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
	}

	return 0;
}

static ssize_t read_clock(struct bt_conn *conn, const struct bt_gatt_attr *attr, void *buf,
						  uint16_t len, uint16_t offset)
{
	// get a pointer to state which is passed in the BT_GATT_CHARACTERISTIC() and stored in attr->user_data
	const char *value = attr->user_data;

	LOG_DBG("Attribute read, handle: %u, conn: %p", attr->handle, (void *)conn);

	if (dclk_cb.clock_cb)
	{
		// Call the application callback function to update the get the current value of the button
		clock = dclk_cb.clock_cb();
		return bt_gatt_attr_read(conn, attr, buf, len, offset, value, sizeof(*value));
	}

	return 0;
}

/* DCLK Service Declaration */
BT_GATT_SERVICE_DEFINE(
	dclk_svc, BT_GATT_PRIMARY_SERVICE(BT_UUID_DCLK),
	/*Button characteristic declaration */
	BT_GATT_CHARACTERISTIC(BT_UUID_DCLK_STATE, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
						   BT_GATT_PERM_READ_ENCRYPT, read_state, NULL, &state),
	/* Client Characteristic Configuration Descriptor */
	BT_GATT_CCC(dclk_ccc_state_cfg_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),

	BT_GATT_CHARACTERISTIC(BT_UUID_DCLK_CLOCK, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
						   BT_GATT_PERM_READ_ENCRYPT, read_clock, NULL, &clock),

	BT_GATT_CCC(dclk_ccc_clock_cfg_changed, BT_GATT_PERM_READ_ENCRYPT | BT_GATT_PERM_WRITE_ENCRYPT),

);

/* A function to register application callbacks for the LED and Button characteristics  */
int dclk_init(struct dclk_cb *callbacks)
{
	int err;

	bt_conn_cb_register(&connection_callbacks);

	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}
	settings_load();
	if (callbacks)
	{
		dclk_cb.clock_cb = callbacks->clock_cb;
		dclk_cb.state_cb = callbacks->state_cb;
	}

	return 0;
}

void start_advertising(void)
{
	k_work_submit(&advertise_acceptlist_work);
}

int start_pairing(void)
{
	int err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);
	if (err)
	{
		LOG_INF("Cannot delete bond (err: %d)\n", err);
	}
	else
	{
		LOG_INF("Bond deleted succesfully \n");
	}
}

int dclk_send_state_notify(uint8_t state)
{
	if (!notify_state_enabled)
	{
		return -EACCES;
	}

	return bt_gatt_notify(NULL, &dclk_svc.attrs[2], &state, sizeof(state));
}

int dclk_send_clock_notify(uint32_t clock)
{
	if (!notify_clock_enabled)
	{
		return -EACCES;
	}
	return bt_gatt_notify(NULL, &dclk_svc.attrs[5], &clock, sizeof(clock));
}
