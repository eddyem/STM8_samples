/*
 * blinky.c
 *
 * Copyright 2018 Edward V. Emelianoff <eddy@sao.ru>
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
#include "uart.h"
#include "interrupts.h"

U8 UART_rx_cmd; // command received
volatile U8 uart_ready = 0;// command ready flag

/**
 * Send one byte through UART
 * @param byte - data to send
 */
void uart_send_byte(U8 byte){
    while(!(UART1_SR & UART_SR_TXE)); // wait until previous byte transmitted
    UART1_DR = byte;
}

void uart_write(char *str){
    while(*str){
        while(!(UART1_SR & UART_SR_TXE));
        UART1_CR2 |= UART_CR2_TEN;
        UART1_DR = *str++;
    }
}

/**
 * Read one byte from Rx buffer
 * @param byte - where to store data read
 * @return 1 in case of non-empty buffer
 */
U8 uart_read_cmd(U8 *byte){
    if(!uart_ready) // buffer is empty
        return 0;
    *byte = UART_rx_cmd;
    uart_ready = 0;
    return 1;
}

void uart_init(){
    // PD5 - UART1_TX
    PORT(UART_PORT, DDR) |= UART_TX_PIN;
    PORT(UART_PORT, CR1) |= UART_TX_PIN;
// Configure UART
    // 8 bit, no parity, 1 stop (UART_CR1/3 = 0 - reset value)
    // 9600 on 16MHz: DIV=0x0693 -> BRR1=0x68, BRR2=0x03
    UART1_BRR1 = 0x68; UART1_BRR2 = 0x03;
    UART1_CR2 = UART_CR2_TEN | UART_CR2_REN | UART_CR2_RIEN; // Allow RX/TX, generate ints on rx
}
