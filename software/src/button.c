/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * button.c: EVSE button driver
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

#include "button.h"

#include "configs/config_evse.h"

#include "bricklib2/hal/system_timer/system_timer.h"

#include "led.h"

#include <string.h>

#define BUTTON_DEBOUNCE 100 // ms

Button button;

void button_init(void) {
	memset(&button, 0, sizeof(Button));
}

void button_tick(void) {
	const bool value = XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN);

	if(value != button.last_value) {
		button.last_value = value;
		button.last_change_time = system_timer_get_ms();
	}

	if(button.last_change_time != 0 && system_timer_is_time_elapsed_ms(button.last_change_time, BUTTON_DEBOUNCE)) {
		button.last_change_time = 0;
		if(!value) {
			button.state = BUTTON_STATE_RELEASED;

			// We always see a button release as a state change that turns the LED on (until standby)
			led_set_on();
		} else {
			button.state = BUTTON_STATE_PRESSED;
			button.was_pressed = true;
		}
	}
}

bool button_reset(void) {
	if((button.state != BUTTON_STATE_PRESSED) && button.was_pressed) {
		button.was_pressed = false;
		return true;
	}

	return false;
}
