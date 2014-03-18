/*
 * main.c
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


#include "stm8l.h"
#include "interrupts.h"
#include "led.h"

#define DIGIT_PER 10
unsigned long Global_time = 0L; // global time in ms
int ADC_value = 0; // value of last ADC measurement
U8 LED_delay = 1; // one digit emitting time

int main() {
	unsigned long T_LED = 0L;  // time of last digit update
	unsigned long T_time = 0L; // timer
	int i = -1200;
	// Configure clocking
	CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz
	// Configure pins
	CFG_GCR |= 1; // disable SWIM
	LED_init();
	// Configure Timer1
	// prescaler = f_{in}/f_{tim1} - 1
	// set Timer1 to 1MHz: 1/1 - 1 = 15
	TIM1_PSCRH = 0;
	TIM1_PSCRL = 15; // LSB should be written last as it updates prescaler
	// auto-reload each 1ms: TIM_ARR = 1000 = 0x03E8
	TIM1_ARRH = 0x03;
	TIM1_ARRL = 0xE8;
	// interrupts: update
	TIM1_IER = TIM_IER_UIE;
	// auto-reload + interrupt on overflow + enable
	TIM1_CR1 = TIM_CR1_APRE | TIM_CR1_URS | TIM_CR1_CEN;
	// configure ADC
	// select PD2[AIN3] & enable interrupt for EOC
	ADC_CSR = 0x23;
	ADC_TDRL = 0x08; // disable Schmitt triger for AIN3
	// right alignment
	ADC_CR2 = 0x08; // don't forget: first read ADC_DRL!
	// f_{ADC} = f/18 & continuous non-buffered conversion & wake it up
	ADC_CR1 = 0x73;
	ADC_CR1 = 0x73; // turn on ADC (this needs second write operation)
	// enable all interrupts
	enableInterrupts();
	display_int(i++);
	show_next_digit(); // show zero
	// Loop
	do {
		if(((unsigned int)(Global_time - T_time) > DIGIT_PER) || (T_time > Global_time)){ // set next timer value
			T_time = Global_time;
			display_int(i++);
			if(i > 9999) i = -1200;
			// check ADC value to light up DPs proportionaly
			if(ADC_value > 819) display_DP_at_pos(0); // big value == 4 points
			if(ADC_value > 614) display_DP_at_pos(1); // less == 3 points
			if(ADC_value > 410) display_DP_at_pos(2); // little == 2 points
			if(ADC_value > 205) display_DP_at_pos(3); // more little == 1 point
			// values less than 206 == 0
		}
		if((U8)(Global_time - T_LED) > LED_delay){
			T_LED = Global_time;
			show_next_digit();
		}
	} while(1);
}

