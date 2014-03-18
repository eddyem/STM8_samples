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

/********** one variant **********/
/*
 * One digit:                          TABLE:
 *   ***A***                   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F - h
 *   *     *         (A) PB4   0  1  0  0  1  0  0  0  0  0  0  1  0  1  0  0 1 1
 *   F     B         (F) PB5   0  1  1  1  0  0  0  1  0  0  0  0  0  1  0  0 1 0
 *   *     *         (B) PC5   0  0  0  0  0  1  1  0  0  0  0  1  1  0  1  1 1 1
 *   ***G***         (G) PC6   1  1  0  0  0  0  0  1  0  0  0  0  1  0  0  0 0 0
 *   *     *         (C) PC7   0  0  1  0  0  0  0  0  0  0  0  0  1  0  1  1 1 0
 *   E     C         (E) PD1   0  1  0  1  1  1  0  1  0  1  0  0  0  0  0  0 1 0
 *   *     *   **    (D) PD2   0  1  0  0  1  0  0  1  0  0  1  0  0  0  0  1 1 1
 *   ***D***  *DP*   (DP)PD3   1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1 1 1
 *             **
 */
/*
// PB, mask: 0x30, PB4: 0x10, PB5: 0x20
#define PB_BLANK 0x30
static U8 PB_bits[18] = {0,0x30,0x20,0x20,0x10,0,0,0x20,0,0,0,0x10,0,0x30,0,0,0x30,0x10};
// PC, mask: 0xe0, PC5: 0x20, PC6: 0x40, PC7: 0x80
#defin PC_BLANK 0xe0
static U8 PC_bits[18] = {0x40,0x40,0x80,0,0,0x20,0x20,0x40,0,0,0,0x20,0xe0,0,0xa0,0xa0,0xa0,0x20};
// PD, mask: 0x0e, PD1: 0x02, PD2: 0x04, PD3: 0x08
#define PD_BLANK 0x0e
static U8 PD_bits[18] = {0x08,0x0e,0x08,0x0a,0x0e,0x0a,0x08,0x0e,0x08,0x0a,0x0c,0x8,0x08,0x08,0x08,0x0c,0x0e,0x0c};
*/
/*
 * Number of digit on indicator with common anode
 * digis 0..3: PC3, PC4, PA3, PD4
 */


/********** current variant **********/
/*
 * One digit:                          TABLE:
 *   ***A***                   0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F  -  h
 *   *     *         (F) PA1   0  1  1  1  0  0  0  1  0  0  0  0  0  1  0  0  1  0
 *   F     B         (B) PB4   0  0  0  0  0  1  1  0  0  0  0  1  1  0  1  1  1  1
 *   *     *         (A) PB5   0  1  0  0  1  0  0  0  0  0  0  1  0  1  0  0  1  1
 *   ***G***         (G) PC3   1  1  0  0  0  0  0  1  0  0  0  0  1  0  0  0  0  0
 *   *     *         (C) PC4   0  0  1  0  0  0  0  0  0  0  0  0  1  0  1  1  1  0
 *   E     C         (DP)PC5   1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1  1
 *   *     *   **    (D) PC6   0  1  0  0  1  0  0  1  0  0  1  0  0  0  0  1  1  1
 *   ***D***  *DP*   (E) PC7   0  1  0  1  1  1  0  1  0  1  0  0  0  0  0  0  1  0
 *             **
 */

/*
 * Number of digit on indicator with common anode
 * digis 0..3: PA3, PD6, PD4, PD1
 */

/************* arrays for ports *************/
// PA, mask: 0x02, PA1
static U8 PA_bits[18] = {0,2,2,2,0,0,0,2,0,0,0,0,0,2,0,0,2,0};
#define PA_BLANK 0x02
// PB, mask: 0x30, PB4:0x10=16, PB5:0x20=32
#define PB_BLANK 0x30
static U8 PB_bits[18] = {0,32,0,0,32,16,16,0,0,0,0,48,16,32,16,16,48,48};
// PC, mask: 0xF8, PC3:0x08=8, PC4:0x10=16, PC5:0x20=32, PC6:0x40=64, PC7:0x80=128
#define PC_BLANK 0xF8
static U8 PC_bits[18] = {40,232,48,160,224,160,32,232,32,160,96,32,56,32,48,112,240,96};

/**
 * Setup for writing a letter
 * @param ltr - letter (0..17 for 0..F, - or h | 0x80 for DP, any other value for 'space')
 */
void write_letter(U8 ltr){
	U8 L = ltr & 0x7f;
	PD_ODR = 0; // turn off digits 1..3
	if(L < 18){ // letter
		PA_ODR = PA_bits[L];
		PB_ODR = PB_bits[L];
		PC_ODR = PC_bits[L];
	}else{ // space
		PA_ODR = PA_BLANK;
		PB_ODR = PB_BLANK;
		PC_ODR = PC_BLANK;
	}
	if(ltr & 0x80){ // DP
		PC_ODR ^= 0x20;
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
	U8 B[4];
	char ch, M = 0, i;
	N_current = 0; // refresh current digit number
	// empty buffer
	for(i = 0; i < 4; i++)
		display_buffer[i] = ' ';
	if(!str) return;
	i = 0;
	for(;(ch = *str) && (i < 4); str++){
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
	ch = 3;
	for(M = i-1; M > -1; M--, ch--){
		display_buffer[ch] = B[M];
	}
}

/**
 * Show Nth digit of buffer (function ran by timer)
 * @param N - number of digit in buffer (0..3)
 */
void show_buf_digit(U8 N){
	if(N > 3) return;
	write_letter(display_buffer[N]);
	light_up_digit(N);
}

/**
 * Show next digit - function calls from main() by some system time value amount
 */
void show_next_digit(){
	show_buf_digit(N_current++);
	if(N_current > 3) N_current = 0;
}

/**
 * convert integer value i into string and display it
 * @param i - value to display, -999 <= i <= 9999, if wrong, displays "---E"
 */
void display_int(int I){
	int rem;
	char N = 3, sign = 0;
	if(I < -999 || I > 9999){
		set_display_buf("---E");
		return;
	}
	set_display_buf(NULL); // empty buffer
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
}


/**
 * displays digital point at position i
 * @param i - position to display DP, concequent calls can light up many DPs
 */
void display_DP_at_pos(U8 i){
	if(i > 3) return;
	display_buffer[i] |= 0x80;
}
