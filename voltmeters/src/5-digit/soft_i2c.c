/*
 * soft_i2c.c - functions emulating I2C
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

#include "soft_i2c.h"
#include "main.h"

// data to transmit: MSB is devaddr, LSB is configuration byte
U16 transmit_cmd;
// == 0 when transmission is in progress
U8 transmission_over = 1;
// tick counter: two ticks for one bit writting and two ticks for empty
U8 tick_counter;
soft_I2C_errors soft_I2C_state = SOFT_I2C_BUSY;

U16 tr_mask = 0; // mask that would be &`ded with transmit_cmd
U32 readed_data; // data readed by I2C
U32 rd_counter = 0; // counter of readed bits == 64 in start of reading; decrements in each readed bit

/*
 ********************* GPIO (page 111) ********************
 * Px_ODR - Output data register bits
 * Px_IDR - Pin input values
 * Px_DDR - Data direction bits (1 - output)
 * Px_CR1 - DDR=0: 0 - floating, 1 - pull-up input; DDR=1: 0 - pseudo-open-drain, 1 - push-pull output [not for "T"]
 * Px_CR2 - DDR=0: 0/1 - EXTI disabled/enabled; DDR=1: 0/1 - 2/10MHz
 *
 */
/**
 * configure pins: SDA - open-drain output
 * SCL - push/pull
 * configure timer2 as CLK (default speed is 100kHz)
 */
void soft_I2C_setup(){
	PORT(I2C_SCL_PORT, DDR) |= I2C_SCL_PIN;
	PORT(I2C_SDA_PORT, DDR) |= I2C_SDA_PIN;
	PORT(I2C_SDA_PORT, CR1) |= I2C_SDA_PIN;
	// Configure timer 2 to generate signals for CLK
	TIM2_PSCR = 0; // 16MHz
	TIM2_IER = TIM_IER_UIE; // update interrupt enable
	TIM2_CR1 |= TIM_CR1_APRE | TIM_CR1_URS; // auto reload + interrupt on overflow
	TIM2_ARRH = 0; // set 2.5ms period (100kHz)
	TIM2_ARRL = 10;
}

/**
 * Set speed of soft I2C
 * @param Pulse_len - period (in us) of one pulse
 */
void soft_I2C_set_speed(U16 Pulse_len){
	Pulse_len << 2; // multiply on 4 as we have four pulses for each bit, each pulse is 62.5ns
	TIM2_ARRH = Pulse_len >> 8; // set speed
	TIM2_ARRL = Pulse_len & 0xff;
}

/*
 * The data transmission runs only when SCL==1 & SDA have constant level,
 * otherwice, if SCL==1 and SDA changes, this is: START(1->0) or STOP(0->1)
 */
void i2c_transmit(){
	soft_I2C_state = SOFT_I2C_BUSY;
	transmission_over = 0;
	tick_counter = 0;
	rd_counter = 0;
	readed_data = 0;
	TIM2_CR1 |= TIM_CR1_CEN; // turn on timer
}

/**
 * write a byte to config register of device dev
 * @param dev  - device address
 * @param conf - configuration byte data
 */
U8 soft_I2C_write_config(U8 dev, U8 conf){
	if(!transmission_over || (TIM2_CR1&TIM_CR1_CEN)) return SOFT_I2C_BUSY;
	dev &= 0xfe; // make sure that r/!w clear
	transmit_cmd = dev<<8 || conf;
	i2c_transmit();
	return SOFT_I2C_OK;
}

/**
 * prepare to read 4 bytes of data
 * @param dev - device address
 */
U8 soft_I2C_read4bytes(U8 dev){
	if(!transmission_over || (TIM2_CR1&TIM_CR1_CEN)) return SOFT_I2C_BUSY;
	dev |= 1; // make sure that r/!w is set
	transmit_cmd = dev<<8;
	i2c_transmit();
	return SOFT_I2C_OK;
}

void process_soft_I2C(){
	U8 nbit, bitinbyte, tick; // bit number
	if(transmission_over){
		TIM2_CR1 &= ~TIM_CR1_CEN; // turn off timer
		// return line to NOT BUSY state
		PORT(I2C_SCL_PORT, ODR) |= I2C_SCL_PIN;
		PORT(I2C_SDA_PORT, ODR) |= I2C_SDA_PIN;
		return;
	}
	nbit = tick_counter >> 2; // divide by 4
	if(nbit == 0){ // start
		switch (tick_counter){
			case 1: // 2nd half of start pulse
				PORT(I2C_SDA_PORT, ODR) &= ~I2C_SDA_PIN; // clear SDA - Start bit
			break;
			case 3: // turn to zero both SCL & SDA
				PORT(I2C_SCL_PORT, ODR) &= ~I2C_SCL_PIN; // clear SCL, SDA is already cleared
			break;
			default: // keep current state
				;
		}
		tr_mask = 1<<15; // MSB first
		return;
	}
	bitinbyte = (nbit-1) % 9; // bit counter in each byte
	tick = tick_counter % 4; // this is a tick inside bit
	// now prepare data
	if(bitinbyte == 8){ // ACK
		switch (tick){
			case 0: // ACK bit
				if(nbit == 9) // check line - set ACK to 1
					PORT(I2C_SDA_PORT, ODR) |= I2C_SDA_PIN;
				else // set ACK to 0 - we are in reading mode
					PORT(I2C_SDA_PORT, ODR) &= ~I2C_SDA_PIN;
			break;
			case 1: // check ACK
				if(nbit == 9 && (PORT(I2C_SDA_PORT, ODR) & I2C_SDA_PIN)){ // not zero - very bad
					transmission_over = 1;
					soft_I2C_state = SOFT_I2C_NO_DEVICE;
					return;
				}
			break;
			default: // do nothing
				;
		}
	}else{//data transmision; or STOP
		if((tr_mask == 0) && (rd_counter == 0)){ // data sent, now we should send STOP in write mode
			// in reading mode - send STOP only when rd_mask == 0
			if(tick == 1){ // STOP is 0->1 when CLK is high
				PORT(I2C_SDA_PORT, ODR) |= I2C_SDA_PIN;
				transmission_over = 1;
				if(transmit_cmd & 1<<8) // reading mode
					soft_I2C_state = SOFT_I2C_DATA_READ_OK;
				else // writting mode
					soft_I2C_state = SOFT_I2C_DATA_SEND_OK;
			}
		}
		if(tick == 0){ // start of bit
			if(tr_mask){ // transmission
				if(transmit_cmd & tr_mask){ // send one
					PORT(I2C_SDA_PORT, ODR) |= I2C_SDA_PIN;
				} // if we need to send 0, we already have data in zero state
				tr_mask >>= 1;
				// stop transmission after first byte in read mode
				if((tr_mask == 1<<7) && (transmit_cmd & 1<<8)){ // go to reading mode
					tr_mask = 0;
					rd_counter = 64;
				}
			}
		}else if(tick == 1){ // capture readed data
			if((tr_mask == 0) && (transmit_cmd & 1<<8)){ // we are waiting for data
				rd_counter--;
				readed_data <<= 1;
				if(PORT(I2C_SDA_PORT, IDR) & I2C_SDA_PIN) // read 1
					readed_data |= 1;
			}
		}
	}
	// and at last - set level of SCL
	switch (tick){ // set proper value of SCL
		case 0: // high pulse
			PORT(I2C_SCL_PORT, ODR) |= I2C_SCL_PIN;
		break;
		case 3: // low pulse & clear SDA
			PORT(I2C_SDA_PORT, ODR) &= ~I2C_SDA_PIN;
			PORT(I2C_SCL_PORT, ODR) &= ~I2C_SCL_PIN;
		break;
		default: // do nothing
			;
	}

}

