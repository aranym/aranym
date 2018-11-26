/*
 * mfp.cpp - MFP emulation
 *
 * Copyright (c) 2001-2005 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 *
 * Copied almost bit-by-bit from STonC's mfp.c (thanks, Laurent!)
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "mfp.h"

#define DEBUG 0
#include "debug.h"

MFP_Timer::MFP_Timer(int value)
{
	name = 'A' + value;
	reset();
}

void MFP_Timer::reset()
{
	control = start_data = current_data = 0;
	state = false;
}

bool MFP_Timer::isRunning()
{
	return ((control & 0x0f) > 0);
}

void MFP_Timer::setControl(uint8 value)
{
	control = value & 0x0f;
	if (value & 0x10)
		state = false;
	// D(bug("Set MFP Timer%c control to $%x", name, value));
}

uint8 MFP_Timer::getControl()
{
	return control | (state << 5);
}

void MFP_Timer::setData(uint8 value)
{
	// D(bug("Set MFP Timer%c data to %d", name, value));
	start_data = value;
	if (! isRunning())
		current_data = value;
}

void MFP_Timer::resetCounter()
{
	// D(bug("reset of Timer%c", name));
	if (isRunning()) {
		state = true;
		current_data = start_data;
	}
}

uint8 MFP_Timer::getData()
{
	// D(bug("get MFP Timer%c data = %d", name, current_data));

	if (isRunning() && current_data > 2)
		current_data--;		// hack to overcome microseconds delays in TOS (e.g. at $E02570)

	return current_data;
}

int MFP_Timer::compute_timer_freq()
{
#define MFP_FREQ	2457600UL
	int freq;
	switch(control) {
		case 1: freq = MFP_FREQ /  4; break;
		case 2: freq = MFP_FREQ / 10; break;
		case 3: freq = MFP_FREQ / 16; break;
		case 4: freq = MFP_FREQ / 50; break;
		case 5: freq = MFP_FREQ / 64; break;
		case 6: freq = MFP_FREQ /100; break;
		case 7: freq = MFP_FREQ /200; break;
		default: freq = MFP_FREQ / 64; break; // TOS default
	}
	if (start_data)
		freq = freq / start_data;
	else
		freq = freq / 192; // TOS default

	return freq;
}

/*************************************************************************/

MFP::MFP(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	reset();
}

#define MFP_VR_VECTOR 0xF0 
#define MFP_VR_SEI 0x08         /* Software end of interrupt, 
                                need software cancelling in inservice ? */
#define MFP_VR_AEI 0x00         /* Automatic end of interrupt,
                                ??? */
void MFP::reset()
{
	input = 0xff;
	GPIP_data = input;
	vr = 0x0100;
	active_edge = 0;
	data_direction = 0;
	irq_enable = 0;
	irq_pending = 0;
	irq_inservice = 0;
	irq_mask = 0;
	timerCounter = 0;

	A.reset();
	B.reset();
	C.reset();
	D.reset();
}

uint8 MFP::handleRead(memptr addr)
{
	addr -= getHWoffset();
	if (addr > getHWsize())
		return 0;	// unhandled

	uint8 value;
	switch(addr) {
		case 0x01:	value = (GPIP_data & ~ 0x21) | getYAMAHA()->parallel->getBusy();
					break;

		case 0x03:	value = active_edge;
					break;

		case 0x05:	value = data_direction;
					break;

		case 0x07:	value = irq_enable >> 8;
					break;

		case 0x09:	value = irq_enable;
					break;

		case 0x0b:	value = 0x20; //(irq_pending >> 8) | (tA->getControl() & 0x10);	// finish
					break;

		case 0x0d:	// D(bug("Read: TimerC IRQ %s pending", (irq_pending & 0x20) ? "" : "NOT"));
					value = irq_pending;
					break;

		case 0xf:	value = irq_inservice >> 8;
					break;

		case 0x11:	// D(bug("Read: TimerC IRQ %s in-service", (irq_inservice & 0x20) ? "" : "NOT"));
					value = irq_inservice;
					break;
						
		case 0x13:	value = irq_mask >> 8;
					break;
						
		case 0x15:	// D(bug("Read: TimerC IRQ %s masked", (irq_mask & 0x20) ? "" : "NOT"));
					value = irq_mask;
					break;
						
		case 0x17:	value = vr;
					break;

		case 0x19:	value = A.getControl();
					break;

		case 0x1b:	value = B.getControl();
					break;

		case 0x1d:	value = (C.getControl() << 4) | D.getControl();
					break;

		case 0x1f:	value = A.getData();
					break;

		case 0x21:	value = B.getData();
					break;

		case 0x23:	value = C.getData();
					break;
						
		case 0x25:	value = D.getData();
					break;
						
		case 0x2d:	value = 0x80;	// for Linux/m68k
					break;

		default: value = 0;
	};
	D(bug("Reading MFP data from %04lx = %d ($%02x) at %06x", addr, value, value, showPC()));
	return value;
}

void MFP::set_active_edge(uint8 value)
{
static int map_gpip_to_ier[8] = {0, 1, 2, 3, 6, 7, 14, 15};	
  /* AER : 1=Rising (0->1), 0=Falling (1->0) */
  
  /* [mfp.txt]
   * The edge bit is simply one input to an exclusive-or
   * gate, with the other input coming from the input buffer and the output going
   * to a 1-0 transition detector. Thus, depending upon the state of the input,
   * writing the AER can cause an interrupt-producing transition
   */
  /* this means : a = input xor aer, and interrupt when a 1->0.
   *  ____before____ ______after_____ ___result___
   *  input  aer  a   input  val  a    interrupt
   *    0     0   0     0     1   1      
   *    1     0   1     1     1   0       yes
   *    0     1   1     0     0   0       yes
   *    1     1   0     1     0   1      
   */
	int i, j;
	for(j = 0, i = 1; j < 8; j++, i <<= 1) {
		if( ((active_edge & i) != (value & i))        /* if AER changes */
				&& (! (data_direction & i))               /* for input lines */
				&& ((active_edge & i) != (input & i))) {
			IRQ(map_gpip_to_ier[j], 1);
		}
	}
 
	active_edge = value;
}

void MFP::handleWrite(memptr addr, uint8 value) {
	addr -= getHWoffset();
	if (addr > getHWsize())
		return;	// unhandled

	D(bug("Writing MFP data to %04lx = %d ($%02x) at %06x", addr, value, value, showPC()));
	switch(addr) {
		case 0x01:	//GPIP_data = value;
					GPIP_data &= ~data_direction;
					GPIP_data |= value & data_direction;
					D(bug("New GPIP=$%x from PC=$%x", GPIP_data, showPC()));
					break;

		case 0x03:	set_active_edge(value);
					break;

		case 0x05:	data_direction = value;
					GPIP_data &= data_direction;
					GPIP_data |= input & (~data_direction);
					D(bug("GPIP Data Direction set to %02x", value));
					break;

		case 0x07:	irq_enable = (irq_enable & 0x00ff) | (value << 8);
					// cancel any pending interrupts, but do not alter ISR
					irq_pending &= irq_enable;
					// mfp_check_timers_ier();
					break;

		case 0x09:
#ifdef DEBUG_IER
					if ((irq_enable ^ value) & 0x20) {
						D(bug("Write: TimerC IRQ %sabled", (value & 20) ? "en" : "dis"));
					}
					if ((irq_enable ^ value) & 0x40) {
						D(bug("Write: IKBD IRQ %sabled", (value & 40) ? "en" : "dis"));
					}
#endif /* DEBUG */
					irq_enable = (irq_enable & 0xff00) | value;
					// cancel any pending interrupts, but do not alter ISR
					irq_pending &= irq_enable;
					// mfp_check_timers_ier();
					break;

  /* [mfp.txt]
   * IPRA and IPRB are also writeable and a pending interrupt can be cleared
   * without going through the acknowledge sequence by writing a zero to the
   * appropriate bit. This allows any one bit to be cleared, without altering any
   * other bits, simply by writing all ones except for the bit position to be
   * cleared on IPRA or IPRB.
   */
 
		case 0x0b:	irq_pending &= (value << 8) | 0xff;
  /* if no unmasked interrupt pending any more, there's no point in 
   * keeping F_MFP active 
   */
				if ((irq_pending & irq_mask) == 0) {
					TriggerMFP(false);
				}
				break;

		case 0x0d:
#ifdef DEBUG_IPR
					if ((irq_pending ^ value) & 0x20) {
						D(bug("Write: TimerC IRQ %s pending", (value & 20) ? "" : "NOT"));
					}
#endif /* DEBUG */
					irq_pending &= 0xff00 | value;
				if ((irq_pending & irq_mask) == 0) {
					TriggerMFP(false);
				}

					break;
 
  /* [mfp.txt]
   * Only a zero may be written into any bit of ISRA and ISRB; thus the 
   * in-service may be cleared in software but cannot be set in software.
   */
		case 0xf:	irq_inservice &= (value << 8) | 0xff;
					if (irq_pending & irq_mask) {
						// unmasked interrupt pending, signal it to cpu
						TriggerMFP(true);
					}
					break;

		case 0x11:
#ifdef DEBUG_ISR
					if ((irq_inservice ^ value) & 0x20) {
						D(bug("Write: TimerC IRQ %s in-service at %08x", (value & 20) ? "" : "NOT", showPC()));
					}
#endif /* DEBUG */
					irq_inservice &= 0xff00 | value;
					if (irq_pending & irq_mask) {
						// unmasked interrupt pending, signal it to cpu
						TriggerMFP(true);
					}
					break;
					
		case 0x13:	irq_mask = (irq_mask & 0x00ff) | (value << 8);
					TriggerMFP(irq_pending & irq_mask);
					break;
					
		case 0x15:
#ifdef DEBUG_IMR
					if ((irq_mask ^ value) & 0x20) {
						D(bug("Write: TimerC IRQ %s masked", (value & 20) ? "" : "NOT"));
					}
#endif /* DEBUG */
					irq_mask = (irq_mask & 0xff00) | value;
					TriggerMFP(irq_pending & irq_mask);
					break;
					
		case 0x17:	vr = value;
					if (vr & MFP_VR_SEI) {
						// software end-of-interrupt mode
					}
					else {
						// automatic end-of-interrupt mode : reset inservice bits?
						irq_inservice = 0;
						// try to pass a vector
						TriggerMFP(irq_pending & irq_mask);
					}
					break;

		case 0x19:	A.setControl(value);
					break;

		case 0x1b:	B.setControl(value);
					break;

		case 0x1d:	C.setControl(value >> 4);
					D.setControl(value & 0x0f);
					break;

		case 0x1f:	A.setData(value);
					break;

		case 0x21:	B.setData(value);
					break;

		case 0x23:	C.setData(value);
					break;
					
		case 0x25:	D.setData(value);
					break;
					
		case 0x27:
		case 0x29:
		case 0x2b:
		case 0x2d:
		case 0x2f:
		default:
			break;
	};
}

/*
 * setGPIPbit sets input port bits (peripheral interrupt request)
 */
void MFP::setGPIPbit(int mask, int value)
{
	static int map_gpip_to_ier[8] = {0, 1, 2, 3, 6, 7, 14, 15};	
	mask &= 0xff;

	int oldGPIP = GPIP_data;
	input &= ~mask;
	input |= (value & mask);
	/* if output port, no interrupt */
	mask &= ~data_direction;

	GPIP_data &= ~mask;
	GPIP_data |= (value & mask);
	GPIP_data &= 0xFF;
	D(bug("setGPIPbit($%x, $%x): old=$%x, new=$%x", mask, value, oldGPIP, GPIP_data));
	int i, j;
	for(j = 0, i = 1; j < 8; j++, i <<= 1) {
		if ((oldGPIP & i) != (GPIP_data & i)) {
			D(bug("setGPIPbit: i=$%x, irq_enable=$%x, old=$%x, new=$%x", i, irq_enable, oldGPIP, GPIP_data));
			if (active_edge & i) {
				/* interrupt when going from 0 to 1  */
				if (oldGPIP & i)
					continue;
			}
			else {
				/* interrupt when going from 1 to 0  */
				if (GPIP_data & i)
					continue;
			}
			D(bug("calling IRQ(%d)->%d", j, map_gpip_to_ier[j]));
			IRQ(map_gpip_to_ier[j], 1);
		}
	}	
}

/*
        Ask for an interrupt. Depending on diverse factors, an
        interrupt will eventually occur the next time 
        MFP:doInterrupt() is called. 

        0:  IO port 0 : Printer Busy
        1:  IO port 1 : Printer Acknowledge (RS232 Carrier detect on ST)
        2:  IO port 2 : MIDI Interrupt (RS232 CTS on ST)
        3:  IO port 3 : DSP Interrupt (Blitter on ST)
        4:  timer D
        5:  timer C
        6:  IO port 4 : ACIA (IKBD/MIDI)
        7:  IO port 5 : FDC/SCSI/IDE
        8:  Timer B   : Display Enable
        9:  Send error
        10: Send buffer empty
        11: Receive error
        12: Receive buffer full
        13: Timer A   : DMA Sound Interrupt
        14: IO port 6 : RS232 ring
        15: IO port 7 : DMA Sound Interrupt (monochrome detect on ST)
*/

void MFP::IRQ(int int_level, int count)
{
	int i = 1 << int_level;

	if (int_level == 5) C.resetCounter(); // special hack for TimerC

        /* interrupt enabled ? */
        if( (irq_enable & i) == 0) {
				// panicbug("interrupt %d not enabled", int_level);
                return;
		}

	if (int_level == 5) timerCounter += count; // special hack for TimerC

        /* same interrupt already pending */
        if( (irq_pending & i) ) {
				D(bug("same interrupt %d already pending", int_level));
                return;
		}
        
        
        /* ok, we will request an interrupt to the mfp */
#if 0
        if(int_level) {
          panicbug( "mfp ask interrupt %d\n", int_level);
        }
#endif
        irq_pending |= i;
        /* interrupt masked ? */
        if( ! (irq_mask & i) ) {
                /* irq_pending set but no irq : stop here */
               D(bug("irq_pending set but no irq"));
          return;
        }
                /* highest priority ? */
        if(irq_inservice > i) {
                /* no, do nothing (the mfp will check when the current
                  interrupt resumes, i.e. when irq_inservice is being cleared). */
				D(bug("irq_inservice has higher priority"));
                return;
        }
        /* say we want to interrupt the cpu */  
        TriggerMFP(true);
        /* when UAE CPU finds this flag set, according to the current IPL,
          the function MFP::doInterrupt() will be called. This function 
          will then decide what to do, and will treat the request that 
          has the highest priority at that time. 
        */
}

// return : vector number, or zero if no interrupt
int MFP::doInterrupt()
{
        int j, vector;
        unsigned i;
        
        /* what's happening here ? */
#if 0
        panicbug( "starting mfp_do_interrupt\n");
        panicbug( "ier %04x, ipr %04x, isr %04x, imr %04x\n",
               irq_enable, irq_pending, irq_inservice, irq_mask);
#endif

        /* any pending interrupts? */
        for(j = 15, i = 0x8000; i; j--, i>>=1) {
                if(irq_pending & i & irq_mask)
                        break;
        }
        if(i == 0) {
          /* this shouldn't happen :-) */
                panicbug( "mfp_do_interrupt called with no pending interrupt\n");
                TriggerMFP(false);
                return 0;
        }
        if(irq_inservice >= i) {
                /* Still busy. We shouldn't come here. */
				
				/*	Running MagiC this happens all the time. 
					The panicbug log makes the whole console output unusable.
					Thus it is commented out.
                
				panicbug( "mfp_do_interrupt called when "
                        "another higher priority interrupt is running\n");
				*/
			
                TriggerMFP(false);
                return 0;
        }
        
        /* ok, do the interrupt, i.e. "pass the vector". */
        vector = (vr & MFP_VR_VECTOR) + j;
	
	if (j != 5 || --timerCounter <= 0) {	// special hack for TimerC
        irq_pending &= ~i;
        TriggerMFP(false);
	}

        if(vr & MFP_VR_SEI) {
                /* software mode of interrupt : irq_inservice will remain set until
                   explicitely cleared by writing on it */
                irq_inservice |= i;
        } else {
                /* automatic mode of interrupt : irq_inservice automatically cleared
                   when the interrupt starts (which is now). In this case,
                   we must keep the flag raised if another unmasked 
                   interrupt remains pending, since irq_inservice will not tell us 
                   to re-raise the flag.
                */
                if((irq_pending & irq_mask)) {
                  /* if other unmasked interrupt pending, keep the flag */
                  TriggerMFP(true);
                }
        }

#if 0
        panicbug( "MFP::doInterrupt : vector %d\n", vector);
#endif
        return vector;
}

int MFP::timerC_ms_ticks()
{
	int freq = C.compute_timer_freq();

	return 1000 / freq;
}
