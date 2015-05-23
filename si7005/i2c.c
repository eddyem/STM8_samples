/*
 * i2c.c - functions to work with HW I2C
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

/*
 * I2C_FREQR - peripherial input clock (>=1MHz - standard, >=4MHz - fast)
 * + clock control & rise time
 * I2C_CR1 - enable
 * I2C_CR2 - start bit (generate + set SB bit + interrupt if ITEVTEN==1)
 * writing can be in 7 & 10 bit modes
 *
 *                        ** 7bit transmit**
 * 1) start; 2) read SR1 & write address to DR; 3) check ADDR, if ACK received
 *   it would be 1 - clear it by read SR1 & SR3, wait for TXE==1 & BTF==1 (ready for transmit)
 *   and send data writing to DR; to send EOT set STOP=1
 *
 *                        ** 10bit transmit**
 * 1) start; 2) read SR1 & write 11110xx0 (xx - MS bits of address) to DR;
 * 3) ADD10=1, clear it by reading SR1 & write last 8 bits of address to DR;
 * 4) similar to 3) for 7bits
 *
 *                        ** receive **
 * 1) clear ADDR after sending address
 * 2) set ACK if need to generate ACK bit
 * 3) check RXNE or get data by interrupt (if ITEVTEN==1 & ITBUFEN==1)
 * in case of byte transfer finished, BTF=1 (to clear it, read SR1 & DR)
 * in 10bit receive mode send start & 11110xx1 after sending address
 * in 7bit receive mode send address with LSB set
 *
 * TRA bit indicates I2C mode
 *
 *                        ** close **
 * Method 1 (higher interrupts):
 * 1) reset ACK=0 to generate NACK; 2) to generate Stop/Restart set STOP/START
 * all this should be done at second last byte reading (to get a single byte
 * do this just after ADDR sent); 3) read last byte
 *
 * Method 2 (low interrupt priority or polling, valid only for N>2):
 * 1) not read DR & wait for RXNE==1&&BTF==1
 * 2) clear ACK
 * 3) read DR (DATA_N-2) & set STOP/START
 * 4) read DR (DATA_N-1), read DR (DATA_N)
 *
 * to read 1 byte use Method 1;
 * to read 2 bytes with Method2, set POS&ACK, wait ADDR, clear ADDR&ACK,
 *  wait BTF, set STOP, read DR twice
 *
 *
 *                            *** REGISTERS ***
 * I2C_CR1:  | NOSTRETCH | ENGC | reserved[5:1] | PE |
 *   NOSTRETCH - ==1 to slave mode
 *   ENGC      - generall call EN/DIS
 *   PE        - ==1 to enable I2C
 *
 * I2C_CR2:  | SWRST | res[6:4] | POS | ACK | STOP | START |
 *  SWRST - ==1 to software reset
 *  POS   - ACK position (==0 for current byte, ==1 for next byte)
 *  ACK   - send ACK after reading byte
 *  STOP  - send STOP after reading byte
 *  START - send repeated START
 *   DO NOT make any changes with I2C_CR2 when sending START/STOP
 *     before they'll be cleared by hardware!
 *
 * I2C_FREQR: | res[7:6] | FREQ[5:0] |
 *   FREQ - I2C clock frequency (in MHz, from 1 to 24)
 *
 * I2C_OARL: | ADD[7:1] | ADD0 | - LSB of address
 *   ADD0 - 0th bit of address in 10bit mode, don't care in 7bit mode
 *
 * I2C_OARH: | ADDMODE | ADDCONF | res[5:3] | ADD[9:8] | res |
 *   ADDMODE - 0 for 7bit, 1 for 10bit address
 *   ADDCONF - must always be written as 1 ("address is set")
 *
 * I2C_DR - data I/O register
 *
 * I2C_SR1: | TXE | RXNE | res | STOPF | ADD10 | BTF | ADDR | SB |
 *   TXE   - set when DR is empty for transmission
 *   RXNE  - set when DR not empty in receiver mode
 *   STOPF - stop detected (slave mode)
 *   ADD10 - 10 bit header sent (first byte)
 *   BTF   - byte transfer finished (set when NOSTRETCH==0)
 *   ADDR  - address sent (not set if NACK)
 *   SB    - start condition
 *
 * I2C_SR2: | res[7:6] | WUFH | res | OVR | AF | ARLO | BERR |
 *   WUFH - wakeup from halt
 *   OVR  - overrun/underrun
 *   AF   - acknowledge failure (must be cleared by software)
 *   ARLO - arbitration lost (another master on line, clear it by SW)
 *   BERR - bus error (clear by SW)
 *
 * I2C_SR3: | res[7:5] | GENCALL | res | TRA | BUSY | MSL |
 *   GENCALL - general call in slave mode
 *   TRA     - transmitter(1)/receiver(0) flag
 *   BUSY    - bus busy (another communication detected)
 *   MSL     - slave(0)/master(1) mode, set/cleared by HW
 *
 * I2C_ITR: | res[7:3] | ITBUFEN | ITEVTEN | ITERREN |
 *   ITBUFEN - enable buffer interrupt
 *   ITEVTEN - enable event interrupt
 *   ITERREN - enable error interrupt
 *
 * I2C_CCRL: | CCR[7:0] |
 *   SCLH clock in master mode
 *    standard: T = 2*CCR*tmaster, Tlow = Thigh = CCR*tmaster
 *    fast: (DUTY==0) T = 3*CCR*tmaster, Thigh = CCR*tmaster, Tlow = 2*Thigh
 *          (DUTY==1) T = 25*CCR*tmaster, Thigh=9*CCR*tmaster, Tlow=16*CCR*tmaster
 *   fmaster - clock by I2C_FREQR
 *  minimum allowed value is 4 (exept FAST DUTY, when it is 1)
 *
 * I2C_CCRH: | F/S | DUTY | res[5:4] | CCR[11:8] |
 *   F/S  - == 1 in fast mode
 *   DUTY - (see upper)
 * IN standard mode 100kHz is: FREQR=8, CCR=0x28
 *
 * I2C_TRISER: | res[7:6] | TRISE[5:0] |
 *   TRISE - maximum rise time (0x09 for standard @100kHz)
 *
 */

#include "i2c.h"
#include "ports_definition.h"

static U8 addr7r = 0, addr7w = 0;

extern volatile unsigned long Global_time;

static U16 _c;
static unsigned long wtm;
#define I2C_WAIT(evt, tmo)  do{wtm = Global_time; \
		while(Global_time-wtm<tmo) if(evt) break;   \
		if(!evt){ret = I2C_SR1; goto eotr;}}while(0)

/*
#define I2C_WAIT(evt) do{for(_c = 0; _c < 60000; _c++){ \
		if(evt) break;} if(_c == 60000) return ret;}while(0)
*/

static U8 _d;
#define I2C_LINEWAIT() do{ for(_d = 0; _d < 16; _d++){\
		if(!(I2C_SR3 & 2)) break; I2C_CR2 |= 2;\
		for(_c = 0; _c < 1000; _c++) if(!(I2C_CR2 & 2)) break;} \
		if(_d == 16) return I2C_LINEBUSY; }while(0)

/*
 * configure 100kHz speed in standard mode & enable i2c
 */
void i2c_setup(){
	// configure pins: PB5 - I2C_SDA; PB4 - I2C_SCL (both opendrain)
	PORT(PB, ODR) |= GPIO_PIN4|GPIO_PIN5; // set to 1
	PORT(PB, DDR) |= GPIO_PIN4|GPIO_PIN5;
	PORT(PB, CR2) |= GPIO_PIN4|GPIO_PIN5; // fast mode
	// Don't forget to connect pullup resistor to I2C foots
	I2C_FREQR = 8; // 8MHz fmaster
	I2C_TRISER = 9; // rise time 1000ns
	I2C_CCRL = 0x28; // 100kHz
	I2C_CCRH = 0;
	I2C_ITR = 0; // disable all I2C interrupts
	I2C_CR2 |= 4; // ACK
	I2C_CR1 |= 1; // enable I2C
}

void i2c_set_addr7(U8 addr){
	addr7w = addr << 1;
	addr7r = addr7w | 1;
}

/*
 * send one byte in 7bit address mode
 * return I2C_OK if success errcode if fails
 */
i2c_status i2c_7bit_send_onebyte(U8 data){
	i2c_status ret = I2C_TMOUT;
	//I2C_CR2 |= 0x80; I2C_CR2 &= ~0x80; // reset I2C
	I2C_LINEWAIT();
	I2C_CR2 |= 1; // send START
	I2C_WAIT(I2C_SR1 & 1, 2); // wait for SB
	I2C_DR = addr7w;
	ret = I2C_NOADDR;
	I2C_WAIT((I2C_SR1 & 2) || I2C_SR2, 2); // wait for ADDR
	if(I2C_SR2){ // NACK or other error
		ret = I2C_NACK;
		goto eotr;
	}
	ret = I2C_HWPROBLEM;
	// clear ADDR reading SR3
	if(!(I2C_SR3 & 4)) goto eotr; // interface is in receiver mode
	I2C_WAIT(I2C_SR1 & 0x80, 2); // wait for TXE
	I2C_DR = data; // send data
	I2C_WAIT(((I2C_SR1 & 0x84) == 0x84) || I2C_SR2, 15); // wait for TXE & BTF
	//I2C_WAIT((I2C_SR1 & 4) || I2C_SR2); // wait for TXE & BTF
	if(!I2C_SR2) ret = I2C_OK;
	else ret = I2C_NACK;
eotr:
	I2C_SR2 = 0; // clear all error flagss
	I2C_CR2 |= 2; // set STOP
	while(I2C_CR2 & 2); // wait for STOP sent
	return ret;
}

/*
 * send datalen bytes over I2C
 * return I2C_OK if OK
 */
i2c_status i2c_7bit_send(U8 *data, U8 datalen){
	i2c_status ret = I2C_TMOUT;
	//I2C_CR2 |= 0x80; I2C_CR2 &= ~0x80; // reset I2C
	I2C_LINEWAIT();
	I2C_CR2 |= 1; // send START
	ret = I2C_TMOUT;
	I2C_WAIT(I2C_SR1 & 1, 2); // wait for SB
	I2C_DR = addr7w;
	ret = I2C_NOADDR;
	I2C_WAIT((I2C_SR1 & 2) || I2C_SR2, 2); // wait for ADDR
	if(I2C_SR2){ // NACK or other error
		ret = I2C_NACK;
		goto eotr;
	}
	ret = I2C_HWPROBLEM;
	if(!(I2C_SR3 & 4)) goto eotr; // interface is in receiver mode
	while(datalen--){
		I2C_WAIT((I2C_SR1 & 0x80) || I2C_SR2, 2); // wait for TXE
		if(I2C_SR2){
			ret = I2C_NACK;
			goto eotr;
		}
		I2C_DR = *data++; // send data
	}
	I2C_WAIT((I2C_SR1 & 0x84 == 0x84) || I2C_SR2, 15); // wait for TXE & BTF
	//I2C_WAIT((I2C_SR1 & 0x84) || I2C_SR2);
	if(!I2C_SR2) ret = I2C_OK;
	else ret = I2C_NACK;
eotr:
	I2C_SR2 = 0; // clear all error flags
	I2C_CR2 |= 2; // set STOP
	while(I2C_CR2 & 2); // wait for STOP sent
	return ret;
}

/*
 * get one byte by I2C
 * return I2C_OK if ok
 * return I2C_NACK if some error present
 */
i2c_status i2c_7bit_receive_onebyte(U8 *data){
	i2c_status ret = I2C_TMOUT;
	//I2C_CR2 |= 0x80; I2C_CR2 &= ~0x80; // reset I2C
	I2C_LINEWAIT();
	I2C_CR2 |= 1; // send START
	ret = I2C_TMOUT;
	I2C_WAIT(I2C_SR1 & 1, 2); // wait for SB
	I2C_DR = addr7r; // send address & read bit
	ret = I2C_NOADDR;
	I2C_WAIT((I2C_SR1 & 2) || I2C_SR2, 2); // wait for ADDR
	if(I2C_SR2){ // NACK or other error
		ret = I2C_NACK;
		goto eotr;
	}
	// clear POS|ACK
	I2C_CR2 &= ~0x0c;
	ret = I2C_HWPROBLEM;
	// read SR3 to clear ADDR
	if((I2C_SR3 & 4)) goto eotr; // interface is in transmitter mode
	// set STOP
	I2C_CR2 |= 2;
	// wait for RxNE
	I2C_WAIT((I2C_SR1 & 0x40) || I2C_SR2, 2);
	if(I2C_SR2){
		ret = I2C_NACK;
		goto eotr; // error
	}
	ret = I2C_OK;
	// read data clearing RxNE
	*data = I2C_DR;
eotr:
	I2C_SR2 = 0; // clear all error flags
	if(!I2C_CR2 & 2) I2C_CR2 |= 2;
	while(I2C_CR2 & 2); // wait for STOP sent
	return ret;
}

i2c_status i2c_7bit_receive_twobyte(U8 *data){
	i2c_status ret = I2C_TMOUT;
	//I2C_CR2 |= 0x80; I2C_CR2 &= ~0x80; // reset I2C
	I2C_LINEWAIT();
	I2C_CR2 |= 1; // send START
	ret = I2C_TMOUT;
	I2C_WAIT(I2C_SR1 & 1, 2); // wait for SB
	I2C_DR = addr7r; // send address & read bit
	// set POS|ACK
	I2C_CR2 |= 0x0c;
	ret = I2C_NOADDR;
	I2C_WAIT((I2C_SR1 & 2) || I2C_SR2, 2); // wait for ADDR
	if(I2C_SR2){ // NACK or other error
		ret = I2C_NACK;
		goto eotr;
	}
	ret = I2C_HWPROBLEM;
	// read SR3 to clear ADDR
	if(I2C_SR3 & 4) goto eotr; // interface is in transmitter mode
	// clear ACK
	I2C_CR2 &= ~4;
	// wait for BTF
	I2C_WAIT((I2C_SR1 & 4) || I2C_SR2, 15);
	if(I2C_SR2){
		ret = I2C_NACK;
		goto eotr;
	}
	// patch from ERRATA
	disableInterrupts();
	// set STOP
	I2C_CR2 |= 2;
	ret = I2C_OK;
	// read data
	data[0] = I2C_DR;
	enableInterrupts();
	data[1] = I2C_DR;
eotr:
	I2C_SR2 = 0; // clear all error flags
	if(!I2C_CR2 & 2) I2C_CR2 |= 2;
	while(I2C_CR2 & 2); // wait for STOP sent
	return ret;
}


INTERRUPT_HANDLER(I2C_IRQHandler, 19){
}
