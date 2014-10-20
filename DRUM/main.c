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
#include "main.h"
#include "noicegen.h"
#include "CD74HC154_LEDs.h"

/*
 * 0 0000
 * 1 0001
 * 2 0010
 * 3 0011
 * 4 0100
 * 5 0101
 * 6 0110
 * 7 0111
 * 8 1000
 * 9 1001
 * a 1010
 * b 1011
 * c 1100
 * d 1101
 * e 1110
 * f 1111
 */

unsigned long Global_time = 0L, boom_start = 0L; // global time in ms
U16 boom_length = 100; // length of "boom" in ms
U16 paused_val = 500; // interval between LED flashing
U8 sample_flag = 0;   // flag is set in interrupt -> next sample in sound

#ifdef UART
U8 UART_rx[UART_BUF_LEN]; // cycle buffer for received data
U8 UART_rx_start_i = 0;   // started index of received data (from which reading starts)
U8 UART_rx_cur_i = 0;     // index of current first byte in rx array (to which data will be written)
U8 UART_is_our = 0;       // ==1 if we get UART
// ATTENTION! to change global variable in PROGRAM memory, it should be CONST!!!
const U8 UART_devNUM = THIS_DEVICE_NUM; // device number, master sais it

/**
 * Send one byte through UART
 * @param byte - data to send
 */
void UART_send_byte(U8 byte){
	UART2_DR = byte;
	while(!(UART2_SR & UART_SR_TC));
}

void uart_write(char *str){
	while(*str){
		UART2_DR = *str++;
		while(!(UART2_SR & UART_SR_TC));
	}
}

/**
 * Read one byte from Rx buffer
 * @param byte - where to store readed data
 * @return 1 in case of non-empty buffer
 */
U8 UART_read_byte(U8 *byte){
	if(UART_rx_start_i == UART_rx_cur_i) // buffer is empty
		return 0;
	*byte = UART_rx[UART_rx_start_i++];
	check_UART_pointer(UART_rx_start_i);
	return 1;
}

void printUint(U8 *val, U8 len){
	unsigned long Number = 0;
	U8 i = len;
	char ch;
	U8 decimal_buff[12]; // max len of U32 == 10 + \n + \0
	if(len > 4 || len == 3 || len == 0) return;
	for(i = 0; i < 12; i++)
		decimal_buff[i] = 0;
	decimal_buff[10] = '\n';
	ch = 9;
	switch(len){
		case 1:
			Number = *((U8*)val);
			break;
		case 2:
			Number = *((U16*)val);
		break;
		case 4:
			Number = *((unsigned long*)val);
		break;
	}
	do{
		i = Number % 10L;
		decimal_buff[ch--] = i + '0';
		Number /= 10L;
	}while(Number && ch > -1);
	uart_write((char*)&decimal_buff[ch+1]);
}

U8 readInt(U16 *val){
	unsigned long T = Global_time;
	unsigned long R = 0;
	U16 readed;
	U8 rb, ret = 0, bad = 0;
	do{
		if(!UART_read_byte(&rb)) continue;
		if(rb < '0' || rb > '9') break; // number ends with any non-digit symbol that will be omitted
		ret = 1; // there's at least one digit
		R = R * 10L + rb - '0';
		if(R > 0xffff){ // bad value
			R = 0;
			bad = 1;
		}
	}while(Global_time - T < 10000); // wait no longer than 10s
	if(bad || !ret) return 0;
	readed = (U16) R;
	*val = readed;
	return 1;
}


void error_msg(char *msg){
	uart_write("\nERROR: ");
	uart_write(msg);
	UART_send_byte('\n');
}
#endif // UART


int main() {
	unsigned long T = 0L;
	unsigned long I;
	U8 bank_i = 0;  // number of sample in meander (even/odd)
	U8 cur_vol;
	U16 Ival;
#ifdef UART
	U8 rb;
#endif
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
	// EXTI: PC1 & PC2 == "boom"
	EXTI_CR1 = 0x10; // PCIS = 01 - rising edge only
	// PC_CR1 = 0x06;   // PC1 & PC2  pullup
	// PC_CR1 = 0x06; // enable EXTI
#ifdef UART
	// Configure pins
	// PC2 - PP output (on-board LED)
	PORT(LED_PORT, DDR) |= LED_PIN;
	PORT(LED_PORT, CR1) |= LED_PIN;
	// PD5 - UART2_TX -- pseudo open-drain output; don't forget an pullup resistor!
	PORT(UART_PORT, DDR) |= UART_TX_PIN;
	PORT(UART_PORT, ODR) &= ~UART_TX_PIN; // turn off N push-down
	//PORT(UART_PORT, CR1) |= UART_TX_PIN;
#endif
	// LEDS on DRUM
	PORT(LEDS_PORT, DDR) |= 0x1f;
	PORT(LEDS_PORT, CR1) |= 0x1f;
	PC_DDR |= GPIO_PIN1; // setup timer's output
	PC_ODR &= ~GPIO_PIN1;
#ifdef UART
	// Configure UART
	// 9 bit, no parity, 1 stop (UART_CR3 = 0 - reset value)
	// 57600 on 16MHz: BRR1=0x11, BRR2=0x06
	UART2_BRR1 = 0x11; UART2_BRR2 = 0x06;
	UART2_CR2 = UART_CR2_TEN | UART_CR2_REN | UART_CR2_RIEN; // Allow RX/TX, generate ints on rx
#endif
	configure_timers();

	// enable all interrupts
	enableInterrupts();

	// Loop
	do{
		if(sample_flag){ // next sample in sound (TIM2 irq)
			sample_flag = 0;
			I = Global_time - boom_start; // amount of us from start
			if(I > boom_length || boom_start > Global_time){
				// end of sound
				stop_snd();
			}else{
				// generate meander
				if(bank_i){
					change_CCR(0);
					bank_i = 0;
				}else{
					I <<= 8; // multiply by PWM len
					I /= boom_length;
					cur_vol = 255 - (I & 0xff); // linear fading
					change_CCR(cur_vol);
					bank_i = 1;
				}
			}
		}
		// paused_val is period of LEDs effects -> move to next number if EFFECT != 0
		if((Global_time - T > paused_val) || (T > Global_time)){
			T = Global_time;
			#ifdef UART
			PORT(LED_PORT, ODR) ^= LED_PIN; // blink on-board LED
			#endif
			if(LED_effect){
				next_LED_in_effects();
			}
		}
		#ifdef UART
		if(UART_read_byte(&rb)){ // buffer isn't empty
			switch(rb){
				case 'h': // help
				case 'H':
					uart_write("\nPROTO:\n"
						"+/-\tLED period\n"
						"P/p\tBoom\n"
						"F\tSet frequency\n"
						"L\tChange boom length (in ms)\n"
						"l\tblink LEDs by mask\n"
						"r\treset LEDs\n"
						"E\tset effect\n"
						);
				break;
				break;
				case '+':
					paused_val += 100;
					if(paused_val > 10000)
						paused_val = 500; // but not more than 10s
				break;
				case '-':
					paused_val -= 100;
					if(paused_val < 100)  // but not less than 0.1s
						paused_val = 500;
				break;
				case 'F':
					if(readInt(&Ival)){
						change_period(Ival);
					}else error_msg("bad period");
				break;
				case 'P':
				case 'p':
					play_snd();
				break;
				case 'L':
					if(readInt(&Ival) && Ival < 1000 && Ival > 1){
						boom_length = Ival;
					}else error_msg("bad length");
				break;
				case 'l':
					if(readInt(&Ival) && (Ival == (Ival & 0x3f))){
						set_LEDs(Ival);
					}else error_msg("bad bitmask");
				break;
				case 'r':
					reset_LEDs();
				break;
				case 'E':
					if(readInt(&Ival)) set_effect(Ival);
				break;
			}
		}
		#endif
	}while(1);
}


