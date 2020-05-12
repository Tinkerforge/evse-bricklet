/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * evse.h: EVSE implementation
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

#ifndef EVSE_H
#define EVSE_H

#include <stdint.h>
#include <stdbool.h>

#define EVSE_CP_PWM_PERIOD    64000 // 1kHz
#define EVSE_MOTOR_PWM_PERIOD 6400  // 10kHz

typedef struct {
	bool     low_level_mode_enabled;
	uint16_t low_level_cp_duty_cycle;
	bool     low_level_motor_direction;
	uint16_t low_level_motor_duty_cycle;
	uint16_t low_level_relay_enabled;
    uint32_t low_level_relay_monoflop;

    uint32_t startup_time;
} EVSE;

extern EVSE evse;
void evse_set_output(const uint16_t cp_duty_cycle, const bool contactor);
void evse_init(void);
void evse_tick(void);

#endif