/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_DCLK
#define BT_DCLK

/**@file
 * @defgroup bt_lbs LED Button Service API
 * @{
 * @brief API for the LED Button Service (LBS).
 */

#ifdef __cplusplus
extern "C" {
#endif

#include <zephyr/types.h>

/** @brief DCLK Service UUID. */
#define BT_UUID_DCLK_VAL BT_UUID_128_ENCODE(0x00001553, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief State Characteristic UUID. */
#define BT_UUID_DCLK_STATE_VAL                                                                     \
	BT_UUID_128_ENCODE(0x00001554, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief LED Characteristic UUID. */
#define BT_UUID_DCLK_LED_VAL BT_UUID_128_ENCODE(0x00001555, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/* STEP 11.1 - Assign a UUID to the MYSENSOR characteristic */
/** @brief Clock Characteristic UUID. */
#define BT_UUID_DCLK_CLOCK_VAL                                                                   \
	BT_UUID_128_ENCODE(0x00001556, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

#define BT_UUID_DCLK BT_UUID_DECLARE_128(BT_UUID_DCLK_VAL)
#define BT_UUID_DCLK_STATE BT_UUID_DECLARE_128(BT_UUID_DCLK_STATE_VAL)
#define BT_UUID_DCLK_LED BT_UUID_DECLARE_128(BT_UUID_DCLK_LED_VAL)
#define BT_UUID_DCLK_CLOCK BT_UUID_DECLARE_128(BT_UUID_DCLK_CLOCK_VAL)

/** @brief Callback type for when an LED state change is received. */
typedef uint32_t (*clock_cb_t)(void);

/** @brief Callback type for when the button state is pulled. */
typedef uint8_t (*state_cb_t)(void);

/** @brief Callback struct used by the LBS Service. */
struct dclk_cb {
	/** LED state change callback. */
	clock_cb_t clock_cb;
	/** Button read callback. */
	state_cb_t state_cb;
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
int dclk_init(struct dclk_cb *callbacks);

void start_advertising(void);

int start_pairing(void);

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
int dclk_send_state_notify(uint8_t state);

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
int dclk_send_clock_notify(uint32_t clock);



#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_LBS_H_ */
