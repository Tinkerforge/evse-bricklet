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
#include "button.h"

#define LOW_LEVEL_PASSWORD 0x4223B00B

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case FID_GET_STATE: return get_state(message, response);
		case FID_GET_HARDWARE_CONFIGURATION: return get_hardware_configuration(message, response);
		case FID_GET_LOW_LEVEL_STATE: return get_low_level_state(message, response);
		case FID_SET_MAX_CHARGING_CURRENT: return set_max_charging_current(message);
		case FID_GET_MAX_CHARGING_CURRENT: return get_max_charging_current(message, response);
		case FID_CALIBRATE: return calibrate(message, response);
		case FID_START_CHARGING: return start_charging(message);
		case FID_STOP_CHARGING: return stop_charging(message);
		case FID_SET_CHARGING_AUTOSTART: return set_charging_autostart(message);
		case FID_GET_CHARGING_AUTOSTART: return get_charging_autostart(message, response);
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

	if((iec61851.state == IEC61851_STATE_D) || (iec61851.state == IEC61851_STATE_EF)) {
		response->vehicle_state = EVSE_VEHICLE_STATE_ERROR;
	} else if(iec61851.state == IEC61851_STATE_C) {
		response->vehicle_state = EVSE_VEHICLE_STATE_CHARGING;
	} else if(iec61851.state == IEC61851_STATE_B) {
		response->vehicle_state = EVSE_VEHICLE_STATE_CONNECTED;
	} else { 
		// For state A we may be not connected or connected with autostart disabled.
		// We check this by looking at the CP/PE resistance. We expect at least 10000 ohm if a vehicle is not connected.
		if(ads1118.cp_pe_resistance > 10000) {
			response->vehicle_state = EVSE_VEHICLE_STATE_NOT_CONNECTED;
		} else {
			response->vehicle_state = EVSE_VEHICLE_STATE_CONNECTED;
		}
	}

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

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_max_charging_current(const SetMaxChargingCurrent *data) {
	// Use a minimum of 6A and a maximum of 32A.
	evse.max_current_configured = BETWEEN(6000, data->max_current, 32000);

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_max_charging_current(const GetMaxChargingCurrent *data, GetMaxChargingCurrent_Response *response) {
	response->header.length              = sizeof(GetMaxChargingCurrent_Response);
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

		response->success = true;
		logd("cal mul %d, div %d\n\r", ads1118.cp_cal_mul, ads1118.cp_cal_div);
	} else if((evse.calibration_state == 1) && (data->state == 2)) {
	    evse.calibration_state = 2;
		ads1118.cp_cal_2700ohm = ads1118.cp_cal_max_voltage - (910*(ads1118.cp_high_voltage - ADS1118_DIODE_DROP) + 2700*ads1118.cp_high_voltage)/2700;

		response->success = true;
		logd("cal 2700ohm %d\n\r", ads1118.cp_cal_2700ohm);

		uint16_t dc = iec61851_get_duty_cycle_for_ma(6000);
		ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 64000 - dc*64);
		evse.low_level_cp_duty_cycle = dc;
	} else if((evse.calibration_state >= 2) && (evse.calibration_state <= 15) && (data->state == (evse.calibration_state + 1))) {
		ads1118.cp_cal_880ohm[evse.calibration_state-2] = ads1118.cp_cal_max_voltage - (910*(ads1118.cp_high_voltage - ADS1118_DIODE_DROP) + 880*ads1118.cp_high_voltage)/880;

		response->success = true;
		logd("cal 880ohm %d -> %d\n\r", evse.calibration_state-2, ads1118.cp_cal_880ohm[evse.calibration_state-2]);

	    evse.calibration_state++;
		if(evse.calibration_state < 16) {
			uint16_t dc = iec61851_get_duty_cycle_for_ma(6000 + (evse.calibration_state-2)*2000);
			ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 64000 - dc*64);	
			evse.low_level_cp_duty_cycle = dc;
		} else if(evse.calibration_state == 16) {
			// Set duty cycle to 0%
			ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 64000 - 0*64);
			evse.low_level_cp_duty_cycle = 1000;
		}
	} else if((evse.calibration_state == 16) && (data->state == 17)) {
	    evse.calibration_state = 0;
		ads1118.cp_cal_diff_voltage = data->value;

		// Set duty cycle back to 100%
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

BootloaderHandleMessageResponse start_charging(const StartCharging *data) {
	// Starting a new charge is the same as "releasing" a button press
	button.was_pressed = false;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse stop_charging(const StopCharging *data) {
	// Stopping the charging is the same as pressing the button
	button.was_pressed = true;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_charging_autostart(const SetChargingAutostart *data) {
	evse.charging_autostart = data->autostart;

	// If autostart is disabled and there is not currently a car charging
	// we set "was_pressed" to make sure that the user needs to call start_charging before
	// the car starts charging.
	if(!evse.charging_autostart && (iec61851.state != IEC61851_STATE_C)) {
		button.was_pressed = true;
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_charging_autostart(const GetChargingAutostart *data, GetChargingAutostart_Response *response) {
	response->header.length = sizeof(GetChargingAutostart_Response);
	response->autostart     = evse.charging_autostart;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}


void communication_tick(void) {
//	communication_callback_tick();
}

void communication_init(void) {
//	communication_callback_init();
}
