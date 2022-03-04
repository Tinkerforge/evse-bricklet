/* evse-bricklet
 * Copyright (C) 2020-2022 Olaf Lüke <olaf@tinkerforge.com>
 *
 * main.c: Initialization for EVSE Bricklet
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

#include <stdio.h>
#include <stdbool.h>

#include "configs/config.h"

#include "bricklib2/bootloader/bootloader.h"
#include "bricklib2/hal/system_timer/system_timer.h"
#include "bricklib2/logging/logging.h"
#include "communication.h"

#include "evse.h"
#include "ads1118.h"
#include "iec61851.h"
#include "lock.h"
#include "contactor_check.h"
#include "led.h"
#include "button.h"
#include "charging_slot.h"

int main(void) {
	logging_init();
	logd("Start EVSE Bricklet\n\r");

	communication_init();
	evse_init();
	charging_slot_init();
	ads1118_init();
	iec61851_init();
	lock_init();
	contactor_check_init();
	led_init();
	button_init();

	while(true) {
		bootloader_tick();
		communication_tick();
		evse_tick();
		ads1118_tick();
//		lock_tick();
		contactor_check_tick();
		led_tick();
		button_tick();
		charging_slot_tick();
	}
}
