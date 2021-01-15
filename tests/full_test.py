#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import sys

from evse_tester import EVSETester

def no_log(s):
    pass

if __name__ == "__main__":
    print('Schaltereinstellung auf 32A stellen')
    input("Enter drücken...")
    print('')

    print('Suche EVSE Bricklet und Tester')
    evse_tester = EVSETester(log_func = no_log)
    print('... OK')

    data = []

    ident = evse_tester.evse.get_identity()
    data.append(ident.uid)

    # Initial config
    evse_tester.set_contactor(True, False)
    evse_tester.set_diode(True)
    evse_tester.set_cp_pe_resistor(False, False, False)
    evse_tester.set_pp_pe_resistor(False, False, True, False)
    evse_tester.evse.reset()

    print('Warte auf DC-Schutz Kalibrierung (15 Sekunden)')
    print('--> Flackert LED? Wenn nicht kaputt! <--')
    time.sleep(15)
    print('... OK')


    hw_conf = evse_tester.evse.get_hardware_configuration()
    print('Teste Jumper-Einstellung')
    if hw_conf.jumper_configuration != 6:
        print('Falsche Jumper-Einstellung: {0}'.format(hw_conf.jumper_configuration))
        print('-----------------> NICHT OK')
        sys.exit(1)
    else:
        print('... OK')

    print('Teste Lock-Switch-Einstellung')
    if hw_conf.has_lock_switch:
        print('Falsche Lock-Switch-Einstellung: {0}'.format(hw_conf.has_lock_switch))
        print('-----------------> NICHT OK')
        sys.exit(1)
    else:
        print('... OK')

    print('Starte CP/PE Kalibrierung (2,5 Sekunden)')
    voltage1 = int(input("CP/PE Spannung eingeben (in mV): "))
    data.append(str(voltage1))
    if 11500 < voltage1 < 12500:
        print('Kalibriere mit {0}mV'.format(voltage1))
    else:
        print('Spannung nicht erlaubt {0}mV (Erwarte zwischen 11500 und 12500)'.format(voltage1))
        sys.exit(1)

    ret = evse_tester.evse.calibrate(1, 0x0BB03201, voltage1)
    if not ret:
        print('Fehler während EVSE-Kalibrierung: {0}, {1}, {2}'.format(1, 0x0BB03201, voltage1))
        sys.exit(1)

    print('Kalibriere mit 2700 Ohm (3,5 Sekunden)')
    time.sleep(1)
    evse_tester.set_cp_pe_resistor(True, False, False)
    time.sleep(2.5)
    ret = evse_tester.evse.calibrate(2, 0x0BB03202, 0)
    if not ret:
        print('Fehler während EVSE-Kalibrierung: 2700ohm')
        sys.exit(1)

    evse_tester.set_cp_pe_resistor(True, True, False)

    cal_state = 3
    for a in range(6, 33, 2):
        print('Kalibriere mit 880 Ohm bei {0}A (3,5 Sekunden)'.format(a))
        time.sleep(3.5)
        ret = evse_tester.evse.calibrate(cal_state, 0x0BB03200 + cal_state, 0)
        cal_state += 1
        if not ret:
            print('Fehler während EVSE-Kalibrierung: 880ohm {0}A'.format(a))
            sys.exit(1)

    evse_tester.set_cp_pe_resistor(False, False, False)
    time.sleep(1)

    voltage2 = int(input("CP/PE Spannung eingeben (in mV): "))
    data.append(str(voltage2))
    if -12500 < voltage2 < -11500:
        print('Kalibriere mit {0}mV'.format(voltage2))
    else:
        print('Spannung nicht erlaubt {0}mV (Erwarte zwischen -11500 und -12500)'.format(voltage2))
        sys.exit(1)

    offset = voltage1 + voltage2
    if -200 < offset < 200:
        print('Setze Offset {0}mV (v1 {1}mV, v2 {2}mV)'.format(offset, voltage1, voltage2))
    else:
        print('Offset nicht erlaubt {0}mV (Erwarte zwischen 200 und -200)'.format(offset))
        sys.exit(1)

    ret = evse_tester.evse.calibrate(17, 0x0BB03211, offset)
    if not ret:
        print('Fehler während EVSE-Kalibrierung: {0}, {1}, {2}'.format(2, 0x0BB03202, offset))
        sys.exit(1)

    print('... OK')

    print('Warte auf Autokalibrierung')
    time.sleep(15)
    print('... OK')
    print('Setze 2700 Ohm Widerstand (2 Sekunden)')
    evse_tester.set_cp_pe_resistor(True, False, False)
    time.sleep(2)
    print('... OK')

    print('Test PP/CP Widerstand (ohne PWM)')
    ll = evse_tester.evse.get_low_level_state()
    data.append(str(ll.resistances[0]))
    if 880*0.8 < ll.resistances[0] < 2700*1.20:
        print('... OK ({0} Ohm)'.format(ll.resistances[0]))
    else:
        print('-----------------> NICHT OK {0}'.format(ll.resistances[0]))
        sys.exit(1)

    print('Setze 2700 Ohm + 1300 Ohm Widerstand')
    evse_tester.set_cp_pe_resistor(True, True, False)
    print('... OK')

    evse_tester.wait_for_contactor_gpio(True)

    print('Aktiviere Schütz (5 Sekunden)')
    evse_tester.set_contactor(True, True)

    time.sleep(5)
    print('... OK')

    print('Prüfe PP/PE Widerstand')
    ll = evse_tester.evse.get_low_level_state()
    if 200 < ll.resistances[1] < 240:
        data.append(str(ll.resistances[1]))
        print('... OK ({0} Ohm)'.format(ll.resistances[1]))
    else:
        print('-----------------> NICHT OK {0}'.format(ll.resistances[1]))

    for a in range(6, 33, 2):
        print('Test CP/PE {0}A'.format(a))
        evse_tester.evse.set_max_charging_current(a*1000)
        time.sleep(2)
        ll = evse_tester.evse.get_low_level_state()
        if 880*0.7 < ll.resistances[0] < 880*1.30:
            data.append(str(ll.resistances[0]))
            print('... OK ({0} Ohm)'.format(ll.resistances[0]))
        else:
            print('-----------------> NICHT OK {0}'.format(ll.resistances[0]))
            sys.exit(1)
    
    print('Ausschaltzeit messen')
    t1 = time.time()
    evse_tester.set_cp_pe_resistor(True, False, False)
    evse_tester.wait_for_contactor_gpio(False)
    evse_tester.set_contactor(True, False)
    t2 = time.time()

    delay = int((t2-t1)*1000)
    data.append(str(delay))
    print('... OK')

    if delay <= 125: # allow maximum of 25ms over standard
        print('Ausschaltzeit: {0}ms OK'.format(delay))
    else:
        print('Ausschaltzeit: {0}ms'.format(delay))
        print('-----------------> NICHT OK')
        sys.exit(1)

    print('Schaltereinstellung auf "Disabled" stellen und dann Taster drücken')
    evse_tester.wait_for_button_gpio(True) # Button True = Pressed
    print('')

    evse_tester.evse.reset()
    time.sleep(2)
    hw_conf = evse_tester.evse.get_hardware_configuration()
    print('Teste Jumper-Einstellung')
    if hw_conf.jumper_configuration != 8:
        print('Falsche Jumper-Einstellung: {0}'.format(hw_conf.jumper_configuration))
        print('-----------------> NICHT OK')
        sys.exit(1)
    else:
        print('... OK')

    print('')
    print('Fertig. Alles OK')

    with open('full_test_log.csv', 'a+') as f:
        f.write(', '.join(data) + '\n')
