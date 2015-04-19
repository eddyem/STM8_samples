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

#include "main.h"
#include "interrupts.h"
#include "led.h"

/*
int temp;
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { temp=(a);(a)=(b);(b)=temp; }
int p[9]; // buffer for median filtering
int opt_med9(){
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[4]) ; PIX_SORT(p[6], p[7]) ;
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[3]) ; PIX_SORT(p[5], p[8]) ; PIX_SORT(p[4], p[7]) ;
    PIX_SORT(p[3], p[6]) ; PIX_SORT(p[1], p[4]) ; PIX_SORT(p[2], p[5]) ;
    PIX_SORT(p[4], p[7]) ; PIX_SORT(p[4], p[2]) ; PIX_SORT(p[6], p[4]) ;
    PIX_SORT(p[4], p[2]) ; return(p[4]) ;
}
*/

U32 Global_time = 0L; // global time in ms

// one digit emitting time
#define LED_delay  1
// pause between buttons actions
#define BTNS_delay  30

int main() {
	U32 T_LED = 0L;  // time of last digit update
	U32 T_time = 0L; // timer
	U32 T_btns = 0L; // buttons timer
	U8 old_btns_state = 0;
	//U8 tmpva = 0;
//	int pidx = 0; // index in median buffer
//	long cntrin = 0, cntrcap = 0; // counters for average

	// Configure clocking
	CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz
	// Configure pins
	CFG_GCR |= 1; // disable SWIM
	LED_init();
	CCR |= 0x28; // make shure that we are on level 3 - disabled SW priority
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

	// enable all interrupts
	enableInterrupts();
	set_display_buf("abc");
	// Loop
	do {
		// onse per 300ms refresh displayed value
		if(((unsigned int)(Global_time - T_time) > 300) || (T_time > Global_time)){
			T_time = Global_time;
		//	display_int((int)tmpva++,0);
//			cntrin = 0;cntrcap = 0;voltagein = 0L;voltagecap = 0L;pidx = 0;
		}
		if((U8)(Global_time - T_LED) > LED_delay){
			T_LED = Global_time;
			show_next_digit();
			/*if(buttons_state != old_btns_state){ // user has pressed or released any button
				;
				old_btns_state = buttons_state;
			}*/
		}
		if((U8)(Global_time - T_btns) > BTNS_delay){
			T_btns = Global_time;
			if(old_btns_state != buttons_state){
				U8 pressed = old_btns_state & ~buttons_state; // pressed buttons are ones
				if(pressed){ // display
					//display_int(pressed, 0);
					display_int(buttons_state, 0);
				}else{ // some buttons released
					if(buttons_state == 0xff){ // all buttons released
						set_display_buf("abc");
					}else{
						display_int(~old_btns_state & buttons_state, 0); // released are ones
					}
					old_btns_state = buttons_state;
				}
			}
		}
	} while(1);
}

/*
			if(ADC_ready){
				// prepare data for rounded output
				p[pidx] = ADC_value;
				if(++pidx == 9){ // buffer is ready
					if(vcap){
						voltagecap += (long)(opt_med9());
						cntrcap++;
						ADC_CSR = 0x24; // switch to ain
						vcap = 0;
					}else{
						voltagein += (long)(opt_med9());
						cntrin++;
						ADC_CSR = 0x26; // switch to vcap
						vcap = 1;
					}
					pidx = 0;
				}
				ADC_ready = 0;
			}
*/
