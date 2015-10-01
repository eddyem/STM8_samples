/*
 * soft_i2c.c - functions to work with SW I2C
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

#include "soft_i2c.h"
#include "ports_definition.h"
/*
 * In file ports_definition.h should be defined:
 * I2C_SDA_PIN, I2C_SDA_PORT, I2C_SCL_PIN and I2C_SCL_PORT
 */

static U8 addr7r = 0, addr7w = 0;

extern volatile unsigned long Global_time;

#define PAUSE(val)  do{TIM2_ARRH = 0; TIM2_ARRL = val; TIM2_CR1 |= TIM_CR1_CEN; \
		while(!TIM2_SR1 & TIM_SR1_UIF); TIM2_SR1 = 0;}while(0)

#define H_DEL   PAUSE(30)
#define Q_DEL   PAUSE(15)
#define H_SCL	PORT(I2C_SCL_PORT, ODR) |= I2C_SCL_PIN
#define L_SCL	PORT(I2C_SCL_PORT, ODR) &= ~I2C_SCL_PIN
#define H_SDA	PORT(I2C_SDA_PORT, ODR) |= I2C_SDA_PIN
#define L_SDA	PORT(I2C_SDA_PORT, ODR) &= ~I2C_SDA_PIN
#define CHK_SDA (PORT(I2C_SDA_PORT, IDR) & I2C_SDA_PIN)
#define CHK_SCL (PORT(I2C_SCL_PORT, IDR) & I2C_SCL_PIN)

/**
 * configure timer for 100kHz speed in standard mode
 */
void i2c_setup(){
	// configure pins: I2C_SDA; I2C_SCL (both opendrain)
	PORT(I2C_SDA_PORT, ODR) |= I2C_SDA_PIN; // set to 1
	PORT(I2C_SCL_PORT, ODR) |= I2C_SCL_PIN;
	PORT(I2C_SDA_PORT, DDR) |= I2C_SDA_PIN;
	PORT(I2C_SCL_PORT, DDR) |= I2C_SCL_PIN;
	PORT(I2C_SDA_PORT, CR2) |= I2C_SDA_PIN;
	PORT(I2C_SCL_PORT, CR2) |= I2C_SCL_PIN;
	// configure TIM2 for delays
	TIM2_PSCR = 0; // 16MHz  (for working @ 100kHz: 1us \approx 16tacts)
	TIM2_CR1 = TIM_CR1_OPM; // one-pulse mode
}

void i2c_set_addr7(U8 addr){
	addr7w = addr << 1;
	addr7r = addr7w | 1;
}

void SoftStart(){
	H_SCL;
	H_DEL;
	L_SDA;
	H_DEL;
	L_SCL;
	H_DEL;
}

void SoftStop(){
	L_SDA;
	L_SCL;
	H_DEL;
	H_SCL;
	H_DEL;
	H_SDA;
}

/**
 * send 1 byte without start/stop
 */
U8 softi2c_send(U8 data){
	U8 i;
	for(i=0; i < 8; i++){
		L_SCL;
		if(data & 0x80)
			H_SDA;
		else
			L_SDA;
		H_DEL;
		H_SCL;
		H_DEL;
		data <<= 1;
	}
	// ACK
	L_SCL;
	H_SDA;
	H_DEL;
	H_SCL;
	Q_DEL;
	i = !(CHK_SDA);
//	Q_DEL;
	L_SCL;
//	Q_DEL;
	return i;
}
/**
 * receive one byte without start/stop
 */
U8 softi2c_receive(U8 ack){
	U8 data = 0, i;
	for(i=0; i<8; i++){
		data <<= 1;
		L_SCL;
		H_DEL;
		H_SCL;
		if(CHK_SDA) data |= 1;
		H_DEL;
	}
	// prepare for ACK/NACK
	L_SCL;
	if(ack) L_SDA;
	else H_SDA;
	H_DEL;
	H_SCL;
	H_DEL;
	L_SCL;
	H_SDA;
	return data;
}

/**
 * send one byte in 7bit address mode
 * @param data - data to write
 * @param stop - ==1 to send stop event
 * @return I2C_OK if success errcode if fails
 */
i2c_status i2c_7bit_send_onebyte(U8 data, U8 stop){
	i2c_status ret = I2C_LINEBUSY;
	U8 err = 1;
	H_SCL; H_SDA;
	if(!CHK_SDA || !CHK_SCL) goto eotr;
	SoftStart();
	ret = I2C_NACK;
	if(!softi2c_send(addr7w)) goto eotr;
	if(softi2c_send(data)){
		ret = I2C_OK;
		err = 0;
	}
eotr:
	if(stop || err){
		SoftStop();
	}
	return ret;
}

/**
 * send datalen bytes over I2C
 * @param data - data to write
 * @param datalen - amount of bytes in data array
 * @param stop - ==1 to send stop event
 * return I2C_OK if OK
 */
i2c_status i2c_7bit_send(U8 *data, U8 datalen, U8 stop){
	i2c_status ret = I2C_LINEBUSY;
	U8 err = 1;
	H_SCL; H_SDA;
	if(!CHK_SDA || !CHK_SCL) goto eotr;
	SoftStart();
	ret = I2C_NACK;
	if(!softi2c_send(addr7w)) goto eotr;
	while(datalen--){
		if(!softi2c_send(*data++)) goto eotr;
	}
	ret = I2C_OK;
	err = 0;
eotr:
	if(stop || err){
		SoftStop();
	}
	return I2C_OK;
}

/**
 * get one byte by I2C
 * @param data - data to read (one byte)
 * @param wait - leaved for compatibility with HW I2C
 * @return I2C_OK if ok  || error code
 */
i2c_status i2c_7bit_receive_onebyte(U8 *data, U8 wait){
	i2c_status ret = I2C_NACK;
	H_SCL; H_SDA;
	(void) wait;
	if(!CHK_SDA || !CHK_SCL){
		ret = I2C_LINEBUSY;
		goto eotr;
	}
	SoftStart();
	if(!softi2c_send(addr7r)) goto eotr;
	*data = softi2c_receive(0);
	ret = I2C_OK;
eotr:
	SoftStop();
	return ret;
}

/**
 * receive 2 bytes by I2C
 * @param data - data to read (two bytes array, 0 first)
 * @param wait - ==1 to wait while LINEBUSY (can send STOP before reading)
 * @return I2C_OK if ok  || error code
 */
i2c_status i2c_7bit_receive_twobyte(U8 *data, U8 wait){
	i2c_status ret = I2C_NACK;
	H_SCL; H_SDA;
	(void) wait;
	if(!CHK_SDA || !CHK_SCL){
		ret = I2C_LINEBUSY;
		goto eotr;
	}
	SoftStart();
	if(!softi2c_send(addr7r)) goto eotr;
	data[0] = softi2c_receive(1);
	data[1] = softi2c_receive(0);
	ret = I2C_OK;
eotr:
	SoftStop();
	return ret;
}


INTERRUPT_HANDLER(I2C_IRQHandler, 19){
}
