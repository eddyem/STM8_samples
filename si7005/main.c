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
#include "si7005.h"

volatile unsigned long Global_time = 0L; // global time in ms
U16 paused_val = 500; // interval between LED flashing

#if defined STM8S105
U8 unlock_EEPROM(){
	// unlock memory
	FLASH_DUKR = EEPROM_KEY1;
	FLASH_DUKR = EEPROM_KEY2;
	// check bit DUL=1 in FLASH_IAPSR
	if(!(FLASH_IAPSR & 0x08))
		return 0;
	return 1;
}

void lock_EEPROM(){
	while(!(FLASH_IAPSR & 0x04)); // wait till end
	// clear DUL to lock write
	FLASH_IAPSR &= ~0x08;
}

/**
 * check OPT2 bit AFR6 for I2C remapping to PB4/PB5
 */
U8 opt2_default_setup(){
	U8 ret = 0;
	U8 val = OPT2 | 0x40;
	if(OPT2 & 0x40) return 0;
	disableInterrupts();
	FLASH_CR2 = 0x80; // enable write OPT
	FLASH_NCR2 = ~0x80;
	if(!unlock_EEPROM()){ret = 1; goto out;}
	if(!(FLASH_CR2 & 0x80)){ret = 3; goto out;}
	// set AFR6 in OPT2 & reset in NOPT2
	OPT2 = val;
	NOPT2 = ~val;
	lock_EEPROM();
	FLASH_CR2 &= ~0x80; // disable write OPT
	FLASH_NCR2 |= 0x80;
	ret = 2;
out:
	enableInterrupts();
	return ret;
}
#endif // STM8S105


int main() {
	unsigned long T = 0L, siT = 0L;
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

#if defined STM8S105
	opt2_default_setup();
#endif
	uart_init();

	si7005_setup();

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
					uart_write("\nPROTO:\n+/-\tLED period\n"
						"I\tread Si7005 device id\n"
						"T\tread themperature\n"
						"P\tread pressure\n"
						//"O\tread OPT"
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
				case 'I':
					si7005_read_ID();
				break;
				case 'T':
					si7005_read_T();
				break;
				case 'P':
					si7005_read_P();
				break;
/*				case 'O':
					printUHEX(OPT2);
					printUHEX(NOPT2);
				break;
				case 'W':
					printUHEX(opt2_default_setup());
				break;
*/
			}
		}
		if(Global_time != siT){
			siT = Global_time;
			si7005_process();
		}
	}while(1);
}


