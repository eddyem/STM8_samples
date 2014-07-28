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
EELAYER 27 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 3 3
Title ""
Date "28 jul 2014"
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Comp
L SW_PUSH SW?
U 1 1 53D66E36
P 1700 2050
F 0 "SW?" H 1850 2160 50  0000 C CNN
F 1 "SW_PUSH" H 1700 1970 50  0000 C CNN
F 2 "" H 1700 2050 60  0000 C CNN
F 3 "" H 1700 2050 60  0000 C CNN
	1    1700 2050
	1    0    0    -1  
$EndComp
$Comp
L BATTERY BT?
U 1 1 53D66E57
P 1450 4300
F 0 "BT?" H 1450 4500 50  0000 C CNN
F 1 "BATTERY" H 1450 4110 50  0000 C CNN
F 2 "" H 1450 4300 60  0000 C CNN
F 3 "" H 1450 4300 60  0000 C CNN
	1    1450 4300
	0    1    1    0   
$EndComp
Wire Wire Line
	1400 2050 1200 2050
Wire Wire Line
	2000 2050 2000 1850
Wire Wire Line
	2000 1850 1200 1850
$Comp
L SW_PUSH SW?
U 1 1 53D66E75
P 1700 2450
F 0 "SW?" H 1850 2560 50  0000 C CNN
F 1 "SW_PUSH" H 1700 2370 50  0000 C CNN
F 2 "" H 1700 2450 60  0000 C CNN
F 3 "" H 1700 2450 60  0000 C CNN
	1    1700 2450
	1    0    0    -1  
$EndComp
Wire Wire Line
	1400 2450 1200 2450
Wire Wire Line
	2000 2450 2000 2250
Wire Wire Line
	2000 2250 1200 2250
$Comp
L SW_PUSH SW?
U 1 1 53D66E80
P 1700 2850
F 0 "SW?" H 1850 2960 50  0000 C CNN
F 1 "SW_PUSH" H 1700 2770 50  0000 C CNN
F 2 "" H 1700 2850 60  0000 C CNN
F 3 "" H 1700 2850 60  0000 C CNN
	1    1700 2850
	1    0    0    -1  
$EndComp
Wire Wire Line
	1400 2850 1200 2850
Wire Wire Line
	2000 2850 2000 2650
Wire Wire Line
	2000 2650 1200 2650
$Comp
L SW_PUSH SW?
U 1 1 53D66E8B
P 1700 3250
F 0 "SW?" H 1850 3360 50  0000 C CNN
F 1 "SW_PUSH" H 1700 3170 50  0000 C CNN
F 2 "" H 1700 3250 60  0000 C CNN
F 3 "" H 1700 3250 60  0000 C CNN
	1    1700 3250
	1    0    0    -1  
$EndComp
Wire Wire Line
	1400 3250 1200 3250
Wire Wire Line
	2000 3250 2000 3050
Wire Wire Line
	2000 3050 1200 3050
$Comp
L SW_PUSH SW?
U 1 1 53D66E9C
P 1700 3650
F 0 "SW?" H 1850 3760 50  0000 C CNN
F 1 "SW_PUSH" H 1700 3570 50  0000 C CNN
F 2 "" H 1700 3650 60  0000 C CNN
F 3 "" H 1700 3650 60  0000 C CNN
	1    1700 3650
	1    0    0    -1  
$EndComp
Wire Wire Line
	1400 3650 1200 3650
Wire Wire Line
	2000 3650 2000 3450
Wire Wire Line
	2000 3450 1200 3450
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
Text HLabel 1200 4600 0    60   Input ~ 0
BAT-
Wire Wire Line
	1200 4600 1450 4600
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
Connection ~ 1450 4600
$Comp
L PWR_FLAG #FLG?
U 1 1 53D67208
P 1300 4900
F 0 "#FLG?" H 1300 4995 30  0001 C CNN
F 1 "PWR_FLAG" H 1300 5080 30  0000 C CNN
F 2 "" H 1300 4900 60  0000 C CNN
F 3 "" H 1300 4900 60  0000 C CNN
	1    1300 4900
	1    0    0    -1  
$EndComp
$Comp
L PWR_FLAG #FLG?
U 1 1 53D67215
P 1600 5100
F 0 "#FLG?" H 1600 5195 30  0001 C CNN
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
$EndSCHEMATC
