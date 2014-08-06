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
}
