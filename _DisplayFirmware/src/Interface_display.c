
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

#include "Interface_display.h"

#include <zephyr/drivers/led_strip.h>
#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/sys/util.h>

#define STRIP_NODE DT_ALIAS(led_strip)
#define STRIP_NUM_PIXELS DT_PROP(DT_ALIAS(led_strip), chain_length)

#define DELAY_TIME K_MSEC(50)

#define RGB(_r, _g, _b)                 \
	{                                   \
		.r = (_r), .g = (_g), .b = (_b) \
	}

static const struct led_rgb colors[] = {
	RGB(0x0f, 0x00, 0x00), /* red */
	RGB(0x00, 0x0f, 0x00), /* green */
	RGB(0x00, 0x00, 0x0f), /* blue */
};

struct led_rgb pixels[STRIP_NUM_PIXELS];

static const struct device *const strip = DEVICE_DT_GET(STRIP_NODE);

#define SW0_NODE DT_NODELABEL(button0)
#define SW1_NODE DT_NODELABEL(button1)
#define SW2_NODE DT_NODELABEL(button2)
#define PAIRING_BUTTON DT_NODELABEL(button3)



struct k_timer d_timer;
static uint8_t d_state = 0;		 // shot clock state
static uint32_t d_clock = 10000; // shot clock in ms

static struct interface_cb inter_cb;
/*DCLOCK*/



static void strip_init(void)
{
	if (device_is_ready(strip))
	{
		LOG_INF("Found LED strip device %s", strip->name);
	}
	else
	{
		LOG_ERR("LED strip device %s is not ready", strip->name);
		return;
	}
}

int interface_init(struct interface_cb *app_cb)
{
	strip_init();

	return 0;
}

int interface_write_display(uint32_t clock)
{
	

}

