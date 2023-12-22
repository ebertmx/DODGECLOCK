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
#include <dk_buttons_and_leds.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/settings/settings.h>
#include "DCLK.h"

// static struct bt_le_adv_param *adv_param = BT_LE_ADV_PARAM(
// 	(BT_LE_ADV_OPT_CONNECTABLE |
// 	 BT_LE_ADV_OPT_USE_IDENTITY), /* Connectable advertising and use identity address */
// 	1000,						  /* Min Advertising Interval 500ms (800*0.625ms) */
// 	1001,						  /* Max Advertising Interval 500.625ms (801*0.625ms) */
// 	NULL);						  /* Set to NULL for undirected advertising */

LOG_MODULE_REGISTER(DCLK_app, LOG_LEVEL_INF);

#define DEVICE_NAME CONFIG_BT_DEVICE_NAME
#define DEVICE_NAME_LEN (sizeof(DEVICE_NAME) - 1)

#define RUN_STATUS_LED DK_LED1
#define CON_STATUS_LED DK_LED2
#define USER_LED DK_LED3
// #define USER_BUTTON DK_BTN1_MSK

#define SW0_NODE DT_NODELABEL(button0)
#define SW1_NODE DT_NODELABEL(button1)
#define SW2_NODE DT_NODELABEL(button2)

// Pair Button
#define PAIRING_BUTTON DT_NODELABEL(button3)

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(SW1_NODE, gpios);
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET(SW2_NODE, gpios);
static const struct gpio_dt_spec pairbtn = GPIO_DT_SPEC_GET(PAIRING_BUTTON, gpios);

static struct gpio_callback button0_cb_data;
static struct gpio_callback button1_cb_data;
static struct gpio_callback button2_cb_data;
static struct gpio_callback pairbtn_cb_data;

#define STACKSIZE 1024
#define PRIORITY 7

#define RUN_LED_BLINK_INTERVAL 1000

#define SYNC_INTERVAL 300

struct k_timer d_timer;
static uint8_t d_state = 0;		 // shot clock state
static uint32_t d_clock = 10000; // shot clock in ms

static volatile bool reset_flag = false;
static volatile bool pause_flag = false;
static volatile bool resume_flag = false;

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
K_WORK_DEFINE(initiate_pairing, initiate_pair);

static void on_connected(struct bt_conn *conn, uint8_t err)
{
	if (err)
	{
		LOG_INF("Connection failed (err %u)\n", err);
		return;
	}

	LOG_INF("Connected\n");

	dk_set_led_on(CON_STATUS_LED);
}

static void on_disconnected(struct bt_conn *conn, uint8_t reason)
{
	LOG_INF("Disconnected (reason %u)\n", reason);
	dk_set_led_off(CON_STATUS_LED);
	/* STEP 3.5 - Start advertising with Accept List */
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

/*DCLOCK*/

void d_clock_expire(struct k_timer *timer_id)
{
	d_state = 2;
	d_clock = 0;
}

void update_dclock(void)
{
	while (1)
	{
		if (0 == d_state)
		{
			d_clock = k_timer_remaining_get(&d_timer);
		}

		// uint32_t temp_clock = d_clock;
		// uint32_t temp_state = d_state;
		dclk_send_clock_notify(d_clock);
		dclk_send_state_notify(d_state);
		// printk("dclock = %d\n", d_clock);

		k_sleep(K_MSEC(SYNC_INTERVAL));
	}
}

/*UI*/

void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{

	if (pins == BIT(button0.pin))
	{
		d_state = 0;
		d_clock = 10000;
		k_timer_start(&d_timer, K_MSEC(d_clock), K_NO_WAIT);
	}
	else if (pins == BIT(button1.pin))
	{
		d_state = 2;
		d_clock = 0;
		k_timer_stop(&d_timer);
	}
	else if (pins == BIT(button2.pin))
	{
		d_state = 0;
		k_timer_start(&d_timer, K_MSEC(d_clock), K_NO_WAIT);
	}

	else if (pins == BIT(pairbtn.pin))
	{
		k_work_submit(&initiate_pairing);
	}
}

static int init_button(void)
{
	int err;
	err = device_is_ready(button0.port);
	err &= device_is_ready(button1.port);
	err &= device_is_ready(button2.port);
	err &= device_is_ready(pairbtn.port);
	if (!err)
	{
		return !err;
	}

	gpio_pin_configure_dt(&button0, GPIO_INPUT);
	gpio_pin_configure_dt(&button1, GPIO_INPUT);
	gpio_pin_configure_dt(&button2, GPIO_INPUT);
	gpio_pin_configure_dt(&pairbtn, GPIO_INPUT);

	gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_EDGE_RISING);
	gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_RISING);
	gpio_pin_interrupt_configure_dt(&button2, GPIO_INT_EDGE_RISING);
	gpio_pin_interrupt_configure_dt(&pairbtn, GPIO_INT_EDGE_RISING);

	gpio_init_callback(&button0_cb_data, button_pressed, BIT(button0.pin));
	gpio_init_callback(&button1_cb_data, button_pressed, BIT(button1.pin));
	gpio_init_callback(&button2_cb_data, button_pressed, BIT(button2.pin));
	gpio_init_callback(&pairbtn_cb_data, button_pressed, BIT(pairbtn.pin));

	gpio_add_callback(button0.port, &button0_cb_data);
	gpio_add_callback(button1.port, &button1_cb_data);
	gpio_add_callback(button2.port, &button2_cb_data);
	gpio_add_callback(pairbtn.port, &pairbtn_cb_data);

	return 0;
}

/*DCLK Service and BLE*/
static uint32_t app_clock_cb(void)
{
	return d_clock;
}

static uint8_t app_state_cb(void)
{
	return d_state;
}

static struct dclk_cb app_callbacks = {
	.clock_cb = app_clock_cb,
	.state_cb = app_state_cb,
};

void main(void)
{
	int blink_status = 0;
	int err;

	LOG_INF("Starting Lesson 4 - Exercise 2 \n");

	err = dk_leds_init();
	if (err)
	{
		LOG_ERR("LEDs init failed (err %d)\n", err);
		return;
	}

	err = init_button();
	if (err)
	{
		printk("Button init failed (err %d)\n", err);
		return;
	}

	err = bt_conn_auth_cb_register(&conn_auth_callbacks);
	if (err)
	{
		LOG_INF("Failed to register authorization callbacks.\n");
		return;
	}
	bt_conn_cb_register(&connection_callbacks);

	err = bt_enable(NULL);
	if (err)
	{
		LOG_ERR("Bluetooth init failed (err %d)\n", err);
		return;
	}
	settings_load();

	err = dclk_init(&app_callbacks);
	if (err)
	{
		printk("Failed to init LBS (err:%d)\n", err);
		return;
	}
	LOG_INF("Bluetooth initialized\n");

	k_work_submit(&advertise_acceptlist_work);



	// err = bt_le_adv_start(adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
	// if (err)
	// {
	// 	LOG_ERR("Advertising failed to start (err %d)\n", err);
	// 	return;
	// }

	// LOG_INF("Advertising successfully started\n");
	for (;;)
	{
		dk_set_led(RUN_STATUS_LED, (++blink_status) % 2);
		k_sleep(K_MSEC(RUN_LED_BLINK_INTERVAL));
	}
}

// Start timer thread
K_TIMER_DEFINE(d_timer, d_clock_expire, NULL);
K_THREAD_DEFINE(dclock_update, STACKSIZE, update_dclock, NULL, NULL, NULL, PRIORITY, 0, 0);
