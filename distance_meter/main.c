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
#include "ports_definition.h"
#include "interrupts.h"
#include "main.h"

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

unsigned long Global_time = 0L; // global time in ms
U16 paused_val = 500; // interval between LED flashing

volatile U16 ADC_value = 0; // value of last ADC measurement

volatile U16 Pulse_length = 11; // length of ultrasonic echo pulse
U8 US_flag = 0;// 0 - conversion in progress; 1 - conversion error; 2 - conversion done; 3 - overflow

U8 UART_rx[UART_BUF_LEN]; // cycle buffer for received data
U8 UART_rx_start_i = 0;   // started index of received data (from which reading starts)
U8 UART_rx_cur_i = 0;     // index of current first byte in rx array (to which data will be written)
// ATTENTION! to change global variable in PROGRAM memory, it should be CONST!!!
//const U8 UART_devNUM = THIS_DEVICE_NUM; // device number, master sais it

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
/*
U8 U8toHEX(U8 val){
	val &= 0x0f;
	if(val < 10) val += '0';
	else val += 'a' - 10;
	return val;
}

void printUintHEX(U8 *val, U8 len){
	U8 i, V;
	uart_write("0x");
	for(i = 0; i < len; i++){
		V = *val++;
		UART_send_byte(U8toHEX(V>>4)); // MSB
		UART_send_byte(U8toHEX(V));    // LSB
	}
	UART_send_byte('\n');
}*/

U8 readInt(int *val){
	unsigned long T = Global_time;
	unsigned long R = 0;
	int readed;
	U8 sign = 0, rb, ret = 0, bad = 0;
	do{
		if(!UART_read_byte(&rb)) continue;
		if(rb == '-' && R == 0){ // negative number
			sign = 1;
			continue;
		}
		if(rb < '0' || rb > '9') break; // number ends with any non-digit symbol that will be omitted
		ret = 1; // there's at least one digit
		R = R * 10L + rb - '0';
		if(R > 0x7fff){ // bad value
			R = 0;
			bad = 0;
		}
	}while(Global_time - T < 10000); // wait no longer than 10s
	if(bad || !ret) return 0;
	readed = (int) R;
	if(sign) readed *= -1;
	*val = readed;
	return 1;
}

void error_msg(char *msg){
	uart_write("\nERROR: ");
	uart_write(msg);
	UART_send_byte('\n');
}

/**
 * Change variable stored in program memory
 * !!! You can change only const values (non-constants are initializes on program start)
 * @param addr - variable address
 * @param new value
 * @return 0 in case of error
 */
U8 change_progmem_value(U8 *addr, U8 val){
	// unlock memory
	FLASH_PUKR = EEPROM_KEY2;
	FLASH_PUKR = EEPROM_KEY1;
	// check bit PUL=1 in FLASH_IAPSR
	if(!FLASH_IAPSR & 0x02)
		return 0;
	*addr = val;
	// clear PUL to lock write
	FLASH_IAPSR &= ~0x02;
	return 1;
}

/*
U8 change_eeprom_value(U8 *addr, U8 val){
	// unlock memory
	FLASH_DUKR = EEPROM_KEY1;
	FLASH_DUKR = EEPROM_KEY2;
	// check bit DUL=1 in FLASH_IAPSR
	if(!FLASH_IAPSR & 0x08)
		return 0;
	*addr = val;
	// clear DUL to lock write
	FLASH_IAPSR &= ~0x08;
	return 1;
}
*/

/**
 * Start measurements by ultrasonic distance meter
 * @param
 * @return
 */
void US_start(){
	U8 i;
	TIM1_CNTRH = 0; TIM1_CNTRL = 0;
	TIM1_CCR2H = 0; TIM1_CCR2L = 0;
	US_flag = 0;
	// post initial pulse
	PORT(US_OUT_PORT, ODR) |= US_OUT_PIN;
	for(i = 0; i < 160; i++) nop(); // wait at least for 10us
	PORT(US_OUT_PORT, ODR) &= ~US_OUT_PIN;
	// Enable the captures by writing the CC1E and CC2E bits to 1 in the TIM1_CCER1 register.
	TIM1_CCER1 |= 0x11;
	TIM1_CR1 = TIM_CR1_CEN; // turn on timer
}

/**
 * Check ultrasonic flag value and send message
 * @param
 * @return
 */
void US_check(){
	unsigned long L;
	if(!US_flag) return;
	if(US_flag == 1){ // error - write message
		error_msg("measurement overcapture");
	}else if(US_flag == 3){ // overflow
		error_msg("TIM1 overflow");
	}else{ // all OK - write measured length
		Pulse_length =  ((U16)TIM1_CCR2H << 8) | TIM1_CCR2L;
		printUint((U8*)&Pulse_length,2);
		// sound velocity == 340m/s in normal conditions
		// So distance = pulse * 1e-6(us) * 340 / 2
		// in millimeters this is equal of pulse*170/1000
		L = ((unsigned long) Pulse_length) * 170L;
		L /= 1000L;
		uart_write("Distance (mm): ");
		printUint((U8*)&L, 4);
	}
	US_flag = 0;
}

int main(){
	unsigned long T = 0L;
	//int Ival;
//	unsigned long ul;
//	U16 u16;
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

	// Configure Timer1 for measurement of US pulse width:
	// main frequency: 1MHz
	// prescaler = f_{in}/f_{tim1} - 1
	// set Timer1 to 1MHz: 16/1 - 1 = 15
	TIM1_PSCRH = 0;
	TIM1_PSCRL = 15; // LSB should be written last as it updates prescaler
	// capture/compare channel
	// channel CC1 (0->1) stores low pulse length,
	// channel CC2 (1->0) stores time period between two consequent zero-transitions
	TIM1_IER = TIM_IER_CC2IE | TIM_IER_CC1IE | TIM_IER_UIE;// enable interrupt on CC2 & overflow
	// 1. Select the active input capture or trigger input for TIM1_CCR1 by writing the CC1S bits
	// 		to 01 in the TIM1_CCMR1 register (TI1FP1 selected).
	// IC1F = 0 - no filter
	// IC1PSC = 0 - no prescaler
	// TIM1_CCMR1: IC1F[3:0] | IC1PSC[1:0] | CC1S[1:0]
	TIM1_CCMR1 = 1;
	// 3. Select the active input for TIM1_CCR2 by writing the CC2S bits to 10 in the
	//		TIM1_CCMR2 register (TI1FP2 selected).
	TIM1_CCMR2 = 2;
	// 2. Select the active polarity for TI1FP1 (used for both capture and counter clear in
	// 		TIMx_CCR1) by writing the CC1P bit to 0 (TI1FP1 active on rising edge).
	// 4. Select the active polarity for TI1FP2 (used for capture in TIM1_CCR2) by writing the
	//		CC2P bit to 1 (TI1FP2 active on falling edge).
	// TIM1_CCER1: CC2NP | CC2NE | CC2P | CC2E | CC1NP | CC1NE | CC1P | CC1E
	TIM1_CCER1 = 0x20;
	// 5. Select the valid trigger input by writing the TS bits to 101 in the
	//		TIM1_SMCR register (TI1FP1 selected).
	// 6. Configure the clock/trigger controller in reset mode by writing the
	//		SMS bits to 100 in the TIM1_SMCR register
	// TIM1_SMCR: MSM | TS[2:0] | (reserved) | SMS[2:0]
	TIM1_SMCR = 0x54;


	//TIM1_CR1 = 0;



	// Configure ADC
	// select PD2[AIN3] & enable interrupt for EOC
	ADC_CSR = 0x23;
	ADC_TDRL = 0x08; // disable Schmitt triger for AIN3
	// right alignment
	ADC_CR2 = 0x08; // don't forget: first read ADC_DRL!
	// f_{ADC} = f/18 & continuous non-buffered conversion & wake it up
	ADC_CR1 = 0x73;
	ADC_CR1 = 0x73; // turn on ADC (this needs second write operation)

	// Configure pins
	// PC2 - PP output (on-board LED)
	PORT(LED_PORT, DDR) |= LED_PIN;
	PORT(LED_PORT, CR1) |= LED_PIN;
	// PD5 - UART2_TX -- pseudo open-drain output; don't forget an pullup resistor!
	PORT(UART_PORT, DDR) |= UART_TX_PIN;
	PORT(UART_PORT, ODR) &= ~UART_TX_PIN; // turn off N push-down
	//PORT(UART_PORT, CR1) |= UART_TX_PIN;
	// Ultrasonic
	// out, PP
	PORT(US_OUT_PORT, DDR) |= US_OUT_PIN;
	PORT(US_OUT_PORT, CR1) |= US_OUT_PIN;
	//

	// Configure UART
	// 9 bit, no parity, 1 stop (UART_CR3 = 0 - reset value)
	// 57600 on 16MHz: BRR1=0x11, BRR2=0x06
	UART2_BRR1 = 0x11; UART2_BRR2 = 0x06;
	UART2_CR1  = UART_CR1_M; // M = 1 -- 9bits
	UART2_CR2  = UART_CR2_TEN | UART_CR2_REN | UART_CR2_RIEN; // Allow RX, generate ints on rx

	// enable all interrupts
	enableInterrupts();
	// Loop
	do{
		if((Global_time - T > paused_val) || (T > Global_time)){
			T = Global_time;
			PORT(LED_PORT, ODR) ^= LED_PIN; // blink on-board LED
		}
		if(US_flag) US_check(); // end of measurement with ultrasonic?
		if(UART_read_byte(&rb)){ // buffer isn't empty
			switch(rb){
				case 'h': // help
				case 'H':
					uart_write("\nPROTO:\n"
						"+/-\tLED period\n"
						"A\tprint ADC value\n"
						"D\tmeasure distance by US\n"
						);
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
				case 'A':
//					ul = ADC_value * 3300L;
//					u16 = (U16)(ul >> 10); // 0..3300 - U in mv
//					printUint((U8*)&u16,2);
					printUint((U8*)&ADC_value,2);
				break;
				case 'D':
					US_start();
				break;
/*				case 'N':
					if(readInt(&Ival) && Ival > 0 && Ival < 256)
						if(!change_progmem_value(&UART_devNUM, (unsigned int) Ival))
							error_msg("can't change val");
				break;
*/
				default:
				break;
			}
		}
	}while(1);
}


