/*

  btt_skr_mini_e3_3.0.c - driver code for STM32G0xx ARM processors

  Part of grblHAL

  Copyright (c) 2021-2026 Terje Io

  grblHAL is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  grblHAL is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with grblHAL. If not, see <http://www.gnu.org/licenses/>.

*/

#include "driver.h"

#if defined(BOARD_BTT_SKR_MINI_E3_V30)

#include <string.h>

#include "serial.h"

static io_stream_t tmc_uart;

static void trinamic_on_driver_preinit (
    motor_map_t motor,
    trinamic_driver_config_t *config)
{
    switch(motor.axis) {

        case X_AXIS:
            config->address = 0;
            break;

        case Y_AXIS:
            config->address = 2;
            break;

        case Z_AXIS:
            config->address = 1;
            break;

        default:
            config->address = motor.id;
            break;
    }
}

TMC_uart_write_datagram_t *tmc_uart_read (trinamic_motor_t driver, TMC_uart_read_datagram_t *dgr)
{
    static TMC_uart_write_datagram_t wdgr = {0};
    volatile uint32_t dly = 50, ms = hal.get_elapsed_ticks();

    tmc_uart.write_n(dgr->data, sizeof(TMC_uart_read_datagram_t));

    while(tmc_uart.get_tx_buffer_count());

    while(--dly);

    tmc_uart.reset_read_buffer();

    // Wait for response with 3ms timeout
    while(tmc_uart.get_rx_buffer_count() < 8) {
        if(hal.get_elapsed_ticks() - ms >= 3)
            break;
    }

    if(tmc_uart.get_rx_buffer_count() >= 8) {
        wdgr.data[0] = tmc_uart.read();
        wdgr.data[1] = tmc_uart.read();
        wdgr.data[2] = tmc_uart.read();
        wdgr.data[3] = tmc_uart.read();
        wdgr.data[4] = tmc_uart.read();
        wdgr.data[5] = tmc_uart.read();
        wdgr.data[6] = tmc_uart.read();
        wdgr.data[7] = tmc_uart.read();
    } else
        wdgr.msg.addr.value = 0xFF;

    dly = 5000;
    while(--dly);

    return &wdgr;
}

void tmc_uart_write (trinamic_motor_t driver, TMC_uart_write_datagram_t *dgr)
{
    tmc_uart.write_n(dgr->data, sizeof(TMC_uart_write_datagram_t));

    while(tmc_uart.get_tx_buffer_count());	// Wait while the datagram is delivered
}

void board_init (void)
{
    io_stream_t const *stream;

    // Register board-specific Trinamic configuration
    trinamic_driver_if_t trinamic_if = {
        .on_driver_preinit = trinamic_on_driver_preinit
    };

    trinamic_if_init(&trinamic_if);

    if((stream = stream_open_instance(TRINAMIC_STREAM, 115200, NULL, "Trinamic UART")) == NULL)
        stream = stream_null_init(115200);

    memcpy(&tmc_uart, stream, sizeof(io_stream_t));
    tmc_uart.disable_rx(true);
    tmc_uart.set_enqueue_rt_handler(stream_buffer_all);
}

#endif
