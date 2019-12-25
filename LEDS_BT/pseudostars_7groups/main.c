
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
#define LEDNMBR		7
/*
    LED channels:
    0 - D3
    1 - D2
    2 - D1
    3 - C7
    4 - C6
    5 - C5
    6 - C4
    7 channels for LEDs, 8th channel switch effects
 */

// number of pseudo-random numbers
#define RANDSIZE 	19
// number of variants
#define RANDCNT		4

const U8 rand[RANDCNT][RANDSIZE] = {
	{12, 12, 3, 16, 8, 20, 4, 3, 15, 1, 15, 4, 6, 15, 19, 9, 10},
	{12, 12, 3, 16, 8, 50, 4, 3, 15, 100, 15, 4, 6, 15, 19, 9, 10},
	{23, 22, 2, 11, 13, 28, 25, 1, 29, 16, 9, 7, 17, 16, 13, 21, 18},
	{30, 8, 31, 5, 24, 23, 33, 24, 18, 35, 30, 23, 29, 1, 22, 24, 25}
};

int main() {
	unsigned long T = 0L;
	U8 ON[7] = {0,0,0,0,0,0,0};
	unsigned long timers[7] = {0,0,0,0,0,0,0};
	U8 rb, i, cnt=0, rnmb=0;
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
    // leds - pushpull
    PD_DDR = 0x0e;
    PC_DDR = 0xf8;
    PD_CR1 = 0x0e;
    PC_CR1 = 0xf8;

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
		for(i = 0; i < LEDNMBR-1; ++i){
			if(ON[i]){
				if(Global_time > timers[i]){
					timers[i] = Global_time + rand[rnmb][cnt];
					if(++cnt > RANDSIZE-1) cnt = 0;
					switch(i){
						case 0:
		                    PD_ODR ^= (1<<3);
		                break;
		                case 1:
		                    PD_ODR ^= (1<<2);
		                break;
		                case 2:
		                    PD_ODR ^= (1<<1);
		                break;
		                case 3:
		                    PC_ODR ^= (1<<7);
		                break;
		                case 4:
		                    PC_ODR ^= (1<<6);
		                break;
		                case 5:
		                    PC_ODR ^= (1<<5);
		                break;
		                case 6:
		                    PC_ODR ^= (1<<4);
		                break;
		            }
				}
			}
		}
		if(uart_read_cmd(&rb)){ // buffer isn't empty
            switch(rb){
                case '1':
                    PD_ODR |= (1<<3);
                    ON[0] = 1;
                    timers[0] = Global_time;
                break;
                case '2':
                    PD_ODR |= (1<<2);
                    ON[1] = 1;
                    timers[1] = Global_time;
                break;
                case '3':
                    PD_ODR |= (1<<1);
                    ON[2] = 1;
                    timers[2] = Global_time;
                break;
                case '4':
                    PC_ODR |= (1<<7);
                    ON[3] = 1;
                    timers[3] = Global_time;
                break;
                case '5':
                    PC_ODR |= (1<<6);
                    ON[4] = 1;
                    timers[4] = Global_time;
                break;
                case '6':
                    PC_ODR |= (1<<5);
                    ON[5] = 1;
                    timers[5] = Global_time;
                break;
                case '7':
                    PC_ODR |= (1<<4);
                    ON[6] = 1;
                    timers[6] = Global_time;
                break;
                case '8': // increment rnmb
                    if(++rnmb > RANDCNT-1) rnmb = 0;
                break;
                case 'A':
                    PD_ODR &= ~(1<<3);
                    ON[0] = 0;
                break;
                case 'B':
                    PD_ODR &= ~(1<<2);
                    ON[1] = 0;
                break;
                case 'C':
                    PD_ODR &= ~(1<<1);
                    ON[2] = 0;
                break;
                case 'D':
                    PC_ODR &= ~(1<<7);
                    ON[3] = 0;
                break;
                case 'E':
                    PC_ODR &= ~(1<<6);
                    ON[4] = 0;
                break;
                case 'F':
                    PC_ODR &= ~(1<<5);
                    ON[5] = 0;
                break;
                case 'G':
                    PC_ODR &= ~(1<<4);
                    ON[6] = 0;
                break;
                case 'H':
                    // increment rnmb
                	if(++rnmb > RANDCNT-1) rnmb = 0;
                break;
                case '9':
                    PD_ODR |= 0x0e;
                    PC_ODR |= 0xf8;
                    for(i = 0; i < LEDNMBR-1; ++i){ ON[i] = 1; timers[i] = Global_time;}
                break;
                case 'I':
                    PD_ODR &= ~0x0e;
                    PC_ODR &= ~0xf8;
                    for(i = 0; i < LEDNMBR-1; ++i) ON[i] = 0;
                break;
            }
		}
	}while(1);
}

