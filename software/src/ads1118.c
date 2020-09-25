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

#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/utility/moving_average.h"
#include "bricklib2/os/coop_task.h"
#include "bricklib2/logging/logging.h"

#define ADS1118_MOVING_AVERAGE_LENGTH 4
#define ADS1118_CONFIGURE_TIMEOUT 200

#include "evse.h"

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

// channel 0 = measure CP
// channel 1 = measure PP
// channel 2 = measure temperature
uint8_t *ads1118_get_config_for_mosi(const uint8_t channel) {
	static uint8_t mosi[2] = {0, 0};

	uint16_t config = ADS1118_CONFIG_SINGLE_SHOT | ADS1118_CONFIG_POWER_DOWN | ADS1118_CONFIG_GAIN_4_096V | ADS1118_CONFIG_DATA_RATE_8SPS | ADS1118_CONFIG_PULL_UP_ENABLE | ADS1118_CONFIG_NOP;
	switch(channel) {
		case 0: config |= ADS1118_CONFIG_INP_IS_IN0_AND_INN_IS_IN3; break;
		case 1: config |= ADS1118_CONFIG_INP_IS_IN2_AND_INN_IS_IN3; break;
		case 2: config |= ADS1118_CONFIG_TEMPERATURE_MODE;          break;
		default: break;
	}

	mosi[0] = (config >> 8) & 0xFF;
	mosi[1] = (config >> 0) & 0xFF;
	return mosi;
}

void ads1118_cp_voltage_from_miso(const uint8_t *miso) {
	ads1118.cp_adc_value = (miso[1] | (miso[0] << 8));

	// 0.8217V => -12V
	// 3.9554V =>  12V
	// 1 LSB = 125uV
	// ===>
	// 6574 LSB  => -12V
	// 31643 LSB =>  12V
	
	// TODO: Use calibration values here instead of fixed 6574 and 31643
	ads1118.cp_voltage = SCALE(ads1118.cp_adc_value, 6574, 31643, -12000, 12000);

	ads1118.cp_high_voltage = (ads1118.cp_voltage - ads1118.cp_cal_min_voltage)*1000/evse.low_level_cp_duty_cycle + ads1118.cp_cal_min_voltage;

	uint32_t new_resistance;
	// If the measured high voltage is near the calibration max voltage
	// we assume that there is no resistance
	if(ABS(ads1118.cp_high_voltage - ads1118.cp_cal_max_voltage) < 1000) {
		new_resistance = 0xFFFF;
	} else {
		new_resistance = 1000*ads1118.cp_high_voltage/(ads1118.cp_cal_max_voltage - ads1118.cp_high_voltage);
	}

	if(ads1118.moving_average_cp_new) {
		ads1118.moving_average_cp_new = false;
		moving_average_init(&ads1118.moving_average_cp, new_resistance, ADS1118_MOVING_AVERAGE_LENGTH);
	} else {
		moving_average_handle_value(&ads1118.moving_average_cp, new_resistance);
	}

	ads1118.cp_pe_resistance = moving_average_get(&ads1118.moving_average_cp);
}

void ads1118_pp_voltage_from_miso(const uint8_t *miso) {
	ads1118.pp_adc_value = (miso[1] | (miso[0] << 8));

	// 1 LSB = 125uV
	ads1118.pp_voltage = ads1118.pp_adc_value/8;

	uint32_t new_resistance;
	// If the measured high voltage is near the calibration max voltage
	// we assume that there is no resistance
	if(ABS(ads1118.pp_voltage - 4095) < 150) {
		new_resistance = 0xFFFFFFFF;
	} else {
		new_resistance = 1000*ads1118.pp_voltage/(5000 - ads1118.pp_voltage);
	}

	if(ads1118.moving_average_pp_new) {
		ads1118.moving_average_pp_new = false;
		moving_average_init(&ads1118.moving_average_pp, new_resistance, ADS1118_MOVING_AVERAGE_LENGTH);
	} else {
		moving_average_handle_value(&ads1118.moving_average_pp, new_resistance);
	}

	ads1118.pp_pe_resistance = moving_average_get(&ads1118.moving_average_pp);
}

void ads1118_task_tick(void) {
	const XMC_GPIO_CONFIG_t config_low = {
		.mode         = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level = XMC_GPIO_OUTPUT_LEVEL_LOW,
	};

	const XMC_GPIO_CONFIG_t config_select = {
		.mode         = ADS1118_SELECT_PIN_MODE,
		.output_level = XMC_GPIO_OUTPUT_LEVEL_LOW,
	};

	uint8_t miso[2] = {0, 0};

	// Configure CP
	spi_fifo_coop_transceive(&ads1118.spi_fifo, 2, ads1118_get_config_for_mosi(0), miso);

	uint32_t configure_time = 0;

	// ADS1118 runs with 8 samples per second,
	// so each loop takes about 250ms
	while(true) {
		// Wait for DRDY
		coop_task_sleep_ms(10);
		XMC_GPIO_Init(ADS1118_SELECT_PORT, ADS1118_SELECT_PIN, &config_low);

		configure_time = system_timer_get_ms();
		while(XMC_GPIO_GetInput(ADS1118_MISO_PORT, ADS1118_MISO_PIN)) {
			if(system_timer_is_time_elapsed_ms(configure_time, ADS1118_CONFIGURE_TIMEOUT)) {
				XMC_GPIO_Init(ADS1118_SELECT_PORT, ADS1118_SELECT_PIN, &config_select);
				spi_fifo_coop_transceive(&ads1118.spi_fifo, 2, ads1118_get_config_for_mosi(0), miso);
				XMC_GPIO_Init(ADS1118_SELECT_PORT, ADS1118_SELECT_PIN, &config_low);
				configure_time = system_timer_get_ms();
			}
			coop_task_yield();
		}
		XMC_GPIO_Init(ADS1118_SELECT_PORT, ADS1118_SELECT_PIN, &config_select);
		// Read CP -> Configure PP
		spi_fifo_coop_transceive(&ads1118.spi_fifo, 2, ads1118_get_config_for_mosi(1), miso);
		if(ads1118.cp_invalid_counter > 0) {
			ads1118.cp_invalid_counter--;
		} else {
			ads1118_cp_voltage_from_miso(miso);
		}

		// Wait for DRDY
		coop_task_sleep_ms(10);
		XMC_GPIO_Init(ADS1118_SELECT_PORT, ADS1118_SELECT_PIN, &config_low);

		configure_time = system_timer_get_ms();
		while(XMC_GPIO_GetInput(ADS1118_MISO_PORT, ADS1118_MISO_PIN)) {
			if(system_timer_is_time_elapsed_ms(configure_time, ADS1118_CONFIGURE_TIMEOUT)) {
				XMC_GPIO_Init(ADS1118_SELECT_PORT, ADS1118_SELECT_PIN, &config_select);
				spi_fifo_coop_transceive(&ads1118.spi_fifo, 2, ads1118_get_config_for_mosi(1), miso);
				XMC_GPIO_Init(ADS1118_SELECT_PORT, ADS1118_SELECT_PIN, &config_low);
				configure_time = system_timer_get_ms();
			}
			coop_task_yield();
		}
		XMC_GPIO_Init(ADS1118_SELECT_PORT, ADS1118_SELECT_PIN, &config_select);
		// Read PP -> Configure CP
		spi_fifo_coop_transceive(&ads1118.spi_fifo, 2, ads1118_get_config_for_mosi(0), miso);
		if(ads1118.pp_invalid_counter > 0) {
			ads1118.pp_invalid_counter--;
		} else {
			ads1118_pp_voltage_from_miso(miso);
		}

		// TODO: Read temperature?

		coop_task_yield();
	}
}

void ads1118_init(void) {
	memset(&ads1118, 0, sizeof(ADS1118));

	// TODO: Calibrate min/max voltage
	ads1118.cp_cal_max_voltage = 12280;
	ads1118.cp_cal_min_voltage = -12435;

	ads1118.moving_average_cp_new = true;
	ads1118.moving_average_pp_new = true;

	ads1118_init_spi();
	coop_task_init(&ads1118_task, ads1118_task_tick);
}

void ads1118_tick(void) {
	coop_task_tick(&ads1118_task);
}