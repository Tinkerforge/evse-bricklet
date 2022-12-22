/* evse-bricklet
 * Copyright (C) 2020-2022 Olaf Lüke <olaf@tinkerforge.com>
 *
 * iec61851.c: Implementation of IEC 61851 EVSE state machine
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

#include "iec61851.h"

#include <stdint.h>
#include <string.h>

#include "bricklib2/utility/util_definitions.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/hal/ccu4_pwm/ccu4_pwm.h"
#include "configs/config_evse.h"
#include "ads1118.h"
#include "iec61851.h"
#include "lock.h"
#include "evse.h"
#include "contactor_check.h"
#include "led.h"
#include "button.h"
#include "charging_slot.h"

IEC61851 iec61851;

void iec61851_set_state(IEC61851State state) {
	if(state != iec61851.state) {
		// If we change the state from an error state to something else we save the time
		if(iec61851.state == IEC61851_STATE_EF) {
			iec61851.last_error_time = system_timer_get_ms();
		}

		// If we change to state C and we were in an error state before, we wait at least 30 seconds
		if((state == IEC61851_STATE_C) && (iec61851.last_error_time != 0)) {
			if(!system_timer_is_time_elapsed_ms(iec61851.last_error_time, 30*1000)) {
				return;
			}
			iec61851.last_error_time = 0;
		}

		// If we change to state C and the charging timer was not started, we start it
		if((state == IEC61851_STATE_C) && (evse.charging_time == 0)) {
			evse.charging_time = system_timer_get_ms();
		}

		if((state == IEC61851_STATE_A ) || (state == IEC61851_STATE_B)) {
			// Turn LED on with timer for standby if we have a state change to state A or B
			led_set_on(false);
		}

		if((iec61851.state != IEC61851_STATE_A) && (state == IEC61851_STATE_A)) {
			// If state changed from to A we invalidate the managed current
			// we have to handle the clear on dusconnect slots
			charging_slot_handle_disconnect();

			// If the charging timer is running and the car is disconnected, stop the charging timer
			evse.charging_time = 0;
		}

		iec61851.state             = state;
		iec61851.last_state_change = system_timer_get_ms();
	}
}

// TODO: We can find out that no cable is connected here
//       if resistance > 10000. Do we want to have a specific
//       state for that?
uint32_t iec61851_get_ma_from_pp_resistance(void) {
	if(ads1118.pp_pe_resistance >= 1000) {
		return 13000; // 13A
	} else if(ads1118.pp_pe_resistance >= 330) {
		return 20000; // 20A
	} else if(ads1118.pp_pe_resistance >= 150) {
		return 32000; // 32A
	} else {
		return 64000; // 64A
	}
}

uint32_t iec61851_get_max_ma(void) {
	return charging_slot_get_max_current();
}

// Duty cycle in pro mille (1/10 %)
uint16_t iec61851_get_duty_cycle_for_ma(uint32_t ma) {
	// Special case for managed mode.
	// In managed mode we support a temporary stop of charging without disconnecting the vehicle.
	if(ma == 0) {
		// 100% duty cycle => charging not allowed
		// we do 100% here instead of 0% (both mean charging not allowed) 
		// to be able to still properly measure the resistance that the car applies.
		return 1000; 
	}

	uint32_t duty_cycle;
	if(ma <= 51000) {
		duty_cycle = ma/60; // For 6A-51A: xA = %duty*0.6
	} else {
		duty_cycle = ma/250 + 640; // For 51A-80A: xA= (%duty - 64)*2.5
	}

	// The standard defines 8% as minimum and 100% as maximum
	return BETWEEN(80, duty_cycle, 1000); 
}

void iec61851_state_a(void) {
	// In the case that a charging was stopped by pressing the button,
	// we only allow to start charging again after we reach state A.
	if(button.was_pressed) {
		button.was_pressed = false;
		if(button.state == BUTTON_STATE_RELEASED) {
			charging_slot_start_charging_by_button();
		}
	}
	// Apply +12V to CP, disable contactor
	evse_set_output(1000, false);
}

void iec61851_state_b(void) {
	// Apply 1kHz square wave to CP with appropriate duty cycle, disable contactor
	uint32_t ma = iec61851_get_max_ma();
	evse_set_output(iec61851_get_duty_cycle_for_ma(ma), false);
}

void iec61851_state_c(void) {
	// Apply 1kHz square wave to CP with appropriate duty cycle, enable contactor
	uint32_t ma = iec61851_get_max_ma();
	evse_set_output(iec61851_get_duty_cycle_for_ma(ma), true);
	led_set_breathing();
}

void iec61851_state_d(void) {
	// State D is not supported
	// Apply +12V to CP, disable contactor
	evse_set_output(1000, false);
}

void iec61851_state_ef(void) {
	// In case of error apply +12V to CP, disable contactor
	evse_set_output(1000, false);
}

void iec61851_tick(void) {
	if(evse.calibration_state != 0) {
		return;
	}

	if(contactor_check.error != 0) {
		led_set_blinking(4);
		iec61851_set_state(IEC61851_STATE_EF);
	} else if(evse.config_jumper_current == EVSE_CONFIG_JUMPER_UNCONFIGURED) {
		// We don't allow the jumper to be unconfigured
		led_set_blinking(2);
		iec61851_set_state(IEC61851_STATE_EF);
	} else {
		// Wait for ADC measurements to be valid
		if(ads1118.cp_invalid_counter > 0) {
			return;
		}

		// When an ID.3 is connected to the WARP charger and the duty cycle is already
		// below 100% (the wallbox is ready) but the contactor is not yet activated, the
		// ID.3 somtimes generates a spike in the resistance that we measure when it
		// engages the resistor to apply 880 ohm between CP/PE. We have not seen this in
		// other cars, we assume this is some kind of capacitive effect. To make sure
		// that we don't cancel the charging here, we increase the STATE A threshold for
		// this scenario.
		const uint16_t current_cp_duty_cycle = evse_get_cp_duty_cycle();
		const bool id3_mode = (current_cp_duty_cycle != 1000) && !XMC_GPIO_GetInput(EVSE_RELAY_PIN);
		if(!id3_mode) {
			iec61851.id3_mode_time = 0;
		}

		if(id3_mode && (ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_A*3)) {
			if(iec61851.id3_mode_time == 0) {
				iec61851.id3_mode_time = system_timer_get_ms();
			} else {
				// wait for at least 2500ms between B->A state change in ID.3 mode
				if(system_timer_is_time_elapsed_ms(iec61851.id3_mode_time, 2500)) {
					iec61851_set_state(IEC61851_STATE_A);
				}
			}
		} else if(!id3_mode && (ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_A)) {
			iec61851_set_state(IEC61851_STATE_A);
		} else if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_B) {
			iec61851_set_state(IEC61851_STATE_B);
		} else if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_C) {
			if(charging_slot_get_max_current() == 0) {
				evse.charging_time = 0;
				iec61851_set_state(IEC61851_STATE_B);
			} else {
				iec61851_set_state(IEC61851_STATE_C);
			}
		} else if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_D) {
			led_set_blinking(5);
			iec61851_set_state(IEC61851_STATE_D);
		} else {
			led_set_blinking(5);
			iec61851_set_state(IEC61851_STATE_EF);
		}
	}

	switch(iec61851.state) {
		case IEC61851_STATE_A:  iec61851_state_a();  break;
		case IEC61851_STATE_B:  iec61851_state_b();  break;
		case IEC61851_STATE_C:  iec61851_state_c();  break;
		case IEC61851_STATE_D:  iec61851_state_d();  break;
		case IEC61851_STATE_EF: iec61851_state_ef(); break;
	}
}

void iec61851_init(void) {
	memset(&iec61851, 0, sizeof(IEC61851));
	iec61851.last_state_change = system_timer_get_ms();
}

