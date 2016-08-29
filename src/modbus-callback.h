/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#ifndef MODBUS_CALLBACK_H
#define MODBUS_CALLBACK_H

#include "modbus.h"

MODBUS_BEGIN_DECLS

/* Modbus_Application_Protocol_V1_1b.pdf Chapter 4 Section 1 Page 5
 * RS232 / RS485 ADU = 253 bytes + slave (1 byte) + CRC (2 bytes) = 256 bytes
 */
#define MODBUS_CALLBACK_MAX_ADU_LENGTH  256

// NOTE: select() not currently used
MODBUS_API modbus_t* modbus_new_callback(ssize_t (*callback_send)(modbus_t *ctx, const uint8_t *req, int req_length),
                                         int (*callback_select)(modbus_t *ctx, int msec),
                                         int (*callback_open)(modbus_t *ctx),
                                         void (*callback_close)(modbus_t *ctx),
                                         int (*callback_flush)(modbus_t *ctx)
                                        );

MODBUS_API int modbus_callback_handle_incoming_data(modbus_t *ctx, uint8_t *data, int dataLen);

MODBUS_END_DECLS

#endif /* MODBUS_CALLBACK_H */
