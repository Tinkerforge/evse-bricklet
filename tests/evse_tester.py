#!/usr/bin/env python3
# -*- coding: utf-8 -*-

HOST     = "localhost"
PORT     = 4223

UID_EVSE = None
UID_IDAI = "Mj8"
UID_IDO4 = "HjH"
UID_IDI4 = "QQ3"
UID_IQR1 = "FYF"
UID_IQR2 = "NWq"
UID_ICOU = "Gh4"

from tinkerforge.ip_connection import IPConnection
from tinkerforge.bricklet_evse import BrickletEVSE

from tinkerforge.bricklet_industrial_dual_analog_in_v2 import BrickletIndustrialDualAnalogInV2
from tinkerforge.bricklet_industrial_digital_out_4_v2  import BrickletIndustrialDigitalOut4V2
from tinkerforge.bricklet_industrial_digital_in_4_v2   import BrickletIndustrialDigitalIn4V2
from tinkerforge.bricklet_industrial_quad_relay_v2     import BrickletIndustrialQuadRelayV2
from tinkerforge.bricklet_industrial_counter           import BrickletIndustrialCounter

import time

class EVSETester:
    def __init__(self):
        self.ipcon = IPConnection()
        self.ipcon.connect(HOST, PORT)
        self.ipcon.register_callback(IPConnection.CALLBACK_ENUMERATE, self.cb_enumerate)
        self.ipcon.enumerate()

        print("Trying to find EVSE Bricklet...")
        while UID_EVSE == None:
            time.sleep(0.1)
        print("Found EVSE Bricklet: {0}".format(UID_EVSE))

        self.evse = BrickletEVSE(UID_EVSE, self.ipcon)
    
        self.idai = BrickletIndustrialDualAnalogInV2(UID_IDAI, self.ipcon)
        self.ido4 = BrickletIndustrialDigitalOut4V2(UID_IDO4,  self.ipcon) 
        self.idi4 = BrickletIndustrialDigitalIn4V2(UID_IDI4,   self.ipcon)
        self.iqr1 = BrickletIndustrialQuadRelayV2(UID_IQR1,    self.ipcon)
        self.iqr2 = BrickletIndustrialQuadRelayV2(UID_IQR2,    self.ipcon)
        self.icou = BrickletIndustrialCounter(UID_ICOU,        self.ipcon)

    def cb_enumerate(self, uid, connected_uid, position, hardware_version, firmware_version, device_identifier, enumeration_type):
        global UID_EVSE
        if device_identifier == BrickletEVSE.DEVICE_IDENTIFIER:
            UID_EVSE = uid
    
    # Live = True
    def set_contactor(self, contactor_input, contactor_output):
        if contactor_input:
            self.ido4.set_pwm_configuration(0, 500, 5000)
            print('AC0 live')
        else:
            self.ido4.set_pwm_configuration(0, 500, 0)
            print('AC0 off')

        if contactor_output:
            self.ido4.set_pwm_configuration(1, 500, 5000)
            print('AC1 live')
        else:
            self.ido4.set_pwm_configuration(1, 500, 0)
            print('AC1 off')

    def set_diode(self, enable):
        value = list(self.iqr1.get_value())
        value[0] = enable
        self.iqr1.set_value(value)
        if enable:
            print("Enable lock switch configuration diode")
        else:
            print("Disable lock switch configuration diode")
    
    def set_cp_pe_resistor(self, r2700, r880, r240):
        value = list(self.iqr1.get_value())
        value[1] = r2700
        value[2] = r880
        value[3] = r240
        self.iqr1.set_value(value)

        l = []
        if r2700: l.append("2700 Ohm")
        if r880:  l.append("880 Ohm")
        if r240:  l.append("240 Ohm")
    
        print("Set CP/PE resistor: " + ', '.join(l))
      
    def set_pp_pe_resistor(self, r1500, r680, r220, r100):
        value = [r1500, r680, r220, r100]
        self.iqr2.set_value(value)

        l = []
        if r1500: l.append("1500 Ohm")
        if r680:  l.append("680 Ohm")
        if r220:  l.append("220 Ohm")
        if r100:  l.append("110 Ohm")
    
        print("Set PP/PE resistor: " + ', '.join(l)) 

    def wait_for_contactor_gpio(self, active):
        if active:
            print("Waiting for contactor GPIO to become active...")
        else:
            print("Waiting for contactor GPIO to become inactive...")

        while True:
            state = self.evse.get_low_level_state()
            if state.gpio[3] == active:
                break
            time.sleep(0.01)

        print("Done")

if __name__ == "__main__":
    evse_tester = EVSETester()

    # Initial config
    evse_tester.set_contactor(True, False)
    evse_tester.set_diode(True)
    evse_tester.set_cp_pe_resistor(False, False, False)
    evse_tester.set_pp_pe_resistor(False, False, True, False)
    evse_tester.evse.reset()
    time.sleep(5)

    while True:
        time.sleep(5)
        evse_tester.set_cp_pe_resistor(True, False, False)
        time.sleep(5)
        evse_tester.set_cp_pe_resistor(True, True, False)

        evse_tester.wait_for_contactor_gpio(True)
        evse_tester.set_contactor(True, True)

        time.sleep(10)
        evse_tester.set_cp_pe_resistor(True, False, False)
        evse_tester.wait_for_contactor_gpio(False)
        evse_tester.set_contactor(True, False)

        time.sleep(5)
        evse_tester.set_cp_pe_resistor(False, False, False)
        time.sleep(5)
