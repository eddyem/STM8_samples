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

volatile int Nsteps[3]={0,0,0}; // Number of steps
volatile int usteps[3] = {0,0,0};// microstepping counters
U8 Motor_number = 5;            // Number of motor to move, 5 -- not moving
U16 Stepper_speed[3] = {1000,1000,1000};  // length of one MICROstep in us
U8* Timers[3] = {0x5250, 0x5300, 0x5320}; // {&TIM1_CR1, &TIM2_CR1, &TIM3_CR1}
U8 EPs[3] = {0,0,0};            // value of conditional stop-on-EP terminals status
U8 Stop_on_EP[3] = {0,0,0};     // boolean: whether motor should freely move or stop on EPs
U8 USteps = 16;                 // amount of microsteps on each step
U8 StepperInfty = 0;            // infinity moving

#define pause_motor(N)			*Timers[N] &= ~TIM_CR1_CEN
#define resume_motor(N)			*Timers[N] |= TIM_CR1_CEN
#define check_motor(N)			*Timers[N] & TIM_CR1_CEN
#define PPOUT(P, PIN)			do{PORT(P, DDR) |= PIN; PORT(P, CR1) |= PIN;}while(0)
#define _MTR(X, N)				PPOUT(STP ## N ## _ ## X ## _PORT, STP ## N ## _ ## X ## _PIN)
#define SETUP_MOTOR_PORT(X)		do{_MTR(X,0); _MTR(X,1); _MTR(X,2);}while(0)
#define TMR(a, b)				CONCAT(a , b)
#define TIMER_CONF(reg, val)	do{TMR(TIM1, reg) = val; TMR(TIM2, reg) = val; TMR(TIM3, reg) = val;}while(0)

/**
 * Setup pins of stepper motor (all - PP out) & timers
 */
void setup_stepper_pins(){
	// CLK
	SETUP_MOTOR_PORT(CLK);
	// DIR
	SETUP_MOTOR_PORT(DIR);
	// EN
	SETUP_MOTOR_PORT(EN);
	// End point switches:
	SETUP_EP(0);
	SETUP_EP(1);
	SETUP_EP(2);
	/**** TIMERS (all - 1MHz, default speed - 1000 Hz) ****/
	// Motor x - timer x+1
	TIM1_PSCRH = 0; // this timer have 16 bit prescaler
	TIM1_PSCRL = 15; // LSB should be written last as it updates prescaler
	TIM2_PSCR = 4;
	TIM3_PSCR = 4;
	// auto-reload each 1ms: TIM_ARR = 1000 = 0x03E8
	TIMER_CONF(ARRH, 0x03);
	TIMER_CONF(ARRL, 0xE8);
	// 50% duty cycle: TIM_CCR = 500 = 0x01F4
	TIMER_CONF(CCR1H, 0x01);
	TIMER_CONF(CCR1L, 0xF4);
	// channel 1 generates PWM pulses
	TIMER_CONF(CCMR1, 0x60); // OC1M = 110b - PWM mode 1 ( 1 -> 0)
	TIMER_CONF(CCER1, 1); // Channel 1 is on. Active is high
	// interrupts: update
	TIMER_CONF(IER, TIM_IER_UIE);
	// auto-reload + interrupt on overflow
	TIMER_CONF(CR1, TIM_CR1_APRE | TIM_CR1_URS);
	// enable PWM output for timer1
	TIM1_BKR |= 0x80; // MOE
	// Turn off EN
	PORT(STP0_EN_PORT, ODR) |= STP0_EN_PIN;
	PORT(STP1_EN_PORT, ODR) |= STP1_EN_PIN;
	PORT(STP2_EN_PORT, ODR) |= STP2_EN_PIN;
}

/**
 * Set speed of stepper motor
 * @param N - number of motor
 * @param Sps - period (in us) of one MICROstep
 */
void set_stepper_speed(U8 N, U16 SpS){
	U8 AH, AL, CH, CL;
	if(N > 2) return;
	Stepper_speed[N] = SpS;
	AH = SpS >> 8;
	AL = SpS & 0xff;
	SpS >>= 1; // divide to 2 - 50% duty cycle
	CH = SpS >> 8;
	CL = SpS & 0xff;
	switch(N){
		case 0:
			TIM1_ARRH  = AH;
			TIM1_ARRL  = AL;
			TIM1_CCR1H = CH;
			TIM1_CCR1L = CL;
		break;
		case 1:
			TIM2_ARRH  = AH;
			TIM2_ARRL  = AL;
			TIM2_CCR1H = CH;
			TIM2_CCR1L = CL;
		break;
		case 2:
			TIM3_ARRH  = AH;
			TIM3_ARRL  = AL;
			TIM3_CCR1H = CH;
			TIM3_CCR1L = CL;
		break;
	}
}

/**
 * Move motor N for 'Steps' amount of steps
 * @param N - number of motor
 * @param Steps - number of steps to move (negative value means to move CCV)
 */
void move_motor(U8 N, int Steps){
	if(N > 2) return;
	pause_motor(N);
	if(Steps < 0){// dir to left
		switch(N){
			case 0:
				PORT(STP0_DIR_PORT, ODR) |= STP0_DIR_PIN;
			break;
			case 1:
				PORT(STP1_DIR_PORT, ODR) |= STP1_DIR_PIN;
			break;
			case 2:
				PORT(STP2_DIR_PORT, ODR) |= STP2_DIR_PIN;
			break;
		}
		Steps *= -1;
	}
	// turn on EN
	switch(N){
		case 0:
			PORT(STP0_EN_PORT, ODR) &= ~STP0_EN_PIN;
		break;
		case 1:
			PORT(STP1_EN_PORT, ODR) &= ~STP1_EN_PIN;
		break;
		case 2:
			PORT(STP2_EN_PORT, ODR) &= ~STP2_EN_PIN;
		break;
	}
	Nsteps[N] = Steps;
	resume_motor(N);
	uart_write("move");
	printUint(&N, 1);
}

/**
 * Stop motor N
 * @param N - number of motor
 */
void stop_motor(U8 N){
	if(N > 2) return;
	pause_motor(N);
	switch(N){ // turn off DIR & EN
		case 0:
			PORT(STP0_DIR_PORT, ODR) &= ~STP0_DIR_PIN;
			PORT(STP0_EN_PORT,  ODR) |=  STP0_EN_PIN;
		break;
		case 1:
			PORT(STP1_DIR_PORT, ODR) &= ~STP1_DIR_PIN;
			PORT(STP1_EN_PORT,  ODR) |=  STP1_EN_PIN;
		break;
		case 2:
			PORT(STP2_DIR_PORT, ODR) &= ~STP2_DIR_PIN;
			PORT(STP2_EN_PORT,  ODR) |=  STP2_EN_PIN;
		break;
	}
	StepperInfty = 0; // clear infinity flag
	Nsteps[N] = 0;
	usteps[N] = 0;
	uart_write("stop");
	printUint(&N, 1);
}

/**
 * Pause or resume motor N
 * @param N - number of motor
 */
void pause_resume(U8 N){
	if(N > 2) return;
	if(Nsteps[N] == 0) return; // motor is stopped
	if(check_motor(N)){ // motor is running - pause
		pause_motor(N);
		uart_write("pause");
	}else{ // resume
		resume_motor(N);
		uart_write("resume");
	}
	printUint(&N, 1);
}

/**
 * Get current value of EP switches
 * @param N - number of motor
 * @return value of EPs
 */
U8 get_ep_value(U8 N){
	U8 val = 0;
	switch(N){
		case 0:
			val = READ_EP(0);
		break;
		case 1:
			val = READ_EP(1);
		break;
		case 2:
			val = READ_EP(2);
		break;
	}
	return val;
}

/**
 * Checks for "stop-on-EP" condition
 * @param
 * @return
 */
void check_EP(){
	U8 i;
	for(i = 0; i < 3; i++){
		if(Stop_on_EP[i] == 0) continue;
		if(EPs[i] == get_ep_value(i)){
			stop_motor(i);
			uart_write("endpoint\n");
		//	Stop_on_EP[i] = 0; // reset off-condition
		}
	}
}
