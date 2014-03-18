/*
 * noicegen.h
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

#pragma once
#ifndef __NOICEGEN_H__
#define __NOICEGEN_H__

#include "stm8l.h"

#define TIM_EN  (TIM_CR1_APRE | TIM_CR1_URS | TIM_CR1_CEN)

// soundbank
enum {
	B_SAWTOOTH, B_SINE, B_EXP, B_LOG, B_RAND
};

extern U8 *current_bank;
void configure_timers();
void change_snd_bank(U8 i);

// change period (in us)
#define change_period(F) do{TIM2_ARRH = F >> 8; TIM2_ARRL = F;}while(0)
// change CCR value. U = Vcc *
#define change_CCR(C)  do{TIM1_CCR1H = 0; TIM1_CCR1L = C;}while(0)
#define play_snd()     do{TIM1_CR1 = TIM_EN; TIM2_CR1 = TIM_EN;}while(0)
//#define stop_snd()     do{TIM1_CR1 = 0; TIM2_CR1 = 0; PC_ODR &= ~GPIO_PIN1; }while(0)
#define stop_snd()     do{TIM1_CR1 |= TIM_CR1_OPM; TIM2_CR1 = 0;}while(0)


#endif // __NOICEGEN_H__
