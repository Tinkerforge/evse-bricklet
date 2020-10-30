#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time

from evse_tester import EVSETester

if __name__ == "__main__":
    evse_tester = EVSETester()

    # Initial config
    evse_tester.set_contactor(True, False)
    evse_tester.set_diode(True)
    evse_tester.set_cp_pe_resistor(False, False, False)
    evse_tester.set_pp_pe_resistor(False, False, True, False)
    evse_tester.evse.reset()
    time.sleep(10)

    evse_tester.set_cp_pe_resistor(True, False, False)
    time.sleep(5)
    evse_tester.set_cp_pe_resistor(True, True, False)

    evse_tester.wait_for_contactor_gpio(True)
    evse_tester.set_contactor(True, True)
