/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/logging/log.h>

#include "DCLK_client.h"

#define LOG_MODULE_NAME DCLK_CLIENT
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

enum
{
    DCLK_C_INITIALIZED,
    DCLK_C_dclock_NOTIF_ENABLED,
    DCLK_C_dstate_WRITE_PENDING
};

int bt_DCLK_client_init(struct bt_DCLK_client *DCLK_c,
                        const struct bt_DCLK_client_init_param *DCLK_c_init)
{
    if (!DCLK_c || !DCLK_c_init)
    {
        return -EINVAL;
    }

    if (atomic_test_and_set_bit(&DCLK_c->state, DCLK_C_INITIALIZED))
    {
        return -EALREADY;
    }

    memcpy(&DCLK_c->cb, &DCLK_c_init->cb, sizeof(DCLK_c->cb));

    return 0;
}

int bt_DCLK_handles_assign(struct bt_gatt_dm *dm,
                           struct bt_DCLK_client *DCLK_c)
{

    const struct bt_gatt_dm_attr *gatt_service_attr =
        bt_gatt_dm_service_get(dm);
    const struct bt_gatt_service_val *gatt_service =
        bt_gatt_dm_attr_service_val(gatt_service_attr);
    const struct bt_gatt_dm_attr *gatt_chrc;
    const struct bt_gatt_dm_attr *gatt_desc;

    if (bt_uuid_cmp(gatt_service->uuid, BT_UUID_DCLK))
    {
        LOG_INF("Failed");
        return -ENOTSUP;
    }
    LOG_INF("Getting handles from DCLK service.");
    memset(&DCLK_c->handles, 0xFF, sizeof(DCLK_c->handles));

    /* DCLK dclock Characteristic */
    gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_DCLK_CLOCK);
    if (!gatt_chrc)
    {
        LOG_ERR("Missing DCLK dclock characteristic.");
        return -EINVAL;
    }

    /* DCLK dclock */
    gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_DCLK_CLOCK);
    if (!gatt_desc)
    {
        LOG_ERR("Missing DCLK dclock value descriptor in characteristic.");
        return -EINVAL;
    }
    LOG_INF("Found handle for DCLK dclock characteristic.");
    DCLK_c->handles.dclock = gatt_desc->handle;

    /* DCLK dclock CCC */
    gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
    if (!gatt_desc)
    {
        LOG_ERR("Missing DCLK dclock CCC in characteristic.");
        return -EINVAL;
    }
    LOG_INF("Found handle for CCC of DCLK dclock characteristic.");
    DCLK_c->handles.dclock_ccc = gatt_desc->handle;

    /* DCLK dstate Characteristic */
    gatt_chrc = bt_gatt_dm_char_by_uuid(dm, BT_UUID_DCLK_STATE);
    if (!gatt_chrc)
    {
        LOG_ERR("Missing DCLK dstate characteristic.");
        return -EINVAL;
    }

    /* DCLK dstate */
    gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_DCLK_STATE);
    if (!gatt_desc)
    {
        LOG_ERR("Missing DCLK dclock value descriptor in characteristic.");
        return -EINVAL;
    }
    LOG_INF("Found handle for DCLK dstate characteristic.");
    DCLK_c->handles.dstate = gatt_desc->handle;

    /* DCLK dstate CCC */
    gatt_desc = bt_gatt_dm_desc_by_uuid(dm, gatt_chrc, BT_UUID_GATT_CCC);
    if (!gatt_desc)
    {
        LOG_ERR("Missing DCLK dclock CCC in characteristic.");
        return -EINVAL;
    }
    LOG_INF("Found handle for CCC of DCLK dstate characteristic.");
    DCLK_c->handles.dstate_ccc = gatt_desc->handle;

    /* Assign connection instance. */
    DCLK_c->conn = bt_gatt_dm_conn_get(dm);
    return 0;
}



static uint8_t on_received(struct bt_conn *conn,
                           struct bt_gatt_subscribe_params *params,
                           const void *data, uint16_t length)
{
    struct bt_DCLK_client *DCLK;

    /* Retrieve DCLK Client module context. */
    DCLK = CONTAINER_OF(params, struct bt_DCLK_client, dclock_notif_params);

    if (!data)
    {
        LOG_INF("[UNSUBSCRIBED]");
        params->value_handle = 0;
        atomic_clear_bit(&DCLK->state, DCLK_C_dclock_NOTIF_ENABLED);
        if (DCLK->cb.unsubscribed)
        {
            DCLK->cb.unsubscribed(DCLK);
        }
        return BT_GATT_ITER_STOP;
    }

    uint32_t val = *(uint32_t*)data;
    LOG_INF("[NOTIFICATION] %p data %lu length %u", params->value, val, length);
    if (DCLK->cb.received)
    {
        return DCLK->cb.received(DCLK, data, length);
    }

    return BT_GATT_ITER_CONTINUE;
}

int bt_DCLK_subscribe_receive(struct bt_DCLK_client *DCLK_c)
{
    int err;

    if (atomic_test_and_set_bit(&DCLK_c->state, DCLK_C_dclock_NOTIF_ENABLED))
    {
        return -EALREADY;
    }

    DCLK_c->dclock_notif_params.notify = on_received;
    DCLK_c->dclock_notif_params.value = BT_GATT_CCC_NOTIFY;
    DCLK_c->dclock_notif_params.value_handle = DCLK_c->handles.dclock;
    DCLK_c->dclock_notif_params.ccc_handle = DCLK_c->handles.dclock_ccc;
    atomic_set_bit(DCLK_c->dclock_notif_params.flags,
                   BT_GATT_SUBSCRIBE_FLAG_VOLATILE);

    err = bt_gatt_subscribe(DCLK_c->conn, &DCLK_c->dclock_notif_params);
    if (err)
    {
        LOG_ERR("Subscribe DCLOCK failed (err %d)", err);
        atomic_clear_bit(&DCLK_c->state, DCLK_C_dclock_NOTIF_ENABLED);
    }
    else
    {
        LOG_DBG("[SUBSCRIBED DCLOCK]");
    }

    return err;
}