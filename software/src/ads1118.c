/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * ads1118.c: ADS1118 16-Bit SPI ADC driver
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "ads1118.h"
#include "configs/config_ads1118.h"

#include "bricklib2/os/coop_task.h"

CoopTask ads1118_task;
ADS1118 ads1118;


static void ads1118_init_spi(void) {
	ads1118.spi_fifo.channel             = ADS1118_USIC_SPI;
	ads1118.spi_fifo.baudrate            = ADS1118_SPI_BAUDRATE;

	ads1118.spi_fifo.rx_fifo_size        = ADS1118_RX_FIFO_SIZE;
	ads1118.spi_fifo.rx_fifo_pointer     = ADS1118_RX_FIFO_POINTER;
	ads1118.spi_fifo.tx_fifo_size        = ADS1118_TX_FIFO_SIZE;
	ads1118.spi_fifo.tx_fifo_pointer     = ADS1118_TX_FIFO_POINTER;

	ads1118.spi_fifo.slave               = ADS1118_SLAVE;
	ads1118.spi_fifo.clock_output        = ADS1118_CLOCK_OUTPUT;
	ads1118.spi_fifo.clock_passive_level = ADS1118_CLOCK_PASSIVE_LEVEL;

	ads1118.spi_fifo.sclk_pin            = ADS1118_SCLK_PIN;
	ads1118.spi_fifo.sclk_port           = ADS1118_SCLK_PORT;
	ads1118.spi_fifo.sclk_pin_mode       = ADS1118_SCLK_PIN_MODE;

	ads1118.spi_fifo.select_pin          = ADS1118_SELECT_PIN;
	ads1118.spi_fifo.select_port         = ADS1118_SELECT_PORT;
	ads1118.spi_fifo.select_pin_mode     = ADS1118_SELECT_PIN_MODE;

	ads1118.spi_fifo.mosi_pin            = ADS1118_MOSI_PIN;
	ads1118.spi_fifo.mosi_port           = ADS1118_MOSI_PORT;
	ads1118.spi_fifo.mosi_pin_mode       = ADS1118_MOSI_PIN_MODE;

	ads1118.spi_fifo.miso_pin            = ADS1118_MISO_PIN;
	ads1118.spi_fifo.miso_port           = ADS1118_MISO_PORT;
	ads1118.spi_fifo.miso_input          = ADS1118_MISO_INPUT;
	ads1118.spi_fifo.miso_source         = ADS1118_MISO_SOURCE;

	spi_fifo_init(&ads1118.spi_fifo);
}

void ads1118_task_tick(void) {
	while(true) {
		// spi_fifo_coop_transceive(&ads1118.spi_fifo, LENGTH, data, data);
		coop_task_yield();
	}
}

void ads1118_init(void) {
	memset(&ads1118, 0, sizeof(ADS1118));

	ads1118_init_spi();
	coop_task_init(&ads1118_task, ads1118_task_tick);
}

void ads1118_tick(void) {
	coop_task_tick(&ads1118_task);
}