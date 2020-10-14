/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication.c: TFP protocol message handling
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

#include "communication.h"

#include "bricklib2/utility/communication_callback.h"
#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/hal/ccu4_pwm/ccu4_pwm.h"
#include "bricklib2/logging/logging.h"
#include "bricklib2/utility/util_definitions.h"

#include "configs/config_evse.h"
#include "configs/config_contactor_check.h"
#include "evse.h"
#include "ads1118.h"
#include "iec61851.h"
#include "led.h"
#include "contactor_check.h"
#include "lock.h"

#define LOW_LEVEL_PASSWORD 0x4223B00B

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case FID_GET_STATE: return get_state(message, response);
		case FID_GET_HARDWARE_CONFIGURATION: return get_hardware_configuration(message, response);
		case FID_GET_LOW_LEVEL_STATE: return get_low_level_state(message, response);
		case FID_SET_MAX_CHARGING_CURRENT: return set_max_charging_current(message);
		case FID_GET_MAX_CHARGING_CURRENT: return get_max_charging_current(message, response);
		case FID_CALIBRATE: return calibrate(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}


BootloaderHandleMessageResponse get_state(const GetState *data, GetState_Response *response) {
	response->header.length            = sizeof(GetState_Response);
	response->iec61851_state           = iec61851.state;
	response->contactor_state          = contactor_check.state;
	response->contactor_error          = contactor_check.error;
	response->allowed_charging_current = iec61851_get_max_ma();
	response->lock_state               = lock.state;
	response->uptime                   = system_timer_get_ms();
	response->time_since_state_change  = response->uptime - iec61851.last_state_change;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_hardware_configuration(const GetHardwareConfiguration *data, GetHardwareConfiguration_Response *response) {
	response->header.length        = sizeof(GetHardwareConfiguration_Response);
	response->jumper_configuration = evse.config_jumper_current;
	response->has_lock_switch      = evse.has_lock_switch;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_low_level_state(const GetLowLevelState *data, GetLowLevelState_Response *response) {
	response->header.length          = sizeof(GetLowLevelState_Response);
	response->low_level_mode_enabled = evse.low_level_mode_enabled;
	response->led_state              = led.state;
	response->cp_pwm_duty_cycle      = (64000 - ccu4_pwm_get_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER))/64;
	response->adc_values[0]          = ads1118.cp_adc_value;
	response->adc_values[1]          = ads1118.pp_adc_value;
	response->voltages[0]            = ads1118.cp_voltage_calibrated;
	response->voltages[1]            = ads1118.pp_voltage;
	response->voltages[2]            = ads1118.cp_high_voltage;
	response->resistances[0]         = ads1118.cp_pe_resistance;
	response->resistances[1]         = ads1118.pp_pe_resistance;
	response->gpio[0]                = XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN) | (XMC_GPIO_GetInput(EVSE_OUTPUT_GP_PIN) << 1) | (XMC_GPIO_GetInput(EVSE_MOTOR_INPUT_SWITCH_PIN) << 2) | (XMC_GPIO_GetInput(EVSE_RELAY_PIN) << 3) | (XMC_GPIO_GetInput(EVSE_MOTOR_FAULT_PIN) << 4);
	response->motor_direction        = evse.low_level_motor_direction;
	response->motor_duty_cycle       = evse.low_level_motor_duty_cycle;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_max_charging_current(const SetMaxChargingCurrent *data) {
	// Use a minimum of 6A and a maximum of 32A.
	evse.max_current_configured = BETWEEN(6000, data->max_current, 32000);

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_max_charging_current(const GetMaxChargingCurrent *data, GetMaxChargingCurrent_Response *response) {
	response->header.length = sizeof(GetMaxChargingCurrent_Response);
	response->max_current_configured     = evse.max_current_configured;
	response->max_current_outgoing_cable = iec61851_get_ma_from_pp_resistance();
	response->max_current_incoming_cable = iec61851_get_ma_from_jumper();

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse calibrate(const Calibrate *data, Calibrate_Response *response) {
	response->header.length = sizeof(Calibrate_Response);
	logd("calibrate (iec61851.state %d): %d %x -> %d\n\r", iec61851.state, data->state, data->password, data->value);
	if(((ads1118.cp_pe_resistance != 0xFFFF) && (evse.calibration_state == 0)) || (data->password != (0x0BB03200 + data->state))) {
		response->success = false;
		return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
	}

	logd("calibration_state: %d\n\r", evse.calibration_state);
    if((evse.calibration_state == 0) && (data->state == 1)) {
	    evse.calibration_state = 1;
		ads1118.cp_cal_mul = data->value;        // multiply by calibrated voltage
		ads1118.cp_cal_div = ads1118.cp_voltage; // divide by uncalibrated voltage

		// Set duty cycle to 0%
		ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 64000 - 0*64);
		response->success = true;
		logd("cal mul %d, div %d\n\r", ads1118.cp_cal_mul, ads1118.cp_cal_div);
	} else if((evse.calibration_state == 1) && (data->state == 2)) {
	    evse.calibration_state = 0;
		ads1118.cp_cal_diff_voltage = data->value;

		// Set duty cycle backto 100%
		ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 64000 - 1000*64);
		response->success = true;

		evse_save_calibration();

	// Currently only two states/values are calibrated
	// Other values may be calibrated in the future
	} else { 
		response->success = false;
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}


void communication_tick(void) {
//	communication_callback_tick();
}

void communication_init(void) {
//	communication_callback_init();
}
