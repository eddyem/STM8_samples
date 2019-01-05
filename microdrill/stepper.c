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
#include "statemachine.h"

volatile long Nsteps = 0;  // Number of steps
volatile char Dir = 0;     // direction of moving: 0/1
U8 Stepper_speed = 95;     // length of one MICROstep in %

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
 * @param Sps - speed in %%
 */
void set_stepper_speed(U8 SpS){
    U16 tmp;
    if(SpS > 100) return;
    Stepper_speed = SpS;
    SpS = 100 - SpS; // reverse (convert period into speed)
    tmp = SpS * (U16)(MAX_STEPPER_PERIOD - MIN_STEPPER_PERIOD);
    tmp /= 100;
    tmp += MIN_STEPPER_PERIOD;
    if(tmp > MAX_STEPPER_PERIOD) tmp = MAX_STEPPER_PERIOD;
    else if(tmp < MIN_STEPPER_PERIOD) tmp = MIN_STEPPER_PERIOD;
	TIM2_ARRH = tmp >> 8; // set speed
	TIM2_ARRL = tmp & 0xff;
}

void move_fast(int Steps){
    stpstate = STPR_FAST;
    TIM2_ARRH = 0;
    TIM2_ARRL = MIN_STEPPER_PERIOD;
    move_motor(Steps);
}

void move_motor(int Steps){
    if(stpstate != STPR_FAST){ // !fast -> check speed & set state
        stpstate = STPR_NORMAL;
        if(TIM2_ARRH == 0 && TIM2_ARRL == MIN_STEPPER_PERIOD && Stepper_speed != 100)
            set_stepper_speed(Stepper_speed); // change speed to previous after max speed moving
    }
	if(Steps < 0){
		Dir = 1;
		Steps *= -1;
	}else
		Dir = 0;
	Nsteps = (long)Steps; // multiply by 3 (to get number of full steps)
	TIM2_CR1 |= TIM_CR1_CEN; // turn on timer
}

void stop_motor(){
    stpstate = STPR_STOPPED;
	TIM2_CR1 &= ~TIM_CR1_CEN; // Turn off timer
	Nsteps = 0;
	PORT(STP_PORT, ODR) &= 0xf0; // turn off power
	uart_write("stop\n");
}

void stp_pause_resume(){
	if(stpstate == STPR_STOPPED) return; // motor is stopped
    DBG("Stepper ");
	if(stpstate != STPR_PAUSED){ // pause
        stpstate = STPR_PAUSED;
		TIM2_CR1 &= ~TIM_CR1_CEN;
		DBG("pause\n");
	}else{ // resume
        stpstate = STPR_NORMAL;
		TIM2_CR1 |= TIM_CR1_CEN;
		DBG("resume\n");
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
		uart_write("reverse\n");
		Dir = !Dir; // invert direction
		Nsteps *= -1L;
	}
	// resume if Nsteps != 0
	if(Nsteps)
		TIM2_CR1 |= TIM_CR1_CEN;
}
