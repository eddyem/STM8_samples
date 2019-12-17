/*
 * blinky.c
 *
 * Copyright 2014 Edward V. Emelianoff <eddy@sao.ru>
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
#include "ports_definition.h"
#include "interrupts.h"
#include "uart.h"

volatile unsigned long Global_time = 0L; // global time in ms
/*
    LED channels:
    0 - D3
    1 - D2
    2 - D1
    3 - C7
    4 - C6
    5 - C5
    6 - C4
    7 - C3
 */
/* don't work!
static U8* LEDODRS[8] = {(U8*)0x500F, &PD_ODR, &PD_ODR, &PC_ODR, &PC_ODR, &PC_ODR, &PC_ODR, &PC_ODR};
static const U8 LEDPINS[8] = {1<<3, 1<<2, 1<<1, 1<<7, 1<<6, 1<<5, 1<<4, 1<<3};
*/

int main() {
	unsigned long T = 0L;
	U8 rb;
	CFG_GCR |= 1; // disable SWIM
	// Configure clocking
	CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz
    // TIM1 - system timer (1ms)
    TIM1_PSCRH = 0;
	TIM1_PSCRL = 15; // LSB should be written last as it updates prescaler
	// auto-reload each 1ms: TIM_ARR = 1000 = 0x03E8
	TIM1_ARRH = 0x03;
	TIM1_ARRL = 0xE8;
	// interrupts: update
	TIM1_IER = TIM_IER_UIE;
	// auto-reload + interrupt on overflow + enable
	TIM1_CR1 = TIM_CR1_APRE | TIM_CR1_URS | TIM_CR1_CEN;
    // leds - opendrain
    PD_DDR = 0x0e;
    PC_DDR = 0xf8;
/*
    PD_CR1 = 0x0e;
    PC_CR1 = 0xf8;
*/
	PORT(LED_PORT, DDR)  |= LED_PIN;
	PORT(LED_PORT, CR1)  |= LED_PIN;

    uart_init();

	// enable all interrupts
	enableInterrupts();

	// Loop
	do{
		if(Global_time - T > 499){
			T = Global_time;
			PORT(LED_PORT, ODR) ^= LED_PIN; // blink on-board LED
		}
		if(uart_read_cmd(&rb)){ // buffer isn't empty
            switch(rb){
                case '1':
                    PD_ODR |= (1<<3);
                break;
                case '2':
                    PD_ODR |= (1<<2);
                break;
                case '3':
                    PD_ODR |= (1<<1);
                break;
                case '4':
                    PC_ODR |= (1<<7);
                break;
                case '5':
                    PC_ODR |= (1<<6);
                break;
                case '6':
                    PC_ODR |= (1<<5);
                break;
                case '7':
                    PC_ODR |= (1<<4);
                break;
                case '8':
                    PC_ODR |= (1<<3);
                break;
                case 'A':
                    PD_ODR &= (1<<3);
                break;
                case 'B':
                    PD_ODR &= (1<<2);
                break;
                case 'C':
                    PD_ODR &= (1<<1);
                break;
                case 'D':
                    PC_ODR &= (1<<7);
                break;
                case 'E':
                    PC_ODR &= (1<<6);
                break;
                case 'F':
                    PC_ODR &= (1<<5);
                break;
                case 'G':
                    PC_ODR &= (1<<4);
                break;
                case 'H':
                    PC_ODR &= (1<<3);
                break;
                case '9':
                    PD_ODR |= 0x0e;
                    PC_ODR |= 0xf8;
                break;
                case 'I':
                    PD_ODR &= ~0x0e;
                    PC_ODR &= ~0xf8;
                break;
            }
		}
	}while(1);
}

