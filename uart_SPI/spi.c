/*
 * spi.c
 *
 * Copyright 2016 Edward V. Emelianov <eddy@sao.ru, edward.emelianoff@gmail.com>
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
#include "spi.h"

// global variables for sending big buffer
static U16 sendbuflen = 0;
static U8 *sendbuf = NULL, *inbuff = NULL;

/*
 * SPI registers
 * SPI_CR1 (page 271): | LSBFIRST | SPE | BR[2:0] | MSTR | CPOL | CPHA |
 * BR: xxx - f/2^{xxx+1}
 *    01111100
 * SPI_CR2 (page 272): | BDM | BDOE | CRCEN | CRCNEXT | - | RXONLY | SSM | SSI |
 *    00000000
 * SPI_ICR (page 273): | TXIE | RXIE | ERRIE | WKIE | - | - | - | - |
 * SPI_SR (page 274):  | BSY | OVR | MODF | CRCERR | WKUP | - | TXE | RXNE |
 *   set it to zero before send new byte
 */

void spi_init(){
	// pins config: MISO is pullup in, MOSI & SCK are pp out
	PORT(SPI_PORT, DDR) |= SPI_MOSI_PIN | SPI_SCK_PIN;
	PORT(SPI_PORT, CR1) |= SPI_MOSI_PIN | SPI_SCK_PIN | SPI_MISO_PIN;
	// setup line: f/256, enable, master
	SPI_CR1 = SPI_CR1_SPE | SPI_CR1_BRMASK | SPI_CR1_MSTR;
	SPI_CR2 = 0;
}

/**
 * static routine waiting SPI free
 */
static void spi_wait(){
	while(!(SPI_SR & SPI_SR_TXE))
	while(SPI_SR & SPI_SR_BSY);
}

/**
 * send given data buffer
 * WARNING! Buffer should have livetime at least while !spi_buf_sent()
 * set _get == 1 to put received data into the same buffer
 */
void spi_send_buffer(U8 *buff, U16 len, U8 *inb){
	inbuff = inb;
	sendbuflen = len;
	sendbuf = buff;
	spi_wait();
	SPI_DR = *sendbuf;
	// interrupts: RXNE
	SPI_ICR = SPI_ICR_RXIE;
}

/**
 * send next byte from sendbuf (called by interrupt routine)
 */
void spi_send_next(){
	if(!sendbuf) return;
	if(inbuff){
		//while(!(SPI_SR & SPI_SR_RXNE));
		*inbuff++ = SPI_DR;
	}
	if(--sendbuflen == 0){
		sendbuf = NULL;
		SPI_ICR = 0;
		return;
	}
	SPI_DR = *(++sendbuf);
}

/**
 * blocking send 1 byte & return ansver
 */
U8 spi_send_byte(U8 data){
	spi_wait();
	SPI_ICR = 0;
	SPI_DR = data;
	while(!(SPI_SR & SPI_SR_RXNE));
	return SPI_DR;
}

/**
 * return 1 if there's no data in queue to send
 */
U8 spi_buf_sent(){
	if(!sendbuf) return 1;
	else return 0;
}
