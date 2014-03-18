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

/*
 ********************* Internal timer (HSI) ********************
 * on startup: HSI = 2MHz (16/8)
 * HSI divisor: CLK_CKDIVR: bits 4,3: f_{HSI}/2^x; bits2..0: f_{CPU}=f/2^x (page 93)
 * CLK_PCKENR1/2 - enable periph clocking (page 94,95) reset value: all enabled
 */

/*
 ********************* Timer1 ********************
 * prescaler: TIM1_PSCRH/L, f = f_{in}/(TIM1_PSCR + 1)
 * other registers:
 * TIM1_CR1 (page 185): | ARPE | CMS[1:0] | DIR | OPM | URS | UDIS | CEN |
 * 		ARPE - Auto-reload preload enable (for TIM1_ARR)
 * 		CMS[1:0]: Center-aligned mode selection (0 - simple counter up/down)
 * 		DIR: Direction (0 - up, 1 - down)
 * 		OPM: One-pulse mode (1 - opm enabled)
 * 		URS: Update request source (When enabled by the UDIS bit, 1 - interrupt only on counter overflow/underflow)
 * 		UDIS: Update disable (1 - disable update int)
 * 		CEN: Counter enable (1 - enable)
 * TIM1_CR2 (page 187): | - | MMS [2:0] | - | COMS | - | CCPS |
 * 		MMS[2:0]: Master mode selection (for ADC or other timers)
 * 		COMS: Capture/compare control update selection
 * 		CCPC: Capture/compare preloaded control
 * TIM1_IER (page 191): | BIE | TIE | COMIE | CC4IE | CC3IE | CC2IE | CC1IE | UIE |
 * 		B - break; T - trigger; COM - commutation; CC - comp/capt; U - update <--
 * TIM1_SR1 (page 192): similar (but instead of IE -> IF)
 * 		interrupt flags
 * TIM1_CNTRH, TIM1_CNTRL - counter value (automatical)
 * TIM1_PSCRH, TIM1_PSCRL - prescaler value
 * TIM1_ARRH, TIM1_ARRL   - auto-reload value (while zero, timer is stopped) (page 206)
 */

/*
 ********************* External interrupts (page 69) ********************
 * EXTI_CR1: | PDIS[1:0] | PCIS[1:0] | PBIS[1:0] | PAIS[1:0] |
 * 		per-port sensivity bits:
 * 			00: Falling edge and low level
 * 			01: Rising edge only
 * 			10: Falling edge only
 * 			11: Rising and falling edge
 * EXTI_CR2: | -reserved[7:3]- | TLIS | PEIS[1:0] |
 * 		TLIS: Top level interrupt sensitivity (0: Falling edge, 1 - Rising)
 * 		PEIS[1:0]: Port E external interrupt sensitivity bits
 * after config run enableInterrupts()
 * ports:
 * 		5 lines on Port A: PA[6:2]
 * 		8 lines on Port B: PB[7:0]
 * 		8 lines on Port C: PC[7:0]
 * 		7 lines on Port D: PD[6:0]
 * 		8 lines on Port E: PE[7:0]
 * 	PD7 is the Top Level Interrupt source (TLI), except for 20-pin packages
 * 		on which the Top Level Interrupt source (TLI) can be available on the
 * 		PC3 pin using an alternate function remapping option bit
 */

/*
 ********************* GPIO (page 111) ********************
 * Px_ODR - Output data register bits
 * Px_IDR - Pin input values
 * Px_DDR - Data direction bits (1 - output)
 * Px_CR1 - DDR=0: 0 - floating, 1 - pull-up input; DDR=1: 0 - pseudo-open-drain, 1 - push-pull output [not for "T"]
 * Px_CR2 - DDR=0: 0/1 - EXTI disabled/enabled; DDR=1: 0/1 - 2/10MHz
 *
 */

/*
 ********************* UART ********************
 * ALGO:
 * 		1. Program the M bit in UART_CR1 to define the word length [M=0, PCEN=0 - 8bit without parity]
 * 		2. Program the number of stop bits in UART_CR3
 * 		3. Select the desired baud rate (UART_BRR1/2) [57600 on 16MHz: BRR1=0x11, BRR2=0x06]
 * 		4. Set the TEN bit in UART_CR2 to enable transmitter mode
 * 		5. Write the data to send in the UART_DR register (this clears the TXE bit)
 * 		6. Once the last data is written to the UART_DR register, wait until TC is set to ‘1’, which indicates that the last data transmission is complete
 * baud rate: regs UART_BRR1/2  !!!VERY STUPID!!!
 * f_{UART} = f_{master} / UART_DIV
 * if UART_DIV = 0xABCD then
 * 		UART_BRR1 = UART_DIV[11:4] = 0xBC;
 * 		UART_BRR2 = UART_DIV[15:12|3:0] = 0xAD
 *           registers
 * UART_SR:  | TXE | TC | RXNE | IDLE | OR/LHE | NF | FE | PE |
 * 		TXE: Transmit data register empty
 * 		TC: Transmission complete
 * 		RXNE: Read data register not empty
 * 		IDLE: IDLE line detected
 * 		OR: Overrun error / LHE: LIN Header Error (LIN slave mode)
 * 		NF: Noise flag
 * 		FE: Framing error
 * 		PE: Parity error
 * UART_DR: data register (when readed returns coming byte, when writed fills output shift register)
 * UART_BRR1 / UART_BRR2 - see upper
 * UART_CR1: | R8 | T8 | UARTD | M | WAKE | PCEN | PS | PIEN |
 * 		R8, T8 - ninth bit (in 9-bit mode)
 * 		UARTD: UART Disable (for low power consumption)
 * 		M: word length (0 - 8bits, 1 - 9bits)
 * 		WAKE: Wakeup method
 * 		PCEN: Parity control enable
 * 		PS: Parity selection (0 - even)
 * 		PIEN: Parity interrupt enable
 * UART_CR2: | TIEN | TCEN | RIEN | ILIEN | TEN | REN | RWU | SBK |
 * 		TIEN: Transmitter interrupt enable
 * 		TCIEN: Transmission complete interrupt enable
 * 		RIEN: Receiver interrupt enable
 * 		ILIEN: IDLE Line interrupt enable
 * 		TEN: Transmitter enable   <----------------------------------------
 * 		REN: Receiver enable      <----------------------------------------
 * 		RWU: Receiver wakeup
 * 		SBK: Send break
 * UART_CR3: | - | LINEN | STOP[1:0] | CLCEN | CPOL | CPHA | LBCL |
 *		LINEN: LIN mode enable
 * 		STOP: STOP bits
 * 		CLKEN: Clock enable (CLC pin)
 * 		CPOL: Clock polarity
 * 		CPHA: Clock phase
 * 		LBCL: Last bit clock pulse
 */

/*
 ********************* ADC (page 413) ********************
 * ADC_DBxRH / ADC_DRH: 9:2 data bits in left-aligned or 9:8 bits in right-aligned mode
 * ADC_DBxRL / ADC_DRL: 1:0 data bits in left-aligned or 7:0 bits in right-aligned mode
 * ADC_CSR: | EOC | AWD | EOCIE | AWDIE | CH[3:0] |
 * 		EOC: End of conversion
 * 		AWD: Analog Watchdog flag
 * 		EOCIE: Interrupt enable for EOC
 * 		AWDIE: Analog watchdog interrupt enable
 * 		CH[3:0]: Channel selection bits (0..15)
 * ADC_CR1: | - | SPSEL[2:0] | - | - | CONT | ADON |
 * 		SPSEL[2:0]: Prescaler selection
 * 		CONT: Continuous conversion (0 for single)
 * 		ADON: A/D Converter on/off   <----------------------------------------
 * ADC_CR2: | - | EXTTRIG | EXTSEL[1:0] | ALIGN | - | SCAN | - |
 * 		EXTTRIG: External trigger enable
 * 		EXTSEL[1:0]: External event selection
 * 		ALIGN: Data alignment (1 - right alignment, first read ADC_DRL)
 * 		SCAN: Scan mode enable
 * ADC_CR3: | DBUF | OVR | reserved[5:0] |
 * 		DBUF: Data buffer enable (on buffered mode data stored not in ADC_DBhl but in ADC_DBxRhl)
 * 		OVR: Overrun flag
 * ADC_TDRH/L - trigger shmidt disable (1 - disable)
 */

unsigned long Global_time = 0L; // global time in ms
U16 paused_val = 500; // interval between LED flashing

U8 UART_rx[UART_BUF_LEN]; // cycle buffer for received data
U8 UART_rx_start_i = 0;   // started index of received data (from which reading starts)
U8 UART_rx_cur_i = 0;     // index of current first byte in rx array (to which data will be written)

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
					uart_write("\nPROTO:\n+/-\tLED period\nS/s\tset/get Mspeed\nm\tget steps\nx\tstop\np\tpause/resume\n0..4\tmove xth motor\na\tadd Nstps\n");
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
					Ival = Nsteps >> Ustepping;
					printUint((U8*)&Ival, 2);
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
				default:
					if(rb >= '0' && rb <= '4'){ // motor
						if(Motor_number != 5){
							error_msg("moving!");
							break;
						}
						Motor_number = rb - '0';
						if(readInt(&Ival) && Ival)
							move_motor(Ival);
						else{
							error_msg("bad Nsteps");
							Motor_number = 5;
						}
					}
			}
		}
	}while(1);
}


