/*
 * ports_definition.h - definition of ports pins & so on
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 */

#pragma once
#ifndef __PORTS_DEFINITION_H__
#define __PORTS_DEFINITION_H__

#include "stm8s.h"
#include "main.h"
#include "statemachine.h"

/*
 * Stepper Motor type:
 *   MOTOR_TYPE_UNIPOLAR for 5-wires unipolar motors with darlington array as driver
 *   MOTOR_TYPE_BIPOLAR for 4-wires bipolar motor with L9110-like H-bridges
 */
#define MOTOR_TYPE_BIPOLAR
// anti-clash pause (30ms)
#define ANTICLASH_PAUSE     (30)

// macro for using in port constructions like PORT(LED_PORT, ODR) = xx
#define CONCAT(a, b)	a ## _ ## b
#define PORT(a, b)		CONCAT(a , b)

// ADC threshold (in ADU) - 1% error
#define ADC_THRESHOLD     (10)

// signal LED (LED2) - PE5
#define LED_PORT		PE
#define LED_PIN			GPIO_PIN5
// LED0/LED1 (light & so on): PA1/PA2
#define LED01_PORT      PA
#define LED0_PIN        GPIO_PIN1
#define LED1_PIN        GPIO_PIN2
// Tray pins: PD2/PD3
#define TRAY_PORT       PD
#define TRAY_PINS       (GPIO_PIN2|GPIO_PIN3)

// UART2_TX
#define UART_PORT		PD
#define UART_TX_PIN		GPIO_PIN5
#define newline() do{while(!(UART2_SR & UART_SR_TXE)); UART2_DR = '\n';}while(0)

/***** Stepper motor *****/
// Clocking
#define STP_PORT        PB // PB0..3 -- pins A..D of stepper
// amount of steps on all trace
#define FULL_SCALE_STEPS    (180)
// amount of steps to move up (for PCB moving)
#define MOVEUP_STEPS        (30)
// min/max periods in ticks @ 1kHz (1Hz & 250Hz)
#define MAX_STEPPER_PERIOD  (500)
#define MIN_STEPPER_PERIOD  (2)

/* drill motor PC1 - timer 1 PWM output 1; PC5 - footswitch */
// speed (in ADU values of voltage on schunt)
#define MAX_DRILL_CURRENT (50)
#define NORMAL_DRILL_CURRENT (30)
// lowest (starting) speed in percents
#define DRILL_LOWSPEED  (10)
#define DRILL_ON()      do{TIM1_CCR1L = DRILL_LOWSPEED; TIM1_CR1 |= TIM_CR1_CEN; TIM1_BKR |= 0x80; curstate = DRL_ACCEL;}while(0) // turn on drill motor
#define DRILL_OFF()     do{TIM1_BKR &= ~0x80; TIM1_CR1 &= ~TIM_CR1_CEN; curstate = DRL_RELAX;}while(0) // turn it off
#define DRILL_FASTER()  do{if(drill_maxspeed < 100) ++drill_maxspeed;}while(0)
#define DRILL_SLOWER()  do{if(drill_maxspeed > DRILL_LOWSPEED) --drill_maxspeed;}while(0)
// external buttons & switches: PEDAL (PD0), IN0/IN1 (PD4, PD7)
#define FOOTSWITCH      ((PD_IDR & GPIO_PIN0))
#define INPUT0          ((PD_IDR & GPIO_PIN4))
#define INPUT1          ((PD_IDR & GPIO_PIN7))

/* tray motor: PD2, PD3 - rotation direction; PC2/3 - end-switches (bottom/top) */
#define TRAY_TOP_SW     ((PC_IDR & GPIO_PIN3))
#define TRAY_BTM_SW     ((PC_IDR & GPIO_PIN2))
#define TRAYSW_TEST(x)  ((x & (GPIO_PIN2 | GPIO_PIN3)))
#define TRAYSW_PRSD(x)  (((x & (GPIO_PIN2 | GPIO_PIN3)) != (GPIO_PIN2 | GPIO_PIN3)))
#define TRAY_STOP()     do{PD_ODR &= ~0x0C;}while(0)
#define TRAY_UP()       do{if(TRAY_TOP_SW){PD_ODR &= ~0x0C; PD_ODR |= 0x08;}}while(0)
#define TRAY_DOWN()     do{if(TRAY_BTM_SW){PD_ODR &= ~0x0C; PD_ODR |= 0x04;}}while(0)

/* Buttons: PC4 - BTN1 & PC5 - BTN2 */
#define TRAY_BTN1           ((PC_IDR & GPIO_PIN4))
#define TRAY_BTN2           ((PC_IDR & GPIO_PIN5))
#define TRAY_BTN12_TEST(x)  (((x & (GPIO_PIN4 | GPIO_PIN5)) == (GPIO_PIN4 | GPIO_PIN5)))

// EXTI for all tray buttons: PC2..5
#define BTNS0_IDR               PC_IDR
#define BTNS1_IDR               PD_IDR
// PC2-PC5
#define PC_EXTI_MASK            (0x3c)
// PD0,PD4,PD7
#define PD_EXTI_MASK            (0x91)
#define BTNS_EXTI_DISABLE()  do{PC_CR2 = 0; PD_CR2 = 0;}while(0)
#define BTNS_EXTI_ENABLE()   do{PC_CR2 = PC_EXTI_MASK; PD_CR2 = PD_EXTI_MASK;}while(0)
// setup PC/PD EXTI mask & pullup resistors for PC2..5 & PD0,PD4,PD7
#define BTNS_SETUP()         do{EXTI_CR1 = 0xf0; PC_CR1 |= PC_EXTI_MASK; PD_CR1 |= PD_EXTI_MASK;}while(0)

#endif // __PORTS_DEFINITION_H__

/*
 * PORTS:
 * 	DRILL
 * 		PC1 - PWM (TIM1_CH1)
 * 		PF4 - Sence (AIN12)
 * 		PD0 - Pedal switch
 * 	Stepper motor
 * 		PB0, PB1, PB2, PB3 - phases of motor
 * 	Slider (tray) motor
 * 		PD2, PD3 - rotation direction
 * 		PC2 - down end-switch
 * 		PC3 - up end-switch
 * 	On-tray buttons & resistor
 * 		PB4 - variable resistor (AIN4)
 * 		PC4 - BTN1
 * 		PC5 - BTN2
 * 	UART
 * 		PD5 - TX
 * 		PD6 - RX
 * LEDS
 *      PE5 - LED2 (system blink)
 *      PA1 - LED0  \ external 3..9v LEDs through MOSFET
 *      PA2 - LED1  /
 * Extra inputs
 *      PD4 - IN0
 *      PD7 - IN1
 */
