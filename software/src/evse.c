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
#include "bricklib2/hal/ccu4_pwm/ccu4_pwm.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"

#include "ads1118.h"
#include "iec61851.h"
#include "lock.h"
#include "contactor_check.h"

#define EVSE_RELAY_MONOFLOP_TIME 10000 // 10 seconds

EVSE evse;

void evse_set_output(const uint16_t cp_duty_cycle, const bool contactor) {
	if(evse.low_level_cp_duty_cycle != cp_duty_cycle) {
		evse.low_level_cp_duty_cycle = cp_duty_cycle;

		// Ignore the next two ADC measurements between CP/PE after we 
		// change PWM duty cycle of CP to be sure that that the measurement
		// is not of any in-between state.
		ads1118.cp_invalid_counter = 2;
		ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 64000 - cp_duty_cycle*64);
	}

	// If the contactor is to be enabled and the lock is currently
	// not completely closed, we start the locking procedure and return.
	// The contactor will only be enabled after the lock is closed.
	if(contactor) {
		if(lock_get_state() != LOCK_STATE_CLOSE) {
			lock_set_locked(true);
			return;
		}
	}

	if(((bool)XMC_GPIO_GetInput(EVSE_RELAY_PIN)) != contactor) {
		// Ignore all ADC measurements for a while if the contactor is
		// switched on or off, to be sure that the resulting EMI spile does
		// not give us a wrong measurement.
		ads1118.cp_invalid_counter = 2;
		ads1118.pp_invalid_counter = 2;

		// Also ignore contactor check for a while when contactor changes state
		contactor_check.invalid_counter = 2;

		if(contactor) {
			XMC_GPIO_SetOutputHigh(EVSE_RELAY_PIN);
		} else {
			XMC_GPIO_SetOutputLow(EVSE_RELAY_PIN);
		}
	}

	if(!contactor) {
		if(lock_get_state() != LOCK_STATE_OPEN) {
			lock_set_locked(false);
		}
	}
}

void evse_init(void) {
	const XMC_GPIO_CONFIG_t pin_config_output = {
		.mode             = XMC_GPIO_MODE_OUTPUT_PUSH_PULL,
		.output_level     = XMC_GPIO_OUTPUT_LEVEL_LOW
	};

	const XMC_GPIO_CONFIG_t pin_config_input = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	XMC_GPIO_Init(EVSE_RELAY_PIN,        &pin_config_output);
	XMC_GPIO_Init(EVSE_MOTOR_PHASE_PIN,  &pin_config_output);

	XMC_GPIO_Init(EVSE_MOTOR_INPUT_SWITCH_PIN, &pin_config_input);

	ccu4_pwm_init(EVSE_CP_PWM_PIN, EVSE_CP_PWM_SLICE_NUMBER, EVSE_CP_PWM_PERIOD-1); // 1kHz
	ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 0);

	ccu4_pwm_init(EVSE_MOTOR_ENABLE_PIN, EVSE_MOTOR_ENABLE_SLICE_NUMBER, EVSE_MOTOR_PWM_PERIOD-1); // 10 kHz
	ccu4_pwm_set_duty_cycle(EVSE_MOTOR_ENABLE_SLICE_NUMBER, EVSE_MOTOR_PWM_PERIOD);

	evse.startup_time = system_timer_get_ms();
}

#if 0
void evse_tick_low_level(void) {
	ccu4_pwm_set_duty_cycle(EVSE_MOTOR_ENABLE_SLICE_NUMBER, 6400 - evse.low_level_motor_duty_cycle*64/10);

	if(evse.low_level_motor_direction) {
		XMC_GPIO_SetOutputHigh(EVSE_MOTOR_PHASE_PIN);
	} else {
		XMC_GPIO_SetOutputLow(EVSE_MOTOR_PHASE_PIN);
	}

	if((evse.low_level_relay_monoflop != 0) && system_timer_is_time_elapsed_ms(evse.low_level_relay_monoflop, EVSE_RELAY_MONOFLOP_TIME)) {
		evse.low_level_relay_enabled  = false;
		evse.low_level_relay_monoflop = 0;
	}

	evse_set_output(evse.low_level_cp_duty_cycle, evse.low_level_relay_enabled);
}
#endif

void evse_tick(void) {
	if(evse.low_level_mode_enabled) {
		// If low level mode is enabled,
		// everything is handled through the low level API.
#if 0
		evse_tick_low_level();
#endif
	} else {
		if(evse.startup_time == 0 || system_timer_is_time_elapsed_ms(evse.startup_time, 1000)) {
			evse.startup_time = 0;

			// Otherwise we implement the EVSE according to IEC 61851.
			lock_tick();
			iec61851_tick();
		}
	}
}
