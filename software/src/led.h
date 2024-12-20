/* evse-bricklet
 * Copyright (C) 2020 Olaf Lüke <olaf@tinkerforge.com>
 *
 * led.h: EVSE LED driver
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

#ifndef LED_H
#define LED_H

#include <stdint.h>
#include <stdbool.h>

#define LED_FLICKER_DURATION    50

#define LED_BLINK_DURATION_ON   250
#define LED_BLINK_DURATION_OFF  250
#define LED_BLINK_DURATION_WAIT 2000

#define LED_STANDBY_TIME (1000*60*15) // Standby after 15 minutes

typedef enum {
	LED_STATE_OFF,
	LED_STATE_ON,
	LED_STATE_BLINKING,
	LED_STATE_FLICKER,
	LED_STATE_BREATHING,
	LED_STATE_API
} LEDState;

typedef struct {
	LEDState state;

	uint32_t on_time;

	uint32_t blink_num;
	uint32_t blink_count;
	bool blink_on;
	uint32_t blink_last_time;
	int16_t blink_external;

	bool flicker_on;
	uint32_t flicker_last_time;

	uint32_t breathing_time;
	int16_t breathing_index;
	bool breathing_up;

	int16_t api_indication;
	uint16_t api_duration;
	uint32_t api_start;

	uint8_t api_ack_counter;
	uint8_t api_ack_index;
	uint32_t api_ack_time;

	uint8_t api_nack_counter;
	uint8_t api_nack_index;
	uint32_t api_nack_time;

	uint8_t api_nag_counter;
	uint8_t api_nag_index;
	uint32_t api_nag_time;
	bool currently_in_wait_state;
} LED;

extern LED led;

void led_set_on(const bool force);
void led_set_off(void);
void led_set_breathing(void);
void led_set_blinking(const uint8_t num);
void led_init();
void led_tick();

#endif