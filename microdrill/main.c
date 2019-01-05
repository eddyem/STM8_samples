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
#include "statemachine.h"

unsigned long Global_time = 0L; // global time in ms
volatile char exti_event = -1; // flag & counter of EXTI interrupt
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
 *10 1010 a
 *11 1011 b
 *12 1100 c
 *13 1101 d
 *14 1110 e
 *15 1111 f
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

volatile U16 Vpot, Vcap, Vshunt;

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
	unsigned long T = 0L, TT = 0L;
	int Ival;
	U8 rb, drlctr = 0;
    U16 _u16;
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

// Timer1 is PWM generator for drill motor: 1MHz -> 10kHz PWM from 0 to 100
	TIM1_PSCRH = 0; // this timer have 16 bit prescaler
	TIM1_PSCRL = 3; // LSB should be written last as it updates prescaler
	// PWM frequency is 10kHz: 1000/10 = 100
	TIM1_ARRH = 0;
	TIM1_ARRL = 100;
	TIM1_CCR1H = 0; TIM1_CCR1L = DRILL_LOWSPEED; // default: 10%
	// channel 1 generates PWM pulses
	TIM1_CCMR1 = 0x60; // OC1M = 110b - PWM mode 1 ( 1 -> 0)
	//TIM1_CCMR1 = 0x70; // OC1M = 111b - PWM mode 2 ( 0 -> 1)
	TIM1_CCER1 =  1; // Channel 1 is on. Active is high
	//TIM1_CCER1 =  3; // Channel 1 is on. Active is low
	// interrupts: none for timer 1
	TIM1_IER = 0;
	// auto-reload + interrupt on overflow + enable
	TIM1_CR1 = TIM_CR1_URS;
    PC_DDR |= GPIO_PIN1;  // setup timer's output

    // Configure timer 2 to generate signals for CLK
	TIM2_PSCR = 13; // ~2kHz (16MHz / 8192) (1000 steps per second if ARR=1)
	TIM2_IER = TIM_IER_UIE; // update interrupt enable
	TIM2_CR1 |= TIM_CR1_APRE | TIM_CR1_URS; // auto reload + interrupt on overflow

// configure ADC
    // PB5 (AIN5) - Vcap value
	// PB4 (AIN4) - potentiometer regulated motor speed
	// select PF4 - Sence (AIN12) & enable interrupt for EOC
	ADC_CSR = 0x0c; // EOCIE = 0 - no interrupt @ EOC; CH[3:0] = 0x0c (12)
	ADC_TDRH = 0x10;// disable Schmitt triger for AIN12
	ADC_TDRL = 0x30;// disable Schmitt triger for AIN4 & AIN5
	// right alignment
	ADC_CR2 = 0x08; // don't forget: first read ADC_DRL!
	// f_{ADC} = f/18 & continuous non-buffered conversion & wake it up
	ADC_CR1 = 0x71;
	ADC_CR1 = 0x71; // turn on ADC (this needs second write operation)

// Configure pins
	// EXTI
	BTNS_SETUP();
	BTNS_EXTI_ENABLE();  // enable interrupts

	DRILL_OFF();          // power off motor
    // tray
    PORT(TRAY_PORT, DDR) |= TRAY_PINS;
    PORT(TRAY_PORT, CR1) |= TRAY_PINS;

	// LEDS, LED2 (signal):
	PORT(LED_PORT, DDR)  |= LED_PIN;
	PORT(LED_PORT, CR1)  |= LED_PIN;
    // LED0/1
    PORT(LED01_PORT, DDR)  |= LED0_PIN|LED1_PIN;
	PORT(LED01_PORT, CR1)  |= LED0_PIN|LED1_PIN;
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

	set_stepper_speed(95); // 95% of max speed
	setup_stepper_pins();

	// Loop
	do{
		if(Global_time != TT){ // once per 1ms
			TT = Global_time;
			// check EXTI counter
            if(exti_event == ANTICLASH_PAUSE) check_buttons(); // button pressed just now
			if(exti_event > 0){ // delay for 50us before turn on buttons EXTI
				--exti_event;
			}else if(exti_event == 0){
				exti_event = -1;
                BTNS_EXTI_ENABLE();
			}
            // check drill speed & TIM1_CCR1L
            switch(curstate){
                case DRL_ACCEL: // acceleration after power ON
                    if(++drlctr > 9){
                        drlctr = 0;
                        // acceleration for ~1 second
                        if(drill_maxspeed > TIM1_CCR1L) TIM1_CCR1L = TIM1_CCR1L + 1;
                        else{
                            curstate = DRL_WORK;
                        }
                    }
                break;
                case DRL_WORK: // check if user change drill speed
                    if(drill_maxspeed != TIM1_CCR1L){
                        TIM1_CCR1L = drill_maxspeed;
                    }
                break;
                default:
            }
		}
		if(Global_time - T > paused_val){
			T = Global_time;
#ifdef EBUG
			PORT(LED_PORT, ODR) ^= LED_PIN; // blink on-board LED
#endif
            // check changing state for short press
            if(exti_event == -1) check_buttons();
		}
		if(UART_read_byte(&rb)){ // buffer isn't empty
			switch(rb){
				case 'h': // help
				case 'H':
					uart_write("\nPROTO:\n+/-\tLED period\nS/s\tset/get Mspeed\n"
					"m\tget steps\nx\tstop\np\tpause/resume\nM\tmove motor\na\tadd Nstps\n"
					"0\tturn drill OFF\n1\tturn drill ON\n"
					">\trotate faster\n<\trotate slower\n"
					"u\ttray up\nd\ttray down\nw\ttray stop\n"
					"g\tget speed\nAx\tADC chan x\n"
                    "T\tcurrent time\n");
				break;
                case 'T':
                    uart_write("T=");
                    printUint((U8*)&Global_time, 4);
                break;
				case '+':
					paused_val += 100;
					if(paused_val > 10000)
						paused_val = 500; // but not more than 10s
				break;
				case '-':
					paused_val -= 100;
					if(paused_val < 100)  // but not less than 0.1s
						paused_val = 100;
				break;
				case 'S': // set stepper speed
					if(readInt(&Ival) && Ival > -1 && Ival < 101)
						set_stepper_speed(Ival);
					else
						error_msg("bad speed");
				break;
				case 's': // get stepper speed
					printUint(&Stepper_speed, 1);
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
					stp_pause_resume();
				break;
				case 'a': // add N steps
					if(readInt(&Ival) && Ival){
						add_steps(Ival);
					}else{
						error_msg("bad value");
					}
				break;
                case 'A': // ADC: Vpot, Vcap, Vshunt
                    _u16 = 0xffff;
                    if(readInt(&Ival)){
                        switch(Ival){
                        case 0:
                            uart_write("Vpot");
                            _u16 = Vpot;
                        break;
                        case 1:
                            uart_write("Vcap");
                            _u16 = Vcap;
                        break;
                        case 2:
                            uart_write("Vshunt");
                            _u16 = Vshunt;
                        break;
                        default:
                        }

                    }
                    if(_u16 == 0xffff) error_msg("wrong channel");
                    else{
                        UART_send_byte('=');
                        printUint((U8*)&_u16, 2);
                    }
                break;
				case '0': // turn off drill
					DRILL_OFF();
				break;
				case '1': // turn on drill
					DRILL_ON();
				break;
				case '>': // faster
					DRILL_FASTER();
                    printUint(&TIM1_CCR1L, 1);
				break;
				case '<': // slower
					DRILL_SLOWER();
                    printUint(&TIM1_CCR1L, 1);
				break;
				case 'u':
                    DRILL_OFF();
					TRAY_UP();
				break;
				case 'd':
                    DRILL_OFF();
					TRAY_DOWN();
				break;
                case 'w':
                    TRAY_STOP();
                break;
				case 'g':
					_u16 = (TIM1_CCR1H << 8)| TIM1_CCR1L;
					printUint((U8*)&_u16, 2);
				break;
			}
		}
        process_state();
	}while(1);
}


