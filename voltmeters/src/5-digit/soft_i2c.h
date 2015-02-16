/*
 * soft_i2c.h
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
#ifndef __SOFT_I2C_H__
#define __SOFT_I2C_H__

#include "stm8l.h"

// SDA - PC7, SCL - PD1
#define I2C_SDA_PORT	PC
#define I2C_SDA_PIN		GPIO_PIN7
#define I2C_SCL_PORT	PD
#define I2C_SCL_PIN		GPIO_PIN1

typedef enum{
	 SOFT_I2C_OK            // all OK
	,SOFT_I2C_BUSY          // previous transmission is active
	,SOFT_I2C_NO_DEVICE     // no device found
	,SOFT_I2C_DATA_SEND_OK  // data transmitted without errors
	,SOFT_I2C_DATA_READ_OK  // data readed without errors
} soft_I2C_errors;

extern soft_I2C_errors soft_I2C_state;
extern U32 readed_data;
extern U8 tick_counter;

void soft_I2C_setup();
void soft_I2C_set_speed(U16 Pulse_len);
void process_soft_I2C();
U8 soft_I2C_write_config(U8 dev, U8 conf);
U8 soft_I2C_read4bytes(U8 dev);

#endif // __SOFT_I2C_H__
