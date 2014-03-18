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

U8 Ustepping = 4;          // 2^Ustepping = N of microsteps
volatile long Nsteps = 0;           // Number of steps
U8 Motor_number = 5;       // Number of motor to move, 5 -- not moving
U16 Stepper_speed = 0;     // length of one MICROstep in us

/**
 * Setup pins of stepper motor (all - PP out)
 */
void setup_stepper_pins(){
	// CLK
	PORT(STP_CLK_PORT, DDR) |= STP_CLK_PIN;
	PORT(STP_CLK_PORT, CR1) |= STP_CLK_PIN;
	// EN
	PORT(STP_EN_PORT, DDR)  |= STP_EN_MASK;
	PORT(STP_EN_PORT, CR1)  |= STP_EN_MASK;
	// DIR
	PORT(STP_DIR_PORT, DDR) |= STP_DIR_PIN;
	PORT(STP_DIR_PORT, CR1) |= STP_DIR_PIN;
}

/**
 * Set speed of stepper motor
 * @param Sps - period (in us) of one MICROstep
 */
void set_stepper_speed(U16 SpS){
	Stepper_speed = SpS;
	// Configure timer 2 to generate signals for CLK
	TIM2_PSCR = 4; // 1MHz
	//SpS >>= Ustepping; // divide to microsteps
	TIM2_ARRH = SpS >> 8; // set speed
	TIM2_ARRL = SpS & 0xff;
	SpS >>= 1;// divide to 2 - 50% duty cycle
	TIM2_CCR1H = SpS >> 8;
	TIM2_CCR1L = SpS & 0xff;
	// channel 1 generates PWM pulses
	TIM2_CCMR1 = 0x60; // OC1M = 110b - PWM mode 1 ( 1 -> 0)
	TIM2_CCER1 = 1; // Channel 1 is on. Active is high
	//TIM2_IER = TIM_IER_UIE | TIM_IER_CC1IE; // update interrupt enable
	TIM2_IER = TIM_IER_UIE; // update interrupt enable
	TIM2_CR1 |= TIM_CR1_APRE | TIM_CR1_URS; // auto reload + interrupt on overflow & RUN
}

void move_motor(int Steps){
	if(Motor_number > 4) return;
	if(Steps < 0){
		PORT(STP_DIR_PORT, ODR) &= ~STP_DIR_PIN; // dir to left
		Steps *= -1;
	}
	Nsteps = (long)Steps << Ustepping;
	PORT(STP_EN_PORT, ODR) |= 1 << Motor_number; // enable moving
	TIM2_CR1 |= TIM_CR1_CEN; // turn on timer
}

void stop_motor(){
	TIM2_CR1 &= ~TIM_CR1_CEN; // Turn off timer
	PORT(STP_EN_PORT, ODR)  &= ~STP_EN_MASK; // disable moving
	PORT(STP_DIR_PORT, ODR) |= STP_DIR_PIN; // turn off DIR
	Nsteps = 0;
	Motor_number = 5; // All OK. Motors are stopped
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
	U8 sign = 0;
	// pause
	TIM2_CR1 &= ~TIM_CR1_CEN;
	if(Motor_number == 5){ // motors are stopped - just move last active motor
		move_motor(Steps);
		return;
	}
//	if(PORT(STP_DIR_PORT, IDR) & STP_DIR_PIN) // left direction
//		Nsteps *= -1L;
	if(Steps < 0){
		sign = 1;
		Steps *= -1;
	}
	S = (long)Steps << Ustepping;
	if(sign)
		S *= -1L;
	Nsteps += S;
	// now change direction
	if(Nsteps < 0){
		uart_write("reverce\n");
		PORT(STP_DIR_PORT, ODR) ^= STP_DIR_PIN; // go to the opposite side
		Nsteps *= -1L;
	}
	// resume if Nsteps != 0
	if(Nsteps)
		TIM2_CR1 |= TIM_CR1_CEN;
}
