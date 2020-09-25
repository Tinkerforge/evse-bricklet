/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * ads1118.h: ADS1118 16-Bit SPI ADC driver
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

#ifndef ADS1118_H
#define ADS1118_H

#include <stdint.h>

#include "bricklib2/hal/spi_fifo/spi_fifo.h"
#include "bricklib2/utility/moving_average.h"

typedef struct {
    uint16_t cp_adc_value;
    int16_t  cp_voltage;
    int16_t  cp_high_voltage;
    uint32_t cp_pe_resistance;
    int16_t  cp_cal_min_voltage;
    int16_t  cp_cal_max_voltage;
    uint16_t cp_cal_min_adc_value;
    uint16_t cp_cal_max_adc_value;
    uint8_t  cp_invalid_counter;

    uint16_t pp_adc_value;
    int16_t  pp_voltage;
    uint32_t pp_pe_resistance;
    uint8_t  pp_invalid_counter;

	SPIFifo  spi_fifo;

    MovingAverage moving_average_cp;
    MovingAverage moving_average_pp;
    bool moving_average_cp_new;
    bool moving_average_pp_new;
} ADS1118;

extern ADS1118 ads1118;

void ads1118_init(void);
void ads1118_tick(void);

#define ADS1118_CONFIG_SINGLE_SHOT               (1     << 15)
#define ADS1118_CONFIG_INP_IS_IN0_AND_INN_IS_IN1 (0b000 << 12)
#define ADS1118_CONFIG_INP_IS_IN0_AND_INN_IS_IN3 (0b001 << 12)
#define ADS1118_CONFIG_INP_IS_IN1_AND_INN_IS_IN3 (0b010 << 12)
#define ADS1118_CONFIG_INP_IS_IN2_AND_INN_IS_IN3 (0b011 << 12)
#define ADS1118_CONFIG_INP_IS_IN0_AND_INN_IS_GND (0b100 << 12)
#define ADS1118_CONFIG_INP_IS_IN1_AND_INN_IS_GND (0b101 << 12)
#define ADS1118_CONFIG_INP_IS_IN2_AND_INN_IS_GND (0b110 << 12)
#define ADS1118_CONFIG_INP_IS_IN3_AND_INN_IS_GND (0b111 << 12)
#define ADS1118_CONFIG_GAIN_6_114V               (0b000 <<  9)
#define ADS1118_CONFIG_GAIN_4_096V               (0b001 <<  9)
#define ADS1118_CONFIG_GAIN_2_048V               (0b010 <<  9)
#define ADS1118_CONFIG_GAIN_1_024V               (0b011 <<  9)
#define ADS1118_CONFIG_GAIN_0_512V               (0b100 <<  9)
#define ADS1118_CONFIG_GAIN_0_256V               (0b101 <<  9)
#define ADS1118_CONFIG_POWER_DOWN                (1     <<  8)
#define ADS1118_CONFIG_DATA_RATE_8SPS            (0b000 <<  5)
#define ADS1118_CONFIG_DATA_RATE_16SPS           (0b001 <<  5)
#define ADS1118_CONFIG_DATA_RATE_32SPS           (0b010 <<  5)
#define ADS1118_CONFIG_DATA_RATE_64SPS           (0b011 <<  5)
#define ADS1118_CONFIG_DATA_RATE_128SPS          (0b100 <<  5)
#define ADS1118_CONFIG_DATA_RATE_250SPS          (0b101 <<  5)
#define ADS1118_CONFIG_DATA_RATE_475SPS          (0b110 <<  5)
#define ADS1118_CONFIG_DATA_RATE_860SPS          (0b111 <<  5)
#define ADS1118_CONFIG_TEMPERATURE_MODE          (1     <<  4)
#define ADS1118_CONFIG_PULL_UP_ENABLE            (1     <<  3)
#define ADS1118_CONFIG_NOP                       (0b01  <<  1)

#endif