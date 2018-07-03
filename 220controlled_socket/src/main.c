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
U16 lastMax = 0, lastMin = 0xffff; // min & max of current in ADU
U16 npts = 0; // number of ADC measurement points

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

// show current values of in/out
static void show_stat(U8 cmd){
    U8 ch;
    switch(cmd){
        case 'a':
        case 'A':
            uart_write("TRIAC0=");
            ch = CHK_TRIAC0() ? '1' : '0';
        break;
        case 'b':
        case 'B':
            uart_write("TRIAC1=");
            ch = CHK_TRIAC1() ? '1' : '0';
        break;
        case 'c':
        case 'C':
            uart_write("In");
            ch = (cmd == 'c') ? '0' : '1';
            uart_send_byte(ch);
            uart_send_byte('=');
            if(cmd == 'c') ch = CHK_IN0() ? '1' : '0';
            else ch = CHK_IN1() ? '1' : '0';
        break;
        case 'i':
        case 'I':
            uart_write("ADCVALUE=");
            printUint((U8*)&ADC_value, 2);
            uart_write("ADCMAX=");
            printUint((U8*)&lastMax, 2);
            uart_write("ADCMIN=");
            printUint((U8*)&lastMin, 2);
            uart_write("NPTS=");
            printUint((U8*)npts, 2);
            return;
        break;
        case 'k':
        case 'K':
            uart_write("PKEY1=");
            ch = CHK_PKEY1() ? '1' : '0';
        break;
        case 'l':
        case 'L':
            uart_write("NKEY1=");
            ch = CHK_NKEY1() ? '1' : '0';
        break;
        case 'm':
        case 'M':
            uart_write("NKEY2=");
            ch = CHK_NKEY2() ? '1' : '0';
        break;
        case 'n':
        case 'N':
            uart_write("OUT0=");
            ch = CHK_OUT0() ? '1' : '0';
        break;
        case 'o':
        case 'O':
            uart_write("OUT1=");
            ch = CHK_OUT1() ? '1' : '0';
        break;
        case 'y':
        case 'Y':
            uart_write("RELAY0=");
            ch = CHK_RELAY0() ? '1' : '0';
        break;
        case 'z':
        case 'Z':
            uart_write("RELAY1=");
            ch = CHK_RELAY1() ? '1' : '0';
        break;
        default:
            return;
    }
    uart_send_byte(ch);
    newline();
}

int main() {
    unsigned long Tmeas = 0L; // I measurement time
    U16 curMin = 0xffff, curMax = 0; // min & max of current in ADU
    U8 rb;
    U16 curval, nmeas = 0;
    const char * const allstatcmds = "abcCiklmnoyz";
    const char *ch;
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
            //ADC_CR1 = 0x71; // start measurement again
        }
        if(Global_time - Tmeas > 199){ // 10 periods past, make current measurement
            Tmeas = Global_time;
            npts = nmeas;
            nmeas = 0;
            lastMax = curMax;
            lastMin = curMin;
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

        if(uart_read_cmd(&rb)){ // buffer isn't empty
            switch(rb){
                case 'a': // turn ON triac0
                    SET_TRIAC0();
                break;
                case 'A': // turn OFF triac0
                    RESET_TRIAC0();
                break;
                case 'b': // turn ON triac1
                    SET_TRIAC1();
                break;
                case 'B': // turn OFF triac1
                    RESET_TRIAC1();
                break;
                case 'c': // check IN0
                case 'C': // check IN1
                case 'I': // current amplitude in ADU
                case 'i': // --//--
                break; // this is OK: show later
                case 'k': // activate PKEY1
                    SET_PKEY1();
                case 'K': // deactivate PKEY1
                    RESET_PKEY1();
                break;
                case 'l': // activate NKEY1
                    SET_NKEY1();
                break;
                case 'L': // deactivate NKEY1
                    RESET_NKEY1();
                break;
                case 'm': // activate NKEY2
                    SET_NKEY2();
                break;
                case 'M': // deactivate NKEY2
                    RESET_NKEY2();
                break;
                case 'n': // activate OUT0
                    SET_OUT0();
                break;
                case 'N': // deactivate OUT0
                    RESET_OUT0();
                break;
                case 'o': // activate OUT1
                    SET_OUT1();
                break;
                case 'O': // deactivate OUT1
                    RESET_OUT1();
                break;
                case 's': // all statistics
                case 'S':
                    ch = allstatcmds;
                    while(*ch){
                        show_stat(*ch++);
                    }
                break;
                case 'y': // relay 0 ON
                    if(CHK_TRIAC0()) SET_RELAY0();
                    else{
                        SET_TRIAC0();
                        Relay0 = Global_time;
                        if(!Relay0) Relay0 = 1;
                        rb = 'a'; // change value to display triac state
                    }
                break;
                case 'Y': // relay 0 OFF
                    if(CHK_TRIAC0()){ // triac ON - turn it off after small delay
                        Triac0 = Global_time;
                        if(!Triac0) Triac0 = 1;
                    }
                    RESET_RELAY0();
                break;
                case 'z': // relay 1 ON
                    if(CHK_TRIAC1()) SET_RELAY1();
                    else{
                        SET_TRIAC1();
                        Relay1 = Global_time;
                        if(!Relay1) Relay1 = 1;
                        rb = 'b';
                    }
                break;
                case 'Z': // relay 1 OFF
                    if(CHK_TRIAC1()){
                        Triac1 = Global_time;
                        if(!Triac1) Triac1 = 1;
                    }
                    RESET_RELAY1();
                break;
                default:
                    uart_write( "\nPROTO:\n"
                                "a/A - turn on/off triac0\n"
                                "b/B - turn on/off triac1\n"
                                "c/C - check in0/1\n"
                                "i/I - show current ampl. (ADU)\n"
                                "k/K - set/reset PKEY1\n"
                                "l/L - set/reset NKEY1\n"
                                "m/M - set/reset NKEY2\n"
                                "n/o - activate out0/1\n"
                                "N/O - deactivate out0/1\n"
                                "s/S - show all statistic\n"
                                "y/Y - turn on/off relay0\n"
                                "z/Z - turn on/off relay1\n"
                    );
                    continue;
            }
            show_stat(rb);
        }
    }while(1);
}



/*
 * errors:
 * IN1 = 0 - need 2 add ext.PU (no int)
 * ADCMAX=99, ADCMIN=99, NPTS=25706
 * k/K pkey1=0 - need 2 add ext.PU (true opendrain)
  */
