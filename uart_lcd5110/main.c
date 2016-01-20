/*
 * blinky.c
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
#include "ports_definition.h"
#include "interrupts.h"
#include "uart.h"
#include "pcd8544.h"
#include "spi.h"

volatile unsigned long Global_time = 0L; // global time in ms
U16 paused_val = 500; // interval between LED flashing
U8 bias = 4, vop = 60, temp = 1;

void setbias(){
	pcd8544_setbias(bias);
	uart_write("\nbias value: ");
	UART_send_byte('0' + bias);
	UART_send_byte('\n');
}
void setvop(){
	pcd8544_setvop(vop);
	uart_write("\nvop value: ");
	printUint(&vop, 1);
	UART_send_byte('\n');
}
void settemp(){
	pcd8544_settemp(temp);
	uart_write("\ntemp. value: ");
	UART_send_byte('0' + temp);
	UART_send_byte('\n');
}

int main() {
	unsigned long T = 0L;
	U8 rb, wb;
	CFG_GCR |= 1; // disable SWIM
	// Configure clocking
	CLK_CKDIVR = 0; // F_HSI = 16MHz, f_CPU = 16MHz
// Timer 4 (8 bit) used as system tick timer
	// prescaler == 128 (2^7), Tfreq = 125kHz
	// period = 1ms, so ARR = 125
	TIM4_PSCR = 7;
	TIM4_ARR = 125;
	// interrupts: update
	TIM4_IER = TIM_IER_UIE;
	// auto-reload + interrupt on overflow + enable
	TIM4_CR1 = TIM_CR1_APRE | TIM_CR1_URS | TIM_CR1_CEN;

	// PC2 - PP output (on-board LED)
	PORT(LED_PORT, DDR)  |= LED_PIN;
	PORT(LED_PORT, CR1)  |= LED_PIN;
	// LCD ~RST, ~CE and D/~C configuration: all PP outputs, clear all at start
	PORT(LCD_PORT, ODR) &= ~(LCD_DC_PIN | LCD_CE_PIN | LCD_RST_PIN);
	PORT(LCD_PORT, DDR) |= LCD_DC_PIN | LCD_CE_PIN | LCD_RST_PIN;
	PORT(LCD_PORT, CR1) |= LCD_DC_PIN | LCD_CE_PIN | LCD_RST_PIN;

	uart_init();
	spi_init();
	// enable all interrupts
	enableInterrupts();

	pcd8544_init();
	pcd8544_print("Всем привет!\n");

	// Loop
	do{
		if((Global_time - T > paused_val) || (T > Global_time)){
			T = Global_time;
			PORT(LED_PORT, ODR) ^= LED_PIN; // blink on-board LED
		}
		if(UART_read_byte(&rb)){ // buffer isn't empty
			switch(rb){
				case 'h': // help
				case 'H':
					UART_send_byte(rb);
					uart_write("\nPROTO:\n"
						"i - init LCD\n"
						"+/- Contrast\n"
						"V/v +/- vop\n"
						"T/t +/- temp\n"
					);
				break;
				case 'i':
					PORT(LCD_PORT, ODR) &= ~(LCD_DC_PIN | LCD_CE_PIN | LCD_RST_PIN);
					spi_init();
					pcd8544_init();
				break;
				case '+':
					if(bias < PCD8544_BIAS_MASK){
						++bias;
						setbias();
					}
				break;
				case '-':
					if(bias > 0){
						--bias;
						setbias();
					}
				break;
				case 'V':
					if(vop < PCD8544_VOP_MASK){
						++vop;
						setvop();
					}
				break;
				case 'v':
					if(vop > 0){
						--vop;
						setvop();
					}
				break;
				case 'T':
					if(temp < PCD8544_TEMP_MASK){
						++temp;
						settemp();
					}
				break;
				case 't':
					if(temp > 0){
						--temp;
						settemp();
					}
				break;
				default:
					wb = pcd8544_putch(rb);
					if(wb == rb){
						pcd8544_roll_screen();
						wb = pcd8544_putch(rb);
					}
					if(wb != rb){
						UART_send_byte(rb);
						if(wb == '\n') UART_send_byte('\n');
					}
			}
		}
	}while(1);
}

void msleep(U16 pause){
	unsigned long T = Global_time + pause;
	while(T > Global_time);
}
