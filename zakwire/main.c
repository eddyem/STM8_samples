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
#include "zacwire.h"

unsigned long Global_time = 0L; // global time in ms
int ADC_value = 0; // value of last ADC measurement
U8 LED_delay = 1; // one digit emitting time

int main() {
	unsigned long T_LED = 0L;  // time of last digit update
	unsigned long T_time = 0L; // timer
	U8 dp_err_pos = 0;
	U8 curbit = 0;
	// Configure clocking
	CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz
	// Configure pins
	CFG_GCR |= 1; // disable SWIM
	LED_init();
	// configure PD3[TIM2_CH2] as input for zacwire
	PD_CR1 |= GPIO_PIN3; // weak pullup
	CCR |= 0x28; // make shure that we are on level 3
	EXTI_CR1 = 0x80; // PDIS = 10 - falling edge
	// PA2 is output for zacwire powering
	PA_DDR |= GPIO_PIN2; // output
	PA_CR1 |= GPIO_PIN2; // push-pull
	// Configure Timer1
	// prescaler = f_{in}/f_{tim1} - 1
	// set Timer1 to 1MHz: 16/1 - 1 = 15
	TIM1_PSCRH = 0;
	TIM1_PSCRL = 15; // LSB should be written last as it updates prescaler
	// auto-reload each 1ms: TIM_ARR = 1000 = 0x03E8
	TIM1_ARRH = 0x03;
	TIM1_ARRL = 0xE8;
	// interrupts: update
	TIM1_IER = TIM_IER_UIE;
	// auto-reload + interrupt on overflow + enable
	TIM1_CR1 = TIM_CR1_APRE | TIM_CR1_URS | TIM_CR1_CEN;
	// Configure Timer2:
	// capture/compare channel
	// channel CC1 (0->1) stores low pulse length,
	// channel CC2 (1->0) stores time period between two consequent zero-transitions
	//TIM2_IER = TIM_IER_CC2IE;// |TIM_IER_UIE ;
	//TIM2_CCER1 = 0x30; // CC2: enable, capture on 1->0; CC1: disable
	// TIM2 frequency == 16MHz / 2^TIM2_PSCR
	// main frequency: 1MHz
//	TIM2_PSCR = 15;
/*
	TIM2_PSCR = 1; // 8MHz
	TIM2_CCMR2 = 1;
	TIM2_CCER1 = 0x10; // CC2: enable, capture on 0->1
	TIM2_IER = TIM_IER_CC2IE;
	*/
/*
	// configure ADC
	// select PD2[AIN3] & enable interrupt for EOC
	ADC_CSR = 0x23;
	ADC_TDRL = 0x08; // disable Schmitt triger for AIN3
	// right alignment
	ADC_CR2 = 0x08; // don't forget: first read ADC_DRL!
	// f_{ADC} = f/18 & continuous non-buffered conversion & wake it up
	ADC_CR1 = 0x73;
	ADC_CR1 = 0x73; // turn on ADC (this needs second write operation)
*/
	// enable all interrupts
	enableInterrupts();
	set_display_buf("----"); // on init show ----
	show_next_digit();
	// Loop
	do {
		if(((unsigned int)(Global_time - T_time) > 1000) || (T_time > Global_time)){ // once per 3 seconds we start measurement
			T_time = Global_time;
			if(ZW_data_ready){ // measurement is ready - display results
				ZW_data_ready = 0;
				display_int(Temperature_value); // fill display buffer with current temperature
				display_DP_at_pos(2); // we always show DP here
				/*display_int((int)ZW_bits[curbit++]);
				if(curbit == 20){
					curbit = 0;
					ZW_data_ready = 0;
				}
				display_DP_at_pos(dp_err_pos++); // turn off old DP
				if(dp_err_pos > 3)
					dp_err_pos = 0;*/
			}//else
			if(!temp_measurement){ // there's no active measurements
				ZW_on();
			}else{ // error: no sensor
				ZW_off();
				set_display_buf("E  0");
				display_DP_at_pos(dp_err_pos++); // turn off old DP
				if(dp_err_pos > 3)
					dp_err_pos = 0;
			}
		}
		if((U8)(Global_time - T_LED) > LED_delay){
			T_LED = Global_time;
			show_next_digit();
		}
	} while(1);
}

