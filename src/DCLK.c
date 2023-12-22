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

#include "DCLK.h"

LOG_MODULE_DECLARE(DCLK_app);

static bool notify_state_enabled;
static bool notify_clock_enabled;
static uint8_t state;
static int32_t clock;
static struct dclk_cb dclk_cb;

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
						   BT_GATT_PERM_READ, read_state, NULL, &state),
	/* Client Characteristic Configuration Descriptor */
	BT_GATT_CCC(dclk_ccc_state_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

	BT_GATT_CHARACTERISTIC(BT_UUID_DCLK_CLOCK, BT_GATT_CHRC_READ | BT_GATT_CHRC_NOTIFY,
						   BT_GATT_PERM_READ, read_clock, NULL, &clock),

	BT_GATT_CCC(dclk_ccc_clock_cfg_changed, BT_GATT_PERM_READ | BT_GATT_PERM_WRITE),

);

/* A function to register application callbacks for the LED and Button characteristics  */
int dclk_init(struct dclk_cb *callbacks)
{
	if (callbacks)
	{
		dclk_cb.clock_cb = callbacks->clock_cb;
		dclk_cb.state_cb = callbacks->state_cb;
	}

	return 0;
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
