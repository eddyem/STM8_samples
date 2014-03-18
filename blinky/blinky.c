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


#include "stm8l.h"
#include "interrupts.h"
#include "blinky.h"
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
char onboard_blink = 1; // == 1 to blink on-board LED
int ADC_value = 500; // value of last ADC measurement

int main() {
	unsigned long T = 0L;
	unsigned char LedCntr = 0;
	// Configure clocking
	CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz
	// Configure pins
	PD_DDR = GPIO_PIN3; // PD3 - output, other are inputs
	PD_CR1 = GPIO_PIN3|GPIO_PIN5; // PD3 is PPout, PD5 is input with pull-up, PD2 is floating input (for ADC)
	PD_CR2 = GPIO_PIN5; // enable interrupts for PD5
	PD_ODR = GPIO_PIN3; // turn on LED on PD3
	// pin interrupts for PD2&PD5
	EXTI_CR1 = 0x80; // PDIS = 10 - falling edge
	// 5 LEDs on PC3..PC7
	PC_DDR = 0xf8;
	PC_CR1 = 0xf8; // PPout
	// Configure Timer1
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
	// configure ADC
	// select PD2[AIN3] & enable interrupt for EOC
	ADC_CSR = 0x23;
	ADC_TDRL = 0x08; // disable Schmitt triger for AIN3
	// right alignment
	ADC_CR2 = 0x08; // don't forget: first read ADC_DRL!
	// f_{ADC} = f/18 & continuous non-buffered conversion & wake it up
	ADC_CR1 = 0x73;
	ADC_CR1 = 0x73; // turn on ADC (this needs second write operation)
	// enable all interrupts
	enableInterrupts();
	// Loop
	do {
		// ADC_value sets half-period in ms
		if((Global_time - T > (long)ADC_value) || (T > Global_time)){
			T = Global_time;
			if(onboard_blink) PD_ODR ^= GPIO_PIN3; // blink on-board LED
			PC_ODR = (LedCntr++) << 3;
			if(LedCntr == 0x20) LedCntr = 0;
		}
		//if(!(PD_IDR & 0x20) && T < 650000L) T+= 5000L;
		//else if(!(PD_IDR & 0x04) & T > 5000L) T -= 5000L; // PD2
	} while(1);
}

