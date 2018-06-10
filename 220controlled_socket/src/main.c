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



int main() {
    unsigned long Tmeas = 0L;
    U16 curMin = 0xffff, curMax = 0, curRange = 0; // min, max & range measured of current in ADU
    U8 rb, ch;
    U16 curval, npts = 0;
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
                ++npts;
                // measure max & min values
                curval = opt_med9();
                if(curMax < curval) curMax = curval;
                if(curMin > curval) curMin = curval;
            }
            ADC_ready = 0;
            ADC_CR1 = 0x71;
        }
        if(Global_time - Tmeas > 199){ // 10 periods left, make current measurement
            Tmeas = Global_time;
            npts = 0;
            if(curMax) curRange = curMax - curMin;
            curMax = 0; curMin = 0xffff;
        }

        if(uart_read_byte(&rb)){ // buffer isn't empty
            switch(rb){
                case 'h': // help
                case 'H':
                    uart_write( "\nPROTO:\n"
                                "c/C - check in0/1\n"
                                "I - show current ampl. (ADU)\n"
                                "s/S - activate out0/1\n"
                                "r/R - deactivate out0/1\n"
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
                case 's':
                case 'S':
                    ch = (rb == 's') ? '0' : '1';
                    uart_write("Out");
                    uart_send_byte(ch);
                    uart_write("=1\n");
                    if(rb == 's') SET_OUT0();
                    else SET_OUT1();
                break;
                case 'r':
                case 'R':
                    ch = (rb == 'r') ? '0' : '1';
                    uart_write("Out");
                    uart_send_byte(ch);
                    uart_write("=0\n");
                    if(rb == 'r') RESET_OUT0();
                    else RESET_OUT1();
                break;
            }
        }
    }while(1);
}


