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

    while True:
        status = evse.get_low_level_status()
        print(status)
        time.sleep(1)

    ipcon.disconnect()
