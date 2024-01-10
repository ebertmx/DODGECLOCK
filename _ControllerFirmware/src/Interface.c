
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


#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
LOG_MODULE_DECLARE(Controller_app, LOG_LEVEL_DBG);

#define SW0_NODE DT_NODELABEL(button0)
#define SW1_NODE DT_NODELABEL(button1)
#define SW2_NODE DT_NODELABEL(button2)
#define SW3_NODE DT_NODELABEL(button3)

/*


*/
/* Button Event Handler */

void handle_button_event(struct k_work *work)
{

	struct button_t * btn = CONTAINER_OF(work, struct button_t, btn_work);
	LOG_INF("handle_button_event");

}

/*


*/
/*Callback Buttons*/

static void button_event_cb(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	struct button_t * btn = CONTAINER_OF(cb, struct button_t, gpio_cb_data);
	LOG_INF("button_event_cb");
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
		.gpio_spec = GPIO_DT_SPEC_GET(SW0_NODE, gpios),
		.gpio_cb_handler = button_event_cb,
		.gpio_flags = GPIO_INT_EDGE_BOTH,

		.btn_work_handler = handle_button_event,
};
static struct button_t user_btn =
	{
		.val = 2,
		.gpio_spec = GPIO_DT_SPEC_GET(SW1_NODE, gpios),
		.gpio_cb_handler = button_event_cb,
		.gpio_flags = GPIO_INT_EDGE_BOTH,

		.btn_work_handler = handle_button_event,

};
static struct button_t start_btn =
	{
		.val = 3,
		.gpio_spec = GPIO_DT_SPEC_GET(SW2_NODE, gpios),
		.gpio_cb_handler = button_event_cb,
		.gpio_flags = GPIO_INT_EDGE_BOTH,

		.btn_work_handler = handle_button_event,
};
static struct button_t stop_btn =
	{
		.val = 4,
		.gpio_spec = GPIO_DT_SPEC_GET(SW3_NODE, gpios),
		.gpio_cb_handler = button_event_cb,
		.gpio_flags = GPIO_INT_EDGE_BOTH,


		.btn_work_handler = handle_button_event,
};


/*


*/
/*Initialize*/

int setup_button(struct button_t *button)
{

	/*GPIO*/
	int err = device_is_ready(button->gpio_spec.port);

	gpio_pin_configure_dt(&button->gpio_spec, GPIO_INPUT | GPIO_PULL_UP);

	/*Interrupt*/

	gpio_pin_interrupt_configure_dt(&button->gpio_spec, button->gpio_flags);

	gpio_init_callback(&button->gpio_cb_data, button->gpio_cb_handler, BIT(button->gpio_spec.pin));

	err = gpio_add_callback(button->gpio_spec.port, &button->gpio_cb_data);

	/* WORK */

	k_work_init(&button->btn_work, handle_button_event);

	return err;
}

int interface_init(struct interface_cb *app_cb)
{
	int err;

	err = setup_button(&pair_btn);
	err = setup_button(&user_btn);
	err = setup_button(&start_btn);
	err = setup_button(&stop_btn);

	if (!err)
	{
		return !err;
	}

	// if (app_cb)
	// {
	// 	inter_cb.pair_cb = app_cb->pair_cb;
	// 	inter_cb.user_cb = app_cb->user_cb;
	// }

	return 0;
}
