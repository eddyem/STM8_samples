EESchema Schematic File Version 2
LIBS:power
LIBS:device
LIBS:transistors
LIBS:conn
LIBS:linear
LIBS:regul
LIBS:74xx
LIBS:cmos4000
LIBS:adc-dac
LIBS:memory
LIBS:xilinx
LIBS:special
LIBS:microcontrollers
LIBS:dsp
LIBS:microchip
LIBS:analog_switches
LIBS:motorola
LIBS:texas
LIBS:intel
LIBS:audio
LIBS:interface
LIBS:digital-audio
LIBS:philips
LIBS:display
LIBS:cypress
LIBS:siliconi
LIBS:opto
LIBS:atmel
LIBS:contrib
LIBS:valves
LIBS:stm8s105k4t6c
LIBS:tda2822
LIBS:CD74HC154
LIBS:drum-cache
EELAYER 24 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 3 3
Title ""
Date "13 oct 2014"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L BATTERY BT2
U 1 1 53D66E57
P 1450 4300
F 0 "BT2" H 1450 4500 50  0000 C CNN
F 1 "BATTERY" H 1450 4110 50  0000 C CNN
F 2 "" H 1450 4300 60  0000 C CNN
F 3 "" H 1450 4300 60  0000 C CNN
	1    1450 4300
	0    1    1    0   
$EndComp
Wire Wire Line
	1400 2050 1200 2050
Text HLabel 1200 1850 0    60   Input ~ 0
BTN0a
Text HLabel 1200 2050 0    60   Input ~ 0
BTN0b
Text HLabel 1200 2250 0    60   Input ~ 0
BTN1a
Text HLabel 1200 2450 0    60   Input ~ 0
BTN1b
Text HLabel 1200 2650 0    60   Input ~ 0
BTN2a
Text HLabel 1200 2850 0    60   Input ~ 0
BTN2b
Text HLabel 1200 3050 0    60   Input ~ 0
BTN3a
Text HLabel 1200 3250 0    60   Input ~ 0
BTN3b
Text HLabel 1200 3450 0    60   Input ~ 0
BTN4a
Text HLabel 1200 3650 0    60   Input ~ 0
BTN4b
Text HLabel 1200 4000 0    60   Input ~ 0
BAT+
Wire Wire Line
	1200 4000 1450 4000
Text HLabel 1200 4900 0    60   Input ~ 0
SNDa
Text HLabel 1200 5100 0    60   Input ~ 0
SNDb
Text HLabel 1200 5300 0    60   Input ~ 0
SND_gnd
Wire Wire Line
	1200 5300 1450 5300
Wire Wire Line
	1450 5300 1450 4600
$Comp
L PWR_FLAG #FLG029
U 1 1 53D67208
P 1300 4900
F 0 "#FLG029" H 1300 4995 30  0001 C CNN
F 1 "PWR_FLAG" H 1300 5080 30  0000 C CNN
F 2 "" H 1300 4900 60  0000 C CNN
F 3 "" H 1300 4900 60  0000 C CNN
	1    1300 4900
	1    0    0    -1  
$EndComp
$Comp
L PWR_FLAG #FLG030
U 1 1 53D67215
P 1600 5100
F 0 "#FLG030" H 1600 5195 30  0001 C CNN
F 1 "PWR_FLAG" H 1600 5280 30  0000 C CNN
F 2 "" H 1600 5100 60  0000 C CNN
F 3 "" H 1600 5100 60  0000 C CNN
	1    1600 5100
	1    0    0    -1  
$EndComp
Wire Wire Line
	1600 5100 1200 5100
Wire Wire Line
	1200 4900 1300 4900
$Comp
L CONN_2 P2
U 1 1 541D2C34
P 1750 1950
F 0 "P2" V 1700 1950 40  0000 C CNN
F 1 "CONN_2" V 1800 1950 40  0000 C CNN
F 2 "" H 1750 1950 60  0000 C CNN
F 3 "" H 1750 1950 60  0000 C CNN
	1    1750 1950
	1    0    0    -1  
$EndComp
Wire Wire Line
	1200 1850 1400 1850
Wire Wire Line
	1400 2450 1200 2450
$Comp
L CONN_2 P3
U 1 1 541D2C57
P 1750 2350
F 0 "P3" V 1700 2350 40  0000 C CNN
F 1 "CONN_2" V 1800 2350 40  0000 C CNN
F 2 "" H 1750 2350 60  0000 C CNN
F 3 "" H 1750 2350 60  0000 C CNN
	1    1750 2350
	1    0    0    -1  
$EndComp
Wire Wire Line
	1200 2250 1400 2250
Wire Wire Line
	1400 2850 1200 2850
$Comp
L CONN_2 P4
U 1 1 541D2C5F
P 1750 2750
F 0 "P4" V 1700 2750 40  0000 C CNN
F 1 "CONN_2" V 1800 2750 40  0000 C CNN
F 2 "" H 1750 2750 60  0000 C CNN
F 3 "" H 1750 2750 60  0000 C CNN
	1    1750 2750
	1    0    0    -1  
$EndComp
Wire Wire Line
	1200 2650 1400 2650
Wire Wire Line
	1400 3250 1200 3250
$Comp
L CONN_2 P5
U 1 1 541D2C67
P 1750 3150
F 0 "P5" V 1700 3150 40  0000 C CNN
F 1 "CONN_2" V 1800 3150 40  0000 C CNN
F 2 "" H 1750 3150 60  0000 C CNN
F 3 "" H 1750 3150 60  0000 C CNN
	1    1750 3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	1200 3050 1400 3050
Wire Wire Line
	1400 3650 1200 3650
$Comp
L CONN_2 P6
U 1 1 541D2C6F
P 1750 3550
F 0 "P6" V 1700 3550 40  0000 C CNN
F 1 "CONN_2" V 1800 3550 40  0000 C CNN
F 2 "" H 1750 3550 60  0000 C CNN
F 3 "" H 1750 3550 60  0000 C CNN
	1    1750 3550
	1    0    0    -1  
$EndComp
Wire Wire Line
	1200 3450 1400 3450
Text Notes 900  1500 0    60   ~ 0
Buttons' contacts mark:\n"a" - inner circle;\n"b" - outern ring
$EndSCHEMATC
