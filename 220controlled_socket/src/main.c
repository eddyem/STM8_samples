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
#include "stm8s.h"
#include "hardware.h"
#include "interrupts.h"
#include "uart.h"

volatile unsigned long Global_time = 0L; // global time in ms
unsigned long Relay0 = 0L, Relay1 = 0L; // timer for relay ON (after triac is on)
unsigned long Triac0 = 0L, Triac1 = 0L; // timer for triac OFF (after relay is off)


U16 temp;
U8 pidx;
#define PIX_SORT(a,b) { if ((a)>(b)) PIX_SWAP((a),(b)); }
#define PIX_SWAP(a,b) { temp=(a);(a)=(b);(b)=temp; }
U16 p[9]; // buffer for median filtering
U16 opt_med9(){
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[1]) ; PIX_SORT(p[3], p[4]) ; PIX_SORT(p[6], p[7]) ;
    PIX_SORT(p[1], p[2]) ; PIX_SORT(p[4], p[5]) ; PIX_SORT(p[7], p[8]) ;
    PIX_SORT(p[0], p[3]) ; PIX_SORT(p[5], p[8]) ; PIX_SORT(p[4], p[7]) ;
    PIX_SORT(p[3], p[6]) ; PIX_SORT(p[1], p[4]) ; PIX_SORT(p[2], p[5]) ;
    PIX_SORT(p[4], p[7]) ; PIX_SORT(p[4], p[2]) ; PIX_SORT(p[6], p[4]) ;
    PIX_SORT(p[4], p[2]) ; return(p[4]) ;
}

static void triac_ONOFF(U8 cmd){
    U8 ch;
    uart_write("TRIAC");
    switch (cmd){
        case 'a': // turn ON triac0
        case 'A': // turn OFF triac0
            ch = (cmd == 'a') ? '1' : '0';
            uart_write("0=");
            uart_send_byte(ch);
            if(cmd == 'a') SET_TRIAC0();
            else RESET_TRIAC0();
        break;
        case 'b': // turn ON triac1
        case 'B': // turn OFF triac1
            ch = (cmd == 'a') ? '1' : '0';
            uart_write("1=");
            uart_send_byte(ch);
            if(cmd == 'b') SET_TRIAC1();
            else RESET_TRIAC1();
        break;
    }
    newline();
}

int main() {
    unsigned long Tmeas = 0L; // I measurement time
    U16 curMin = 0xffff, curMax = 0, curRange = 0; // min, max & range measured of current in ADU
    U8 rb, ch;
    U16 curval, npts = 0, nmeas = 0;
    hw_init();

    uart_init();

    // enable all interrupts
    enableInterrupts();

    // Loop
    do{
        if(ADC_ready){
            p[pidx] = ADC_value;
            if(++pidx == 9){
                pidx = 0;
                ++nmeas;
                // measure max & min values
                curval = opt_med9();
                if(curMax < curval) curMax = curval;
                if(curMin > curval) curMin = curval;
            }
            ADC_ready = 0;
            ADC_CR1 = 0x71;
        }
        if(Global_time - Tmeas > 199){ // 10 periods past, make current measurement
            Tmeas = Global_time;
            npts = nmeas;
            nmeas = 0;
            if(curMax) curRange = curMax - curMin;
            curMax = 0; curMin = 0xffff;
        }
        if(Relay0){
            if(Global_time - Relay0 > 100){
                Relay0 = 0;
                SET_RELAY0();
                uart_write("RELAY0=1\n");
            }
        }
        if(Relay1){
            if(Global_time - Relay1 > 100){
                Relay1 = 0;
                SET_RELAY1();
                uart_write("RELAY1=1\n");
            }
        }
        if(Triac0){
            if(Global_time - Triac0 > 100){
                Triac0 = 0;
                RESET_TRIAC0();
                uart_write("TRIAC0=0\n");
            }
        }
        if(Triac1){
            if(Global_time - Triac1 > 100){
                Triac1 = 0;
                RESET_TRIAC1();
                uart_write("TRIAC1=0\n");
            }
        }

        if(uart_read_byte(&rb)){ // buffer isn't empty
            switch(rb){
                case 'a': // turn ON triac0
                case 'A': // turn OFF triac0
                case 'b': // turn ON triac1
                case 'B': // turn OFF triac1
                    triac_ONOFF(rb);
                break;
                case 'h': // help
                case 'H':
                    uart_write( "\nPROTO:\n"
                                "a/A - turn on/off triac0\n"
                                "b/B - turn on/off triac1\n"
                                "c/C - check in0/1\n"
                                "I   - show current ampl. (ADU)\n"
                                "k/K - set/reset PKEY1\n"
                                "l/L - set/reset NKEY1\n"
                                "m/M - set/reset NKEY2\n"
                                "r/R - deactivate out0/1\n"
                                "s/S - activate out0/1\n"
                                "y/Y - turn on/off relay0\n"
                                "z/Z - turn on/off relay1\n"
                    );
                break;
                case 'I': // current amplitude in ADU
                    curval = curRange / 2;
                    uart_write("Imax(ADU)=");
                    printUint((U8*)&curval, 2);
                    uart_write(", Npts=");
                    printUint((U8*)npts, 2);
                    newline();
                break;
                case 'c': // check IN0
                case 'C': // check IN1
                    uart_write("In");
                    ch = (rb == 'c') ? '0' : '1';
                    uart_send_byte(ch);
                    uart_send_byte('=');
                    if(rb == 'c') ch = CHK_IN0() ? '1' : '0';
                    else ch = CHK_IN1() ? '1' : '0';
                    uart_send_byte(ch);
                    newline();
                break;
                case 's': // activate OUT0
                case 'r': // activate OUT1
                    ch = (rb == 's') ? '0' : '1';
                    uart_write("Out");
                    uart_send_byte(ch);
                    uart_write("=1\n");
                    if(rb == 's') SET_OUT0();
                    else SET_OUT1();
                break;
                case 'S': // deactivate OUT0
                case 'R': // deactivate OUT1
                    ch = (rb == 'S') ? '0' : '1';
                    uart_write("Out");
                    uart_send_byte(ch);
                    uart_write("=0\n");
                    if(rb == 'S') RESET_OUT0();
                    else RESET_OUT1();
                break;
                case 'k': // activate PKEY1
                case 'K': // deactivate PKEY1
                    uart_write("PKEY1=");
                    ch = (rb == 'k') ? '1' : '0';
                    uart_send_byte(ch);
                    if(rb == 'k') SET_PKEY1();
                    else RESET_PKEY1();
                    newline();
                break;
                case 'l': // activate NKEY1
                case 'L': // deactivate NKEY1
                    uart_write("NKEY1=");
                    ch = (rb == 'l') ? '1' : '0';
                    uart_send_byte(ch);
                    if(rb == 'l') SET_NKEY1();
                    else RESET_NKEY1();
                    newline();
                break;
                case 'm': // activate NKEY2
                case 'M': // deactivate NKEY2
                    uart_write("NKEY2=");
                    ch = (rb == 'm') ? '1' : '0';
                    uart_send_byte(ch);
                    if(rb == 'm') SET_NKEY2();
                    else RESET_NKEY2();
                    newline();
                break;
                case 'y': // relay 0 ON
                    if(CHK_TRIAC0()) SET_RELAY0();
                    else{
                        triac_ONOFF('a');
                        Relay0 = Global_time;
                        if(!Relay0) Relay0 = 1;
                    }
                break;
                case 'Y': // relay 0 OFF
                    SET_TRIAC0();
                    Triac0 = Global_time;
                    if(!Triac0) Triac0 = 1;
                    RESET_RELAY0();
                    uart_write("RELAY0=0\n");
                break;
                case 'z': // relay 1 ON
                    if(CHK_TRIAC1()) SET_RELAY1();
                    else{
                        triac_ONOFF('b');
                        Relay1 = Global_time;
                        if(!Relay1) Relay1 = 1;
                    }
                break;
                case 'Z': // relay 1 OFF
                    SET_TRIAC1();
                    Triac1 = Global_time;
                    if(!Triac1) Triac1 = 1;
                    RESET_RELAY1();
                    uart_write("RELAY1=0\n");
                break;
            }
        }
    }while(1);
}


