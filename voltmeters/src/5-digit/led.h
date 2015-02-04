/*
 * led.h
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
#pragma once
#ifndef __LED_H__
#define __LED_H__

#include "stm8l.h"

void set_display_buf(char *str);
void show_buf_digit(U8 N);
void show_next_digit();
void lights_off();
void display_long(long L, char voltmeter);
void display_DP_at_pos(U8 i);

/**
 * Initialize ports
 * PD4, PD5, PA3, PD2, PD3 - anodes
 * PA1,PA2,PB4,PB5,PC3,PC4,PC5,PC6 - cathodes
 * ALL - push-pull
 * PA(1,2,3)   = 0x0e
 * PB(4,5)     = 0x30
 * PC(3,4,5,6) = 0x78
 * PD(2,3,4,5) = 0x3C
 *
*/

#define LED_init()	do{ \
		PA_DDR = 0x0e; PB_DDR = 0x30; PC_DDR = 0x78; PD_DDR = 0x3C; \
		PA_CR1 = 0x0e; PB_CR1 = 0x30; PC_CR1 = 0x78; PD_CR1 = 0x3C; \
	}while(0)

#endif // __LED_H__
