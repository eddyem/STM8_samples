/*
 * stepper.h
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

#pragma once
#ifndef __STEPPER_H__
#define __STEPPER_H__

#include "ports_definition.h"
#include "main.h"

extern volatile int Nsteps[];
extern volatile int usteps[];
extern U16 Stepper_speed[];
extern U8 EPs[];
extern U8 Stop_on_EP[];
extern U8 USteps;
extern U8 StepperInfty;

void setup_stepper_pins();
void set_stepper_speed(U8 N, U16 SpS);
void move_motor(U8 N, int Steps);
void stop_motor(U8 N);
void pause_resume(U8 N);

U8 get_ep_value(U8 N);

void check_EP();

#endif // __STEPPER_H__
