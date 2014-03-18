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
U8 Motor_number = 5;             // Number of motor to move, 5 -- not moving
U16 Stepper_speed[3] = {1000,1000,1000};  // length of one MICROstep in us
//U8* Timers[3] = {&TIM1_CR1, &TIM2_CR1, &TIM3_CR1};
U8* Timers[3] = {0x5250, 0x5300, 0x5320};

#define pause_motor(N)			*Timers[N] &= ~TIM_CR1_CEN
#define resume_motor(N)			*Timers[N] |= TIM_CR1_CEN
#define check_motor(N)			*Timers[N] & TIM_CR1_CEN
#define PPOUT(P, PIN)			PORT(P, DDR) |= PIN; PORT(P, CR1) |= PIN
#define TMR(a, b)				CONCAT(a , b)
#define TIMER_CONF(reg, val)	TMR(TIM1, reg) = val; TMR(TIM2, reg) = val; TMR(TIM3, reg) = val;

/**
 * Setup pins of stepper motor (all - PP out)
 */
void setup_stepper_pins(){
	// CLK
	PPOUT(STP0_CLK_PORT, STP0_CLK_PIN);
	PPOUT(STP1_CLK_PORT, STP1_CLK_PIN);
	PPOUT(STP2_CLK_PORT, STP2_CLK_PIN);
	// DIR
	PPOUT(STP0_DIR_PORT, STP0_DIR_PIN);
	PPOUT(STP1_DIR_PORT, STP1_DIR_PIN);
	PPOUT(STP2_DIR_PORT, STP2_DIR_PIN);
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
}

/**
 * Set speed of stepper motor
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

/*
void add_steps(U8 N, int Steps){
	long NS;
	U8 sign = 0;
	if(N > 3) return;
	// pause
	pause_motor(N);
	NS = Nsteps[N];
	if(PORT(STP_DIR_PORT, ODR) & STP_DIR_PIN == 0)
		NS *= -1L; // direction to opposite side
	NS += (long)Steps;
	if(NS == 0){ // there's nothing to move
		stop_motor(N);
		return;
	}
	// now change direction
	if(Nsteps < 0){
		uart_write("reverce\n");
		//??PORT(STP_DIR_PORT, ODR) ^= STP_DIR_PIN; // go to the opposite side
		Nsteps *= -1L;
	}
	// resume
	*resume_motor(N);
}
* */

void move_motor(U8 N, int Steps){
	if(N > 2) return;
	pause_motor(N);
	if(Steps < 0){// dir to left
		switch(N){
			case 0:
				PORT(STP0_DIR_PORT, ODR) &= ~STP0_DIR_PIN;
			break;
			case 1:
				PORT(STP1_DIR_PORT, ODR) &= ~STP1_DIR_PIN;
			break;
			case 2:
				PORT(STP2_DIR_PORT, ODR) &= ~STP2_DIR_PIN;
			break;
		}
		Steps *= -1;
	}
	Nsteps[N] = Steps;
	resume_motor(N);
}

void stop_motor(U8 N){
	if(N > 2) return;
	pause_motor(N);
	switch(N){ // turn off DIR
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
	Nsteps[N] = 0;
	uart_write("stop");
	printUint(&N, 1);
}

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


