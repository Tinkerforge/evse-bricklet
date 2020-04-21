#!/usr/bin/env python3
# -*- coding: utf-8 -*-

HOST = "localhost"
PORT = 4223
UID = "XYZ"

from tinkerforge.ip_connection import IPConnection
from tinkerforge.bricklet_evse import BrickletEVSE
import time


if __name__ == "__main__":
    ipcon = IPConnection() # Create IP connection
    evse = BrickletEVSE(UID, ipcon) # Create device object

    ipcon.connect(HOST, PORT) # Connect to brickd
    # Don't use device before ipcon is connected

    # IN
    # 0: EVSE_RELAY_PIN
    # 1: EVSE_MOTOR_PHASE_PIN
    # 2: EVSE_MOTOR_ENABLE_PIN
    # 3: EVSE_CHARGE_LED_PIN
    # 4: EVSE_ERROR_LED_PIN
    # 5: EVSE_CP_PWM_PIN

    # OUT
    # 0: EVSE_INPUT_GP_PIN
    # 1: EVSE_AC1_PIN
    # 2: EVSE_AC2_PIN
    # 3: EVSE_INPUT_MOTOR_SWITCH_PIN
    # 4: EVSE_MOTOR_FAULT_PIN
    # 5: EVSE_CP_PWM_PIN

    while True:
        test_in = [0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0]
        test_out = evse.test(test_in)
        print('in', test_in)
        print('out', test_out)
        time.sleep(5)

        test_in = [0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0]
        test_out = evse.test(test_in)
        print('in', test_in)
        print('out', test_out)
        time.sleep(5)


    ipcon.disconnect()
