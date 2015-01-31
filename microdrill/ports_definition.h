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

#include "stm8l.h"

/*
 * Stepper Motor type:
 *   MOTOR_TYPE_UNIPOLAR for 5-wires unipolar motors with darlington array as driver
 *   MOTOR_TYPE_BIPOLAR for 4-wires bipolar motor with L9110-like H-bridges
 */
#define MOTOR_TYPE_BIPOLAR

// macro for using in port constructions like PORT(LED_PORT, ODR) = xx
#define CONCAT(a, b)	a ## _ ## b
#define PORT(a, b)		CONCAT(a , b)

// Potentiometer threshold (in ADU) - 0.5% error
#define POTENT_TRESHOLD     (5)

// on-board LED
#define LED_PORT		PC
#define LED_PIN			GPIO_PIN2

// UART2_TX
#define UART_PORT		PD
#define UART_TX_PIN		GPIO_PIN5

/***** Stepper motor *****/
// Clocking
#define STP_PORT        PB // PB0..3 -- pins A..D of stepper
// amount of steps on all trace
#define FULL_SCALE_STEPS   (3000)
#define MAX_STEPPER_SPEED  (500)
#define MIN_STEPPER_SPEED  (20)

/* drill motor PC1 - timer 1 PWM output 1; PC5 - footswitch */
// speed (in ADU values of voltage on schunt)
#define MAX_DRILL_SPEED (50)
#define NORMAL_DRILL_SPEED (30)
extern U8 Upper_TIM1_CCR1L; // max speed set by user
#define DRILL_ON()      do{TIM1_BKR |= 0x80; drill_works = 1;}while(0) // turn on drill motor
#define DRILL_OFF()     do{TIM1_BKR &= ~0x80; PC_ODR &= ~GPIO_PIN1; drill_works = 0;}while(0) // turn it off
#define DRILL_SETMAX(X) do{Upper_TIM1_CCR1L = X; TIM1_CCR1L = X;}while(0)
#define DRILL_FASTER()  do{U8 r = TIM1_CCR1L; if(r < Upper_TIM1_CCR1L) TIM1_CCR1L = r+1;}while(0)// increase current
#define DRILL_SLOWER()  do{U8 r = TIM1_CCR1L; if(r > 0)   TIM1_CCR1L = r-1;}while(0)  // decrease it
#define FOOTSWITCH      ((PC_IDR & GPIO_PIN5))
#define FOOTSW_TEST(x)  ((x & GPIO_PIN5))

/* tray motor: PD2, PD3 - rotation direction; PC3, PC4 - end-switches (bottom/top) */
#define TRAY_TOP_SW     ((PC_IDR & GPIO_PIN4))
#define TRAY_BTM_SW     ((PC_IDR & GPIO_PIN3))
#define TRAYSW_TEST(x)  ((x & (GPIO_PIN3 | GPIO_PIN4)))
#define TRAYSW_PRSD(x)  (((x & (GPIO_PIN3 | GPIO_PIN4)) != (GPIO_PIN3 | GPIO_PIN4)))
#define TRAY_STOP()     do{PD_ODR &= ~0x0C;}while(0)
#define TRAY_UP()       do{if(!TRAY_TOP_SW){PD_ODR &= ~0x0C; PC_ODR |= 0x04;}}while(0)
#define TRAY_DOWN()     do{if(!(TRAY_BTM_SW)){PD_ODR &= ~0x0C; PC_ODR |= 0x08;}}while(0)

/* Buttons: PC6 - BTN1 & PC7 - BTN2 */
#define BTN1          ((PC_IDR & GPIO_PIN6))
#define BTN2          ((PC_IDR & GPIO_PIN7))
#define BTN1_TEST(x)  ((x & GPIO_PIN6))
#define BTN2_TEST(x)  ((x & GPIO_PIN7))
#define BTN12_TEST(x) (((x & (GPIO_PIN7 | GPIO_PIN6)) == (GPIO_PIN7 | GPIO_PIN6)))

// EXTI for all buttons: PC3..7
#define BTNS_IDR             PC_IDR
#define BTNS_EXTI_MASK       (0xf8)
#define BTNS_EXTI_DISABLE()  do{PC_CR2 = 0;}while(0)
#define BTNS_EXTI_ENABLE()   do{PC_CR2 = BTNS_EXTI_MASK;}while(0)
#define BTNS_SETUP()         do{EXTI_CR1 = 0x30; PC_CR1 |= BTNS_EXTI_MASK;}while(0)

#endif // __PORTS_DEFINITION_H__

/*
 * PORTS:
 * 	DRILL
 * 		PC1 - PWM (TIM1_CH1)
 * 		PF4 - Sence (AIN12)
 * 		PC5 - Pedal switch
 * 	Stepper motor
 * 		PB0, PB1, PB2, PB3 - phases of motor
 * 	Slider (tray) motor
 * 		PD2, PD3 - rotation direction
 * 		PC3 - down end-switch
 * 		PC4 - up end-switch
 * 	On-tray buttons & resistor
 * 		PB4 - variable resistor (AIN4)
 * 		PC6 - BTN1
 * 		PC7 - BTN2
 * 	UART
 * 		PD5 - TX
 * 		PD6 - RX
 */
