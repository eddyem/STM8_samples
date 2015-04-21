/*
 * onewire.c
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
#include "onewire.h"

// last gotten ROM in reverse order
U8 ROM[8];

eeprom_data *saved_data = (eeprom_data*)EEPROM_START_ADDR;
// mode of 1-wire
volatile OW_modes ow_mode = OW_MODE_OFF;
// length of zero-pulse (TIM2_CCR1)
volatile U16 onewire_gotlen = 0;
volatile U8 is_receiver = 0;
volatile U8 ow_data;
// remaining ticks counter
volatile U8 onewire_tick_ctr;
// array for sending/receiving data
// DATA STORED IN REVERCE ORDER! FIRST BYTE ID ow_data_array[N] AND LAST IS ow_data_array[0]!!!
U8 ow_data_array[10];
// how much bytes to send/receive
U8 ow_byte_ctr;
// this function will be called after receiving/transmitting of N bytes
// inside this function should be: ow_process_resdata = NULL;
void (*ow_process_resdata)() = NULL;


/*
 ********************* Timer2 ********************
 * prescaler: TIM2_PSCR f = f_{in}/2^{TIM2_PSCR}
 * other registers:
 * TIM2_CR1 (page 223): | ARPE | reserved[2:0] | OPM | URS | UDIS | CEN |
 *              ARPE - Auto-reload preload enable (for TIM2_ARR)
 *              OPM: One-pulse mode (not implemented!)
 *              URS: Update request source (When enabled by the UDIS bit, 1 - interrupt only on counter overflow/underflow)
 *              UDIS: Update disable (1 - disable update int)
 *              CEN: Counter enable (1 - enable)
 * TIM2_IER (page 226): | res | TIE | res | res | CC3IE | CC2IE | CC1IE | UIE |
 *              T - trigger; CC - comp/capt; U - update <--
 * TIM2_SR1 (page 227): similar (but instead of IE -> IF)
 *              interrupt flags
 * TIM2_SR2 (page 228): overcapture flags
 * TIM2_EGR (page 229): event generation
 * TIM2_CCMR1 (page 230):
 * OUTPUT:  | res | OC1M[2:0] | OC1PE | res | CC1S[1:0] |
 * INPUT:   | IC1F[3:0] | IC1PSC[1:0] | CC1S[1:0] |
 *              OC1M: compare mode !!! writeable only when channel is off (CC1E=0) !!!
 *               000: no
 *               001: set channel 1 to active level on match
 *               010: set chan1 to inactive ...
 *               011: toggle
 *               100: force 0
 *               101: force 1
 *               110: PWM mode 1 (1->0)
 *               111: PWM mode 2 (0->1)
 *              OC1PE: output compare 1 preload enable (0 - loads immediately)
 *              CC1S: comp/capt selection
 *               00: CC1 is out
 *               01: CC1 is in (TI1FP1)
 *               10: TI2FP1
 *               11: (only for TIM5)
 *              IC1F: input capture filter  (0 - no filter)
 *              IC1PSC: capture prescaler (0 - no, xx - 2^{xx} events)
 * TIM2_CCMRx - the same for channels 2 & 3
 * TIM2_CCERx - CC polarity/enable (lowest 2 bytes in each 4): P=1:active high/capture on rising edge;
 * TIM2_CNTRH, TIM2_CNTRL - counter value (automatical)
 * TIM2_PSCR - prescaler value, lower 4 bits
 * TIM1_ARRH, TIM1_ARRL   - auto-reload value (while zero, timer is stopped) (page 206)
 * TIM2_CCRxL/H - compare/capture registers
 */

void onewire_setup(){
// freq = 1MHz: psc=16 -> TIM2_PSCR = 4
	TIM2_PSCR = 4;
// AIN: PD3 / TIM2_CH2 -> CC1 will be input on TI2FP1 & CC2 will be output
	// configure pin CC2 (PD3): open-drain output
	PORT(PD, DDR) |= GPIO_PIN3; // output & pseudo open-drain
	PORT(PD, CR2) |= GPIO_PIN3; // fast
	// out: PWM mode 1 (active -> inactive), preload enable
	// CCMR2: | 0 | 110 | 1 | 0 | 00 |
	TIM2_CCMR2 = 0x68;
	// in: TI2FP1
	TIM2_CCMR1 = 0x02;
	// polarity: out - active LOW, in - capture on rising edge, enable
	TIM2_CCER1 = 0x31;
	// interrupts: CC1IE
	TIM2_IER = TIM_IER_CC1IE;
	// enable preload for registers to refresh their values only by UEV
	TIM2_CR1 = TIM_CR1_APRE | TIM_CR1_URS;
	// ARR values: 1000 for reset, 70 for data in/out
	// CCR2 values: 500 for reset, 60 for sending 0 or reading, <15 for sending 1
	// CCR1 values: >550 if there's devices on line (on reset), >12 (typ.15) - read 0, < 12 (typ.1) - read 1
	// WARN! on reset there would be two CC events generated
}

/**
 * reset 1-wire bus
 * return 0 if no devices found, else return 1
 */
U8 onewire_reset(){
	is_receiver = 0; // send data, not receive
	onewire_tick_ctr = 1; // if there's devices on the bus CC1 int would be generated twice!
	TIM2REG(ARR, RESET_LEN);
	TIM2REG(CCR2, RESET_P);
	//TIM2_CCR1H = TIM2_CCR1L = 0;
	TIM2_EGR = TIM_EGR_UG; // generate UEV to update values
	TIM2_CR1 |= TIM_CR1_CEN; // turn on timer
	ow_mode = OW_MODE_RESET;
	while(OW_BUSY); // wait until transmission is over
	return ((onewire_gotlen > RESET_BARRIER) ? 1 : 0);
}

/**
 * Send one byte through 1-wire
 */
void onewire_send_byte(U8 byte){
	ow_data = byte;
	is_receiver = 0;
	onewire_tick_ctr = 7; // 7 bits remain
	TIM2REG(ARR, BIT_LEN);
	if(ow_data & 1){ // transmit 1
		TIM2REG(CCR2, BIT_ONE_P);
	}else{ // transmit 0
		TIM2REG(CCR2, BIT_ZERO_P);
	}
	TIM2_EGR = TIM_EGR_UG; // generate UEV to update values
	ow_data >>= 1;
	TIM2_CR1 |= TIM_CR1_CEN; // turn on timer
}

/**
 * get 1 byte through 1-wire
 */
void onewire_wait_for_receive(){
	ow_data = 0;
	is_receiver = 1;
	onewire_tick_ctr = 8; // eight bits remain!
	TIM2REG(ARR, BIT_LEN);
	TIM2REG(CCR2, BIT_READ_P); // reading length
	TIM2_EGR = TIM_EGR_UG; // generate UEV to update values
	TIM2_CR1 |= TIM_CR1_CEN; // turn on timer
}

// DATA STORED IN REVERCE ORDER! FIRST BYTE ID ow_data_array[N] AND LAST IS ow_data_array[0]!!!
/**
 * wait for receiveing N bytes from 1-wire
 * N shoud be not great than 10
 */
void onewire_receive_bytes(U8 N){
	ow_byte_ctr = N;
	ow_mode = OW_MODE_RECEIVE_N;
	onewire_wait_for_receive();
}

void onewire_send_bytes(U8 N){
	ow_byte_ctr = N;
	ow_mode = OW_MODE_TRANSMIT_N; // first data portion will be send by process_onewire
}

/**
 * process 1-wire events
 */
void process_onewire(){
	if(OW_BUSY) return; // let data to be processed
	switch(ow_mode){
		case OW_MODE_RECEIVE_N: // wait for receiving of N bytes -> send next byte
			ow_data_array[--ow_byte_ctr] = ow_data;
			if(ow_byte_ctr){ // there's some more data to receive
				onewire_wait_for_receive(); // wait for another byte
				return;
			}
			// we have received all data - process it!
			ow_mode = OW_MODE_OFF;
			if(ow_process_resdata){
				ow_process_resdata();
			}
		break;
		case OW_MODE_TRANSMIT_N:
			if(ow_byte_ctr){ // we have some more data to transmit
				onewire_send_byte(ow_data_array[--ow_byte_ctr]);
				return;
			}
			ow_mode = OW_MODE_OFF;
			if(ow_process_resdata){
				ow_process_resdata();
			}
		break;
		case OW_MODE_RESET:
			ow_mode = OW_MODE_OFF;
		break;
		default: // OW_MODE_OFF
			return;
	}
}

/**
 * convert temperature from ow_data_array (scratchpad)
 * in case of error return 200000 (ERR_TEMP_VAL)
 * return value in 10th degrees centigrade
 * don't forget that bytes in ow_data_array have reverce order!!!
 * so:
 * 8 - themperature LSB
 * 7 - themperature MSB (all higher bits are sign)
 * 6 - T_H
 * 5 - T_L
 * 4 - B20: Configuration register (only bits 6/5 valid: 9..12 bits resolution); 0xff for S20
 * 3 - 0xff (reserved)
 * 2 - (reserved for B20); S20: COUNT_REMAIN (0x0c)
 * 1 - COUNT PER DEGR (0x10)
 * 0 - CRC
 */
long gettemp(){
	// detect DS18S20
	long t = 0L;
	U8 l,m;
	char v;
	if(ow_data_array[1] == 0xff) // 0xff can be only if there's no such device or some other error
		return ERR_TEMP_VAL;
	m = ow_data_array[7];
	l = ow_data_array[8];
	if(ow_data_array[4] == 0xff){ // DS18S20
		v = l >> 1 | (m & 0x80); // take signum from MSB
		t = ((long)v) * 10L;
		if(l&1) t += 5L; // decimal 0.5
	}
	else{
		v = l>>4 | ((m & 7)<<4);
		if(m&0x80){ // minus
			v |= 0x80;
		}
		t = ((long)v) * 10L;
		m = l&0x0f; // add decimal
		t += (long)m; // t = v*10 + l*1.25 -> convert
		if(m > 1) t++; // 1->1, 2->3, 3->4, 4->5, 4->6
		else if(m > 5) t+=2L; // 6->8, 7->9
	}
	return t;
}

U8 unlock_EEPROM(){
	// unlock memory
	FLASH_DUKR = EEPROM_KEY1;
	FLASH_DUKR = EEPROM_KEY2;
	// check bit DUL=1 in FLASH_IAPSR
	if(!(FLASH_IAPSR & 0x08))
		return 0;
	return 1;
}

void lock_EEPROM(){
	while(!(FLASH_IAPSR & 0x04)); // wait till end
	// clear DUL to lock write
	FLASH_IAPSR &= ~0x08;
}

/**
 * setup EEPROM after first run: mark all cells as free
 */
void eeprom_default_setup(){
	U8 i;
	if(saved_data->magick == EEPROM_MAGICK) return; // all OK
	if(!unlock_EEPROM()) return;
	saved_data->magick = EEPROM_MAGICK;
	for(i = 0; i < MAX_SENSORS; i++)
		(saved_data->ROMs[i]).is_free = 1;
	lock_EEPROM();
}

/**
 * erase cell with number num
 * return 0 if fails
 */
U8 erase_saved_ROM(U8 num){
	if(!unlock_EEPROM()) return 0;
	saved_data->ROMs[num].is_free = 1;
	lock_EEPROM();
	return 1;
}

/**
 * check ROMs for current value
 * Return MAX_SENSORS if not found or cell number
 */
U8 check_ROM(){
	U8 r,i;
	U8 *cellbytes;
	for(r = 0; r < MAX_SENSORS; r++){
		if(!(saved_data->ROMs[r].is_free)){ // not free cell: check
			cellbytes = saved_data->ROMs[r].ROM_bytes;
			for(i = 0; i < 8; i++)
				if(cellbytes[i] != ROM[i]) break;
			if(i == 8) break; // we found this cell
		}
	}
	return r;
}

/**
 * store last ROM in EEPROM
 * return 0 if fails or return cell number + 1
 */
U8 store_ROM(){
	U8 i, r;
	ow_ROM *cell = NULL;
	r = check_ROM();
	if(r != MAX_SENSORS) return r+1; // we already have stored this value
	for(r = 0; r < MAX_SENSORS; r++)
		if(saved_data->ROMs[r].is_free) break;
	if(r == MAX_SENSORS) return 0; // fail: all cells are busy
	cell = &(saved_data->ROMs[r]);
	if(!unlock_EEPROM()) return 0;
	cell->is_free = 0;
	for(i = 0; i < 8; i++)
		cell->ROM_bytes[i] = ROM[i];
	lock_EEPROM();
	return r+1;
}
