/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
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
#include "lock.h"
#include "evse.h"
#include "contactor_check.h"

// Resistance between CP/PE
// inf  Ohm -> no car present
// 2700 Ohm -> car present
//  880 Ohm -> car charging
//  240 Ohm -> car charging with ventilation
// ==>
// > 10000 -> State A
// >  1790 -> State B
// >  560 -> State C
// >  150 -> State D
// <  150 -> State E/F
#define IEC61851_CP_RESISTANCE_STATE_A 10000
#define IEC61851_CP_RESISTANCE_STATE_B  1790
#define IEC61851_CP_RESISTANCE_STATE_C   560
#define IEC61851_CP_RESISTANCE_STATE_D   150


// Resistance between PP/PE
// 1000..2200 Ohm => 13A
// 330..1000 Ohm  => 20A
// 150..330 Ohm   => 32A
// 75..150 Ohm    => 63A
#define IEC61851_PP_RESISTANCE_13A 1000
#define IEC61851_PP_RESISTANCE_20A  330
#define IEC61851_PP_RESISTANCE_32A  150


IEC61851 iec61851;

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

// Duty cycle in pro mille (1/10 %)
uint16_t iec61851_get_duty_cycle_for_ma(uint32_t ma) {
	uint32_t duty_cycle = ma/60;

	// TODO: Cap at specific duty cycles?
	return BETWEEN(0, duty_cycle, 1000);
}

void iec61851_state_a(void) {
	// Apply +12V to CP, disable contactor
	evse_set_output(1000, false);
}

void iec61851_state_b(void) {
	// Apply 1kHz square wave to CP with appropriate duty cycle, disable contactor
	uint32_t ma_pp = iec61851_get_ma_from_pp_resistance();
	evse_set_output(iec61851_get_duty_cycle_for_ma(ma_pp), false);
}

void iec61851_state_c(void) {
	// Apply 1kHz square wave to CP with appropriate duty cycle, enable contactor
	uint32_t ma_pp = iec61851_get_ma_from_pp_resistance();
	evse_set_output(iec61851_get_duty_cycle_for_ma(ma_pp), true);
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
	static IEC61851State last_state = IEC61851_STATE_EF;

	if(contactor_check.error != 0) {
		iec61851.state = IEC61851_STATE_EF;
	} else if(!XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN)) {
		iec61851.state = IEC61851_STATE_A;
	} else {
		// Wait for ADC measurements to be valid
		if(ads1118.cp_invalid_counter > 0) {
			return;
		}

		if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_A) {
			iec61851.state = IEC61851_STATE_A;
		} else if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_B) {
			iec61851.state = IEC61851_STATE_B;
		} else if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_C) {
			iec61851.state = IEC61851_STATE_C;
		} else if(ads1118.cp_pe_resistance > IEC61851_CP_RESISTANCE_STATE_D) {
			iec61851.state = IEC61851_STATE_D;
		} else {
			iec61851.state = IEC61851_STATE_EF;
		}
	}

	if(iec61851.state != last_state) {
		last_state = iec61851.state;
#if 0
		logd("New State: %c\n\r", 'A' + iec61851.state);
		logd("PP adc %d, vol %d, res %u\n\r", ads1118.pp_adc_value, ads1118.pp_voltage, ads1118.pp_pe_resistance);
		logd("CP adc %d, vol %d, high %d, res %u\n\r\n\r", ads1118.cp_adc_value, ads1118.cp_voltage, ads1118.cp_high_voltage, ads1118.cp_pe_resistance);
#endif
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
}

