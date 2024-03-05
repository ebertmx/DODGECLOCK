
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

/** @brief Initialize the LBS Service.
 *
 * This function registers application callback functions with the My LBS
 * Service
 *
 * @param[in] clock value to display on clock
 *
 *
 * @retval 0 If the operation was successful.
 *           Otherwise, a (negative) error code is returned.
 */
int interface_write_display(uint32_t clock);



#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LBS_H_ */
