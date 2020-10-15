EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 1
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L tinkerforge:SW_DIP_4 SW101
U 1 1 5F881FF4
P 2400 2550
F 0 "SW101" H 2400 2987 60  0000 C CNN
F 1 "SW_DIP_4" H 2400 2881 60  0000 C CNN
F 2 "kicad-libraries:SW_DIP_4_THT" H 2550 2850 60  0001 C CNN
F 3 "" H 2550 2850 60  0000 C CNN
	1    2400 2550
	1    0    0    -1  
$EndComp
$Comp
L tinkerforge:CONN_02X03 P101
U 1 1 5F882661
P 3650 2500
F 0 "P101" H 3650 2815 50  0000 C CNN
F 1 "CONN_02X03" H 3650 2724 50  0000 C CNN
F 2 "kicad-libraries:Pin_Header_Straight_Female_2x03_127" H 3650 1300 50  0001 C CNN
F 3 "" H 3650 1300 50  0000 C CNN
	1    3650 2500
	1    0    0    -1  
$EndComp
Text Notes 2750 3950 0    50   ~ 0
Maximum Power Setting\nno jumper: invalid\n1<->3: 6A\n3<->5: 10A\n2<->4: 13A\n4<->6: 16A\n1<->3, 2<->4: 20A\n1<->3, 4<->6: 25A\n3<->5, 2<->4: 32A\n3<->5, 4<->6: reserved
Wire Wire Line
	2200 2400 2100 2400
Wire Wire Line
	2100 2400 2100 1950
Wire Wire Line
	3400 2500 2900 2500
Wire Wire Line
	2900 2500 2900 2400
Wire Wire Line
	2900 2400 2650 2400
Wire Wire Line
	2100 1950 3000 1950
Wire Wire Line
	3000 1950 3000 2400
Wire Wire Line
	3000 2400 3400 2400
Wire Wire Line
	2600 2500 2650 2500
Wire Wire Line
	2650 2500 2650 2400
Connection ~ 2650 2400
Wire Wire Line
	2650 2400 2600 2400
Wire Wire Line
	3400 2600 3100 2600
Wire Wire Line
	3100 2600 3100 1850
Wire Wire Line
	3100 1850 2000 1850
Wire Wire Line
	2000 1850 2000 2500
Wire Wire Line
	2000 2500 2200 2500
Wire Wire Line
	3900 2500 4100 2500
Wire Wire Line
	4100 2500 4100 2750
Wire Wire Line
	4100 2750 2750 2750
Wire Wire Line
	2750 2750 2750 2700
Wire Wire Line
	2750 2600 2600 2600
Wire Wire Line
	2600 2700 2750 2700
Connection ~ 2750 2700
Wire Wire Line
	2750 2700 2750 2600
Wire Wire Line
	2200 2600 2000 2600
Wire Wire Line
	2000 2600 2000 3000
Wire Wire Line
	2000 3000 4200 3000
Wire Wire Line
	4200 3000 4200 2400
Wire Wire Line
	4200 2400 3900 2400
Wire Wire Line
	2200 2700 2100 2700
Wire Wire Line
	2100 2700 2100 2900
Wire Wire Line
	2100 2900 4000 2900
Wire Wire Line
	4000 2900 4000 2600
Wire Wire Line
	4000 2600 3900 2600
Text Notes 3750 2100 0    50   ~ 0
20021321-00006C4LF
Text Notes 4400 2900 0    50   ~ 0
Lightpipes \n515-1301-0125F 3.2mm\n515 -13 01- 0 25 0 F 6.35mm
$EndSCHEMATC
