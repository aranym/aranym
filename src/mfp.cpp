/*
 * mfp.cpp - MFP emulation
 *
 * Copyright (c) 2001-2004 Petr Stehlik of ARAnyM dev team (see AUTHORS)
 *
 * Based on info gathered in STonX source code 
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
#include "memory.h"
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

/*************************************************************************/

MFP::MFP(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	reset();
}

void MFP::reset()
{
	GPIP_data = 0xff;
	vr = 0x0100;
	active_edge = 0;
	data_direction = 0;
	irq_enable = 0;
	irq_pending = 0;
	irq_inservice = 0;
	irq_mask = 0;
	automaticServiceEnd = 0;
	flags = 0;
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
		case 0x01:	value = (GPIP_data & ~ 0x21) | parallel.getBusy();
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
						
		case 0x17:	value = (vr & 0xf0 ) | (automaticServiceEnd ? 8 : 0);
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
	D(bug("Reading MFP data from %04lx = %d ($%02x) at %06x\n", addr, value, value, showPC()));
	return value;
}

void MFP::handleWrite(memptr addr, uint8 value) {
	addr -= getHWoffset();
	if (addr > getHWsize())
		return;	// unhandled

	D(bug("Writing MFP data to %04lx = %d ($%02x) at %06x\n", addr, value, value, showPC()));
	switch(addr) {
		case 0x01:	//GPIP_data = value;
					GPIP_data &= ~data_direction ;
					GPIP_data |= value & data_direction;
					D(bug("New GPIP=$%x from PC=$%x", GPIP_data, showPC()));
					break;

		case 0x03:	active_edge = value;
					break;

		case 0x05:	data_direction = value;
					D(bug("GPIP Data Direction set to %02x", value));
					break;

		case 0x07:	irq_enable = (irq_enable & 0x00ff) | (value << 8);
					break;

		case 0x09:
#if DEBUG_IER
					if ((irq_enable ^ value) & 0x20) {
						D(bug("Write: TimerC IRQ %sabled", (value & 20) ? "en" : "dis"));
					}
					if ((irq_enable ^ value) & 0x40) {
						D(bug("Write: IKBD IRQ %sabled", (value & 40) ? "en" : "dis"));
					}
#endif /* DEBUG */
					irq_enable = (irq_enable & 0xff00) | value;
					break;

		case 0x0b:	irq_pending = (irq_pending & 0x00ff) | (value << 8);
					break;

		case 0x0d:
#if DEBUG_IPR
					if ((irq_pending ^ value) & 0x20) {
						D(bug("Write: TimerC IRQ %s pending", (value & 20) ? "" : "NOT"));
					}
#endif /* DEBUG */
					irq_pending = (irq_pending & 0xff00) | value;
					break;

		case 0xf:	irq_inservice = (irq_inservice & 0x00ff) | (value << 8);
					break;

		case 0x11:
#if DEBUG_ISR
					if ((irq_inservice ^ value) & 0x20) {
						D(bug("Write: TimerC IRQ %s in-service at %08x", (value & 20) ? "" : "NOT", showPC()));
					}
#endif /* DEBUG */
					irq_inservice = (irq_inservice & 0xff00) | (irq_inservice & value);
					break;
					
		case 0x13:	irq_mask = (irq_mask & 0x00ff) | (value << 8);
					break;
					
		case 0x15:
#if DEBUG_IMR
					if ((irq_mask ^ value) & 0x20) {
						D(bug("Write: TimerC IRQ %s masked", (value & 20) ? "" : "NOT"));
					}
#endif /* DEBUG */
					irq_mask = (irq_mask & 0xff00) | value;
					break;
					
		case 0x17:	vr = value;
					automaticServiceEnd = (value & 0x08) ? true : false;
					// D(bug("MFP autoServiceEnd: %s", automaticServiceEnd ? "YES" : "NO"));
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
	static int map_gpip_to_ier[8] = {0, 1, 2, 3, 6, 7, 14, 15} ;	
	mask &= 0xff;
	int oldGPIP = GPIP_data;
	GPIP_data &= ~mask;
	GPIP_data |= (value & mask);
	D(bug("setGPIPbit($%x, $%x): old=$%x, new=$%x", mask, value, oldGPIP, GPIP_data));
	int i, j;
	for(j = 0, i = 1 ; j < 8 ; j++, i <<= 1) {
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
			IRQ(map_gpip_to_ier[j], 1) ;
		}
	}	
}

void MFP::IRQ(int irq, int count)
{
	switch(irq) {
		// printer BUSY interrupt
		case 0:	break;

		// TimerC 200 Hz interrupt
		case 5: C.resetCounter();
				if (irq_enable & 0x0020) {
					flags |= F_TIMERC;
					timerCounter += count;
					TriggerMFP(true);
				}
				break;

		// ACIA received data interrupt
		case 6: flags |= F_ACIA;
				TriggerMFP(true);
				break;
	}
}

int MFP::doInterrupt() {
	int irq = 0;
	/* ACIA */
	if ((flags & F_ACIA) && !(irq_inservice & (1<<6))) {
		irq_inservice |= (1<<6);
		TriggerMFP(false);
		flags &= ~F_ACIA;
		irq = 6;
	}
	/* TIMER C */
	else if ((flags & F_TIMERC) && ! (irq_inservice & (1<<5))) {
		if (automaticServiceEnd) {
			irq_inservice |= (1<<5);
		}
		if (--timerCounter <= 0) {
			TriggerMFP(false);
			flags &= ~F_TIMERC;
		}
		irq = 5;
	}

	if (irq)
		irq |= (vr & 0xf0);

	return irq;
}
