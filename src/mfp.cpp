/* Joy 2001 */

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory.h"
#include "mfp.h"

const int HW = 0xfffa00;

	MFP_Timer::MFP_Timer(char value) {
		name = value;
		control = start_data = current_data = 0;
		state = false;
	}

	bool MFP_Timer::isRunning() {
		return ((control & 0x0f) > 0);
	}

	void MFP_Timer::setControl(uae_u8 value) {
		control = value & 0x0f;
		if (value & 0x10)
			state = false;
		fprintf(stderr, "Set MFP Timer%c control to $%x\n", name, value);
	}

	uae_u8 MFP_Timer::getControl() {
		return control | (state << 5);
	}

	void MFP_Timer::setData(uae_u8 value) {
		fprintf(stderr, "Set MFP Timer%c data to %d\n", name, value);
		start_data = value;
		if (! isRunning())
			current_data = value;
	}

	uae_u8 MFP_Timer::getData() {
		if (isRunning()) {
			if (current_data > 1)
				current_data--;
			else {
				state = true;
				// interrupt !!
				current_data = start_data;
			}
		}
		fprintf(stderr, "get MFP Timer%c data = %d\n", name, current_data);

		return current_data;
	}

	MFP::MFP() {
		tA = new MFP_Timer('A');
		tB = new MFP_Timer('B');
		tC = new MFP_Timer('C');
		tD = new MFP_Timer('D');
	}

	uae_u32 MFP::handleRead(uaecptr addr) {
		addr -= HW;
		if (addr < 0 || addr > 0x2f)
			return 0;	// unhandled

		switch(addr) {
			case 0x01:	return GPIP_data;
						break;

			case 0x03:	return active_edge;
						break;

			case 0x05:	return data_direction;
						break;

			case 0x07:	return irq_enable >> 8;
						break;

			case 0x09:	return irq_enable;
						break;

			case 0x0b:	return (irq_pending >> 8) | (tA->getControl() & 0x10);	// finish
						break;

			case 0x0d:	return irq_pending;
						break;

			case 0xf:	return irq_inservice >> 8;
						break;

			case 0x11:	return irq_inservice;
						break;
						
			case 0x13:	return irq_mask >> 8;
						break;
						
			case 0x15:	return irq_mask;
						break;
						
			case 0x17:	return irq_vector;
						break;

			case 0x19:	return tA->getControl();
						break;

			case 0x1b:	return tB->getControl();
						break;

			case 0x1d:	return (tC->getControl() >> 4) | tD->getControl();
						break;

			case 0x1f:	return tA->getData();
						break;

			case 0x21:	return tB->getData();
						break;

			case 0x23:	return tC->getData();
						break;
						
			case 0x25:	return tD->getData();
						break;
						
			case 0x27:
			case 0x29:
			case 0x2b:
			case 0x2d:
			case 0x2f:
			default:
				return 0;
		};
	}

	void MFP::handleWrite(uaecptr addr, uae_u8 value) {
		addr -= HW;
		if (addr < 0 || addr > 0x2f)
			return;	// unhandled

		switch(addr) {
			case 0x01:	GPIP_data = value;
						break;

			case 0x03:	active_edge = value;
						break;

			case 0x05:	data_direction = value;
						break;

			case 0x07:	irq_enable = (irq_enable & 0x00ff) | (value << 8);
						break;

			case 0x09:	irq_enable = (irq_enable & 0xff00) | value;
						break;

			case 0x0b:	irq_pending = (irq_pending & 0x00ff) | (value << 8);
						break;

			case 0x0d:	irq_pending = (irq_pending & 0xff00) | value;
						break;

			case 0xf:	irq_inservice = (irq_inservice & 0x00ff) | (value << 8);
						break;

			case 0x11:	irq_inservice = (irq_inservice & 0xff00) | value;
						break;
						
			case 0x13:	irq_mask = (irq_mask & 0x00ff) | (value << 8);
						break;
						
			case 0x15:	irq_mask = (irq_mask & 0xff00) | value;
						break;
						
			case 0x17:	irq_vector = 0x40 | (value & 0x0f);	/* wrong, the lowest 3 bits should be obtained elsewhere */
						break;

			case 0x19:	tA->setControl(value);
						break;

			case 0x1b:	tB->setControl(value);
						break;

			case 0x1d:	tC->setControl(value >> 4);
						tD->setControl(value & 0x0f);
						break;

			case 0x1f:	tA->setData(value);
						break;

			case 0x21:	tB->setData(value);
						break;

			case 0x23:	tC->setData(value);
						break;
						
			case 0x25:	tD->setData(value);
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
