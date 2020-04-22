/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * evse.c: EVSE implementation
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

#include "evse.h"

#include "configs/config_evse.h"

EVSE evse;

void evse_init(void) {
	const XMC_GPIO_CONFIG_t pin_config = {
		.mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_LOW
	};

	XMC_GPIO_Init(EVSE_RELAY_PIN,        &pin_config);
	XMC_GPIO_Init(EVSE_MOTOR_PHASE_PIN,  &pin_config);
	XMC_GPIO_Init(EVSE_MOTOR_ENABLE_PIN, &pin_config);
	XMC_GPIO_Init(EVSE_CHARGE_LED_PIN,   &pin_config);
	XMC_GPIO_Init(EVSE_ERROR_LED_PIN,    &pin_config);
	XMC_GPIO_Init(EVSE_CP_PWM_PIN,    &pin_config);
}

void evse_tick(void) {

}
