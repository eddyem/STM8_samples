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

// on-board LED
#define LED_PORT		PC
#define LED_PIN			GPIO_PIN2

// UART2_TX
#define UART_PORT		PD
#define UART_TX_PIN		GPIO_PIN5

/***** Stepper motor *****/
// Clocking
#define STP_PORT		PB // PB0..3 -- pins A..D of stepper
#endif // __PORTS_DEFINITION_H__


/*
 * PORTS:
 * 	DRILL
 * 		PC1 - PWM (TIM1_CH1)
 * 		PF4 - Sence (AIN12)
 * 		PC4 - Pedal switch
 * 	Stepper motor
 * 		PB0, PB1, PB2, PB3 - phases of motor
 * 	Slider (tray) motor
 * 		PB4, PB5 - rotation direction
 * 		PC2 - down end-switch
 * 		PC3 - up end-switch
 */
