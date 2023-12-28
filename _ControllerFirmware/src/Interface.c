
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

#include "Interface.h"

#define SW0_NODE DT_NODELABEL(button0)
#define SW1_NODE DT_NODELABEL(button1)
#define SW2_NODE DT_NODELABEL(button2)
#define PAIRING_BUTTON DT_NODELABEL(button3)

static const struct gpio_dt_spec button0 = GPIO_DT_SPEC_GET(SW0_NODE, gpios);
static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET(SW1_NODE, gpios);
static const struct gpio_dt_spec userbtn = GPIO_DT_SPEC_GET(SW2_NODE, gpios);
static const struct gpio_dt_spec pairbtn = GPIO_DT_SPEC_GET(PAIRING_BUTTON, gpios);

static struct gpio_callback button0_cb_data;
static struct gpio_callback button1_cb_data;
static struct gpio_callback userbtn_cb_data;
static struct gpio_callback pairbtn_cb_data;

struct k_timer d_timer;
static uint8_t d_state = 0;		 // shot clock state
static uint32_t d_clock = 10000; // shot clock in ms

static struct interface_cb inter_cb;
/*DCLOCK*/

static void d_clock_expire(struct k_timer *timer_id)
{
	d_state = 2;
	d_clock = 0;
	d_clock = 10000;
	k_timer_start(&d_timer, K_MSEC(d_clock), K_NO_WAIT);
}

static void pair_work_cb(void)
{
	inter_cb.pair_cb();
}
K_WORK_DEFINE(initiate_pairing, pair_work_cb);
/*UI*/

static void button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
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
	else if (pins == BIT(userbtn.pin))
	{
		d_state = 0;
		k_timer_start(&d_timer, K_MSEC(d_clock), K_NO_WAIT);
	}

	else if (pins == BIT(pairbtn.pin))
	{
		k_work_submit(&initiate_pairing);
	}
}

uint32_t get_dclock(void)
{
	if (d_state == 0)
	{
		d_clock = k_timer_remaining_get(&d_timer);
	}

	return d_clock;
}

uint32_t get_dstate(void)
{
	return d_state;
}

int interface_init(struct interface_cb *app_cb)
{
	int err;
	err = device_is_ready(button0.port);
	err &= device_is_ready(button1.port);
	err &= device_is_ready(userbtn.port);
	err &= device_is_ready(pairbtn.port);
	if (!err)
	{
		return !err;
	}

	gpio_pin_configure_dt(&button0, GPIO_INPUT);
	gpio_pin_configure_dt(&button1, GPIO_INPUT);
	gpio_pin_configure_dt(&userbtn, GPIO_INPUT);
	gpio_pin_configure_dt(&pairbtn, GPIO_INPUT);

	gpio_pin_interrupt_configure_dt(&button0, GPIO_INT_EDGE_RISING);
	gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_RISING);
	gpio_pin_interrupt_configure_dt(&userbtn, GPIO_INT_EDGE_RISING);
	gpio_pin_interrupt_configure_dt(&pairbtn, GPIO_INT_EDGE_RISING);

	gpio_init_callback(&button0_cb_data, button_pressed, BIT(button0.pin));
	gpio_init_callback(&button1_cb_data, button_pressed, BIT(button1.pin));
	gpio_init_callback(&userbtn_cb_data, button_pressed, BIT(userbtn.pin));
	gpio_init_callback(&pairbtn_cb_data, button_pressed, BIT(pairbtn.pin));

	gpio_add_callback(button0.port, &button0_cb_data);
	gpio_add_callback(button1.port, &button1_cb_data);
	gpio_add_callback(userbtn.port, &userbtn_cb_data);
	gpio_add_callback(pairbtn.port, &pairbtn_cb_data);

	if (app_cb)
	{
		inter_cb.pair_cb = app_cb->pair_cb;
		inter_cb.user_cb = app_cb->user_cb;
	}

	
	d_clock = 10000;
	k_timer_start(&d_timer, K_MSEC(d_clock), K_NO_WAIT);
	return 0;
}

K_TIMER_DEFINE(d_timer, d_clock_expire, NULL);