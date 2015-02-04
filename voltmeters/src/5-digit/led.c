/*
 * led.c - 5-digit LED indicator for "voltmeter"
 *
 * Copyright 2015 Edward V. Emelianoff <eddy@sao.ru>
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

/********** one variant **********/
/*
 * One digit:                          TABLE:
 *   ***A***                   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F - h
 *   *     *         (A) PC6   0  1  0  0  1  0  0  0  0  0  0  1  0  1  0  0 1 1
 *   F     B         (F) PC5   0  1  1  1  0  0  0  1  0  0  0  0  0  1  0  0 1 0
 *   *     *         (B) PB5   0  0  0  0  0  1  1  0  0  0  0  1  1  0  1  1 1 1
 *   ***G***         (G) PA2   1  1  0  0  0  0  0  1  0  0  0  0  1  0  0  0 0 0
 *   *     *         (C) PB4   0  0  1  0  0  0  0  0  0  0  0  0  1  0  1  1 1 0
 *   E     C         (E) PC3   0  1  0  1  1  1  0  1  0  1  0  0  0  0  0  0 1 0
 *   *     *   **    (D) PC4   0  1  0  0  1  0  0  1  0  0  1  0  0  0  0  1 1 1
 *   ***D***  *DP*   (DP)PA1   1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1 1 1
 *             **
 * Use octave's bitshift to calculate constants!
 * e.g.
 * P=bitshift(PC3,3)+bitshift(PC4,4)+bitshift(PC5,5)+bitshift(PC6,6);
 * printf("%d,",P)
 * 0,120,32,40,88,8,0,56,0,8,16,64,0,96,0,16,120,80
 */


/*
 * Number of digit on indicator with common anode
 * digis 0..4 (from left to the right!): PD4, PD5, PA3, PD2, PD3
 */
#define CLEAR_ANODES() do{PD_ODR &= ~(0x3C);PA_ODR &= ~(0x08);}while(0)

/************* arrays for ports *************/
// PA, mask: 0x06, PA1,PA2
static U8 PA_bits[18] = {6,6,2,2,2,2,2,6,2,2,2,2,6,2,2,2,2,2};
#define PA_BLANK 0x06
// PB, mask: 0x30, PB4:0x10=16, PB5:0x20=32
#define PB_BLANK 0x30
static U8 PB_bits[18] = {0,0,16,0,0,32,32,0,0,0,0,32,48,0,48,48,48,32};
// PC, mask: 0x78, PC3:0x08=8, PC4:0x10=16, PC5:0x20=32, PC6:0x40=64
#define PC_BLANK 0x78
static U8 PC_bits[18] = {0,120,32,40,88,8,0,56,0,8,16,64,0,96,0,16,120,80};

/**
 * Setup for writing a letter
 * @param ltr - letter (0..17 for 0..F, - or h | 0x80 for DP, any other value for 'space')
 */
void write_letter(U8 ltr){
	U8 L = ltr & 0x7f;
	// first turn all off
	CLEAR_ANODES();
	// light up all segments
	PA_ODR &= ~PA_BLANK;
	PB_ODR &= ~PB_BLANK;
	PC_ODR &= ~PC_BLANK;
	// now clear spare segments
	if(L < 18){ // letter
		PA_ODR |= PA_bits[L];
		PB_ODR |= PB_bits[L];
		PC_ODR |= PC_bits[L];
	}else{ // space - turn all OFF
		PA_ODR |= PA_BLANK;
		PB_ODR |= PB_BLANK;
		PC_ODR |= PC_BLANK;
	}
	if(ltr & 0x80){ // DP
		PA_ODR &= ~GPIO_PIN1;
	}
}

/**
 * Turn on anode power for digit N (0..4:  PD4, PD5, PA3, PD2, PD3 -- D0x10, D0x20, A0x08, D0x04, D0x08)
 * @param N - number of digit (0..3), if other - no action (display off)
 * @return
 */
void light_up_digit(U8 N){
	switch(N){
		case 0:
			PD_ODR |= 0x10;
		break;
		case 1:
			PD_ODR |= 0x20;
		break;
		case 2:
			PA_ODR |= 0x08;
		break;
		case 3:
			PD_ODR |= 0x04;
		break;
		case 4:
			PD_ODR |= 0x08;
		break;
	}
}

static U8 display_buffer[5] = {' ',' ',' ',' ',' '}; // blank by default
static U8 N_current = 0; // current digit to display

/**
 * fills buffer to display
 * @param str - string to display, contains "0..f" for digits, " " for space, "." for DP
 * 				for example: " 1.22" or "h1ab" (something like "0...abc" equivalent to "0.abc"
 * 				register independent!
 * 			any other letter would be omitted
 * 			if NULL - fill buffer with spaces
 */
void set_display_buf(char *str){
	U8 B[5];
	char ch, M = 0, i;
	N_current = 0; // refresh current digit number
	// empty buffer
	for(i = 0; i < 5; i++)
		display_buffer[i] = ' ';
	if(!str) return;
	i = 0;
	for(;(ch = *str) && (i < 5); str++){
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
	ch = 4;
	for(M = i-1; M > -1; M--, ch--){
		display_buffer[ch] = B[M];
	}
}

/**
 * Show Nth digit of buffer (function ran by timer)
 * @param N - number of digit in buffer (0..3)
 */
void show_buf_digit(U8 N){
	if(N > 4) return;
	write_letter(display_buffer[N]);
	light_up_digit(N);
}

/**
 * Show next digit - function calls from main() by some system time value amount
 */
void show_next_digit(){
	show_buf_digit(N_current++);
	if(N_current > 4) N_current = 0;
}

/**
 * Turn off current digit: to change display brightness
 */
void lights_off(){
	U8 N;
	if(N_current) N = N_current - 1;
	else N = 4;
	light_up_digit(N);
}

/**
 * convert long integer value i into string and display it
 * @param i - value to display, -9999 <= i <= 99999, if wrong, displays "----E"
 * @param voltmeter == 0 to simply show long value; == 1 to show pseudo-float with DP in fourth position
 */
void display_long(long L, char voltmeter){
	long rem;
	char N = 4, sign = 0, i, *bufini = NULL;
	if(L < -9999 || L > 99999){
		set_display_buf("----E");
		return;
	}
	if(voltmeter) bufini = " 0000"; // prepare buffer for voltmeter's values
	set_display_buf(bufini);
	if(L == 0){ // just show zero
		display_buffer[4] = 0;
		return;
	}
	if(L < 0){
		sign = 1;
		L *= -1;
	}
	do{
		rem = L % 10;
		display_buffer[N] = rem;
		L /= 10;
	}while(--N > -1 && L);
	if(sign && N > -1) display_buffer[N] = 16; // minus sign
	if(voltmeter) display_buffer[1] |= 0x80; // digital point
}



/**
 * displays digital point at position i
 * @param i - position to display DP, concequent calls can light up many DPs
 */
void display_DP_at_pos(U8 i){
	if(i > 4) return;
	display_buffer[i] |= 0x80;
}
