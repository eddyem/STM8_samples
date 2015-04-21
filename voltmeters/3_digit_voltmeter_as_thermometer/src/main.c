/*
 * main.c
 *
 * Copyright 2014 Edward V. Emelianoff <eddy@sao.ru>
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

#include "stm8l.h"
#include "onewire.h"
#include "interrupts.h"
#include "led.h"

/*
 * DISPLAYER ERRORS:
 * --- - starting
 * e00 - Not found any DS18x20
 * eab - Error when read (or sensor is absent)
 * eee - EEPROM error (zeroed when running)
 * eff - can't store last ROM: no space left or EEPROM error
 *
 * BUTTONS:
 * 0 - PB4, 1 - PB5, 2 - PD1, 3..7 - PC3..PC7
 * here we use:
 * btn2 (PD1) - STORE NEW ROM
 * btn3 (PC3) - DELETE NONEXISTANT
 * btn4 (PC4) - DELETE ALL
 */
#define STORE_BTN   (1<<2)
#define DELETE_BTN  (1<<3)
#define DELALL_BTN  (1<<4)

// one digit emitting time
#define   LED_delay      (1)
// pause between buttons actions
#define   BTNS_delay     (30)
// delays: time of showing sensor number & time of showing temperature (0.3s & 0.7s)
#define   NUMBER_delay   (300)
#define   TEMPER_delay   (700)
// pause to show stored value
#define   STORE_delay    (2000)

U32 Global_time = 0L;    // global time in ms
volatile U8 waitforread = 0;
long temp_readed = ERR_TEMP_VAL;     // value of themperature (in 10th of Celsius)
U8 temp_ready = 0;       // ready flag
U8 matchROM = 0,         // scan all stored ROMs
starting_val = 0,        // starting ROM number for next portion of scan
delete_notexistant = 0;  // delete all ROMs that not exists
U32 T_time = 0L;         // temperature timer
U16 temper_delay = NUMBER_delay; // pause for showing different info
U8 block_keys = 0;

void show_received_data();
void send_read_seq(){
	onewire_receive_bytes(9);
	ow_process_resdata = show_received_data;
}

/**
 * get starting value of sensor number
 * return 1 if found or 0 if there's no ROMs data stored
 */
U8 get_starting_val(){
	for(;starting_val < MAX_SENSORS; starting_val++)
		if((saved_data->ROMs[starting_val]).is_free == 0){
			return 1;
		}
	//delete_notexistant = 0;
	return 0;
}

void read_next_sensor(){
	U8 i;
	U8 *rom = (saved_data->ROMs[starting_val]).ROM_bytes;
	if(!onewire_reset()){
		ow_process_resdata = NULL;
		return;
	}
	ow_data_array[0] = OW_READ_SCRATCHPAD;
	ow_process_resdata = send_read_seq;
	for(i = 0; i < 8; i++){
		ow_data_array[i+1] = rom[i];
	}
	ow_data_array[9] = OW_MATCH_ROM;
	onewire_send_bytes(10);
}

void show_received_data(){
	ow_process_resdata = NULL;
	temp_readed = gettemp();
	if(temp_readed == ERR_TEMP_VAL){
		if(delete_notexistant){
			erase_saved_ROM(starting_val);
		}
	}
	temp_ready = 1;
}

void wait_reading(){
	if(ow_data_array[0] == 0xff){ // the conversion is done!
		waitforread = 1;
		ow_process_resdata = NULL;
	}else{
		onewire_receive_bytes(1); // send read seq waiting for end of conversion
	}
}

U8 start_temp_reading(){
	if(!onewire_reset()){
		set_display_buf("e00");
		return 0;
	}
	ow_data_array[0] = OW_CONVERT_T;
	ow_data_array[1] = OW_SKIP_ROM;
	ow_process_resdata = wait_reading;
	onewire_send_bytes(2);
	return 1;
}

void store_last_ROM(){
	char i;
	U8 r;
	for(i = 7; i > -1; i--)
		ROM[i] = ow_data_array[i];
	T_time = Global_time;
	temper_delay = STORE_delay;
	r = store_ROM();
	if(r){
		display_int((int) r, 0);
	}else{
		set_display_buf("eff");
	}
	block_keys = 0;
}

int main() {
	U32 T_LED = 0L;  // time of last digit update
	U32 Tow = 0L;    // 1-wire time
	U32 T_btns = 0L; // buttons timer
	U8 old_btns_state = 0;
	U8 show_temp = 0; // =0 to show number; = 1 to show temp

	// Configure clocking
	CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz
	// Configure pins
	CFG_GCR |= 1; // disable SWIM
	LED_init();
	CCR |= 0x28; // make shure that we are on level 3 - disabled SW priority
	// Configure Timer1
	// prescaler = f_{in}/f_{tim1} - 1
	// set Timer1 to 1MHz: 16/1 - 1 = 15
	TIM1_PSCRH = 0;
	TIM1_PSCRL = 15; // LSB should be written last as it updates prescaler
	// auto-reload each 1ms: TIM_ARR = 1000 = 0x03E8
	TIM1_ARRH = 0x03;
	TIM1_ARRL = 0xE8;
	// interrupts: update
	TIM1_IER = TIM_IER_UIE;
	// auto-reload + interrupt on overflow + enable
	TIM1_CR1 = TIM_CR1_APRE | TIM_CR1_URS | TIM_CR1_CEN;

	onewire_setup();
	eeprom_default_setup();

	// enable all interrupts
	enableInterrupts();
	set_display_buf("---");
	if(get_starting_val()){
		matchROM = 1; // if ROMs not empty, start scanning
		starting_val = 0; // reset starting value
	}
	start_temp_reading();
	// Loop
	do {
		if(((U16)(Global_time - T_time) > temper_delay) || (T_time > Global_time)){
			T_time = Global_time;
			if(show_temp){ // show temperature
				temper_delay = TEMPER_delay;
				show_temp = 0;
				if(!temp_ready || temp_readed == ERR_TEMP_VAL){
					set_display_buf("eab");
				}else{
					temp_ready = 0;
					display_int((int)temp_readed,0);
					display_DP_at_pos(1);
				}
				if(matchROM) ++starting_val;
			}else{ // show number
				show_temp = 1;
				temper_delay = NUMBER_delay;
				if(matchROM){
					if(get_starting_val()){
						display_int(starting_val+1,0);
						read_next_sensor();
					}else{
						starting_val = 0;
						if(start_temp_reading()){
							if(!get_starting_val()){
								matchROM = 0;
								set_display_buf("eee");
							}else{
								display_int(starting_val+1,0);
							}
						}
					}
				}else{
					start_temp_reading();
				}
			}
		}
		if((U8)(Global_time - T_LED) > LED_delay){
			T_LED = Global_time;
			show_next_digit();
		}
		if((U8)(Global_time - T_btns) > BTNS_delay){
			T_btns = Global_time;
			if(!block_keys && old_btns_state != buttons_state){
				U8 pressed = old_btns_state & ~buttons_state; // pressed buttons are ones
				if(pressed){ // some buttons were pressed
					if(pressed & STORE_BTN){
						if(onewire_reset()){
							delete_notexistant = 0;
							block_keys = 1;
							onewire_send_byte(OW_READ_ROM);
							while(OW_BUSY);
							ow_process_resdata = store_last_ROM;
							onewire_receive_bytes(8);
						}
					}else if(pressed & DELETE_BTN){
						delete_notexistant = 1;
					}else if(pressed & DELALL_BTN){
						U8 i;
						for(i = 0; i < MAX_SENSORS; i++){
							erase_saved_ROM(i);
							set_display_buf("---");
							matchROM = 0;
						}
					}
					//display_int(pressed, 0);
				}else{ // some buttons released
					//display_int(~old_btns_state & buttons_state, 0); // released are ones
				}
			}
			old_btns_state = buttons_state;
		}
		if(Global_time != Tow){ // process every byte not frequently than once per 1ms
			Tow = Global_time;
			process_onewire();
		}
		if(waitforread){
			if(onewire_reset()){
				ow_process_resdata = send_read_seq;
				waitforread = 0;
				if(!matchROM){
					ow_data_array[0] = OW_READ_SCRATCHPAD;
					ow_data_array[1] = OW_SKIP_ROM;
					onewire_send_bytes(2);
				}else{
					read_next_sensor();
				}
			}
		}
	} while(1);
}

