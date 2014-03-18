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

// macro for using in port constructions like PORT(LED_PORT, ODR) = xx
#define CONCAT(a,b)		a##_##b
#define PORT(a,b)		CONCAT(a,b)

// on-board LED
#define LED_PORT		PC
#define LED_PIN			GPIO_PIN2

// UART2_TX
#define UART_PORT		PD
#define UART_TX_PIN		GPIO_PIN5

/***** Stepper motor *****/
// Clocking
#define STP0_CLK_PORT	PC
#define STP0_CLK_PIN	GPIO_PIN1
#define STP1_CLK_PORT	PD
#define STP1_CLK_PIN	GPIO_PIN4
#define STP2_CLK_PORT	PD
#define STP2_CLK_PIN	GPIO_PIN2
// Direction
#define STP0_DIR_PORT	PD
#define STP0_DIR_PIN	GPIO_PIN0
#define STP1_DIR_PORT	PD
#define STP1_DIR_PIN	GPIO_PIN7
#define STP2_DIR_PORT	PF
#define STP2_DIR_PIN	GPIO_PIN4
// Enable
#define STP0_EN_PORT	PE
#define STP0_EN_PIN		GPIO_PIN5
#define STP1_EN_PORT	PD
#define STP1_EN_PIN		GPIO_PIN3
#define STP2_EN_PORT	PD
#define STP2_EN_PIN		GPIO_PIN1

/** sensors for each motor **/
// SETUP (interrupts: only falling edge; inputs: all with weak pull-up)
#define SETUP_EP(x) SET_EP ## x ## UP()

// motor 0: PC3..PC7, 5EPs
#define SET_EP0UP()  do{PORT(PC, CR2) |= 0xfc; PORT(PC, CR1) |= 0xfc; EXTI_CR1 |= 0x20;}while(0)
// motor 1: PB0..PB3, 4EPs
#define SET_EP1UP()  do{PORT(PB, CR2) |= 0x0f; PORT(PB, CR1) |= 0x0f; EXTI_CR1 |= 0x08;}while(0)
// motor 2: PB4, PB5, PA1, PA2, 4EPs
#define SET_EP2UP()  do{PORT(PB, CR2) |= 0x30; PORT(PA, CR2) |= 0x06; \
						PORT(PB, CR1) |= 0x30; PORT(PA, CR1) |= 0x06; EXTI_CR1 |= 0x0a; }while(0)

/*
// motor 0: PC3..PC7, 5EPs
#define SET_EP0UP()  do{PORT(PC, CR1) |= 0xfc; }while(0)
// motor 1: PB0..PB3, 4EPs
#define SET_EP1UP()  do{PORT(PB, CR1) |= 0x0f; }while(0)
// motor 2: PB4, PB5, PA1, PA2, 4EPs
#define SET_EP2UP()  do{PORT(PB, CR1) |= 0x30; PORT(PA, CR1) |= 0x06;  }while(0)
*/
// GET VALUE
#define READ_EP(x) GET_EP ## x ## _()
#define GET_EP0_()	(( PORT(PC, IDR) >> 3 ))
#define GET_EP1_()	(( PORT(PB, IDR) & 0x0f ))
#define GET_EP2_()	(( ((PORT(PB,IDR) >> 4) & 0x03) | ((PORT(PA,IDR) << 1) & 0x0c) ))


#endif // __PORTS_DEFINITION_H__
