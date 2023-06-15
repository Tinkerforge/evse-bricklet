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
#include "bricklib2/warp/contactor_check.h"

#include "configs/config_evse.h"
#include "configs/config_contactor_check.h"
#include "evse.h"
#include "ads1118.h"
#include "iec61851.h"
#include "led.h"
#include "lock.h"
#include "button.h"
#include "charging_slot.h"

#define LOW_LEVEL_PASSWORD 0x4223B00B

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	// Restart communication watchdog timer.
	evse.communication_watchdog_time = system_timer_get_ms();

	switch(tfp_get_fid_from_message(message)) {
		case FID_GET_STATE: return get_state(message, response);
		case FID_GET_HARDWARE_CONFIGURATION: return get_hardware_configuration(message, response);
		case FID_GET_LOW_LEVEL_STATE: return get_low_level_state(message, response);
		case FID_SET_CHARGING_SLOT: return set_charging_slot(message);
		case FID_SET_CHARGING_SLOT_MAX_CURRENT: return set_charging_slot_max_current(message);
		case FID_SET_CHARGING_SLOT_ACTIVE: return set_charging_slot_active(message);
		case FID_SET_CHARGING_SLOT_CLEAR_ON_DISCONNECT: return set_charging_slot_clear_on_disconnect(message);
		case FID_GET_CHARGING_SLOT: return get_charging_slot(message, response);
		case FID_GET_ALL_CHARGING_SLOTS: return get_all_charging_slots(message, response);
		case FID_SET_CHARGING_SLOT_DEFAULT: return set_charging_slot_default(message);
		case FID_GET_CHARGING_SLOT_DEFAULT: return get_charging_slot_default(message, response);
		case FID_CALIBRATE: return calibrate(message, response);
		case FID_GET_USER_CALIBRATION: return get_user_calibration(message, response);
		case FID_SET_USER_CALIBRATION: return set_user_calibration(message);
		case FID_GET_DATA_STORAGE: return get_data_storage(message, response);
		case FID_SET_DATA_STORAGE: return set_data_storage(message);
		case FID_GET_INDICATOR_LED: return get_indicator_led(message, response);
		case FID_SET_INDICATOR_LED: return set_indicator_led(message, response);
		case FID_GET_BUTTON_STATE: return get_button_state(message, response);
		case FID_GET_ALL_DATA_1: return get_all_data_1(message, response);
		case FID_FACTORY_RESET: return factory_reset(message);
		case FID_SET_BOOST_MODE: return set_boost_mode(message);
		case FID_GET_BOOST_MODE: return get_boost_mode(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}


BootloaderHandleMessageResponse get_state(const GetState *data, GetState_Response *response) {
	response->header.length            = sizeof(GetState_Response);
	response->iec61851_state           = iec61851.state;
	response->contactor_state          = contactor_check.state;
	response->contactor_error          = contactor_check.error;
	response->allowed_charging_current = iec61851_get_max_ma();
	response->error_state              = led.state == LED_STATE_BLINKING ? led.blink_num : 0;
	response->lock_state               = lock.state;

	if((iec61851.state == IEC61851_STATE_D) || (iec61851.state == IEC61851_STATE_EF)) {
		response->charger_state = EVSE_CHARGER_STATE_ERROR;
	} else if(iec61851.state == IEC61851_STATE_C) {
		response->charger_state = EVSE_CHARGER_STATE_CHARGING;
	} else if(iec61851.state == IEC61851_STATE_B) {
		if(charging_slot_get_max_current() == 0) {
			response->charger_state = EVSE_CHARGER_STATE_WAITING_FOR_CHARGE_RELEASE;
		} else {
			response->charger_state = EVSE_CHARGER_STATE_READY_TO_CHARGE;
		}
	} else { 
		response->charger_state = EVSE_CHARGER_STATE_NOT_CONNECTED;
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_hardware_configuration(const GetHardwareConfiguration *data, GetHardwareConfiguration_Response *response) {
	response->header.length        = sizeof(GetHardwareConfiguration_Response);
	response->jumper_configuration = evse.config_jumper_current;
	response->has_lock_switch      = evse.has_lock_switch;
	response->evse_version         = ads1118.is_v15 ? 15 : 14;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_low_level_state(const GetLowLevelState *data, GetLowLevelState_Response *response) {
	response->header.length            = sizeof(GetLowLevelState_Response);
	response->led_state                = led.state;
	response->cp_pwm_duty_cycle        = evse_get_cp_duty_cycle();
	response->adc_values[0]            = ads1118.cp_adc_value;
	response->adc_values[1]            = ads1118.pp_adc_value;
	response->voltages[0]              = ads1118.cp_voltage_calibrated;
	response->voltages[1]              = ads1118.pp_voltage;
	response->voltages[2]              = ads1118.cp_high_voltage;
	response->resistances[0]           = ads1118.cp_pe_resistance;
	response->resistances[1]           = ads1118.pp_pe_resistance;
	response->gpio[0]                  = XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN) | (XMC_GPIO_GetInput(EVSE_OUTPUT_GP_PIN) << 1) | (XMC_GPIO_GetInput(EVSE_MOTOR_INPUT_SWITCH_PIN) << 2) | (XMC_GPIO_GetInput(EVSE_RELAY_PIN) << 3) | (XMC_GPIO_GetInput(EVSE_MOTOR_FAULT_PIN) << 4);

	if(evse.charging_time == 0) {
		response->charging_time        = 0;
	} else {
		response->charging_time        = system_timer_get_ms() - evse.charging_time;
	}
	response->uptime                   = system_timer_get_ms();
	response->time_since_state_change  = response->uptime - iec61851.last_state_change;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_charging_slot(const SetChargingSlot *data) {
	// The first two slots are read-only
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if((data->max_current > 0) && ((data->max_current < 6000) || (data->max_current > 32000))) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	// If button is pressed (key switch is turned off) we don't allow to change the max current in the button slot
	if((data->slot != CHARGING_SLOT_BUTTON) || (button.state != BUTTON_STATE_PRESSED)) {
		charging_slot.max_current[data->slot]     = data->max_current;
	}
	charging_slot.active[data->slot]              = data->active;
	charging_slot.clear_on_disconnect[data->slot] = data->clear_on_disconnect;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_charging_slot_max_current(const SetChargingSlotMaxCurrent *data) {
	// The first two slots are read-only
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if((data->max_current > 0) && ((data->max_current < 6000) || (data->max_current > 32000))) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	// If button is pressed (key switch is turned off) we don't allow to change the max current in the button slot
	if((data->slot != CHARGING_SLOT_BUTTON) || (button.state != BUTTON_STATE_PRESSED)) {
		charging_slot.max_current[data->slot] = data->max_current;
		if((data->slot == CHARGING_SLOT_BUTTON) && (charging_slot.max_current[data->slot] == 0)) {
			button.was_pressed = true;
		}
	}

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_charging_slot_active(const SetChargingSlotActive *data) {
	// The first two slots are read-only
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	charging_slot.active[data->slot] = data->active;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse set_charging_slot_clear_on_disconnect(const SetChargingSlotClearOnDisconnect *data) {
	// The first two slots are read-only
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	charging_slot.clear_on_disconnect[data->slot] = data->clear_on_disconnect;

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_charging_slot(const GetChargingSlot *data, GetChargingSlot_Response *response) {
	if(data->slot >= CHARGING_SLOT_NUM) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length       = sizeof(GetChargingSlot_Response);
	response->max_current         = charging_slot.max_current[data->slot];
	response->active              = charging_slot.active[data->slot];
	response->clear_on_disconnect = charging_slot.clear_on_disconnect[data->slot];

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_charging_slots(const GetAllChargingSlots *data, GetAllChargingSlots_Response *response) {
	response->header.length = sizeof(GetAllChargingSlots_Response);
	for(uint8_t i = 0; i < CHARGING_SLOT_NUM; i++) {
		response->max_current[i]                    = charging_slot.max_current[i];
		response->active_and_clear_on_disconnect[i] = (charging_slot.active[i] << 0) | (charging_slot.clear_on_disconnect[i] << 1);
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_charging_slot_default(const SetChargingSlotDefault *data) {
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if((data->max_current > 0) && ((data->max_current < 6000) || (data->max_current > 32000))) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	const uint8_t slot = data->slot - 2;

	charging_slot.max_current_default[slot]         = data->max_current;
	charging_slot.active_default[slot]              = data->active;
	charging_slot.clear_on_disconnect_default[slot] = data->clear_on_disconnect;

	evse_save_config();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_charging_slot_default(const GetChargingSlotDefault *data, GetChargingSlotDefault_Response *response) {
	if((data->slot < 2) || (data->slot >= CHARGING_SLOT_NUM)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	const uint8_t slot = data->slot - 2;

	response->header.length       = sizeof(GetChargingSlotDefault_Response);
	response->max_current         = charging_slot.max_current_default[slot];
	response->active              = charging_slot.active_default[slot];
	response->clear_on_disconnect = charging_slot.clear_on_disconnect_default[slot];

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
	} else if((evse.calibration_state >= 2) && (evse.calibration_state <= 15) && (data->state == (evse.calibration_state + 1))) {
		ads1118.cp_cal_880ohm[evse.calibration_state-2] = ads1118.cp_cal_max_voltage - (910*(ads1118.cp_high_voltage - ADS1118_DIODE_DROP) + 880*ads1118.cp_high_voltage)/880;

		response->success = true;
		logd("cal 880ohm %d -> %d\n\r", evse.calibration_state-2, ads1118.cp_cal_880ohm[evse.calibration_state-2]);

	    evse.calibration_state++;
		if(evse.calibration_state < 16) {
			uint16_t dc = iec61851_get_duty_cycle_for_ma(6000 + (evse.calibration_state-2)*2000);
			ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 64000 - dc*64);	
		} else if(evse.calibration_state == 16) {
			// Set duty cycle to 0%
			ccu4_pwm_set_duty_cycle(EVSE_CP_PWM_SLICE_NUMBER, 64000 - 0*64);
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

BootloaderHandleMessageResponse get_user_calibration(const GetUserCalibration *data, GetUserCalibration_Response *response) {
	response->header.length           = sizeof(GetUserCalibration_Response);
	response->user_calibration_active = ads1118.cp_user_cal_active;

	if(ads1118.cp_user_cal_active) {
		response->voltage_mul     = ads1118.cp_user_cal_mul;
		response->voltage_div     = ads1118.cp_user_cal_div;
		response->voltage_diff    = ads1118.cp_user_cal_diff_voltage;
		response->resistance_2700 = ads1118.cp_user_cal_2700ohm;

		for(uint8_t i = 0; i < ADS1118_880OHM_CAL_NUM; i++) {
			response->resistance_880[i] = ads1118.cp_user_cal_880ohm[i];
		}
	} else {
		response->voltage_mul     = ads1118.cp_cal_mul;
		response->voltage_div     = ads1118.cp_cal_div;
		response->voltage_diff    = ads1118.cp_cal_diff_voltage;
		response->resistance_2700 = ads1118.cp_cal_2700ohm;

		for(uint8_t i = 0; i < ADS1118_880OHM_CAL_NUM; i++) {
			response->resistance_880[i] = ads1118.cp_cal_880ohm[i];
		}
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_user_calibration(const SetUserCalibration *data) {
	if(data->password != 0xCA11B4A0) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if(data->user_calibration_active) {
		if(data->voltage_div == 0) {
			return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
		}

		ads1118.cp_user_cal_active       = true;
		ads1118.cp_user_cal_mul          = data->voltage_mul;
		ads1118.cp_user_cal_div          = data->voltage_div;
		ads1118.cp_user_cal_diff_voltage = data->voltage_diff;
		ads1118.cp_user_cal_2700ohm      = data->resistance_2700;

		for(uint8_t i = 0; i < ADS1118_880OHM_CAL_NUM; i++) {
			ads1118.cp_user_cal_880ohm[i] = data->resistance_880[i];
		}
	} else {
		ads1118.cp_user_cal_active       = false;
		ads1118.cp_user_cal_mul          = 1;
		ads1118.cp_user_cal_div          = 1;
		ads1118.cp_user_cal_diff_voltage = -90;
		ads1118.cp_user_cal_2700ohm      = 0;

		for(uint8_t i = 0; i < ADS1118_880OHM_CAL_NUM; i++) {
			ads1118.cp_user_cal_880ohm[i] = 0;
		}
	}
	evse_save_user_calibration();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_data_storage(const GetDataStorage *data, GetDataStorage_Response *response) {
	if(data->page >= EVSE_STORAGE_PAGES) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(GetDataStorage_Response);
	memcpy(response->data, evse.storage[data->page], 63);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_data_storage(const SetDataStorage *data) {
	if(data->page >= EVSE_STORAGE_PAGES) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	memcpy(evse.storage[data->page], data->data, 63);

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_indicator_led(const GetIndicatorLED *data, GetIndicatorLED_Response *response) {
	response->header.length = sizeof(GetIndicatorLED_Response);
	response->indication    = led.api_indication;
	if((led.api_duration == 0) || system_timer_is_time_elapsed_ms(led.api_start, led.api_duration)) {
		response->duration  = 0;
	} else {
		response->duration  = led.api_duration - ((uint32_t)(system_timer_get_ms() - led.api_start));
	}


	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse set_indicator_led(const SetIndicatorLED *data, SetIndicatorLED_Response *response) {
	if((data->indication >= 256) && (data->indication != 1001) && (data->indication != 1002) && (data->indication != 1003)) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	response->header.length = sizeof(SetIndicatorLED_Response);

	// If the state and indication stays the same we just update the duration
	// This way the animation does not become choppy
	if((led.state == LED_STATE_API) && (led.api_indication == data->indication)) {
		led.api_duration = data->duration;
		led.api_start    = system_timer_get_ms();
		response->status = 0;
		return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
	}

	// Otherwise we reset the current animation and start the new one
	if((led.state == LED_STATE_OFF) || (led.state == LED_STATE_ON) || (led.state == LED_STATE_API)) {
		response->status     = 0;

		led.api_ack_counter  = 0;
		led.api_ack_index    = 0;
		led.api_ack_time     = 0;

		led.api_nack_counter = 0;
		led.api_nack_index   = 0;
		led.api_nack_time    = 0;

		led.api_nag_counter  = 0;
		led.api_nag_index    = 0;
		led.api_nag_time     = 0;

		if(data->indication < 0) {
			// If LED state is currently LED_STATE_OFF or LED_STATE_ON we
			// leave the LED where it is (and don't restart the standby timer).
			// If LED state is currently in LED_STATE_API we
			// turn the LED on (which brings it in LED_STATE_ON) and restarts the standby timer.
			if(led.state == LED_STATE_API) {
				led_set_on(true);
			}
		} else {
			led.state          = LED_STATE_API;
			led.api_indication = data->indication;
			led.api_duration   = data->duration;
			led.api_start      = system_timer_get_ms();
		}
	} else {
		response->status = led.state;
	}

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_button_state(const GetButtonState *data, GetButtonState_Response *response) {
	response->header.length       = sizeof(GetButtonState_Response);
	response->button_press_time   = button.press_time;
	response->button_release_time = button.release_time;
	response->button_pressed      = button.state == BUTTON_STATE_PRESSED;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse get_all_data_1(const GetAllData1 *data, GetAllData1_Response *response) {
	response->header.length = sizeof(GetAllData1_Response);

	TFPMessageFull parts;

	get_state(NULL, (GetState_Response*)&parts);
	memcpy(&response->iec61851_state, parts.data, sizeof(GetState_Response) - sizeof(TFPMessageHeader));

	get_hardware_configuration(NULL, (GetHardwareConfiguration_Response*)&parts);
	memcpy(&response->jumper_configuration, parts.data, sizeof(GetHardwareConfiguration_Response) - sizeof(TFPMessageHeader));

	get_low_level_state(NULL, (GetLowLevelState_Response*)&parts);
	memcpy(&response->led_state, parts.data, sizeof(GetLowLevelState_Response) - sizeof(TFPMessageHeader));

	get_indicator_led(NULL, (GetIndicatorLED_Response*)&parts);
	memcpy(&response->indication, parts.data, sizeof(GetIndicatorLED_Response) - sizeof(TFPMessageHeader));

	get_button_state(NULL, (GetButtonState_Response*)&parts);
	memcpy(&response->button_press_time, parts.data, sizeof(GetButtonState_Response) - sizeof(TFPMessageHeader));

	get_boost_mode(NULL, (GetBoostMode_Response*)&parts);
	memcpy(&response->boost_mode_enabled, parts.data, sizeof(GetBoostMode_Response) - sizeof(TFPMessageHeader));

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}

BootloaderHandleMessageResponse factory_reset(const FactoryReset *data) {
	if(data->password == 0x2342FACD) {
		evse.factory_reset_time = system_timer_get_ms();
		return HANDLE_MESSAGE_RESPONSE_EMPTY;
	}

	return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
}

BootloaderHandleMessageResponse set_boost_mode(const SetBoostMode *data) {
	evse.boost_mode_enabled = data->boost_mode_enabled;
	evse_save_config();

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
}

BootloaderHandleMessageResponse get_boost_mode(const GetBoostMode *data, GetBoostMode_Response *response) {
	response->header.length      = sizeof(GetBoostMode_Response);
	response->boost_mode_enabled = evse.boost_mode_enabled;

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}


void communication_tick(void) {
//	communication_callback_tick();
}

void communication_init(void) {
//	communication_callback_init();
}
