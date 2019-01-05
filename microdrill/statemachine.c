/*
 * This file is part of the microdrill project.
 * Copyright 2019 Edward V. Emelianov <edward.emelianoff@gmail.com>, <eddy@sao.ru>.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "main.h"
#include "statemachine.h"
#include "stepper.h"

drill_state curstate = DRL_RELAX; // spindle state
U8 drill_maxspeed = 100;

static pot_state potstate = POT_RELAX; // potentiometer state
stepper_state stpstate = STPR_STOPPED, ostpstate = STPR_STOPPED; // stepper

static U8 set_to_zero = 0; // flag showing that motor is in state of zero point setting up

// bitfield structure for buttons
#define FOOTSBIT    (1<<0)
#define INP0BIT     (1<<1)
#define INP1BIT     (1<<2)
#define TRAYTOPBIT  (1<<3)
#define TRAYBTMBIT  (1<<4)
#define TRAYBTN1BIT (1<<5)
#define TRAYBTN2BIT (1<<6)

void check_buttons(){
    static U8 old_buttons_state = 0xff; // default buttons state - none pressed
#ifdef EBUG
    U8 pr = 0; // 0 - nothing, 1 - press, 2 - release
#endif
	U8 btn_state = 0, btns_changed, twobuttons = 0;
    if(FOOTSWITCH) btn_state |= FOOTSBIT;
    if(INPUT0) btn_state |= INP0BIT;
    if(INPUT1) btn_state |= INP1BIT;
    if(TRAY_TOP_SW) btn_state |= TRAYTOPBIT;
    if(TRAY_BTM_SW) btn_state |= TRAYBTMBIT;
    if(TRAY_BTN1) btn_state |= TRAYBTN1BIT;
    if(TRAY_BTN2) btn_state |= TRAYBTN2BIT;
    if(btn_state == old_buttons_state) return; // none changed
	btns_changed = btn_state ^ old_buttons_state; // XOR -> 1 on changed states
    if(btns_changed & FOOTSBIT){
        DBG("Footswitch");
        if(btn_state & FOOTSBIT){ // released -> move drill up (if it works)
            if(curstate != DRL_RELAX) move_fast(MOVEUP_STEPS);
#ifdef EBUG
            pr = 2;
#endif
        }else{ // pressed
            if((btn_state & TRAYBTN1BIT) == 0 || (btn_state & TRAYBTN2BIT) == 0 ){ // move tray up/down
                stop_motor();
                DRILL_OFF();
                if((btn_state & TRAYBTMBIT) == 0){ // move up when tray is down
                    TRAY_UP();
                }else{ // else move DOWN
                    TRAY_DOWN();
                }
            }else{ // turn ON drill & move motor down
                if(curstate == DRL_RELAX) DRILL_ON();
                move_motor(-FULL_SCALE_STEPS);
            }
#ifdef EBUG
            pr = 1;
#endif
        }
    }
    if(btns_changed & INP0BIT){
        DBG("Input0");
        if(btn_state & INP0BIT){ // released
#ifdef EBUG
            pr = 2;
#endif
            ;
        }else{ // pressed
#ifdef EBUG
            pr = 1;
#endif
        }
    }
    if(btns_changed & INP1BIT){
        DBG("Input1");
        if(btn_state & INP1BIT){ // released
#ifdef EBUG
            pr = 2;
#endif
            ;
        }else{ // pressed
#ifdef EBUG
            pr = 1;
#endif
        }
    }
    if(btns_changed & TRAYTOPBIT){
        DBG("Tray top");
        if(btn_state & TRAYTOPBIT){ // released
#ifdef EBUG
            pr = 2;
#endif
            ;
        }else{ // pressed
            TRAY_STOP();
#ifdef EBUG
            pr = 1;
#endif
        }
    }
    if(btns_changed & TRAYBTMBIT){
        DBG("Tray bottom");
        if(btn_state & TRAYBTMBIT){ // released
#ifdef EBUG
            pr = 2;
#endif
            ;
        }else{ // pressed
            TRAY_STOP();
#ifdef EBUG
            pr = 1;
#endif
        }
    }
    if(btns_changed & TRAYBTN1BIT){ // left button -> potentiometer change drill speed
        DBG("Tray button left");
        if(btn_state & TRAYBTN1BIT){ // released
            potstate = POT_RELAX;
#ifdef EBUG
            pr = 2;
#endif
            ;
        }else{ // pressed
            if((btn_state & TRAYBTN2BIT) == 0){ // press 2 buttons together
                potstate = POT_RELAX;
                DBG(" with right");
                twobuttons = 1;
            }else potstate = POT_DRLSPEED;
#ifdef EBUG
            pr = 1;
#endif
        }
    }
    if(btns_changed & TRAYBTN2BIT){ // right button -> potentiometer change stepper speed
        DBG("Tray button right");
        if(btn_state & TRAYBTN2BIT){ // released
            potstate = POT_RELAX;
#ifdef EBUG
            pr = 2;
#endif
            ;
        }else{ // pressed
            if((btn_state & TRAYBTN1BIT) == 0){ // press 2 buttons together
                potstate = POT_RELAX;
                DBG(" with left");
                twobuttons = 1;
            }else potstate = POT_STPSPEED;
#ifdef EBUG
            pr = 1;
#endif
        }
    }
#ifdef EBUG
    switch(pr){
        case 1:
            uart_write(" pressed\n");
        break;
        case 2:
            uart_write(" released\n");
        break;
        default:
    }
#endif
    if(twobuttons){ // both buttons pressed together -> turn OFF drill
        stop_motor();
        DRILL_OFF();
    }
#if 0
	// check for footswitch
    if(!FOOTSWITCH && !TRAY_BTM_SW){ // move only when tray is down!
		if(!FOOTSW_TEST(btn_state)){ // pedal switch pressed - connect to ground!
			if(curstate != DRL_WORK){
				DRILL_ON();
			}
			add_steps(-5000); // this is a trick to move more than stage allows
			uart_write("move down\n");
		}else{
			if(set_to_zero){
				set_to_zero = 0;
				stop_motor();
			}else{
				add_steps(-5000); // return to previous state (this function moves RELATIVELY)
				uart_write("move up\n");
			}
		}
	}
    // check for tray endswitches. We don't care for their off state, so only check ON
	if(TRAYSW_TEST(btns_changed) && TRAYSW_PRSD(btn_state)){
		uart_write("tray stop\n");
		TRAY_STOP(); // stop tray motor in any moving direction
		if(!TRAY_BTM_SW) stp_pause_resume(); // restore stepper speed in down position
	}
	// check for user buttons pressed (the same - only pressed)
	if(BTN12_TEST(btns_changed) && !BTN12_TEST(btn_state)){ // pressed both buttons
		uart_write("move tray ");
		DRILL_OFF();
		if(!TRAY_TOP_SW){ // tray is up -> move it down & stepper up
			uart_write("down\n");
			move_motor(-FULL_SCALE_STEPS);
			while(Nsteps); // wait until it moves
			TRAY_DOWN();
		}else{ // move tray up & stepper down
			uart_write("up\n");
			set_stepper_speed(100); // move as faster as possible
			move_motor(FULL_SCALE_STEPS);
			while(Nsteps); // wait until it moves
			TRAY_UP();
		}
	}else if(BTN1_TEST(btns_changed) && !BTN1_TEST(btn_state)){ // btn1
		uart_write("button 1\n");
		set_stepper_speed(100);
		move_motor(-FULL_SCALE_STEPS);
		while(Nsteps); // wait until it moves
		set_stepper_speed(10);
		set_to_zero = 1;
	}else if(BTN2_TEST(btns_changed) && !BTN2_TEST(btn_state)){ // btn2
		uart_write("button 2\n");
        potstate = POT_DRLSPEED;
	}
#endif
	old_buttons_state = btn_state;
}

// return absolute difference of two values
static vdiff(U16 a, U16 b){
    if(a > b) return a - b;
    else return b - a;
}

static inline void procVpot(){
    static U16 oVpot = 0;
    U32 spd;
#ifdef EBUG
    U8 p = 0;
#endif
    if(vdiff(oVpot, Vpot) < ADC_THRESHOLD) return;
    // calculate speed in %%
    spd = 101L * Vpot;
    spd >>= 10;
    switch(potstate){
        case POT_DRLSPEED: // Vpot is spindle speed
            if(spd != drill_maxspeed){
                drill_maxspeed = spd;
                DBG("Drill: ");
#ifdef EBUG
                p = 1;
#endif
            }
        break;
        case POT_STPSPEED: // Vpot is stepper speed
            if(Stepper_speed != spd){
                set_stepper_speed(spd);
                DBG("Stepper: ");
#ifdef EBUG
                p = 1;
#endif
            }
        break;
        default:
    }
#ifdef EBUG
    if(p){
        printUint((U8*)&spd, 4);
    }
#endif
    oVpot = Vpot;
}
static inline void procVcap(){
    static U16 oVcap = 0;
    if(vdiff(oVcap, Vcap) < ADC_THRESHOLD) return;
/*
#ifdef EBUG
    uart_write("Vcap: ");
    printUint((U8*)&Vcap, 2);
    newline();
#endif
*/
    oVcap = Vcap;
}
static inline void procVshunt(){
    static U16 oVshunt = 0;
    if(vdiff(oVshunt, Vshunt) < ADC_THRESHOLD) return;
    if(Vshunt > MAX_DRILL_CURRENT){ // big current -> stepper should do a pause
        DBG("big current: ");
        printUint((U8*)&Vshunt, 2);
        stp_pause_resume();
    }else if(Vshunt < NORMAL_DRILL_CURRENT){ // all OK, move further
        if(stpstate == STPR_PAUSED){
            DBG("resume stepper\n");
            stp_pause_resume();
        }
    }
    oVshunt = Vshunt;
}
/**
 * @brief process_ADC - check ADC values (Vpot Vcap Vshunt) for further actions
 */
static inline void process_ADC(){
    static U16 val = 0; // mean
    static U8 ctr = 0; // counter in val
    U16 v;
	U8 chnl;
    if(!(ADC_CSR & 0x80)) return; // no EOC flag
    v = ADC_DRL; // in right-alignment mode we should first read LSB
    chnl = ADC_CSR & 0x0f; // current channel converted
	v |= ADC_DRH << 8;
    val += v;
    if(++ctr == 10){
        val /= 10;
        switch(chnl){
            case 4: // Rpot
                Vpot = val;
                procVpot();
                chnl = 0x05; // clear irq flags & next will be Vcap
            break;
            case 5: // Vcap
                Vcap = val;
                procVcap();
                chnl = 0x0c; // clear irq flags & next will be motor shunt
            break;
            //case 12: // motor schunt
            default:
                Vshunt = val;
                procVshunt();
                chnl = 0x04; // clear irq flags & next will be potentiometer
        }
        val = 0;
        ctr = 0;
    }
    ADC_CSR = chnl; // clear EOC flag & change channel if need
    ADC_CR1 = 0x71; // turn on ADC after everything processed
}

void process_state(){
    process_ADC();
}

