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
#include "bricklib2/logging/logging.h"

#include "configs/config_evse.h"
#include "evse.h"
#include "ads1118.h"

#define LOW_LEVEL_PASSWORD 0x4223B00B

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case FID_SET_LOW_LEVEL_OUTPUT: return set_low_level_output(message);
		case FID_GET_LOW_LEVEL_STATUS: return get_low_level_status(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}

BootloaderHandleMessageResponse set_low_level_output(const SetLowLevelOutput *data) {
	return HANDLE_MESSAGE_RESPONSE_EMPTY;

#if 0
	logd("set_ll pw: %x\n\r", data->password);

	if(data->password != 0x4223B00B) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}
	
	if(evse.low_level_cp_duty_cycle > 1000) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	if(evse.low_level_motor_duty_cycle > 1000) {
		return HANDLE_MESSAGE_RESPONSE_INVALID_PARAMETER;
	}

	evse.low_level_mode_enabled     = data->low_level_mode_enabled;
	evse.low_level_cp_duty_cycle    = data->cp_duty_cycle;
	evse.low_level_motor_direction  = data->motor_direction;
	evse.low_level_motor_duty_cycle = data->motor_duty_cycle;
	evse.low_level_relay_enabled    = data->relay_enabled;
	evse.low_level_relay_monoflop   = system_timer_get_ms();

	logd("set_ll en: %d, cp\%: %u, motor dir: %d, motor\%: %u, relay: %d, mono: %u\n\r", 
	     evse.low_level_mode_enabled,
	     evse.low_level_cp_duty_cycle,
	     evse.low_level_motor_direction,
	     evse.low_level_motor_duty_cycle,
	     evse.low_level_relay_enabled,
	     evse.low_level_relay_monoflop
	);

	return HANDLE_MESSAGE_RESPONSE_EMPTY;
	#endif
}

BootloaderHandleMessageResponse get_low_level_status(const GetLowLevelStatus *data, GetLowLevelStatus_Response *response) {
	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
#if 0
	response->header.length          = sizeof(GetLowLevelStatus_Response);
	response->low_level_mode_enabled = evse.low_level_mode_enabled;
	response->cp_duty_cycle          = evse.low_level_cp_duty_cycle;
	response->motor_direction        = evse.low_level_motor_direction;
	response->motor_duty_cycle       = evse.low_level_motor_duty_cycle;
	response->relay_enabled          = evse.low_level_relay_enabled;
	response->cp_voltage             = ads1118.cp_voltage;
	response->pp_voltage             = ads1118.pp_voltage;
	response->ac_input[0]            = XMC_GPIO_GetInput(EVSE_AC1_PIN) | (XMC_GPIO_GetInput(EVSE_AC2_PIN) << 1);
	response->gp_input               = XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN);
	response->motor_fault            = XMC_GPIO_GetInput(EVSE_MOTOR_FAULT_PIN);
	response->motor_switch           = XMC_GPIO_GetInput(EVSE_MOTOR_INPUT_SWITCH_PIN);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
	#endif
}


void communication_tick(void) {
	communication_callback_tick();
}

void communication_init(void) {
	communication_callback_init();
}
