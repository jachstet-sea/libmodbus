/*
 * Copyright © 2001-2011 Stéphane Raimbault <stephane.raimbault@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1+
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <assert.h>

#include "modbus-private.h"

#include "modbus-callback.h"
#include "modbus-callback-private.h"

#ifdef LOGTAG
#undef LOGTAG
#endif
#define LOGTAG "libmodbusCALLBACK"

/* Table of CRC values for high-order byte */
static const uint8_t table_crc_hi[] = {
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
    0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
    0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
    0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
    0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* Table of CRC values for low-order byte */
static const uint8_t table_crc_lo[] = {
    0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
    0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
    0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
    0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
    0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
    0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
    0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
    0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
    0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
    0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
    0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
    0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
    0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
    0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
    0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
    0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
    0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
    0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
    0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
    0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
    0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
    0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
    0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
    0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
    0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
    0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

/* Define the slave ID of the remote device to talk in master mode or set the
 * internal slave ID in slave mode */
static int _modbus_set_slave(modbus_t *ctx, int slave)
{
    /* Broadcast address is 0 (MODBUS_BROADCAST_ADDRESS) */
    if (slave >= 0 && slave <= 247) {
        ctx->slave = slave;
    } else {
        errno = EINVAL;
        return -1;
    }

    return 0;
}

/* Builds a RTU request header */
static int _modbus_callback_build_request_basis(modbus_t *ctx, int function,
                                                int addr, int nb,
                                                uint8_t *req)
{
    assert(ctx->slave != -1);
    req[0] = ctx->slave;
    req[1] = function;
    req[2] = addr >> 8;
    req[3] = addr & 0x00ff;
    req[4] = nb >> 8;
    req[5] = nb & 0x00ff;

    return _MODBUS_CALLBACK_PRESET_REQ_LENGTH;
}

/* Builds a callback response header */
static int _modbus_callback_build_response_basis(sft_t *sft, uint8_t *rsp)
{
    /* In this case, the slave is certainly valid because a check is already
     * done in _modbus_callback_listen */
    rsp[0] = sft->slave;
    rsp[1] = sft->function;

    return _MODBUS_CALLBACK_PRESET_RSP_LENGTH;
}

/* Calculates the CRC16 checksum of a provided buffer */
static uint16_t crc16(uint8_t *buffer, uint16_t buffer_length)
{
    uint8_t crc_hi = 0xFF; /* high CRC byte initialized */
    uint8_t crc_lo = 0xFF; /* low CRC byte initialized */
    unsigned int i; /* will index into CRC lookup */

    /* pass through message buffer */
    while (buffer_length--) {
        i = crc_hi ^ *buffer++; /* calculate the CRC  */
        crc_hi = crc_lo ^ table_crc_hi[i];
        crc_lo = table_crc_lo[i];
    }

    return (crc_hi << 8 | crc_lo);
}

/* Prepare a transaction id for a response for backends that need it */
static int _modbus_callback_prepare_response_tid(const uint8_t *req, int *req_length)
{
    (*req_length) -= _MODBUS_CALLBACK_CHECKSUM_LENGTH;
    /* No TID */
    return 0;
}

/* Do the final preparations (checksum) before a message is sent */
static int _modbus_callback_send_msg_pre(uint8_t *req, int req_length)
{
    uint16_t crc = crc16(req, req_length);
    req[req_length++] = crc >> 8;
    req[req_length++] = crc & 0x00FF;

    return req_length;
}

/* Called by modbus.c when it wants us to transmit a buffer "to the wire"
 * which in our case it means handing it over to the provided callback
 * function */
static ssize_t _modbus_callback_send(modbus_t *ctx, const uint8_t *req, int req_length)
{
    modbus_callback_t *ctx_callback = (modbus_callback_t*)ctx->backend_data;
    if ((ctx_callback == NULL) || (ctx_callback->callback_send == NULL)) {
      errno = EINVAL;
      return (ssize_t)-1;
    }
    return ctx_callback->callback_send(ctx, req, req_length);
}

/* Called by the backend user when it has received data "from the wire" which
 * we will store into our ring buffer so mobus.c can receive it via the select
 * and recv methods while waiting for a response */
MODBUS_API int modbus_callback_handle_incoming_data(modbus_t *ctx, uint8_t *data, int dataLen)
{
  modbus_callback_t *ctx_callback = (modbus_callback_t*)ctx->backend_data;
  if ((ctx_callback == NULL) || (ctx_callback->rxBuf == NULL)) {
    errno = EINVAL;
    return -1;
  }

  // Assuming we are the only master on the bus, all incoming data are
  // responses to requests issued by us. Therefore, we can safely clear the
  // RX buffer on new, incoming data
  ringbuf_reset(ctx_callback->rxBuf);

  ringbuf_memcpy_into(ctx_callback->rxBuf, data, (size_t)dataLen);

  return dataLen;
}

/* Looks like code only needed when we are slave talking to a master?
 * => UNTESTED since we needed only the case when we are the master */
static int _modbus_callback_receive(modbus_t *ctx, uint8_t *req)
{
    int rc;
    modbus_callback_t *ctx_callback = (modbus_callback_t*)ctx->backend_data;
    if (ctx_callback == NULL) {
      errno = EINVAL;
      return -1;
    }

    if (ctx_callback->confirmation_to_ignore) {
        _modbus_receive_msg(ctx, req, MSG_CONFIRMATION);
        /* Ignore errors and reset the flag */
        ctx_callback->confirmation_to_ignore = FALSE;
        rc = 0;
        if (ctx->debug) {
            printf("Confirmation to ignore\n");
        }
    } else {
        rc = _modbus_receive_msg(ctx, req, MSG_INDICATION);
        if (rc == 0) {
            /* The next expected message is a confirmation to ignore */
            ctx_callback->confirmation_to_ignore = TRUE;
        }
    }
    return rc;
}

/* Called by modbus.c if it knows that there is data available, how much of
 * it and it wants to have the content now */
static ssize_t _modbus_callback_recv(modbus_t *ctx, uint8_t *rsp, int rsp_length)
{
  modbus_callback_t *ctx_callback = (modbus_callback_t*)ctx->backend_data;
  if ((ctx_callback == NULL) || (ctx_callback->rxBuf == NULL)) {
    errno = EINVAL;
    return (ssize_t)-1;
  }

#ifdef ANDROID
  if (ctx->debug)
  {
    __android_log_print(ANDROID_LOG_DEBUG, LOGTAG, "ENTER _modbus_callback_recv !!! Want %d available %d", rsp_length, ringbuf_bytes_used(ctx_callback->rxBuf));
  }
#endif

  if (ringbuf_bytes_used(ctx_callback->rxBuf) == 0)
  {
    return (ssize_t)-1;
  }

  size_t len = (size_t)rsp_length;
  if (ringbuf_bytes_used(ctx_callback->rxBuf) < len)
  {
    len = ringbuf_bytes_used(ctx_callback->rxBuf);
  }

  ringbuf_memcpy_from(rsp, ctx_callback->rxBuf, len);

  return (ssize_t)len;
}

/* This backend's implementation of flush is to simply call the backend's
 * user's flush method */
static int _modbus_callback_flush(modbus_t *ctx)
{
  modbus_callback_t *ctx_callback = (modbus_callback_t*)ctx->backend_data;
  if ((ctx_callback == NULL) || (ctx_callback->callback_flush == NULL)) {
    errno = EINVAL;
    return -1;
  }
  return ctx_callback->callback_flush(ctx);
}

/* Check responding slave is the slave we requested (except for broacast
 * request) */
static int _modbus_callback_pre_check_confirmation(modbus_t *ctx, const uint8_t *req,
                                              const uint8_t *rsp, int rsp_length)
{
    if ((req[0] != rsp[0]) && (req[0] != MODBUS_BROADCAST_ADDRESS)) {
        if (ctx->debug) {
            fprintf(stderr,
                    "The responding slave %d isn't the requested slave %d\n",
                    rsp[0], req[0]);
        }
        errno = EMBBADSLAVE;
        return -1;
    } else {
        return 0;
    }
}

/* The check_crc16 function shall return 0 is the message is ignored and the
   message length if the CRC is valid. Otherwise it shall return -1 and set
   errno to EMBBADCRC. */
static int _modbus_callback_check_integrity(modbus_t *ctx, uint8_t *msg,
                                       const int msg_length)
{
    uint16_t crc_calculated;
    uint16_t crc_received;
    int slave = msg[0];

    /* Filter on the Modbus unit identifier (slave) in callback mode to avoid useless
     * CRC computing. */
    if ((slave != ctx->slave) && (slave != MODBUS_BROADCAST_ADDRESS)) {
        if (ctx->debug) {
            printf("Request for slave %d ignored (not %d)\n", slave, ctx->slave);
        }
        /* Following call to check_confirmation handles this error */
        return 0;
    }

    crc_calculated = crc16(msg, msg_length - 2);
    crc_received = (msg[msg_length - 2] << 8) | msg[msg_length - 1];

    /* Check CRC of msg */
    if (crc_calculated == crc_received) {
        return msg_length;
    } else {
        if (ctx->debug) {
            fprintf(stderr, "ERROR CRC received 0x%0X != CRC calculated 0x%0X\n",
                    crc_received, crc_calculated);
        }

        if (ctx->error_recovery & MODBUS_ERROR_RECOVERY_PROTOCOL) {
            _modbus_callback_flush(ctx);
        }

        errno = EMBBADCRC;
        return -1;
    }
}

/* Initiate a connection. We simply call the open-callback of the backend user
 * so they can do their stuff to open a connection */
static int _modbus_callback_connect(modbus_t *ctx)
{
  modbus_callback_t *ctx_callback = (modbus_callback_t*)ctx->backend_data;
  if ((ctx_callback == NULL) || (ctx_callback->callback_open == NULL)) {
    errno = EINVAL;
    return -1;
  }
  return ctx_callback->callback_open(ctx);
}

/* Close a connection. We simply call the close-callback of the backend user
 *  so they can do their stuff to close the connection */
static void _modbus_callback_close(modbus_t *ctx)
{
    modbus_callback_t *ctx_callback = (modbus_callback_t*)ctx->backend_data;
    if ((ctx_callback == NULL) || (ctx_callback->callback_close == NULL)) {
      errno = EINVAL;
      return;
    }
    ctx_callback->callback_close(ctx);
}

/* Called by modbus.c if it wants know if and how many data available
 * (= has been received) and is waiting to be parsed. The data is NOT
 * actually read from the ringbuffer here */
static int _modbus_callback_select(modbus_t *ctx, fd_set *rset,
                              struct timeval *tv, int length_to_read)
{
  modbus_callback_t *ctx_callback = (modbus_callback_t*)ctx->backend_data;
  if ((ctx_callback == NULL) || (ctx_callback->rxBuf == NULL)) {
    errno = EINVAL;
    return -1;
  }

#ifdef ANDROID
  if (ctx->debug)
  {
    __android_log_print(ANDROID_LOG_DEBUG, LOGTAG, "ENTER _modbus_callback_select !!! Want: %d Available: %d", length_to_read, ringbuf_bytes_used(ctx_callback->rxBuf));
  }
#endif

  long usec = tv->tv_sec*1000000 + tv->tv_usec;

  errno = ETIMEDOUT;
  int ret = -1;

  long i;
  for (i = usec; i > 0; i -= 10000)
  {
#ifdef ANDROID
    if (ctx->debug)
    {
      __android_log_print(ANDROID_LOG_DEBUG, LOGTAG, "POLL _modbus_callback_select POLL !!! Available: %d", ringbuf_bytes_used(ctx_callback->rxBuf));
    }
#endif
    if (ringbuf_bytes_used(ctx_callback->rxBuf) != 0)
    {
      errno = 0;
      return (int)ringbuf_bytes_used(ctx_callback->rxBuf);
    }
    usleep(10000);
  }

#ifdef ANDROID
  if (ctx->debug)
  {
    __android_log_print(ANDROID_LOG_DEBUG, LOGTAG, "_modbus_callback_select !!! TIMEOUT !!!");
  }
#endif
  return ret;
}

/* Free all ressources allocated by this backend */
static void _modbus_callback_free(modbus_t *ctx) {
  modbus_callback_t *ctx_callback = (modbus_callback_t*)ctx->backend_data;
  if ((ctx_callback != NULL) && (ctx_callback->rxBuf != NULL)) {
    ringbuf_free(&ctx_callback->rxBuf);
  }
  free(ctx->backend_data);
  free(ctx);
}

const modbus_backend_t _modbus_callback_backend = {
    _MODBUS_BACKEND_TYPE_CALLBACK,
    _MODBUS_CALLBACK_HEADER_LENGTH,
    _MODBUS_CALLBACK_CHECKSUM_LENGTH,
    MODBUS_CALLBACK_MAX_ADU_LENGTH,
    _modbus_set_slave,
    _modbus_callback_build_request_basis,
    _modbus_callback_build_response_basis,
    _modbus_callback_prepare_response_tid,
    _modbus_callback_send_msg_pre,
    _modbus_callback_send,
    _modbus_callback_receive,
    _modbus_callback_recv,
    _modbus_callback_check_integrity,
    _modbus_callback_pre_check_confirmation,
    _modbus_callback_connect,
    _modbus_callback_close,
    _modbus_callback_flush,
    _modbus_callback_select,
    _modbus_callback_free
};

/* Initialize the callback backend, setting all callback functions */
modbus_t *modbus_new_callback(ssize_t (*callback_send)(modbus_t *ctx, const uint8_t *req, int req_length),
                             int (*callback_select)(modbus_t *ctx, int msec),
                             int (*callback_open)(modbus_t *ctx),
                             void (*callback_close)(modbus_t *ctx),
                             int (*callback_flush)(modbus_t *ctx)
                            )
{
    modbus_t *ctx;
    modbus_callback_t *ctx_callback;

    ctx = (modbus_t *)malloc(sizeof(modbus_t));
    if (ctx == NULL)
    {
      return NULL;
    }
    _modbus_init_common(ctx);
    ctx->backend = &_modbus_callback_backend;
    ctx->backend_data = (modbus_callback_t *)malloc(sizeof(modbus_callback_t));
    if (ctx->backend_data == NULL)
    {
      free(ctx);
      return NULL;
    }
    ctx_callback = (modbus_callback_t *)ctx->backend_data;

    ctx_callback->confirmation_to_ignore = FALSE;

    ctx_callback->rxBuf = ringbuf_new((size_t)65535);
    if (ctx_callback->rxBuf == NULL)
    {
      free(ctx->backend_data);
      free(ctx);
      return NULL;
    }

    // We could check here if one of the provided callbacks is NULL.
    // However, if the backend-using code will never call flush(), there is
    // no need to provide it. Thus, we check for NULL before calling
    // a callback function provided here

    ctx_callback->callback_send = callback_send;
    ctx_callback->callback_select = callback_select;
    ctx_callback->callback_open = callback_open;
    ctx_callback->callback_close = callback_close;
    ctx_callback->callback_flush = callback_flush;

    return ctx;
}
