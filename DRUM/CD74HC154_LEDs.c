/*
 * CD74HC154_LEDs.c
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

#include "CD74HC154_LEDs.h"


// bits array: contains numbers of LED's bits to shine (0..6) with zero 4th bit or 0x1x for OFF
U8 LED_bits[LEDS_AMOUNT];
U8 effect_cntr = 0;
char effect_increment = 1; // adds to effect_cntr (1 or -1)

U8 *current_effect = NULL;

U8 LED_effect = 0;

// arrays of LED effects, array should be ended with 0
#define EFFECTS_NUM 4
U8 growing_strip[] = {64, 1, 3, 7, 15, 31, 63, 0}; // *; **; ***; ****; ...
U8 collider[] = {33, 18, 12, 12, 18, 33, 0}; // *....*; .*..*.; ..**..; ...
U8 harvest[] = {32, 16, 8,  4, 2, 1, // *.....; .*....; ..*...; ...*..; ....*.; .....*;
                33, 17, 9,  5, 3,    // *....*; .*...*; ..*..*; ...*.*; ....**;
                35, 19, 11, 7,       // *...**; .*..**; ..*.**; ...***;
                39, 23, 15,          // *..***; .*.***; ..****;
                47, 31,              // *.****; .*****;
                63,               0};// ******
U8 reflections[] = {1,2,4,8,16,32,0}; // <-*->
U8 *EFFECTS[EFFECTS_NUM] = {growing_strip, collider,
		harvest, reflections}; // array of avaialable effects

/**
 * Initialize bit array for LEDs blinking
 * @param mask - bitmap mask for LEDs: 1 - LED is ON, 0 - LED is OFF, mask &= 0x3f (6 LEDs)!
 */
void set_LEDs(U16 mask){
	U8 i;
	for(i = 0; i < LEDS_AMOUNT; i++, mask >>= 1)
		LED_bits[i] = (mask & 1) ? i : 0x10; // to turn all LEDs off we set !E to high level
}

U8 current_LED = 0;

/**
 * Send next binary value to demultiplexer
 * we have cycle of LEDS_AMOUNT values, blink them consecutively (don't forget
 * 		to reduce LEDs resistor according to LEDS_AMOUNT)
 */
void blink_next_LED(){
	PORT(LEDS_PORT, ODR) = LED_bits[current_LED++];
	if(current_LED == LEDS_AMOUNT)
		current_LED = 0;
}

// turn off all LEDs
void reset_LEDs(){
	U8 i;
	for(i = 0; i < LEDS_AMOUNT; i++)
		LED_bits[i] = 0x10;
	LED_effect = 0;
}

/**
 * Increment effect counter & set new LED number
 */
void next_LED_in_effects(){
	if(!current_effect) return;
	set_LEDs(current_effect[effect_cntr]);
	if(effect_cntr == 0 && effect_increment == -1){ // left border - go to the right
		effect_increment = 1;
	}
	effect_cntr += effect_increment;
	if(current_effect[effect_cntr] == 0){ // right border - go to the left
		effect_increment = -1;
		effect_cntr -= 2; // and go to left from previous element
	}
}

void set_effect(U8 n){
	if(n < EFFECTS_NUM){
		current_effect = EFFECTS[n];
		LED_effect = 1;
	}
}
