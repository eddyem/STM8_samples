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
U16 paused_val = 500; // interval between LED flashing

int main() {
	unsigned long T = 0L;
	U8 rb;
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

	// PC2 - PP output (on-board LED)
	PORT(LED_PORT, DDR)  |= LED_PIN;
	PORT(LED_PORT, CR1)  |= LED_PIN;

	uart_init();

	// enable all interrupts
	enableInterrupts();

	// Loop
	do{
		if((Global_time - T > paused_val) || (T > Global_time)){
			T = Global_time;
			PORT(LED_PORT, ODR) ^= LED_PIN; // blink on-board LED
		}
		if(UART_read_byte(&rb)){ // buffer isn't empty
			switch(rb){
				case 'h': // help
				case 'H':
					uart_write("\nPROTO:\n+/-\tLED period\nS/s\tset/get Mspeed\n"
					);
				break;
				case '+':
					paused_val += 100;
					if(paused_val > 10000)
						paused_val = 500; // but not more than 10s
				break;
				case '-':
					paused_val -= 100;
					if(paused_val < 500)  // but not less than 0.5s
						paused_val = 500;
				break;
			}
		}
	}while(1);
}


