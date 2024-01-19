
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

#include <stdint.h>
#include <zephyr/drivers/display.h>
#include <zephyr/display/cfb.h>

#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/pwm.h>

LOG_MODULE_DECLARE(Controller_app, LOG_LEVEL_ERR);

#define DISPLAY_BUFFER_PITCH 128
static const struct device *display = DEVICE_DT_GET(DT_NODELABEL(ssd1306));

#define SW0_NODE DT_NODELABEL(button0)
#define SW1_NODE DT_NODELABEL(button1)
#define SW2_NODE DT_NODELABEL(button2)
#define SW3_NODE DT_NODELABEL(button3)

// PWM

#define MIN_PERIOD PWM_MSEC(2U) / 128U
#define MAX_PERIOD PWM_MSEC(2U)

static const struct pwm_dt_spec pwm_buzz = PWM_DT_SPEC_GET(DT_ALIAS(pwm_led0));

/*


*/
/* Button Event Handler */

void handle_button_event(struct k_work *work)
{
	LOG_DBG("handle_button_event");
	struct button_t *btn = CONTAINER_OF(work, struct button_t, btn_work);
	btn->evt++;
	btn->active_func_cb(btn->evt);
}

/*


*/
/*Callback Buttons*/

static void button_event_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	LOG_DBG("button_event_cb");
	struct button_t *btn = CONTAINER_OF(cb, struct button_t, gpio_cb_data);

	k_work_submit(&btn->btn_work);
}

static void pair_btn_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
}
static void user_btn_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
}
static void start_btn_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
}
static void stop_btn_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
}

/*Button Declarations*/

static struct button_t pair_btn =
	{
		.val = 1,
		.evt = 0,
		.gpio_spec = GPIO_DT_SPEC_GET(SW0_NODE, gpios),
		.gpio_cb_handler = button_event_cb,
		.gpio_flags = GPIO_INT_EDGE_BOTH,

		.btn_work_handler = handle_button_event,
};
static struct button_t user_btn =
	{
		.val = 2,
		.evt = 0,
		.gpio_spec = GPIO_DT_SPEC_GET(SW1_NODE, gpios),
		.gpio_cb_handler = button_event_cb,
		.gpio_flags = GPIO_INT_EDGE_BOTH,
		.btn_work_handler = handle_button_event,

};
static struct button_t start_btn =
	{
		.val = 3,
		.evt = 0,
		.gpio_spec = GPIO_DT_SPEC_GET(SW2_NODE, gpios),
		.gpio_cb_handler = button_event_cb,
		.gpio_flags = GPIO_INT_EDGE_BOTH,

		.btn_work_handler = handle_button_event,
};
static struct button_t stop_btn =
	{
		.val = 4,
		.evt = 0,
		.gpio_spec = GPIO_DT_SPEC_GET(SW3_NODE, gpios),
		.gpio_cb_handler = button_event_cb,
		.gpio_flags = GPIO_INT_EDGE_BOTH,

		.btn_work_handler = handle_button_event,
};

/*


*/
/*Initialize*/

int setup_button(struct button_t *button, active_func user_cb)
{

	/*GPIO*/
	int err = !device_is_ready(button->gpio_spec.port);

	gpio_pin_configure_dt(&button->gpio_spec, GPIO_INPUT | GPIO_PULL_UP);

	/*Interrupt*/

	gpio_pin_interrupt_configure_dt(&button->gpio_spec, button->gpio_flags);

	gpio_init_callback(&button->gpio_cb_data, button->gpio_cb_handler, BIT(button->gpio_spec.pin));

	err = abs(gpio_add_callback(button->gpio_spec.port, &button->gpio_cb_data));

	/* WORK */

	k_work_init(&button->btn_work, handle_button_event);

	button->active_func_cb = user_cb;

	return err;
}

int buzzer_init()
{

	if (!pwm_is_ready_dt(&pwm_buzz))
	{
		LOG_ERR("Error: PWM device %s is not ready\n",
				pwm_buzz.dev->name);
		return 0;
	}

	return 0;
}

int display_init()
{
	int err = cfb_framebuffer_init(display);
	if (err)
	{
		return err;
	}
	err = cfb_framebuffer_set_font(display, 1);
	if (err)
	{
		return err;
	}
	err = cfb_print(display, "DCLK Start", 0, 0);
	if (err)
	{
		return err;
	}
	err = cfb_framebuffer_invert(display);
	if (err)
	{
		return err;
	}
	err = cfb_framebuffer_finalize(display);
	if (err)
	{
		return err;
	}
	err = cfb_framebuffer_clear(display, true);

	return err;
}

int interface_init(struct interface_cb *app_cb)
{
	int err;

	err = setup_button(&pair_btn, app_cb->pair);
	err = setup_button(&user_btn, app_cb->user);
	err = setup_button(&start_btn, app_cb->start);
	err = setup_button(&stop_btn, app_cb->stop);

	if (err)
	{
		LOG_ERR("Failed to setup buttons");
		return err;
	}

	err = display_init();
	if (err)
	{
		LOG_ERR("Failed to setup display");
		return !err;
	}

	err = buzzer_init();
	if (err)
	{
		LOG_ERR("Failed to setup buzzer");
		return !err;
	}

	return 0;
}

static void stop_buzz(struct k_timer *timer_id)
{
	pwm_set_dt(&pwm_buzz, 0, 0);
}

K_TIMER_DEFINE(buzz_timer, stop_buzz, NULL);

void buzz()
{
	pwm_set_dt(&pwm_buzz, MAX_PERIOD, MAX_PERIOD / 2U);
	k_timer_start(&buzz_timer, K_MSEC(500), K_NO_WAIT);
}

static uint32_t dis_clock = 0;
static uint8_t dis_state = 2;
static char dis_num_conn = 0;

int interface_update(uint32_t *clock, uint8_t *state, char *num_conn)
{
	LOG_INF("Update Display");
	int err = 0;
	bool ref = (dis_clock == *clock) && (dis_state == *state) && (dis_num_conn == *num_conn);
	if (!ref)
	{
		dis_clock = *clock;
		dis_state = *state;
		dis_num_conn = *num_conn;
		char str[15];
		if (10 == dis_clock)
		{
			sprintf(str, "T:- S%d %c", dis_state, dis_num_conn);
		}
		else
		{
			sprintf(str, "T:%d S%d %c", dis_clock, dis_state, dis_num_conn);
		}

		int err = cfb_print(display, str, 0, 0);
		if (err)
		{
			LOG_ERR("Failed to print display");
			return err;
		}
		err = cfb_framebuffer_finalize(display);
		if (err)
		{
			LOG_ERR("Failed to write display");
			return err;
		}
		if (dis_clock < 3)
		{
			buzz();
		}
	}
	else
	{
		err = -1;
		LOG_INF("Update not required");
	}

	return err;
}