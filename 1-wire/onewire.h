/*
 * onewire.h
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

#pragma once
#ifndef __ONEWIRE_H__
#define __ONEWIRE_H__

#include "stm8l.h"

#define ERR_TEMP_VAL  200000

#define TIM2REGH(reg)  TIM2_##reg##H
#define TIM2REGL(reg)  TIM2_##reg##L
#define TIM2REG(reg, val)  do{TIM2REGH(reg) = val >> 8; TIM2REGL(reg) = val & 0xff;}while(0)


// ARR values: 1000 for reset, 100 for data in/out
// CCR2 values: 500 for reset, 60 for sending 0 or reading, <15 for sending 1
// CCR1 values: >550 if there's devices on line (on reset), >12 (typ.15) - read 0, < 12 (typ.1) - read 1
#define RESET_LEN         ((U16)1000)
#define BIT_LEN           ((U16)100)
#define RESET_P           ((U16)500)
#define BIT_ONE_P         ((U16)10)
#define BIT_ZERO_P        ((U16)60)
#define BIT_READ_P        ((U16)5)
#define RESET_BARRIER     ((U16)550)
#define ONE_ZERO_BARRIER  ((U16)10)

#define OW_BUSY  ((TIM2_CR1 & TIM_CR1_CEN))


/*
 * thermometer commands
 * send them with bus reset!
 */
// find devices
#define OW_SEARCH_ROM       (0xf0)
// read device (when it is alone on the bus)
#define OW_READ_ROM         (0x33)
// send device ID (after this command - 8 bytes of ID)
#define OW_MATCH_ROM        (0x55)
// broadcast command
#define OW_SKIP_ROM         (0xcc)
// find devices with critical conditions
#define OW_ALARM_SEARCH     (0xec)
/*
 * thermometer functions
 * send them without bus reset!
 */
// start themperature reading
#define OW_CONVERT_T         (0x44)
// write critical temperature to device's RAM
#define OW_SCRATCHPAD        (0x4e)
// read whole device flash
#define OW_READ_SCRATCHPAD   (0xbe)
// copy critical themperature from device's RAM to its EEPROM
#define OW_COPY_SCRATCHPAD   (0x48)
// copy critical themperature from EEPROM to RAM (when power on this operation runs automatically)
#define OW_RECALL_E2         (0xb8)
// check whether there is devices wich power up from bus
#define OW_READ_POWER_SUPPLY (0xb4)

/*
 * RAM register:
 * 0 - themperature LSB
 * 1 - themperature MSB (all higher bits are sign)
 * 2 - T_H
 * 3 - T_L
 * 4 - B20: Configuration register (only bits 6/5 valid: 9..12 bits resolution); 0xff for S20
 * 5 - 0xff (reserved)
 * 6 - (reserved for B20); S20: COUNT_REMAIN (0x0c)
 * 7 - COUNT PER DEGR (0x10)
 * 8 - CRC
 *
 * To identify S20/B20 use value of confuguration register: its MSbit is 0
 */

typedef enum{
	OW_MODE_OFF,        // sleeping
	OW_MODE_TRANSMIT_N, // transmit N bytes
	OW_MODE_RECEIVE_N,  // receive N bytes
	OW_MODE_RESET       // reset bus
} OW_modes;

typedef struct{
	U8 is_free;
	U8 ROM_bytes[8];
} ow_ROM;

#define EEPROM_MAGICK  (0x1234)

// there's only 128 bytes of EEPROM on STM8S003!!!
// so we have not more than 14 sensors!
#define MAX_SENSORS (14)

typedef struct{
	U16 magick;
	ow_ROM ROMs[MAX_SENSORS];
} eeprom_data;

extern U8 ROM[];
extern eeprom_data *saved_data;

extern volatile U8  ow_data;          // byte to send/receive
extern volatile U8  onewire_tick_ctr; // tick counter
extern volatile U8  is_receiver;
extern volatile U16 onewire_gotlen;   // last captured value
extern U8 ow_data_array[];
extern U8 ow_byte_ctr;
extern volatile OW_modes ow_mode;

extern void (*ow_process_resdata)();

void onewire_setup();
U8 onewire_reset();
void onewire_send_byte(U8 byte);
void onewire_wait_for_receive();
void process_onewire();
void onewire_receive_bytes(U8 N);
void onewire_send_bytes(U8 N);

long gettemp();

void eeprom_default_setup();
U8 erase_saved_ROM(U8 num);
U8 store_ROM();

#endif // __ONEWIRE_H__
