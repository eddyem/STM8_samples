/*
 * si7005.c
 *
 * Copyright 2015 Edward V. Emelianoff <eddy@sao.ru, edward.emelianoff@gmail.com>
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

#include "i2c.h"
#include "uart.h"
#include "si7005.h"

#define DEVID  (0x40)

typedef enum{
	RELAX = 0,
	WAITFORT,
	WAITFORP
}si7005_state;

static si7005_state state = RELAX;

void si7005_setup(){
	i2c_setup();
	i2c_set_addr7(DEVID);
}

/*
 * read device ID
 */
void si7005_read_ID(){
	U8 ID;
	i2c_status st = I2C_OK;
	if(state != RELAX){
		error_msg("measurements are in process");
		return;
	}
	if((st = i2c_7bit_send_onebyte(0x11)) == I2C_OK){
		if((st = i2c_7bit_receive_onebyte(&ID)) == I2C_OK){
			uart_write("got ID: ");
			printUHEX(ID);
			UART_send_byte('\n');
		}
	}
	if(st != I2C_OK){
		uart_write("can't write 0x11, errcode: ");
		printUHEX(st);
		UART_send_byte('\n');
	}
}

/*
 * start themperature reading
 */
void si7005_read_T(){
	const U8 cmd[2] = {0x03, 0x11};
	i2c_status st = I2C_OK;
	if(state != RELAX){
		error_msg("measurements are in process");
		return;
	}
	st = i2c_7bit_send(cmd, 2);
	if(st != I2C_OK){
		error_msg("can't send read sequence ");
		printUHEX(st);UART_send_byte('\n');
		return;
	}
	state = WAITFORT;
}

/*
 * start pressure reading
 */
void si7005_read_P(){
	const U8 cmd[2] = {0x03, 0x01};
	i2c_status st = I2C_OK;
	if(state != RELAX){
		error_msg("measurements are in process");
		return;
	}
	st = i2c_7bit_send(cmd, 2);
	if(st != I2C_OK){
		error_msg("can't send read sequence ");
		printUHEX(st);UART_send_byte('\n');
		return;
	}
	state = WAITFORP;
}

/*
 * display readed value in tenths
 */
static void display_data(U8 *d){
	U16 udata = *((int *)d);
	long idata = 0L;
	switch (state){
		case WAITFORP: // display pressure
			udata >>=4; // get 12 bit data
			idata = (udata*10L)/16L - 240L;
			uart_write("P*10=");
		break;
		case WAITFORT: // display themperature
			udata >>=2; // get 14 bit data
			idata = (udata*100L)/32L - 5000L;
			uart_write("T*100=");
		break;
		default:
			return;
	}
	print_long(idata);
}

/*
 * process state machine
 */
void si7005_process(){
	U8 T[2], b;
	i2c_status st;
	if(state == RELAX) return;
	if(state == WAITFORP || state == WAITFORT){ // poll RDY
		if(i2c_7bit_send_onebyte(0) == I2C_OK){
			if(i2c_7bit_receive_onebyte(&b) == I2C_OK){
				if(b) return; // !RDY
				if((st = i2c_7bit_send_onebyte(1)) == I2C_OK)
					if((st = i2c_7bit_receive_twobyte(T)) == I2C_OK)
						display_data(T);
				state = RELAX;
				if(st){
					uart_write("can't read value, err: ");
					printUHEX(st);
					UART_send_byte('\n');
				}
			}
		}else{
			error_msg("can't poll !RDY");
			state = RELAX;
		}
	}
}
