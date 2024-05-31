/*
 * Matthew Ebert
 *
 * 02-JAN-2024
 * 
 */

#ifndef BT_DCLK
#define BT_DCLK

/**@file
 * @defgroup DCLK.h DCLK service API
 * @{
 * @brief API for the DCLK BLE service
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <zephyr/types.h>

/** @brief DCLK Service UUID. */
#define BT_UUID_DCLK_VAL BT_UUID_128_ENCODE(0x00001553, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief State Characteristic UUID. */
#define BT_UUID_DCLK_STATE_VAL \
	BT_UUID_128_ENCODE(0x00001554, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief LED Characteristic UUID. */
#define BT_UUID_DCLK_LED_VAL BT_UUID_128_ENCODE(0x00001555, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/* STEP 11.1 - Assign a UUID to the MYSENSOR characteristic */
/** @brief Clock Characteristic UUID. */
#define BT_UUID_DCLK_CLOCK_VAL \
	BT_UUID_128_ENCODE(0x00001556, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

#define BT_UUID_DCLK BT_UUID_DECLARE_128(BT_UUID_DCLK_VAL)
#define BT_UUID_DCLK_STATE BT_UUID_DECLARE_128(BT_UUID_DCLK_STATE_VAL)
#define BT_UUID_DCLK_LED BT_UUID_DECLARE_128(BT_UUID_DCLK_LED_VAL)
#define BT_UUID_DCLK_CLOCK BT_UUID_DECLARE_128(BT_UUID_DCLK_CLOCK_VAL)


/** @brief Struct defining DCLK state */
	typedef struct dclk_info
	{
		/** number of active BLE connection */
		uint8_t num_conn;
		/** pairing status */
		bool pair_en;

	}dclk_info;


	
	/** @brief get status of DCLK service
	 *
	 * This function registers application callback functions with the DCLK
	 * Service
	 *
	 * @param[in] dclk_info a struct point to pass back values
	 *
	 *
	 * @retval 0If the operation was successful.
	 *           Otherwise, a (negative) error code is returned.
	 */
	int dclk_get_status(struct dclk_info *status);

	/** @brief Callback type for when an clock is read. */
	typedef uint32_t (*clock_cb_t)(void);

	/** @brief Callback type for when state is read. */
	typedef uint8_t (*state_cb_t)(void);



	/** @brief Callback struct used by DCLK Service. */
	struct dclk_cb
	{
		/** clock read callback. */
		clock_cb_t clock_cb;
		/** state read callback. */
		state_cb_t state_cb;


	};

	/** @brief Initialize the DCLK Service.
	 *
	 * This function registers application callback functions with the DCLK
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
	int dclk_init(struct dclk_cb *callbacks);

	/** @brief Enables/Disables advertizing and
	 * pairing of DCLK blueooth service
	 *
	 * This starts or stops advertizing immediately and
	 * sets the pairing state to enable
	 *
	 * @param[in] enable pairing or not
	 *
	 * @retval 0 If the operation was successful.
	 *           Otherwise, a (negative) error code is returned.
	 */
	int dclk_pairing(bool enable);

	/** @brief Send the clock state as notification.
	 *
	 * This function sends a uint8_t state. The state can be
	 * 0 - running
	 * 1 - paused
	 * 2 - stopped
	 *
	 * @param[in] state The state of the clock
	 *
	 * @retval 0 If the operation was successful.
	 *           Otherwise, a (negative) error code is returned.
	 */
	int dclk_send_state_notify(uint8_t *state);

	/** @brief Send the clock value as notification.
	 *
	 * This function sends an uint32_t  value indicating the
	 * time remaining on the shot clock
	 *
	 * @param[in] clock The value of the shot clock in ms
	 *
	 * @retval 0 If the operation was successful.
	 *           Otherwise, a (negative) error code is returned.
	 */
	int dclk_send_clock_notify(uint32_t *clock);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LBS_H_ */
