/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/types.h>
#include <stddef.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>
#include <bluetooth/gatt_dm.h>

#include <bluetooth/scan.h>

#include <zephyr/settings/settings.h>




#include <zephyr/logging/log.h>

#define LOG_MODULE_NAME DCLK_DISPLAY
LOG_MODULE_REGISTER(LOG_MODULE_NAME);
#define BT_UUID_DCLK BT_UUID_DECLARE_128(BT_UUID_DCLK_VAL)
#define BT_UUID_DCLK_VAL BT_UUID_128_ENCODE(0x00001553, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

static struct bt_conn *default_conn;

static void discovery_complete(struct bt_gatt_dm *dm,
							   void *context)
{
	LOG_INF("Service discovery completed");

	bt_gatt_dm_data_print(dm);

	bt_gatt_dm_data_release(dm);
}

static void discovery_service_not_found(struct bt_conn *conn,
										void *context)
{
	LOG_INF("Service not found");
}

static void discovery_error(struct bt_conn *conn,
							int err,
							void *context)
{
	LOG_WRN("Error while discovering GATT database: (%d)", err);
}

struct bt_gatt_dm_cb discovery_cb = {
	.completed = discovery_complete,
	.service_not_found = discovery_service_not_found,
	.error_found = discovery_error,
};

static void gatt_discover(struct bt_conn *conn)
{
	int err;

	if (conn != default_conn)
	{
		return;
	}

	err = bt_gatt_dm_start(conn,
						   NULL,
						   &discovery_cb,
						   NULL);
	if (err)
	{
		LOG_ERR("could not start the discovery procedure, error "
				"code: %d",
				err);
	}
}

static void exchange_func(struct bt_conn *conn, uint8_t err, struct bt_gatt_exchange_params *params)
{
	if (!err)
	{
		LOG_INF("MTU exchange done");
	}
	else
	{
		LOG_WRN("MTU exchange failed (err %" PRIu8 ")", err);
	}
}

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err)
	{
		LOG_INF("Failed to connect to %s (%d)", addr, conn_err);

		if (default_conn == conn)
		{
			bt_conn_unref(default_conn);
			default_conn = NULL;

			err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
			if (err)
			{
				LOG_ERR("Scanning failed to start (err %d)",
						err);
			}
		}

		return;
	}

	LOG_INF("Connected: %s", addr);

	static struct bt_gatt_exchange_params exchange_params;

	exchange_params.func = exchange_func;
	err = bt_gatt_exchange_mtu(conn, &exchange_params);
	if (err)
	{
		LOG_WRN("MTU exchange failed (err %d)", err);
	}

	err = bt_conn_set_security(conn, BT_SECURITY_L2);
	if (err)
	{
		LOG_WRN("Failed to set security: %d", err);

		gatt_discover(conn);
	}

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY))
	{
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];
	int err;

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Disconnected: %s (reason %u)", addr, reason);

	if (default_conn != conn)
	{
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err)
	{
		LOG_ERR("Scanning failed to start (err %d)",
				err);
	}
}

static void security_changed(struct bt_conn *conn, bt_security_t level,
							 enum bt_security_err err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (!err)
	{
		LOG_INF("Security changed: %s level %u", addr, level);
	}
	else
	{
		LOG_WRN("Security failed: %s level %u err %d", addr,
				level, err);
	}

	gatt_discover(conn);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed};

static void scan_filter_match(struct bt_scan_device_info *device_info,
							  struct bt_scan_filter_match *filter_match,
							  bool connectable)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(device_info->recv_info->addr, addr, sizeof(addr));

	LOG_INF("Filters matched. Address: %s connectable: %d",
			addr, connectable);
}

static void scan_connecting_error(struct bt_scan_device_info *device_info)
{
	LOG_WRN("Connecting failed");
}

static void scan_connecting(struct bt_scan_device_info *device_info,
							struct bt_conn *conn)
{
	default_conn = bt_conn_ref(conn);
}

BT_SCAN_CB_INIT(scan_cb, scan_filter_match, NULL,
				scan_connecting_error, scan_connecting);

static int scan_init(void)
{
	int err;
	struct bt_scan_init_param scan_init = {
		.connect_if_match = 1,
	};

	bt_scan_init(&scan_init);
	bt_scan_cb_register(&scan_cb);

	err = bt_scan_filter_add(BT_SCAN_FILTER_TYPE_UUID, BT_UUID_DCLK);
	if (err)
	{
		LOG_ERR("Scanning filters cannot be set (err %d)", err);
		return err;
	}

	err = bt_scan_filter_enable(BT_SCAN_UUID_FILTER, false);
	if (err)
	{
		LOG_ERR("Filters cannot be turned on (err %d)", err);
		return err;
	}

	LOG_INF("Scan module initialized");
	return err;
}

int bluetooth_init(void)
{

	int err;

	err = bt_enable(NULL);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}

	LOG_INF("Bluetooth initialized");

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		settings_load();
	}

	err = scan_init();
	if (err != 0)
	{
		LOG_ERR("scan_init failed (err %d)", err);
		return 0;
	}

	err = bt_scan_start(BT_SCAN_TYPE_SCAN_ACTIVE);
	if (err)
	{
		LOG_ERR("Scanning failed to start (err %d)", err);
		return 0;
	}

	LOG_INF("Scanning successfully started");

	while (1)
	{
		k_sleep(K_FOREVER);
	}
	return 0;
}