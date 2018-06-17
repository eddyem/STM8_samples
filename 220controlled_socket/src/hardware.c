/*
 *                                                                                                  geany_encoding=koi8-r
 * hardware.c
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
 *
 */
#include "hardware.h"

/**
 * HW:
 * PA1 - ~IN1   - PULLUP IN, optocouple input, 0 active
 * PA2 - OUT1   - PUSHPULL OUT, optocouple output, 1 active
 * PA3 - OUT0   - PUSHPULL OUT, optocouple output, 1 active
 * PB4 - ~IN0   - PULLUP IN, direct input
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

void hw_init(){
    CFG_GCR |= 1; // disable SWIM
    // Configure clocking
    CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz
// Timer 4 (8 bit) used as system tick timer
    // prescaler == 128 (2^7), Tfreq = 125kHz
    // period = 1ms, so ARR = 125
    TIM4_PSCR = 7;
    TIM4_ARR = 125;
    // interrupts: update
    TIM4_IER = TIM_IER_UIE;
    // auto-reload + interrupt on overflow + enable
    TIM4_CR1 = TIM_CR1_APRE | TIM_CR1_URS | TIM_CR1_CEN;

    // default state
    RESET_PKEY1();
    // PA: 1 - PU IN, 2,3 - PP OUT
    PA_DDR = GPIO_PIN2 | GPIO_PIN3;
    PA_CR1 = GPIO_PIN1 | GPIO_PIN2 | GPIO_PIN3;
    // PB: 4 - PU IN, 5 - PP OUT with 1 in default state
    PB_DDR = GPIO_PIN5;
    PB_CR1 = GPIO_PIN4 | GPIO_PIN5;
    // PC: 3-7 - PP OUT
    PC_DDR = GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7;
    PC_CR1 = GPIO_PIN3 | GPIO_PIN4 | GPIO_PIN5 | GPIO_PIN6 | GPIO_PIN7;
    // PD: 2 - PP OUT, 3 - AIN
    PD_DDR = GPIO_PIN2;
    PD_CR1 = GPIO_PIN2;
    // configure ADC
    // select PD3[AIN4] & enable interrupt for EOC
    ADC_CSR = 0x24; // 0x24 - AIN4
    ADC_TDRL = 0x10; // disable Schmitt triger for AIN4
    // right alignment
    ADC_CR2 = 0x08; // don't forget: first read ADC_DRL!
    // f_{ADC} = f/18 & continuous non-buffered conversion & wake it up
    ADC_CR1 = 0x71; // 0x71 - single, 0x73 - continuous
    ADC_CR1 = 0x71; // turn on ADC (this needs second write operation)
}
