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
 * Number of digit on indicator with common anode
 * digis 0..2: PA3, PD4, PD5
 */
#define CLEAR_ANODES() do{PD_ODR &= ~(0x30);PA_ODR &= ~(0x08);}while(0)

/************* arrays for ports *************/
// PB, mask: 0x30, PB4:0x10=16, PB5:0x20=32
#define PB_BLANK 0x30
static U8 PB_bits[18] = {0,16,16,16,0,32,32,16,0,0,0,32,32,16,32,32,48,32};
// PC, mask: 0xF8, PC3:0x08=8, PC4:0x10=16, PC5:0x20=32, PC6:0x40=64, PC7:0x80=128
#define PC_BLANK 0xF8
static U8 PC_bits[18] = {128,184,0,16,56,16,0,176,0,16,32,8,128,8,0,32,56,40};
// PD, mask: 0x02, PD1
static U8 PD_bits[18] = {0,0,2,0,0,0,0,0,0,0,0,0,2,0,2,2,2,0};
#define PD_BLANK 0x02

/**
 * Setup for writing a letter
 * @param ltr - letter (0..17 for 0..F, - or h | 0x80 for DP, any other value for 'space')
 */
void write_letter(U8 ltr){
	U8 L = ltr & 0x7f;
	// first turn all off
	CLEAR_ANODES();
	// light up all segments
	PB_ODR &= ~PB_BLANK;
	PC_ODR &= ~PC_BLANK;
	PD_ODR &= ~PD_BLANK;
	// now clear spare segments
	if(L < 18){ // letter
		PB_ODR |= PB_bits[L];
		PC_ODR |= PC_bits[L];
		PD_ODR |= PD_bits[L];
	}else{ // space - turn all OFF
		PB_ODR |= PB_BLANK;
		PC_ODR |= PC_BLANK;
		PD_ODR |= PD_BLANK;
	}
	if(ltr & 0x80){ // DP
		PC_ODR &= ~GPIO_PIN6;
	}
}

/**
 * Turn on anode power for digit N (0..3: PA3, PD6, PD4, PD1 -- A0x08, D0x40, D0x10, D0x02)
 * @param N - number of digit (0..3), if other - no action (display off)
 * @return
 */
void light_up_digit(U8 N){
	switch(N){
		case 0:
			PA_ODR |= 0x08;
		break;
		case 1:
			PD_ODR |= 0x40;
		break;
		case 2:
			PD_ODR |= 0x10;
		break;
		case 3:
			PD_ODR |= 0x02;
		break;
	}
}

static U8 display_buffer[4] = {' ',' ',' ',' '}; // blank by default
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
 * Show Nth digit of buffer (function ran by timer)
 * @param N - number of digit in buffer (0..3)
 */
void show_buf_digit(U8 N){
	if(N > 2) return;
	write_letter(display_buffer[N]);
	light_up_digit(N);
}

/**
 * Show next digit - function calls from main() by some system time value amount
 */
void show_next_digit(){
	show_buf_digit(N_current++);
	if(N_current > 2) N_current = 0;
}

/**
 * Turn off current digit: to change display brightness
 */
void lights_off(){
	U8 N;
	if(N_current) N = N_current - 1;
	else N = 2;
	light_up_digit(N);
}

/**
 * convert integer value i into string and display it
 * @param i - value to display, -999 <= i <= 9999, if wrong, displays "---E"
 */
void display_int(int I, char voltmeter){
	int rem;
	U8 pos = 0; //DP position
	char N = 2, sign = 0, i;
	if(I < -999 || I > 9999){
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
		display_buffer[3] = 0;
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
