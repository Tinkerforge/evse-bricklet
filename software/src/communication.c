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

#include "xmc_gpio.h"

#include "configs/config_evse.h"

BootloaderHandleMessageResponse handle_message(const void *message, void *response) {
	switch(tfp_get_fid_from_message(message)) {
		case FID_TEST: return test(message, response);
		default: return HANDLE_MESSAGE_RESPONSE_NOT_SUPPORTED;
	}
}


BootloaderHandleMessageResponse test(const Test *data, Test_Response *response) {
	if(data->data[0]) {
		XMC_GPIO_SetOutputHigh(EVSE_RELAY_PIN);
	} else {
		XMC_GPIO_SetOutputLow(EVSE_RELAY_PIN);
	}

	if(data->data[1]) {
		XMC_GPIO_SetOutputHigh(EVSE_MOTOR_PHASE_PIN);
	} else {
		XMC_GPIO_SetOutputLow(EVSE_MOTOR_PHASE_PIN);
	}

	if(data->data[2]) {
		XMC_GPIO_SetOutputHigh(EVSE_MOTOR_ENABLE_PIN);
	} else {
		XMC_GPIO_SetOutputLow(EVSE_MOTOR_ENABLE_PIN);
	}

	if(data->data[3]) {
		XMC_GPIO_SetOutputHigh(EVSE_CHARGE_LED_PIN);
	} else {
		XMC_GPIO_SetOutputLow(EVSE_CHARGE_LED_PIN);
	}

	if(data->data[4]) {
		XMC_GPIO_SetOutputHigh(EVSE_ERROR_LED_PIN);
	} else {
		XMC_GPIO_SetOutputLow(EVSE_ERROR_LED_PIN);
	}

	if(data->data[5]) {
		XMC_GPIO_SetOutputHigh(EVSE_CP_PWM_PIN);
	} else {
		XMC_GPIO_SetOutputLow(EVSE_CP_PWM_PIN);
	}

	response->header.length = sizeof(Test_Response);
	response->data[0] = XMC_GPIO_GetInput(EVSE_INPUT_GP_PIN);
	response->data[1] = XMC_GPIO_GetInput(EVSE_AC1_PIN);
	response->data[2] = XMC_GPIO_GetInput(EVSE_AC2_PIN);
	response->data[3] = XMC_GPIO_GetInput(EVSE_INPUT_MOTOR_SWITCH_PIN);
	response->data[4] = XMC_GPIO_GetInput(EVSE_MOTOR_FAULT_PIN);
	response->data[5] = XMC_GPIO_GetInput(EVSE_CP_PWM_PIN);

	return HANDLE_MESSAGE_RESPONSE_NEW_MESSAGE;
}





void communication_tick(void) {
	communication_callback_tick();
}

void communication_init(void) {
	communication_callback_init();
}
