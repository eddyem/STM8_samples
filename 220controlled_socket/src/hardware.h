/*
 * ports_definition.h - definition of ports pins & so on
 *
 * Copyright 2018 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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

// macro for using in port constructions like PORT(LED_PORT, ODR) = xx
#define FORMPORT(a, b)      a ## _ ## b
#define PORT(a, b)          FORMPORT(a , b)
#define CONCAT(a, b)        a ## b

/**
 * HW:
 * PA1 - ~IN1   - PULLUP IN, optocouple input, 0 active
 * PA2 - OUT1   - PUSHPULL OUT, optocouple output, 1 active
 * PA3 - OUT0   - PUSHPULL OUT, optocouple output, 1 active
 * PB4 - ~IN0   - PULLUP IN, direct input, 0 active
 * PB5 - PKEY1  - PUSHPULL OUT, 0 active
 * PC3 - NKEY2  - PUSHPULL OUT, TIM1_CH3, 1 active
 * PC4 - Relay1 - PUSHPULL OUT, turn ON relay 1, 1 active
 * PC5 - NKEY1  - PUSHPULL OUT, TIM2_CH1, 1 active
 * PC6 - Triac1 - PUSHPULL OUT, turn ON triac 1, 1 active
 * PC7 - Relay0 - PUSHPULL OUT, turn ON relay 0, 1 active
 * PD1 - swim   - Not used
 * PD2 - Triac0 - PUSHPULL OUT, turn ON triac 0, 1 active
 * PD3 - Cur0   - AIN, channel 0 current measurement
 * PD4 - NC     - NC
 * PD5 - Tx \ UART
 * PD6 - Rx /
 */
// getters: 1 active, 0 inactive
#define CHK_IN0()       ((PB_IDR & GPIO_PIN4) == 0)
#define CHK_IN1()       ((PA_IDR & GPIO_PIN1) == 0)
// setters
#define SET_OUT0()      PA_ODR |= GPIO_PIN3
#define SET_OUT1()      PA_ODR |= GPIO_PIN2
#define RESET_OUT0()    PA_ODR &= ~GPIO_PIN3
#define RESET_OUT1()    PA_ODR &= ~GPIO_PIN2
#define CHK_OUT0()      ((PA_ODR & GPIO_PIN3) != 0)
#define CHK_OUT1()      ((PA_ODR & GPIO_PIN2) != 0)
#define SET_PKEY1()     PB_ODR &= ~GPIO_PIN5
#define RESET_PKEY1()   PB_ODR |= GPIO_PIN5
#define CHK_PKEY1()     ((PB_ODR & GPIO_PIN5) == 0)
#define SET_NKEY1()     PC_ODR |= GPIO_PIN5
#define SET_NKEY2()     PC_ODR |= GPIO_PIN3
#define RESET_NKEY1()   PC_ODR &= ~GPIO_PIN5
#define RESET_NKEY2()   PC_ODR &= ~GPIO_PIN3
#define CHK_NKEY1()     ((PC_ODR & GPIO_PIN5) != 0)
#define CHK_NKEY2()     ((PC_ODR & GPIO_PIN3) != 0)
#define SET_TRIAC0()    PD_ODR |= GPIO_PIN2
#define RESET_TRIAC0()  PD_ODR &= ~GPIO_PIN2
#define SET_TRIAC1()    PC_ODR |= GPIO_PIN6
#define RESET_TRIAC1()  PC_ODR &= ~GPIO_PIN6
#define SET_RELAY0()    PC_ODR |= GPIO_PIN7
#define RESET_RELAY0()  PC_ODR &= ~GPIO_PIN7
#define SET_RELAY1()    PC_ODR |= GPIO_PIN4
#define RESET_RELAY1()  PC_ODR &= ~GPIO_PIN4
#define CHK_TRIAC0()    ((PD_ODR & GPIO_PIN2) != 0)
#define CHK_TRIAC1()    ((PC_ODR & GPIO_PIN6) != 0)
#define CHK_RELAY0()    ((PC_ODR & GPIO_PIN7) != 0)
#define CHK_RELAY1()    ((PC_ODR & GPIO_PIN4) != 0)


// UART2_TX
#define UART_PORT       PD
#define UART_TX_PIN     GPIO_PIN5


void hw_init();

#endif // __PORTS_DEFINITION_H__
