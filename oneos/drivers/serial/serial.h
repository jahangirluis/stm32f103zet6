/**
 ***********************************************************************************************************************
 * Copyright (c) 2020, China Mobile Communications Group Co.,Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with 
 * the License. You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
 * an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations under the License.
 *
 * @file        serial.h
 *
 * @brief       This file provides struct/macro definition and serial functions declaration.
 *
 * @revision
 * Date         Author          Notes
 * 2020-02-20   OneOS Team      First Version
 ***********************************************************************************************************************
 */

#ifndef __SERIAL_H__
#define __SERIAL_H__

#include <os_task.h>
#include <os_workqueue.h>
#include <os_sem.h>
#include <device.h>
#include <ring_buff.h>

#if defined(OS_SERIAL_DELAY_CLOSE) && !defined(OS_USING_SYSTEM_WORKQUEUE)
#error: OS_SERIAL_DELAY_CLOSE function need OS_USING_SYSTEM_WORKQUEUE defined or disable OS_SERIAL_DELAY_CLOSE
#endif

#define BAUD_RATE_2400    2400
#define BAUD_RATE_4800    4800
#define BAUD_RATE_9600    9600
#define BAUD_RATE_19200   19200
#define BAUD_RATE_38400   38400
#define BAUD_RATE_57600   57600
#define BAUD_RATE_115200  115200
#define BAUD_RATE_230400  230400
#define BAUD_RATE_460800  460800
#define BAUD_RATE_921600  921600
#define BAUD_RATE_2000000 2000000
#define BAUD_RATE_3000000 3000000

#define DATA_BITS_5 5
#define DATA_BITS_6 6
#define DATA_BITS_7 7
#define DATA_BITS_8 8
#define DATA_BITS_9 9

#define STOP_BITS_1 0
#define STOP_BITS_2 1
#define STOP_BITS_3 2
#define STOP_BITS_4 3

#ifdef _WIN32
#include <windows.h>
#else
#define PARITY_NONE 0
#define PARITY_ODD  1
#define PARITY_EVEN 2
#endif

#define BIT_ORDER_LSB 0
#define BIT_ORDER_MSB 1

#define NRZ_NORMAL   0 /* Non Return to Zero : normal mode */
#define NRZ_INVERTED 1 /* Non Return to Zero : inverted mode */

#ifndef OS_SERIAL_RX_BUFSZ
#define OS_SERIAL_RX_BUFSZ 64
#endif

#ifndef OS_SERIAL_TX_BUFSZ
#define OS_SERIAL_TX_BUFSZ 64
#endif

#define OS_SERIAL_EVENT_RX_IND     0x01 /* Rx indication */
#define OS_SERIAL_EVENT_TX_DONE    0x02 /* Tx complete   */
#define OS_SERIAL_EVENT_RX_DMADONE 0x03 /* Rx DMA transfer done */
#define OS_SERIAL_EVENT_TX_DMADONE 0x04 /* Tx DMA transfer done */
#define OS_SERIAL_EVENT_RX_TIMEOUT 0x05 /* Rx timeout    */

#define OS_SERIAL_DMA_RX 0x01
#define OS_SERIAL_DMA_TX 0x02

#define OS_SERIAL_RX_INT 0x01
#define OS_SERIAL_TX_INT 0x02

#define OS_SERIAL_ERR_OVERRUN 0x01
#define OS_SERIAL_ERR_FRAMING 0x02
#define OS_SERIAL_ERR_PARITY  0x03

#define OS_SERIAL_TX_DATAQUEUE_SIZE 2048
#define OS_SERIAL_TX_DATAQUEUE_LWM  30

#define OS_SERIAL_FLAG_TX_BUSY          0x1000

#define OS_SERIAL_FLAG_MASK             0xff00

/* Default config for serial_configure structure */
#define OS_SERIAL_CONFIG_DEFAULT                    \
{                                                   \
    BAUD_RATE_115200,       /* 115200 bits/s */     \
    DATA_BITS_8,            /* 8 databits */        \
    STOP_BITS_1,            /* 1 stopbit */         \
    PARITY_NONE,            /* No parity  */        \
    BIT_ORDER_LSB,          /* LSB first sent */    \
    NRZ_NORMAL,             /* Normal mode */       \
    OS_SERIAL_RX_BUFSZ,     /* Tx buffer size */    \
    OS_SERIAL_TX_BUFSZ,     /* Tx buffer size */    \
    0                                               \
}

struct serial_configure
{
    os_uint32_t baud_rate;

    os_uint32_t data_bits               :4;
    os_uint32_t stop_bits               :2;
    os_uint32_t parity                  :2;
    os_uint32_t bit_order               :1;
    os_uint32_t invert                  :1;
    os_uint32_t rx_bufsz                :16;
    os_uint32_t tx_bufsz                :16;
    os_uint32_t reserved                :6;
};

struct os_serial_rx_fifo
{
    rb_ring_buff_t rbuff;
    os_uint8_t    *line_buff;
};

struct os_serial_tx_fifo {
    rb_ring_buff_t rbuff;
    os_uint8_t    *line_buff;
    os_uint32_t    line_buff_count;
};

struct os_serial_device
{
    struct os_device parent;

    const struct os_uart_ops *ops;
    struct serial_configure   config;

    struct os_serial_rx_fifo *rx_fifo;
    struct os_serial_tx_fifo *tx_fifo;

#ifdef OS_SERIAL_DELAY_CLOSE
    os_work_t   work;
#endif

    os_sem_t    sem;
    os_sem_t    rx_sem;

#ifdef OS_SERIAL_IDLE_TIMER
    os_timer_t  rx_timer;
    os_uint16_t rx_timer_status;
#endif

    os_uint16_t rx_count;

    os_uint16_t flag;
};
typedef struct os_serial_device os_serial_t;

/**
 ***********************************************************************************************************************
 * @struct      os_uart_ops
 *
 * @brief       Uart operators
 ***********************************************************************************************************************
 */
struct os_uart_ops
{
    int (*init)(struct os_serial_device *serial, struct serial_configure *cfg);
    int (*deinit)(struct os_serial_device *serial);

    int (*start_send)(struct os_serial_device *serial, const os_uint8_t *buff, os_size_t size);
    int (*stop_send)(struct os_serial_device *serial);
    
    int (*start_recv)(struct os_serial_device *serial, os_uint8_t *buff, os_size_t size);
    int (*stop_recv)(struct os_serial_device *serial);

#ifdef OS_SERIAL_IDLE_TIMER
    int (*recv_count)(struct os_serial_device *serial);
#endif
    
    int (*poll_send)(struct os_serial_device *serial, const os_uint8_t *buff, os_size_t size);
};

void os_hw_serial_isr_rxdone(struct os_serial_device *serial, int count);
void os_hw_serial_isr_txdone(struct os_serial_device *serial);

os_err_t os_hw_serial_register(struct os_serial_device *serial, const char *name, void *data);

#endif
