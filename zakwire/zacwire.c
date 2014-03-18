/*
 * zacwire.c - functions for zacwire sensor processing
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
#include "zacwire.h"
#include "interrupts.h"
#include "led.h"

// error codes for zacwire bit converting
#define ERR_STROBE_TOO_SHORT	1
#define ERR_BAD_CRC				2

int Temperature_value = 0;     // measurement result
U8 ZW_data_ready = 0;          // flag - measurement is ready
U8 bit_cntr = 0;               // current bit number (in ZW proto - to omit start/crc bits)
U8 temp_measurement = 0;       // flag - temperature measurement in progress
U8 ZW_bits[20];                // values of pulse lengths

/**
 * Turn on ZW & start measurement
 */
void ZW_on(){
	U8 i;
	//EXTI_CR1 = 0x80; // PDIS = 10 - falling edge
	PD_CR2 |= GPIO_PIN3; // turn on interrupts
	for(i = 0; i < 20; i++)
		ZW_bits[i] = 0; // clear old values
	bit_cntr = 0;
	ZW_data_ready = 0;
	temp_measurement = 1;
	PA_ODR |= GPIO_PIN2; // power on zacwire sensor
}

/**
 * Turn off Zacwire
 */
void ZW_off(){
	PD_CR2 &= ~GPIO_PIN3;
	TIM2_CR1 = 0;
	temp_measurement = 0;
	PA_ODR &= ~GPIO_PIN2; // turn off ZW
}

/**
 * Convert 8 bytes from ptr into 8bit value
 * @param ptr - pointer to 10 bit word (strobe + 8 data bits + CRC)
 * @param val - 8bit value
 * @return error code in case of error
 */
U8 get_byte(U8 *ptr, U8 *val){
	U8 i, data = 0, crc = 0;
	U8 Strobe = ptr[0];
	if(Strobe < 5){ // strobe too short - error!
		return ERR_STROBE_TOO_SHORT;
	}
	for(i = 1; i < 9; i++){ // get bit data
		data <<= 1;
		if(ptr[i] < Strobe){ // logic 1
			data |= 1;
			crc ^= 1;
		}
	}
	// now check CRC:
	if((ptr[9] < Strobe) ^ crc){ // error: bad parity
		return ERR_BAD_CRC;
	}
	*val = data;
	return 0;
}

U8 get_temperature(){
	U8 H, L, err;
	long temper;
	err = get_byte(ZW_bits, &H); // MSB
	if(!err)
		err = get_byte(&ZW_bits[10], &L); // LSB
	if(err){
		if(err == ERR_BAD_CRC)
			set_display_buf("EBAD");
		else
			set_display_buf("E DA");
		return 0;
	}
	/*
	for(i = 6; i < 19; i++){ // omit first 5 zeros and strobe bit
		if(i == 9 || i == 10)
			continue;
		if(ZW_bits[i] < 10){ // logic 1
			Zdata |= 1;
		}
		Zdata <<= 1;
	}*/
	// correct temperature is VAL/2047*70-10, but we have integer (float*10), so:
	// T = (VAL*700)/2047 - 100, result have only 12 bits, so we can do this
	temper = ((long)H<<8 | L) * 700L;
	Temperature_value = (int)(temper / 2047L - 100L);
	//Temperature_value = ((int)H)<<8 | L;
	return 1;
}

/**
 * Get bit value
 * TIM2_CCR1H & TIM2_CCR1L is zero pulse length
 * TIM2_CCR2H & TIM2_CCR2L is time (for previous counter run) since timer up till 1->0 transition
 */
void ZW_catch_bit(){
//	U8 H = TIM2_CCR2H; // length of previous high level signal
//	U8 L = TIM2_CCR2L;
	/*if(H){ // error: pulse too long
		ZW_off();
		display_int(H << 8 | L);
		//set_display_buf("E002");
		return;
	}
	ZW_bits[bit_cntr++] = L;
	*/
		U8 i;
		while((PD_IDR & GPIO_PIN3) == 0){
			ZW_bits[bit_cntr]++;
			for(i = 0; i < 4; i++)
				nop();
		}
	if(++bit_cntr == 20){ // all bits done
		ZW_off();
		if(get_temperature())
			ZW_data_ready = 1;
		else
			ZW_on(); // in case of error start measurement again
	}else
		PD_CR2 |= GPIO_PIN3;
}
