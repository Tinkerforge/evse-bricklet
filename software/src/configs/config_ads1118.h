/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * config_ads1118.h: Config for ADS1118 16-Bit SPI ADC driver
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

#ifndef CONFIG_ADS1118_H
#define CONFIG_ADS1118_H

#include "xmc_gpio.h"
#include "xmc_spi.h"

#define ADS1118_SPI_BAUDRATE           200000
#define ADS1118_USIC_CHANNEL           USIC0_CH1
#define ADS1118_USIC_SPI               XMC_SPI0_CH1

#define ADS1118_RX_FIFO_SIZE           XMC_USIC_CH_FIFO_SIZE_16WORDS
#define ADS1118_RX_FIFO_POINTER        32
#define ADS1118_TX_FIFO_SIZE           XMC_USIC_CH_FIFO_SIZE_16WORDS
#define ADS1118_TX_FIFO_POINTER        48

#define ADS1118_CLOCK_PASSIVE_LEVEL    XMC_SPI_CH_BRG_SHIFT_CLOCK_PASSIVE_LEVEL_0_DELAY_ENABLED
#define ADS1118_CLOCK_OUTPUT           XMC_SPI_CH_BRG_SHIFT_CLOCK_OUTPUT_SCLK
#define ADS1118_SLAVE                  XMC_SPI_CH_SLAVE_SELECT_4

#define ADS1118_SCLK_PORT              XMC_GPIO_PORT0
#define ADS1118_SCLK_PIN               8
#define ADS1118_SCLK_PIN_MODE          (XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT7 | P0_8_AF_U0C1_SCLKOUT)

#define ADS1118_SELECT_PORT            XMC_GPIO_PORT0
#define ADS1118_SELECT_PIN             9
#define ADS1118_SELECT_PIN_MODE        (XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT7 | P0_9_AF_U0C1_SELO0)

#define ADS1118_MOSI_PORT              XMC_GPIO_PORT0
#define ADS1118_MOSI_PIN               7
#define ADS1118_MOSI_PIN_MODE          (XMC_GPIO_MODE_OUTPUT_PUSH_PULL_ALT7 | P0_7_AF_U0C1_DOUT0)

#define ADS1118_MISO_PORT              XMC_GPIO_PORT0
#define ADS1118_MISO_PIN               6
#define ADS1118_MISO_INPUT             XMC_USIC_CH_INPUT_DX0
#define ADS1118_MISO_SOURCE            0b010 // DX0C


#endif