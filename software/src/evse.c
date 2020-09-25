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
#include "bricklib2/utility/util_definitions.h"

#include "ads1118.h"
#include "iec61851.h"
#include "lock.h"
#include "contactor_check.h"
#include "led.h"

#define EVSE_RELAY_MONOFLOP_TIME 10000 // 10 seconds

EVSE evse;

void evse_set_output(const uint16_t cp_duty_cycle, const bool contactor) {
	if(evse.low_level_cp_duty_cycle != cp_duty_cycle) {
		evse.low_level_cp_duty_cycle = cp_duty_cycle;

		// Ignore the next 10 ADC measurements between CP/PE after we 
		// change PWM duty cycle of CP to be sure that that the measurement
		// is not of any in-between state.
		ads1118.cp_invalid_counter = MAX(2, ads1118.cp_invalid_counter);
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
		ads1118.cp_invalid_counter = MAX(4, ads1118.cp_invalid_counter);
		ads1118.pp_invalid_counter = MAX(4, ads1118.pp_invalid_counter);

		// Also ignore contactor check for a while when contactor changes state
		contactor_check.invalid_counter = MAX(5, contactor_check.invalid_counter);

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

// Check for presence of lock motor switch by checking between LED output and switch
void evse_init_lock_switch(void) {
#if LOGGING_LEVEL == LOGGING_NONE
	// Test if there is a connection between the GP output and the motor lock switch input
	// If there is, it means that the EVSE is configured to run without a motor lock switch input
	XMC_GPIO_SetOutputHigh(EVSE_OUTPUT_GP_PIN);
	system_timer_sleep_ms(50);
	const bool test1 = !XMC_GPIO_GetInput(EVSE_MOTOR_INPUT_SWITCH_PIN);

	XMC_GPIO_SetOutputLow(EVSE_OUTPUT_GP_PIN);
	system_timer_sleep_ms(50);
	const bool test2 = XMC_GPIO_GetInput(EVSE_MOTOR_INPUT_SWITCH_PIN);

	evse.has_lock_switch = !(test1 && test2);
#else
	evse.has_lock_switch = false;
#endif
}

// Check pin header for max current
void evse_init_jumper(void) {
	const XMC_GPIO_CONFIG_t pin_config_input_tristate = {
		.mode             = XMC_GPIO_MODE_INPUT_TRISTATE,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	const XMC_GPIO_CONFIG_t pin_config_input_pullup = {
		.mode             = XMC_GPIO_MODE_INPUT_PULL_UP,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	const XMC_GPIO_CONFIG_t pin_config_input_pulldown = {
		.mode             = XMC_GPIO_MODE_INPUT_PULL_DOWN,
		.input_hysteresis = XMC_GPIO_INPUT_HYSTERESIS_STANDARD
	};

	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN0, &pin_config_input_pullup);
	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN1, &pin_config_input_pullup);
	system_timer_sleep_ms(50);
	bool pin0_pu = XMC_GPIO_GetInput(EVSE_CONFIG_JUMPER_PIN0);
	bool pin1_pu = XMC_GPIO_GetInput(EVSE_CONFIG_JUMPER_PIN1);

	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN0, &pin_config_input_pulldown);
	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN1, &pin_config_input_pulldown);
	system_timer_sleep_ms(50);
	bool pin0_pd = XMC_GPIO_GetInput(EVSE_CONFIG_JUMPER_PIN0);
	bool pin1_pd = XMC_GPIO_GetInput(EVSE_CONFIG_JUMPER_PIN1);

	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN0, &pin_config_input_tristate);
	XMC_GPIO_Init(EVSE_CONFIG_JUMPER_PIN1, &pin_config_input_tristate);

	// Differentiate between high, low and open
	char pin0 = 'x';
	if(pin0_pu && !pin0_pd) {
		pin0 = 'o';
	} else if(pin0_pu && pin0_pd) {
		pin0 = 'h';
	} else if(!pin0_pu && !pin0_pd) {
		pin0 = 'l';
	}

	char pin1 = 'x';
	if(pin1_pu && !pin1_pd) {
		pin1 = 'o';
	} else if(pin1_pu && pin1_pd) {
		pin1 = 'h';
	} else if(!pin1_pu && !pin1_pd) {
		pin1 = 'l';
	}

	if(pin0 == 'o' && pin1 == 'o') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_UNCONFIGURED;
	} else if(pin0 == 'o' && pin1 == 'h') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_6A;
	} else if(pin0 == 'o' && pin1 == 'l') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_10A;
	} else if(pin0 == 'h' && pin1 == 'o') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_13A;
	} else if(pin0 == 'h' && pin1 == 'h') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_20A;
	} else if(pin0 == 'h' && pin1 == 'l') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_32A;
	} else if(pin0 == 'l' && pin1 == 'o') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_16A;
	} else if(pin0 == 'l' && pin1 == 'h') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_CURRENT_25A;
	} else if(pin0 == 'l' && pin1 == 'l') {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_SOFTWARE;
	} else {
		evse.config_jumper_current = EVSE_CONFIG_JUMPER_UNCONFIGURED;
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
#if LOGGING_LEVEL == LOGGING_NONE
	XMC_GPIO_Init(EVSE_OUTPUT_GP_PIN,    &pin_config_output);
#endif

	XMC_GPIO_Init(EVSE_MOTOR_INPUT_SWITCH_PIN, &pin_config_input);
	XMC_GPIO_Init(EVSE_INPUT_GP_PIN,           &pin_config_input);

	ccu4_pwm_init(EVSE_CP_PWM_PIN, EVSE_CP_PWM_SLICE_NUMBER, EVSE_CP_PWM_PERIOD-1); // 1kHz
	ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 0);

	ccu4_pwm_init(EVSE_MOTOR_ENABLE_PIN, EVSE_MOTOR_ENABLE_SLICE_NUMBER, EVSE_MOTOR_PWM_PERIOD-1); // 10 kHz
	ccu4_pwm_set_duty_cycle(EVSE_MOTOR_ENABLE_SLICE_NUMBER, EVSE_MOTOR_PWM_PERIOD);

	evse.calibrate = false;
	evse.startup_time = system_timer_get_ms();
	evse.config_jumper_current_software = 6000; // default software configuration is 6A

	evse_init_jumper();
	evse_init_lock_switch();
}

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

void evse_tick_debug(void) {
#if LOGGING_LEVEL != LOGGING_NONE
	static uint32_t debug_time = 0;
	if(system_timer_is_time_elapsed_ms(debug_time, 250)) {
		debug_time = system_timer_get_ms();
		uartbb_printf("\n\r");
		uartbb_printf("IEC61851 State: %d\n\r", iec61851.state);
		uartbb_printf("Has lock switch: %d\n\r", evse.has_lock_switch);
		uartbb_printf("Jumper configuration: %d\n\r", evse.config_jumper_current);
		uartbb_printf("LED State: %d\n\r", led.state);
		uartbb_printf("Resistance: CP %d, PP %d\n\r", ads1118.cp_pe_resistance, ads1118.pp_pe_resistance);
		uartbb_printf("CP PWM duty cycle: %d\n\r", ccu4_pwm_get_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER));
		uartbb_printf("Contactor Check: AC1 %d, AC2 %d, State: %d, Error: %d\n\r", contactor_check.ac1_edge_count, contactor_check.ac2_edge_count, contactor_check.state, contactor_check.error);
		uartbb_printf("GPIO: Input %d, Output %d\n\r", XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN), XMC_GPIO_GetInput(EVSE_OUTPUT_GP_PIN));
		uartbb_printf("Lock State: %d\n\r", lock.state);
	}
#endif
}

void evse_tick(void) {
	if(evse.calibrate) {
		// TODO:
		// 1. change PWM to 100% duty cycle
		// 2. integrate over a few seconds (+12V)
		// 3. change PWM to 0% duty cycle
		// 4. integrate over a few seconds (-12V)
		// 5. calculate 0 value, max, min assuming 125 uV per ADC lsb
		// 6. save calibration
	} else if(evse.low_level_mode_enabled) {
		// If low level mode is enabled,
		// everything is handled through the low level API.
		evse_tick_low_level();
	} else {
		if(evse.startup_time == 0 || system_timer_is_time_elapsed_ms(evse.startup_time, 1000)) {
			evse.startup_time = 0;

			// Otherwise we implement the EVSE according to IEC 61851.
			iec61851_tick();
		}
	}

	evse_tick_debug();
}
