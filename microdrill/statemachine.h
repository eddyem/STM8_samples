/*
 * This file is part of the microdrill project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>, <eddy@sao.ru>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#ifndef __STATEMACHINE_H__
#define __STATEMACHINE_H__

#include "ports_definition.h"

typedef enum{
     DRL_RELAX          // relax: no moving etc
    ,DRL_ACCEL          // acceleration
    ,DRL_WORK           // working
} drill_state;

typedef enum{
     POT_RELAX          // do nothing
    ,POT_DRLSPEED       // potentiometer regulates drill spindle speed
    ,POT_STPSPEED       // -//- stepper speed
} pot_state;

typedef enum{
     STPR_STOPPED       // stepper not moving
    ,STPR_PAUSED        // little pause
    ,STPR_NORMAL        // normal work
    ,STPR_FAST          // fast moving up
} stepper_state;

extern drill_state curstate;
extern stepper_state stpstate;

void process_state();
void check_buttons();

#endif // __STATEMACHINE_H__
