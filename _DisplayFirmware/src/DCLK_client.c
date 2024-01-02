

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

#include "DCLK_client.h"

static unsigned int display_passkey = 123456;

static struct bt_conn *default_conn;
struct bt_DCLK_client DCLK_client;

#define BT_UUID_DCLK BT_UUID_DECLARE_128(BT_UUID_DCLK_VAL)
#define BT_UUID_DCLK_VAL BT_UUID_128_ENCODE(0x00001553, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

// #define LOG_MODULE_NAME DCLK_CLIENT
LOG_MODULE_REGISTER(DCLK_client);

enum
{
	DCLK_INITIALIZED,
	DCLK_CLOCK_NOTIF_ENABLED,
	DCLK_STATE_NOTIF_ENABLED
};



int dclk_client_handles_assign(struct bt_gatt_dm *dm,
							   struct bt_DCLK_client *DCLK_c)
{

	const struct bt_gatt_dm_attr *gatt_service_attr =
		bt_gatt_dm_service_get(dm);
	const struct bt_gatt_service_val *gatt_service =
		bt_gatt_dm_attr_service_val(gatt_service_attr);
	const struct bt_gatt_dm_attr *gatt_chrc;
	const struct bt_gatt_dm_attr *gatt_desc;

	if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_DCLK))
	{
		LOG_INF("Failed");
		return -ENOTSUP;
	}
	LOG_INF("Getting handles from DCLK service.");
	memset(&DCLK_c->handles, 0xFF, sizeof(DCLK_c->handles));

	/* DCLK dclock Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_DCLK_CLOCK);
	if (!gatt_chrc)
	{
		LOG_ERR("Missing DCLK dclock characteristic.");
		return -EINVAL;
	}

	/* DCLK dclock */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_DCLK_CLOCK);
	if (!gatt_desc)
	{
		LOG_ERR("Missing DCLK dclock value descriptor in characteristic.");
		return -EINVAL;
	}
	LOG_INF("Found handle for DCLK dclock characteristic.");
	DCLK_c->handles.dclock = gatt_desc->handle;

	/* DCLK dclock CCC */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc)
	{
		LOG_ERR("Missing DCLK dclock CCC in characteristic.");
		return -EINVAL;
	}
	LOG_INF("Found handle for CCC of DCLK dclock characteristic.");
	DCLK_c->handles.dclock_ccc = gatt_desc->handle;

	/* DCLK dstate Characteristic */
	gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_DCLK_STATE);
	if (!gatt_chrc)
	{
		LOG_ERR("Missing DCLK dstate characteristic.");
		return -EINVAL;
	}

	/* DCLK dstate */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_DCLK_STATE);
	if (!gatt_desc)
	{
		LOG_ERR("Missing DCLK dclock value descriptor in characteristic.");
		return -EINVAL;
	}
	LOG_INF("Found handle for DCLK dstate characteristic.");
	DCLK_c->handles.dstate = gatt_desc->handle;

	/* DCLK dstate CCC */
	gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
	if (!gatt_desc)
	{
		LOG_ERR("Missing DCLK dclock CCC in characteristic.");
		return -EINVAL;
	}
	LOG_INF("Found handle for CCC of DCLK dstate characteristic.");
	DCLK_c->handles.dstate_ccc = gatt_desc->handle;

	/* Assign connection instance. */
	DCLK_c->conn = bt_gatt_dm_conn_get(dm);
	return 0;
}

static uint8_t on_received(struct bt_conn *conn,
						   struct bt_gatt_subscribe_params *params,
						   const void *data, uint16_t length)
{
	struct bt_DCLK_client *DCLK;

	/* Retrieve DCLK Client module context. */
	DCLK = CONTAINER_OF(params, struct bt_DCLK_client, dclock_notif_params);

	if (!data)
	{
		LOG_INF("[UNSUBSCRIBED]");
		params->value_handle = 0;
		atomic_clear_bit(&DCLK->state, DCLK_CLOCK_NOTIF_ENABLED);
		if (DCLK->cb.unsubscribed)
		{
			DCLK->cb.unsubscribed(DCLK);
		}
		return BT_GATT_ITER_STOP;
	}

	uint32_t val = *(uint32_t *)data;
	LOG_INF("[NOTIFICATION] data %lu length %u", val, length);
	if (DCLK->cb.received)
	{
		return DCLK->cb.received(DCLK, data, length);
	}

	return BT_GATT_ITER_CONTINUE;
}

int dclk_client_subscribe(struct bt_DCLK_client *DCLK_c)
{
	int err;

	if (atomic_test_and_set_bit(&DCLK_c->state, DCLK_CLOCK_NOTIF_ENABLED))
	{
		return -EALREADY;
	}

	DCLK_c->dclock_notif_params.notify = on_received;
	DCLK_c->dclock_notif_params.value = BT_GATT_CCC_NOTIFY;
	DCLK_c->dclock_notif_params.value_handle = DCLK_c->handles.dclock;
	DCLK_c->dclock_notif_params.ccc_handle = DCLK_c->handles.dclock_ccc;
	atomic_set_bit(DCLK_c->dclock_notif_params.flags,
				   BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

	err = bt_gatt_subscribe(DCLK_c->conn, &DCLK_c->dclock_notif_params);
	if (err)
	{
		LOG_ERR("Subscribe DCLOCK failed (err %d)", err);
		atomic_clear_bit(&DCLK_c->state, DCLK_CLOCK_NOTIF_ENABLED);
	}
	else
	{
		LOG_DBG("[SUBSCRIBED DCLOCK]");
	}

	return err;
}

static void unsubscribed(struct bt_DCLK_client *DCLK)
{
	LOG_INF("unsubscribe\n");
}

static uint8_t ble_data_received(struct bt_DCLK_client *nus,
								 const uint8_t *data, uint16_t len)
{
	LOG_INF("BLE REC\n");
	return 0;
}

static void discovery_complete(struct bt_gatt_dm *dm,
							   void *context)
{
	LOG_INF("Service discovery completed");
	struct bt_DCLK_client *DCLK = context;

	dclk_client_handles_assign(dm, DCLK);

	dclk_client_subscribe(DCLK);

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
						   BT_UUID_DCLK,
						   &discovery_cb,
						   &DCLK_client);
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
	LOG_INF("Security %d", bt_conn_get_security(conn));

	err = bt_conn_set_security(conn, BT_SECURITY_L4);
	if (err)
	{
		LOG_WRN("Failed to set security: %d", err);

		// gatt_discover(conn);
	}

	err = bt_scan_stop();
	if ((!err) && (err != -EALREADY))
	{
		LOG_ERR("Stop LE scan failed (err %d)", err);
	}

	gatt_discover(conn);
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
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
	.security_changed = security_changed,
};

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

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing completed: %s, bonded: %d", addr, bonded);
}

static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_WRN("Pairing failed conn: %s, reason %d", addr, reason);
}

static struct bt_conn_auth_info_cb conn_auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.pairing_failed = pairing_failed,
};

static void auth_cancel(struct bt_conn *conn)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	LOG_INF("Pairing cancelled: %s\n", addr);
}

static void passkey_confirm(struct bt_conn *conn, unsigned int passkey)
{
}

static struct bt_conn_auth_cb auth_cb_display = {
	.passkey_display = NULL,
	.passkey_entry = NULL,
	.passkey_confirm = passkey_confirm,
	.cancel = auth_cancel,
	.pairing_confirm = NULL, // auth_pairing,
};

int dclk_client_init_2(struct dclk_client_cb *callbacks, unsigned int custom_passkey)
{

	int err;

	err = bt_enable(NULL);
	if (err)
	{
		printk("Bluetooth init failed (err %d)\n", err);
		return 0;
	}
	else
	{
		LOG_INF("Bluetooth initialized");
	}

	display_passkey = custom_passkey;
	err = bt_passkey_set(display_passkey);
	if (err)
	{
		printk("Bluetooth passkey set failed (err %d)\n", err);
		return 0;
	}

	err = bt_conn_auth_cb_register(&auth_cb_display);
	if (err)
	{
		printk("Bluetooth autherization register failed (err %d)\n", err);
		return 0;
	}

	if (IS_ENABLED(CONFIG_SETTINGS))
	{
		settings_load();
	}

	// for Testing
	err = bt_unpair(BT_ID_DEFAULT, BT_ADDR_LE_ANY);

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

	return 0;
}
