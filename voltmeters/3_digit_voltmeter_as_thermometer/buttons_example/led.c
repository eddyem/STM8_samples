/*
 * led.c
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
#include "led.h"

/*
 * bits no     7   6   5   4   3   2   1   0
 * dec value  128  64  32  16  8   4   2   1
 */

/********** current variant **********/
/*
 * One digit:                          TABLE:
 *   ***A***                   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  -  h
 *   *     *         (F) PB4   0  1  1  1  0  0  0  1  0  0  0  0  0  1  0  0  1  0
 *   F     B         (B) PB5   0  0  0  0  0  1  1  0  0  0  0  1  1  0  1  1  1  1
 *   *     *         (A) PC3   0  1  0  0  1  0  0  0  0  0  0  1  0  1  0  0  1  1
 *   ***G***         (G) PC7   1  1  0  0  0  0  0  1  0  0  0  0  1  0  0  0  0  0
 *   *     *         (C) PD1   0  0  1  0  0  0  0  0  0  0  0  0  1  0  1  1  1  0
 *   E     C         (DP)PC6   1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1
 *   *     *   **    (D) PC5   0  1  0  0  1  0  0  1  0  0  1  0  0  0  0  1  1  1
 *   ***D***  *DP*   (E) PC4   0  1  0  1  1  1  0  1  0  1  0  0  0  0  0  0  1  0
 *             **
 */

/*
 * BUTTONS
 * DON'T FORGET: PB4 & PB5 are "real open-drain"! They can't be pullup inputs, so
 * 		you should solder resistor 10..47k to appropriate buttons!
 * 0 - PB4, 1 - PB5, 2 - PD1, 3..7 - PC3..PC7
 * or:
 * 0 - F(10), 1 - B(7), 2 - C(4), 3 - A(11), 4 - E(1), 5 - D(2), 6 - DP(3), 7 - G(5)
 */
volatile U8 buttons_state;

static U8 display_buffer[3] = {' ',' ',' '}; // blank by default
static U8 N_current = 0; // current digit to display

/*
 * Number of digit on indicator with common anode
 * digis 0..2: PA3, PD4, PD5
 */
#define CLEAR_ANODES() do{PD_ODR &= ~(0x30);PA_ODR &= ~(0x08);}while(0)

/************* arrays for ports *************/
// PB, mask: 0x30 (dec: 48), PB4:0x10=16, PB5:0x20=32
// To light up a segment we should setup it as PPout -> this arrays are inverse!
#define PB_BLANK 0x30
static U8 PB_bits[18] = {48,32,32,32,48,16,16,32,48,48,48,16,16,32,16,16,0,16};
// PC, mask: 0xF8 (dec: 248), PC3:0x08=8, PC4:0x10=16, PC5:0x20=32, PC6:0x40=64, PC7:0x80=128
#define PC_BLANK 0xF8
static U8 PC_bits[18] = {56,0,184,168,128,168,184,8,184,168,152,176,56,176,184,152,128,144};
// PD, mask: 0x02, PD1
static U8 PD_bits[18] = {2,2,0,2,2,2,2,2,2,2,2,2,0,2,0,0,0,2};
#define PD_BLANK 0x02

/**
 * Initialize ports
 * anodes (PA3, PD4, PD5) are push-pull outputs,
 * cathodes (PB4, PB5, PD1, PC3..PC7) are ODouts in active mode, pullup inputs in buttons reading and floating inputs in inactive
 * PA3, PB4|5, PC3|4|5|6|7, PD1|4|5
*/
void LED_init(){
	PA_DDR |= 0x08; PD_DDR |= 0x30; // anodes are PPout, cathodes  will be PPout only in active mode
	PA_CR1 |= 0x08; PD_CR1 |= 0x30;
	// prepare cathodes ODR
	PB_ODR &= ~PB_BLANK; PC_ODR &= ~PC_BLANK; PD_ODR &= ~PD_BLANK;
}

/*
 ********************* GPIO (page 111) ********************
 * Px_ODR - Output data register bits
 * Px_IDR - Pin input values
 * Px_DDR - Data direction bits (1 - output)
 * Px_CR1 - DDR=0: 0 - floating, 1 - pull-up input; DDR=1: 0 - pseudo-open-drain, 1 - push-pull output [not for "T"]
 * Px_CR2 - DDR=0: 0/1 - EXTI disabled/enabled; DDR=1: 0/1 - 2/10MHz
 *
 */

/**
 * Show next digit - function calls from main() by some system time value amount
 */
void show_next_digit(){
	U8 L = display_buffer[N_current] & 0x7f;
	// first turn all off
	CLEAR_ANODES();
	// convert all cathodes to pullup inputs (CR1 is already in ones)
	PB_DDR &= ~PB_BLANK;
	PC_DDR &= ~PC_BLANK;
	PD_DDR &= ~PD_BLANK;
	PB_CR1 |= PB_BLANK;
	PC_CR1 |= PC_BLANK;
	PD_CR1 |= PD_BLANK;
// process buttons attached to A1 and cathodes
	if(N_current == 1){ // first digit was lighten up, now we light second -- check buttons
		// now check button states: 0 - PB4, 1 - PB5, 2 - PD1, 3..7 - PC3..PC7
		buttons_state =
			((PB_IDR & PB_BLANK) >> 4) |
			((PD_IDR & PD_BLANK) << 1) |
			(PC_IDR & PC_BLANK);
	}
	// switch all anodes to floating inputs
	PA_DDR &= ~0x08; PA_CR1 &= ~0x08;
	PD_DDR &= ~0x30; PD_CR1 &= ~0x30;
	// turn off pullups of cathodes (also all outputs now will be OD)
	PB_CR1 &= ~PB_BLANK;
	PC_CR1 &= ~PC_BLANK;
	PD_CR1 &= ~PD_BLANK;
	// now set spare segments switching them to ODoutputs
	if(L < 18){ // letter
		PB_DDR |= PB_bits[L];
		PC_DDR |= PC_bits[L];
		PD_DDR |= PD_bits[L];
	}
	if(L & 0x80){ // DP
		PC_DDR |= GPIO_PIN6;
	}

	switch(N_current){
		case 0:
			PA_DDR |= 0x08; // switch to output
			PA_CR1 |= 0x08; // push-pull
			PA_ODR |= 0x08;
		break;
		case 1:
			PD_DDR |= 0x10;
			PD_CR1 |= 0x10;
			PD_ODR |= 0x10;
		break;
		case 2:
			PD_DDR |= 0x20;
			PD_CR1 |= 0x20;
			PD_ODR |= 0x20;
		break;
	}

	if(++N_current > 2) N_current = 0;
}

/**
 * fills buffer to display
 * @param str - string to display, contains "0..f" for digits, " " for space, "." for DP
 * 				for example: " 1.22" or "h1ab" (something like "0...abc" equivalent to "0.abc"
 * 				register independent!
 * 			any other letter would be omitted
 * 			if NULL - fill buffer with spaces
 */
void set_display_buf(char *str){
	U8 B[3];
	char ch, M = 0, i;
	N_current = 0; // refresh current digit number
	// empty buffer
	for(i = 0; i < 3; i++)
		display_buffer[i] = ' ';
	if(!str) return;
	i = 0;
	for(;(ch = *str) && (i < 3); str++){
		M = 0;
		if(ch > '/' && ch < ':'){ // digit
			M = '0';
		}else if(ch > '`' & ch < 'g'){ // a..f
			M = 'a' - 10;
		}else if(ch > '@' & ch < 'G'){ // A..F
			M = 'A' - 10;
		}else if(ch == '-'){ // minus
			M = '-' - 16;
		}else if(ch == 'h'){ // hex
			M = 'h' - 17;
		}else if(ch == 'H'){ // hex
			M = 'H' - 17;
		}else if(ch == '.'){ // DP, set it to previous char
			if(i == 0){ // word starts from '.' - make a space with point
				B[0] = 0xff;
			}else{ // set point for previous character
				B[i-1] |= 0x80;
			}
			continue;
		}else if(ch != ' '){ // bad character - continue
			continue;
		}
		B[i] = ch - M;
		i++;
	}
	// now make align to right
	ch = 2;
	for(M = i-1; M > -1; M--, ch--){
		display_buffer[ch] = B[M];
	}
}

/**
 * convert integer value i into string and display it
 * @param i - value to display, -99 <= i <= 999, if wrong, displays "---E"
 */
void display_int(int I, char voltmeter){
	int rem;
	U8 pos = 0; //DP position
	char N = 2, sign = 0, i;
	if(I < -99 || I > 999){
		set_display_buf("--E");
		return;
	}
	// prepare buffer for voltmeter's values
	if(voltmeter){
		for(i = 0; i < 3; i++)
			display_buffer[i] = 0;
		if(I>999){
			I /= 10;
			pos = 1; // DP is in 2nd position - voltage more than 9.99V
		}
	}else{
		for(i = 0; i < 3; i++)
			display_buffer[i] = ' ';
	}
	if(I == 0){ // just show zero
		display_buffer[2] = 0;
		return;
	}
	if(I < 0){
		sign = 1;
		I *= -1;
	}
	do{
		rem = I % 10;
		display_buffer[N] = rem;
		I /= 10;
	}while(--N > -1 && I);
	if(sign && N > -1) display_buffer[N] = 16; // minus sign
	if(voltmeter) display_buffer[pos] |= 0x80;
}

/**
 * displays digital point at position i
 * @param i - position to display DP, concequent calls can light up many DPs
 */
void display_DP_at_pos(U8 i){
	if(i > 2) return;
	display_buffer[i] |= 0x80;
}
