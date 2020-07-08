#!/usr/bin/env python3
# -*- coding: utf-8 -*-

HOST = "wallbox1"
PORT = 4223
UID = "wb1"

from tinkerforge.ip_connection import IPConnection
from tinkerforge.bricklet_evse import BrickletEVSE
import time
import csv
import traceback

LOW_LEVEL_PASSWORD = 0x4223B00B

if __name__ == "__main__":
    ipcon = IPConnection() # Create IP connection
    evse = BrickletEVSE(UID, ipcon) # Create device object

    ipcon.connect(HOST, PORT) # Connect to brickd
    # Don't use device before ipcon is connected

    f = open('log.csv', 'w')
    writer = csv.writer(f)
    writer.writerow(["Time", "IEC61851 State", "LED State", "Resistance CP/PE", "Resistance PP/PE", "CP PWM Duty Cycle", "Contactor State", "Contactor Check Error", "GPIO Enable", "GPIO LED", "GPIO Motor Switch", "Lock State", "Jumper Configuration", "Lock Switch", "Uptime"])
    while True:
        time.sleep(0.2)
        try:
            state = evse.get_state()
            line = [str(int(time.time()*10)), state.iec61851_state, state.led_state, state.resistance[0], state.resistance[1], state.cp_pwm_duty_cycle, state.contactor_state, state.contactor_error, state.gpio[0], state.gpio[1], state.gpio[2], state.gpio[3], state.lock_state, state.jumper_configuration, state.uptime]
            writer.writerow(line)
        except:
            print("Time: {0}".format(time.time()))
            traceback.print_exc()

    ipcon.disconnect()
