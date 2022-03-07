/* evse-bricklet
 * Copyright (C) 2022 Olaf Lüke <olaf@tinkerforge.com>
 *
 * communication.h: TFP protocol message handling
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

#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include <stdint.h>
#include <stdbool.h>

#include "bricklib2/protocols/tfp/tfp.h"
#include "bricklib2/bootloader/bootloader.h"

// Default functions
BootloaderHandleMessageResponse handle_message(const void *data, void *response);
void communication_tick(void);
void communication_init(void);

// Constants

#define EVSE_IEC61851_STATE_A 0
#define EVSE_IEC61851_STATE_B 1
#define EVSE_IEC61851_STATE_C 2
#define EVSE_IEC61851_STATE_D 3
#define EVSE_IEC61851_STATE_EF 4

#define EVSE_LED_STATE_OFF 0
#define EVSE_LED_STATE_ON 1
#define EVSE_LED_STATE_BLINKING 2
#define EVSE_LED_STATE_FLICKER 3
#define EVSE_LED_STATE_BREATHING 4

#define EVSE_CHARGER_STATE_NOT_CONNECTED 0
#define EVSE_CHARGER_STATE_WAITING_FOR_CHARGE_RELEASE 1
#define EVSE_CHARGER_STATE_READY_TO_CHARGE 2
#define EVSE_CHARGER_STATE_CHARGING 3
#define EVSE_CHARGER_STATE_ERROR 4

#define EVSE_CONTACTOR_STATE_AC1_NLIVE_AC2_NLIVE 0
#define EVSE_CONTACTOR_STATE_AC1_LIVE_AC2_NLIVE 1
#define EVSE_CONTACTOR_STATE_AC1_NLIVE_AC2_LIVE 2
#define EVSE_CONTACTOR_STATE_AC1_LIVE_AC2_LIVE 3

#define EVSE_LOCK_STATE_INIT 0
#define EVSE_LOCK_STATE_OPEN 1
#define EVSE_LOCK_STATE_CLOSING 2
#define EVSE_LOCK_STATE_CLOSE 3
#define EVSE_LOCK_STATE_OPENING 4
#define EVSE_LOCK_STATE_ERROR 5

#define EVSE_ERROR_STATE_OK 0
#define EVSE_ERROR_STATE_SWITCH 2
#define EVSE_ERROR_STATE_CALIBRATION 3
#define EVSE_ERROR_STATE_CONTACTOR 4
#define EVSE_ERROR_STATE_COMMUNICATION 5

#define EVSE_JUMPER_CONFIGURATION_6A 0
#define EVSE_JUMPER_CONFIGURATION_10A 1
#define EVSE_JUMPER_CONFIGURATION_13A 2
#define EVSE_JUMPER_CONFIGURATION_16A 3
#define EVSE_JUMPER_CONFIGURATION_20A 4
#define EVSE_JUMPER_CONFIGURATION_25A 5
#define EVSE_JUMPER_CONFIGURATION_32A 6
#define EVSE_JUMPER_CONFIGURATION_SOFTWARE 7
#define EVSE_JUMPER_CONFIGURATION_UNCONFIGURED 8

#define EVSE_BOOTLOADER_MODE_BOOTLOADER 0
#define EVSE_BOOTLOADER_MODE_FIRMWARE 1
#define EVSE_BOOTLOADER_MODE_BOOTLOADER_WAIT_FOR_REBOOT 2
#define EVSE_BOOTLOADER_MODE_FIRMWARE_WAIT_FOR_REBOOT 3
#define EVSE_BOOTLOADER_MODE_FIRMWARE_WAIT_FOR_ERASE_AND_REBOOT 4

#define EVSE_BOOTLOADER_STATUS_OK 0
#define EVSE_BOOTLOADER_STATUS_INVALID_MODE 1
#define EVSE_BOOTLOADER_STATUS_NO_CHANGE 2
#define EVSE_BOOTLOADER_STATUS_ENTRY_FUNCTION_NOT_PRESENT 3
#define EVSE_BOOTLOADER_STATUS_DEVICE_IDENTIFIER_INCORRECT 4
#define EVSE_BOOTLOADER_STATUS_CRC_MISMATCH 5

#define EVSE_STATUS_LED_CONFIG_OFF 0
#define EVSE_STATUS_LED_CONFIG_ON 1
#define EVSE_STATUS_LED_CONFIG_SHOW_HEARTBEAT 2
#define EVSE_STATUS_LED_CONFIG_SHOW_STATUS 3

// Function and callback IDs and structs
#define FID_GET_STATE 1
#define FID_GET_HARDWARE_CONFIGURATION 2
#define FID_GET_LOW_LEVEL_STATE 3
#define FID_SET_CHARGING_SLOT 4
#define FID_SET_CHARGING_SLOT_MAX_CURRENT 5
#define FID_SET_CHARGING_SLOT_ACTIVE 6
#define FID_SET_CHARGING_SLOT_CLEAR_ON_DISCONNECT 7
#define FID_GET_CHARGING_SLOT 8
#define FID_GET_ALL_CHARGING_SLOTS 9
#define FID_SET_CHARGING_SLOT_DEFAULT 10
#define FID_GET_CHARGING_SLOT_DEFAULT 11
#define FID_CALIBRATE 12
#define FID_GET_USER_CALIBRATION 13
#define FID_SET_USER_CALIBRATION 14
#define FID_GET_DATA_STORAGE 15
#define FID_SET_DATA_STORAGE 16
#define FID_GET_INDICATOR_LED 17
#define FID_SET_INDICATOR_LED 18
#define FID_GET_BUTTON_STATE 19
#define FID_GET_ALL_DATA_1 20


typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetState;

typedef struct {
	TFPMessageHeader header;
	uint8_t iec61851_state;
	uint8_t charger_state;
	uint8_t contactor_state;
	uint8_t contactor_error;
	uint16_t allowed_charging_current;
	uint8_t error_state;
	uint8_t lock_state;
} __attribute__((__packed__)) GetState_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetHardwareConfiguration;

typedef struct {
	TFPMessageHeader header;
	uint8_t jumper_configuration;
	bool has_lock_switch;
	uint8_t evse_version;
} __attribute__((__packed__)) GetHardwareConfiguration_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetLowLevelState;

typedef struct {
	TFPMessageHeader header;
	uint8_t led_state;
	uint16_t cp_pwm_duty_cycle;
	uint16_t adc_values[2];
	int16_t voltages[3];
	uint32_t resistances[2];
	uint8_t gpio[1];
	uint32_t charging_time;
	uint32_t time_since_state_change;
	uint32_t uptime;
} __attribute__((__packed__)) GetLowLevelState_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
	uint16_t max_current;
	bool active;
	bool clear_on_disconnect;
} __attribute__((__packed__)) SetChargingSlot;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
	uint16_t max_current;
} __attribute__((__packed__)) SetChargingSlotMaxCurrent;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
	bool active;
} __attribute__((__packed__)) SetChargingSlotActive;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
	bool clear_on_disconnect;
} __attribute__((__packed__)) SetChargingSlotClearOnDisconnect;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
} __attribute__((__packed__)) GetChargingSlot;

typedef struct {
	TFPMessageHeader header;
	uint16_t max_current;
	bool active;
	bool clear_on_disconnect;
} __attribute__((__packed__)) GetChargingSlot_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetAllChargingSlots;

typedef struct {
	TFPMessageHeader header;
	uint16_t max_current[20];
	uint8_t active_and_clear_on_disconnect[20];
} __attribute__((__packed__)) GetAllChargingSlots_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
	uint16_t max_current;
	bool active;
	bool clear_on_disconnect;
} __attribute__((__packed__)) SetChargingSlotDefault;

typedef struct {
	TFPMessageHeader header;
	uint8_t slot;
} __attribute__((__packed__)) GetChargingSlotDefault;

typedef struct {
	TFPMessageHeader header;
	uint16_t max_current;
	bool active;
	bool clear_on_disconnect;
} __attribute__((__packed__)) GetChargingSlotDefault_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t state;
	uint32_t password;
	int32_t value;
} __attribute__((__packed__)) Calibrate;

typedef struct {
	TFPMessageHeader header;
	bool success;
} __attribute__((__packed__)) Calibrate_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetUserCalibration;

typedef struct {
	TFPMessageHeader header;
	bool user_calibration_active;
	int16_t voltage_diff;
	int16_t voltage_mul;
	int16_t voltage_div;
	int16_t resistance_2700;
	int16_t resistance_880[14];
} __attribute__((__packed__)) GetUserCalibration_Response;

typedef struct {
	TFPMessageHeader header;
	uint32_t password;
	bool user_calibration_active;
	int16_t voltage_diff;
	int16_t voltage_mul;
	int16_t voltage_div;
	int16_t resistance_2700;
	int16_t resistance_880[14];
} __attribute__((__packed__)) SetUserCalibration;

typedef struct {
	TFPMessageHeader header;
	uint8_t page;
} __attribute__((__packed__)) GetDataStorage;

typedef struct {
	TFPMessageHeader header;
	uint8_t data[63];
} __attribute__((__packed__)) GetDataStorage_Response;

typedef struct {
	TFPMessageHeader header;
	uint8_t page;
	uint8_t data[63];
} __attribute__((__packed__)) SetDataStorage;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetIndicatorLED;

typedef struct {
	TFPMessageHeader header;
	int16_t indication;
	uint16_t duration;
} __attribute__((__packed__)) GetIndicatorLED_Response;

typedef struct {
	TFPMessageHeader header;
	int16_t indication;
	uint16_t duration;
} __attribute__((__packed__)) SetIndicatorLED;

typedef struct {
	TFPMessageHeader header;
	uint8_t status;
} __attribute__((__packed__)) SetIndicatorLED_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetButtonState;

typedef struct {
	TFPMessageHeader header;
	uint32_t button_press_time;
	uint32_t button_release_time;
	bool button_pressed;
} __attribute__((__packed__)) GetButtonState_Response;

typedef struct {
	TFPMessageHeader header;
} __attribute__((__packed__)) GetAllData1;

typedef struct {
	TFPMessageHeader header;
	uint8_t iec61851_state;
	uint8_t charger_state;
	uint8_t contactor_state;
	uint8_t contactor_error;
	uint16_t allowed_charging_current;
	uint8_t error_state;
	uint8_t lock_state;
	uint8_t jumper_configuration;
	bool has_lock_switch;
	uint8_t evse_version;
	uint8_t led_state;
	uint16_t cp_pwm_duty_cycle;
	uint16_t adc_values[2];
	int16_t voltages[3];
	uint32_t resistances[2];
	uint8_t gpio[1];
	uint32_t charging_time;
	uint32_t time_since_state_change;
	uint32_t uptime;
	int16_t indication;
	uint16_t duration;
	uint32_t button_press_time;
	uint32_t button_release_time;
	bool button_pressed;
} __attribute__((__packed__)) GetAllData1_Response;


// Function prototypes
BootloaderHandleMessageResponse get_state(const GetState *data, GetState_Response *response);
BootloaderHandleMessageResponse get_hardware_configuration(const GetHardwareConfiguration *data, GetHardwareConfiguration_Response *response);
BootloaderHandleMessageResponse get_low_level_state(const GetLowLevelState *data, GetLowLevelState_Response *response);
BootloaderHandleMessageResponse set_charging_slot(const SetChargingSlot *data);
BootloaderHandleMessageResponse set_charging_slot_max_current(const SetChargingSlotMaxCurrent *data);
BootloaderHandleMessageResponse set_charging_slot_active(const SetChargingSlotActive *data);
BootloaderHandleMessageResponse set_charging_slot_clear_on_disconnect(const SetChargingSlotClearOnDisconnect *data);
BootloaderHandleMessageResponse get_charging_slot(const GetChargingSlot *data, GetChargingSlot_Response *response);
BootloaderHandleMessageResponse get_all_charging_slots(const GetAllChargingSlots *data, GetAllChargingSlots_Response *response);
BootloaderHandleMessageResponse set_charging_slot_default(const SetChargingSlotDefault *data);
BootloaderHandleMessageResponse get_charging_slot_default(const GetChargingSlotDefault *data, GetChargingSlotDefault_Response *response);
BootloaderHandleMessageResponse calibrate(const Calibrate *data, Calibrate_Response *response);
BootloaderHandleMessageResponse get_user_calibration(const GetUserCalibration *data, GetUserCalibration_Response *response);
BootloaderHandleMessageResponse set_user_calibration(const SetUserCalibration *data);
BootloaderHandleMessageResponse get_data_storage(const GetDataStorage *data, GetDataStorage_Response *response);
BootloaderHandleMessageResponse set_data_storage(const SetDataStorage *data);
BootloaderHandleMessageResponse get_indicator_led(const GetIndicatorLED *data, GetIndicatorLED_Response *response);
BootloaderHandleMessageResponse set_indicator_led(const SetIndicatorLED *data, SetIndicatorLED_Response *response);
BootloaderHandleMessageResponse get_button_state(const GetButtonState *data, GetButtonState_Response *response);
BootloaderHandleMessageResponse get_all_data_1(const GetAllData1 *data, GetAllData1_Response *response);

// Callbacks


#define COMMUNICATION_CALLBACK_TICK_WAIT_MS 1
#define COMMUNICATION_CALLBACK_HANDLER_NUM 0
#define COMMUNICATION_CALLBACK_LIST_INIT \


#endif
