/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef BT_DCLK_CLIENT_H_
#define BT_DCLK_CLIENT_H_

/**
 * @file
 * @defgroup bt_DCLK_client Bluetooth LE GATT DCLK Client API
 * @{
 * @brief API for the Bluetooth LE GATT Nordic UART Service (DCLK) Client.
 */

#ifdef __cplusplus
extern "C"
{
#endif

#include <zephyr/bluetooth/gatt.h>
#include <zephyr/bluetooth/conn.h>
#include <bluetooth/gatt_dm.h>

/** @brief DCLK Service UUID. */
#define BT_UUID_DCLK_VAL BT_UUID_128_ENCODE(0x00001553, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/** @brief State Characteristic UUID. */
#define BT_UUID_DCLK_STATE_VAL \
    BT_UUID_128_ENCODE(0x00001554, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

/* STEP 11.1 - Assign a UUID to the MYSENSOR characteristic */
/** @brief Clock Characteristic UUID. */
#define BT_UUID_DCLK_CLOCK_VAL \
    BT_UUID_128_ENCODE(0x00001556, 0x1212, 0xefde, 0x1523, 0x785feabcd123)

#define BT_UUID_DCLK BT_UUID_DECLARE_128(BT_UUID_DCLK_VAL)
#define BT_UUID_DCLK_STATE BT_UUID_DECLARE_128(BT_UUID_DCLK_STATE_VAL)
#define BT_UUID_DCLK_CLOCK BT_UUID_DECLARE_128(BT_UUID_DCLK_CLOCK_VAL)

    /** @brief Handles on the connected peer device that are needed to interact with
     * the device.
     */
    struct bt_DCLK_client_handles
    {

        /** Handle of the DCLK dstate characteristic, as provided by
         *  a discovery.
         */
        uint16_t dstate;

        uint16_t dclock_ccc;

        /** Handle of the DCLK dclock characteristic, as provided by
         *  a discovery.
         */
        uint16_t dclock;

        uint16_t dstate_ccc;
    };

    struct bt_DCLK_client;

    /** @brief DCLK Client callback structure. */
    struct bt_DCLK_client_cb
    {
        /** @brief Data received callback.
         *
         * The data has been received as a notification of the DCLK dclock
         * Characteristic.
         *
         * @param[in] DCLK  DCLK Client instance.
         * @param[in] data Received data.
         * @param[in] len Length of received data.
         *
         * @retval BT_GATT_ITER_CONTINUE To keep notifications enabled.
         * @retval BT_GATT_ITER_STOP To disable notifications.
         */
        uint8_t (*received)(struct bt_DCLK_client *DCLK, const uint8_t *data, uint16_t len);


        /** @brief dclock notifications disabled callback.
         *
         * dclock notifications have been disabled.
         *
         * @param[in] DCLK  DCLK Client instance.
         */
        void (*unsubscribed)(struct bt_DCLK_client *DCLK);
    };

    /** @brief DCLK Client structure. */
    struct bt_DCLK_client
    {

        /** Connection object. */
        struct bt_conn *conn;

        /** Internal state. */
        atomic_t state;

        /** Handles on the connected peer device that are needed
         * to interact with the device.
         */
        struct bt_DCLK_client_handles handles;

        /** GATT subscribe parameters for DCLK dclock Characteristic. */
        struct bt_gatt_subscribe_params dclock_notif_params;

        /** GATT write parameters for DCLK dstate Characteristic. */
        struct bt_gatt_subscribe_params dstate_notif_params;

        /** Application callbacks. */
        struct bt_DCLK_client_cb cb;
    };

    /** @brief DCLK Client initialization structure. */
    struct bt_DCLK_client_init_param
    {

        /** Callbacks provided by the user. */
        struct bt_DCLK_client_cb cb;
    };

    /** @brief Initialize the DCLK Client module.
     *
     * This function initializes the DCLK Client module with callbacks provided by
     * the user.
     *
     * @param[in,out] DCLK    DCLK Client instance.
     * @param[in] init_param DCLK Client initialization parameters.
     *
     * @retval 0 If the operation was successful.
     *           Otherwise, a negative error code is returned.
     */
    int bt_DCLK_client_init(struct bt_DCLK_client *DCLK,
                            const struct bt_DCLK_client_init_param *init_param);

    
    /** @brief Assign handles to the DCLK Client instance.
     *
     * This function should be called when a link with a peer has been established
     * to associate the link to this instance of the module. This makes it
     * possible to handle several links and associate each link to a particular
     * instance of this module. The GATT attribute handles are provided by the
     * GATT DB discovery module.
     *
     * @param[in] dm Discovery object.
     * @param[in,out] DCLK DCLK Client instance.
     *
     * @retval 0 If the operation was successful.
     * @retval (-ENOTSUP) Special error code used when UUID
     *         of the service does not match the expected UUID.
     * @retval Otherwise, a negative error code is returned.
     */
    int bt_DCLK_handles_assign(struct bt_gatt_dm *dm,
                               struct bt_DCLK_client *DCLK);

    /** @brief Request the peer to start sending notifications for the dclock
     *	   Characteristic.
     *
     * This function enables notifications for the DCLK dclock Characteristic at the peer
     * by writing to the CCC descriptor of the DCLK dclock Characteristic.
     *
     * @param[in,out] DCLK DCLK Client instance.
     *
     * @retval 0 If the operation was successful.
     *           Otherwise, a negative error code is returned.
     */
    int bt_DCLK_subscribe_receive(struct bt_DCLK_client *DCLK);

#ifdef __cplusplus
}
#endif

/**
 * @}
 */

#endif /* BT_DCLK_CLIENT_H_ */