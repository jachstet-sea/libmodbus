/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef MODBUS_CALLBACK_PRIVATE_H
#define MODBUS_CALLBACK_PRIVATE_H

#include "stdint.h"

#include "ringbuf.h"

#define _MODBUS_CALLBACK_HEADER_LENGTH      1
#define _MODBUS_CALLBACK_PRESET_REQ_LENGTH  6
#define _MODBUS_CALLBACK_PRESET_RSP_LENGTH  2

#define _MODBUS_CALLBACK_CHECKSUM_LENGTH    2

// NOTE: select() not currently used
typedef struct _modbus_callback {
    ssize_t (*callback_send)(modbus_t *ctx, const uint8_t *req, int req_length);
    int (*callback_select)(modbus_t *ctx, int msec);
    int (*callback_open)(modbus_t *ctx);
    void (*callback_close)(modbus_t *ctx);
    int (*callback_flush)(modbus_t *ctx);

    /* To handle many slaves on the same link */
    int confirmation_to_ignore;

    ringbuf_t rxBuf;
} modbus_callback_t;

#endif /* MODBUS_CALLBACK_PRIVATE_H */
