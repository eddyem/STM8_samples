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
#include "stepper.h"

unsigned long Global_time = 0L; // global time in ms
U16 paused_val = 500; // interval between LED flashing

U8 UART_rx[UART_BUF_LEN]; // cycle buffer for received data
U8 UART_rx_start_i = 0;   // started index of received data (from which reading starts)
U8 UART_rx_cur_i = 0;     // index of current first byte in rx array (to which data will be written)

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
 *10 1010
 *11 1011
 *12 1100
 *13 1101
 *14 1110
 *15 1111
 */
// microsteps: DCBA = 1000, 1100, 0100, 0110, 0010, 0011, 0001, 1001 -- for ULN
// what a shit is this > DCBA = 0001, 0010, 0110, 1010, 1001, 1000, 0100, 0000  - bipolar
// 1000, 1010, 0010, 0110, 0100, 0101, 0001, 1001 - half-step
// 1010, 0110, 0101, 1001 - full step
char usteps[8] =
#ifdef MOTOR_TYPE_UNIPOLAR
	{8, 12, 4, 6, 2, 3, 1, 9}; // ULN - unipolar
#elif defined MOTOR_TYPE_BIPOLAR
	{8, 10, 2, 6, 4, 5, 1, 9}; // bipolar
#else
	#error Define MOTOR_TYPE_UNIPOLAR or MOTOR_TYPE_BIPOLAR
#endif

/**
 * Send one byte through UART
 * @param byte - data to send
 */
void UART_send_byte(U8 byte){
	while(!(UART2_SR & UART_SR_TXE)); // wait until previous byte transmitted
	UART2_DR = byte;
}

void uart_write(char *str){
	while(*str){
		while(!(UART2_SR & UART_SR_TXE));
		UART2_CR2 |= UART_CR2_TEN;
		UART2_DR = *str++;
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

int main() {
	unsigned long T = 0L;
	int Ival;
	U8 rb;
	CFG_GCR |= 1; // disable SWIM
	// Configure clocking
	CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz

	// Configure timer 1 - systick
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

	// Configure pins
	// PC2 - PP output (on-board LED)
	PORT(LED_PORT, DDR) |= LED_PIN;
	PORT(LED_PORT, CR1) |= LED_PIN;
	// PD5 - UART2_TX
	PORT(UART_PORT, DDR) |= UART_TX_PIN;
	PORT(UART_PORT, CR1) |= UART_TX_PIN;

	// Configure UART
	// 8 bit, no parity, 1 stop (UART_CR1/3 = 0 - reset value)
	// 57600 on 16MHz: BRR1=0x11, BRR2=0x06
	UART2_BRR1 = 0x11; UART2_BRR2 = 0x06;
	UART2_CR2 = UART_CR2_TEN | UART_CR2_REN | UART_CR2_RIEN; // Allow RX/TX, generate ints on rx

	// enable all interrupts
	enableInterrupts();

	set_stepper_speed(1000);
	setup_stepper_pins();

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
					"m\tget steps\nx\tstop\np\tpause/resume\nM\tmove motor\na\tadd Nstps\n"
					"u\tunipolar motor\nb\tbipolar motor\n");
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
				case 'S': // set stepper speed
					if(readInt(&Ival) && Ival > MIN_STEP_LENGTH)
						set_stepper_speed(Ival);
					else
						error_msg("bad speed");
				break;
				case 's': // get stepper speed
					printUint((U8*)&Stepper_speed, 2);
				break;
				case 'm': // how much steps there is to the end of moving
					printUint((U8*)&Nsteps, 4);
				break;
				case 'M': // move motor
					if(Nsteps){
						error_msg("moving!");
						break;
					}
					if(readInt(&Ival) && Ival)
						move_motor(Ival);
					else{
						error_msg("bad Nsteps");
					}
				break;
				case 'x': // stop
					stop_motor();
				break;
				case 'p': // pause/resume
					pause_resume();
				break;
				case 'a': // add N steps
					if(readInt(&Ival) && Ival){
						add_steps(Ival);
					}else{
						error_msg("bad value");
					}
				break;
			}
		}
	}while(1);
}


