/*
 * stepper.c
 *
 * Copyright 2014 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#include "stepper.h"

volatile long Nsteps = 0;  // Number of steps
volatile char Dir = 0;     // direction of moving: 0/1
U16 Stepper_speed = 0;     // length of one MICROstep in us

/**
 * Setup pins of stepper motor (all - PP out)
 */
void setup_stepper_pins(){
	// PB0..3
	PORT(STP_PORT, DDR) |= 0x0f;
	PORT(STP_PORT, CR1) |= 0x0f;
}

/**
 * Set speed of stepper motor
 * @param Sps - period (in us) of one MICROstep
 */
void set_stepper_speed(U16 SpS){
	Stepper_speed = SpS;
	// Configure timer 2 to generate signals for CLK
	TIM2_PSCR = 4; // 1MHz
	TIM2_ARRH = SpS >> 8; // set speed
	TIM2_ARRL = SpS & 0xff;
	TIM2_IER = TIM_IER_UIE; // update interrupt enable
	TIM2_CR1 |= TIM_CR1_APRE | TIM_CR1_URS; // auto reload + interrupt on overflow & RUN
}

void move_motor(int Steps){
	if(Steps < 0){
		Dir = 1;
		Steps *= -1;
	}else
		Dir = 0;
	Nsteps = (long)Steps; // multiply by 3 (to get number of full steps)
	TIM2_CR1 |= TIM_CR1_CEN; // turn on timer
}

void stop_motor(){
	TIM2_CR1 &= ~TIM_CR1_CEN; // Turn off timer
	Nsteps = 0;
	PORT(STP_PORT, ODR) &= 0xf0; // turn off power
	uart_write("stop\n");
}

void pause_resume(){
	if(Nsteps == 0) return; // motor is stopped
	if(TIM2_CR1 & TIM_CR1_CEN){ // pause
		TIM2_CR1 &= ~TIM_CR1_CEN;
		uart_write("pause\n");
	}else{ // resume
		TIM2_CR1 |= TIM_CR1_CEN;
		uart_write("resume\n");
	}
}

void add_steps(int Steps){
	long S;
	// pause
	TIM2_CR1 &= ~TIM_CR1_CEN;
	if(Nsteps == 0){ // motor is stopped - just move it
		move_motor(Steps);
		return;
	}
	S = (long)Steps;
	Nsteps += S;
	// now change direction
	if(Nsteps < 0){
		uart_write("reverce\n");
		Dir = !Dir; // invert direction
		Nsteps *= -1L;
	}
	// resume if Nsteps != 0
	if(Nsteps)
		TIM2_CR1 |= TIM_CR1_CEN;
}
