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

U32 Global_time = 0L; // global time in ms
eeprom_data *saved_data = (eeprom_data*)EEPROM_START_ADDR;

U32 testtimer;

// one digit emitting time
#define LED_delay  1

U8 change_eeprom_value(U8 *addr, U8 *val, U8 len){
	U8 i;
	// unlock memory
	FLASH_DUKR = EEPROM_KEY1;
	FLASH_DUKR = EEPROM_KEY2;
	// check bit DUL=1 in FLASH_IAPSR
	if(!(FLASH_IAPSR & 0x08))
		return 0;
	for(i = 0; i < len; i++)
		*addr = val[i];
	while(!(FLASH_IAPSR & 0x04)); // wait till end
	// clear DUL to lock write
	FLASH_IAPSR &= ~0x08;
	return 1;
}

void eeprom_default_setup(){
	eeprom_data template = {
		.magick = EEPROM_MAGICK,
		.ADU_to_mV = DEFAULT_ADU_TO_MV,
		.max_ADU = DEFAULT_MAX_ADU
	};
	if(saved_data->magick != EEPROM_MAGICK){
		if(change_eeprom_value((U8*)saved_data, (U8*)&template, sizeof(eeprom_data)))
			testtimer = saved_data->ADU_to_mV;
		else
			testtimer = 999;
	}else{
		testtimer = 0; // other times - from zero
	}
}

int main() {
	U32 T_LED = 0L;  // time of last digit update
	U32 T_time = 0L; // timer

	// Configure clocking
	CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz
	// Configure pins
	CFG_GCR |= 1; // disable SWIM
	LED_init();
	// configure PD3[TIM2_CH2] as input for zacwire
	PD_CR1 |= GPIO_PIN3; // weak pullup
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
	eeprom_default_setup();
	// enable all interrupts
	enableInterrupts();
	set_display_buf("-----"); // on init show -----
	// Loop
	do {
		if(((unsigned int)(Global_time - T_time) > 1000) || (T_time > Global_time)){ // once per 3 seconds we start measurement
			T_time = Global_time;
			display_long(testtimer++,0);
		}
		if((U8)(Global_time - T_LED) > LED_delay){
			T_LED = Global_time;
			show_next_digit();
		}
	} while(1);
}

