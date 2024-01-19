
#ifndef DCLK_INTERFACE
#define DCLK_INTERFACE

/**
 * @file Interface.h
 * @defgroup DCLK_refController
 * @{
 * @brief API for handling button inputs and display outputs
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <zephyr/types.h>
#include <zephyr/drivers/gpio.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/settings/settings.h>
#include "helper_q.h"
	/** @brief
	 *
	 * @param evt the button event which just occurred
	 * 0 release (falling edge)
	 * 1 push (rising edge)
	 * 2 hold (sustained push)
	 */
	typedef uint8_t (*active_func)(uint8_t evt);

	typedef struct button_t
	{
		uint8_t val;
		uint8_t evt;
		uint16_t debounce;
		uint8_t type;

		const struct gpio_dt_spec gpio_spec;
		struct gpio_callback gpio_cb_data;
		gpio_flags_t gpio_flags;
		gpio_callback_handler_t gpio_cb_handler;

		struct k_work btn_work;
		k_work_handler_t btn_work_handler;

		active_func active_func_cb;
	} btn_t;

	typedef struct interface_cb
	{
		active_func pair;
		active_func user;
		active_func start;
		active_func stop;

	} interface_cb;

	/** @brief Initialize the Interface
	 *
	 * This function registers application callback functions with the My LBS
	 * Service
	 *
	 * @param[in] app_cb Struct containing pointers to callback functions
	 *			used for each button on event. This pointer can be NULL
	 *			if no callback functions are defined.
	 *
	 *
	 * @retval 0 If the operation was successful.
	 *           Otherwise, a (negative) error code is returned.
	 */
	int interface_init(struct interface_cb *app_cb);

	/** @brief Send data to be written to SPI display
	 *
	 * This function sends a string to the SPI buffer to be
	 * written to the display
	 *
	 * @param[in] data void pointer to string data. a null value will clear display
	 *
	 * @retval 0 If the operation was successful.
	 *           Otherwise, a (negative) error code is returned.
	 */
	int interface_update(uint32_t *clock, uint8_t *state, char *conn_status);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LBS_H_ */
