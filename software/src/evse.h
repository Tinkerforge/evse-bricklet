/* evse-bricklet
 * Copyright (C) 2020-2022 Olaf Lüke <olaf@tinkerforge.com>
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

#define EVSE_CONFIG_JUMPER_CURRENT_6A   0
#define EVSE_CONFIG_JUMPER_CURRENT_10A  1
#define EVSE_CONFIG_JUMPER_CURRENT_13A  2
#define EVSE_CONFIG_JUMPER_CURRENT_16A  3
#define EVSE_CONFIG_JUMPER_CURRENT_20A  4
#define EVSE_CONFIG_JUMPER_CURRENT_25A  5
#define EVSE_CONFIG_JUMPER_CURRENT_32A  6
#define EVSE_CONFIG_JUMPER_SOFTWARE     7
#define EVSE_CONFIG_JUMPER_UNCONFIGURED 8

#define EVSE_CALIBRATION_PAGE           1
#define EVSE_CALIBRATION_MAGIC_POS      0
#define EVSE_CALIBRATION_MUL_POS        1
#define EVSE_CALIBRATION_DIV_POS        2
#define EVSE_CALIBRATION_DIFF_POS       3
#define EVSE_CALIBRATION_2700_POS       4
#define EVSE_CALIBRATION_880_POS        5

#define EVSE_CALIBRATION_MAGIC          0x12345678

#define EVSE_USER_CALIBRATION_PAGE      2
#define EVSE_USER_CALIBRATION_MAGIC_POS 0
#define EVSE_USER_CALIBRATION_ACTIV_POS 1
#define EVSE_USER_CALIBRATION_MUL_POS   2
#define EVSE_USER_CALIBRATION_DIV_POS   3
#define EVSE_USER_CALIBRATION_DIFF_POS  4
#define EVSE_USER_CALIBRATION_2700_POS  5
#define EVSE_USER_CALIBRATION_880_POS   6

#define EVSE_USER_CALIBRATION_MAGIC     0x23456789

#define EVSE_CONFIG_PAGE                3
#define EVSE_CONFIG_MAGIC_POS           0
#define EVSE_CONFIG_MANAGED_POS         1
#define EVSE_CONFIG_MAGIC2_POS          2
#define EVSE_CONFIG_BOOST_POS           3
#define EVSE_CONFIG_MAGIC3_POS          4
#define EVSE_CONFIG_SLOT_DEFAULT_POS    48

typedef struct {
	uint16_t current[18];
	uint8_t active_clear[18];
	uint32_t magic;
} __attribute__((__packed__)) EVSEChargingSlotDefault;

#define EVSE_CONFIG_MAGIC               0x34567890
#define EVSE_CONFIG_MAGIC2              0x45678923
#define EVSE_CONFIG_MAGIC3              0x56789234
#define EVSE_CONFIG_SLOT_MAGIC          0x62870616

#define EVSE_STORAGE_PAGES              16

typedef struct {
	uint32_t startup_time;

	uint8_t config_jumper_current;
	uint16_t config_jumper_current_software;

	bool has_lock_switch;
	bool legacy_managed;

	uint32_t factory_reset_time;

	uint8_t calibration_state;
	uint32_t calibration_time;

	int32_t calibration_adc_min;
	int32_t calibration_adc_max;

	uint16_t max_current_configured;
	bool calibration_error;

	bool car_stopped_charging;

	uint32_t communication_watchdog_time;

	uint32_t contactor_turn_off_time;

	bool boost_mode_enabled;

	uint8_t storage[EVSE_STORAGE_PAGES][64];
} EVSE;

extern EVSE evse;

void evse_save_config(void);
void evse_save_calibration(void);
void evse_save_user_calibration(void);
void evse_save_config(void);
void evse_set_output(const uint16_t cp_duty_cycle, const bool contactor);
uint16_t evse_get_cp_duty_cycle(void);
void evse_set_cp_duty_cycle(const uint16_t duty_cycle);
void evse_init(void);
void evse_tick(void);

#endif