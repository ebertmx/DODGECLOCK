
#ifndef DCLK_INTERFACE
#define DCLK_INTERFACE

/**@file
 * @defgroup bt_lbs LED Button Service API
 * @{
 * @brief API for the LED Button Service (LBS).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

/** @brief Callback type for when an LED state change is received. */
typedef uint8_t (*pair_cb_t)(void);

/** @brief Callback type for when the button state is pulled. */
typedef uint8_t (*user_cb_t)(void);

/** @brief Callback struct used by the LBS Service. */
struct interface_cb {
	/** LED state change callback. */
	pair_cb_t pair_cb;
	/** Button read callback. */
	user_cb_t user_cb;
};



/** @brief Initialize the LBS Service.
 *
 * This function registers application callback functions with the My LBS
 * Service
 *
 * @param[in] callbacks Struct containing pointers to callback functions
 *			used by the service. This pointer can be NULL
 *			if no callback functions are defined.
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int interface_init(struct interface_cb *app_cb);


/** @brief Send the button state as notification.
 *
 * This function sends a binary state, typically the state of a
 * button, to all connected peers.
 *
 * @param[in] state The state of the button.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
uint32_t get_dclock (void);

/** @brief Send the sensor value as notification.
 *
 * This function sends an uint32_t  value, typically the value
 * of a simulated sensor to all connected peers.
 *
 * @param[in] clock The value of the simulated sensor.
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
uint8_t get_dstate (void);



#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LBS_H_ */
