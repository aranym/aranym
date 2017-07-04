/*
	Audio Crossbar emulation

	ARAnyM (C) 2005 Patrice Mandin

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
 
#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "audio_crossbar.h"

#define DEBUG 0
#include "debug.h"

#include "SDL_compat.h"

/*--- Defines ---*/

#define	CLK_25M		25175040
#define CLK_32M		32000000
#define CLK_44K		22579200

#define CLOCK_MASK		3
#define CLOCK_25175K	0
#define CLOCK_EXT		1
#define CLOCK_32000K	2

#define OUTPUT_DSP_CONNECT		7
#define OUTPUT_DSP_CLOCK		5
#define OUTPUT_DSP_HANDSHAKE	4

#define OUTPUT_DMA_INPUT		3
#define OUTPUT_DMA_DMAIN		(0<<OUTPUT_DMA_INPUT)
#define OUTPUT_DMA_ALL			(1<<OUTPUT_DMA_INPUT)
#define OUTPUT_DMA_CLOCK		1
#define OUTPUT_DMA_HANDSHAKE	0

#define OUTPUT_ADC_SYNC			12
#define OUTPUT_ADC_INTSYNC		(0<<OUTPUT_ADC_SYNC)
#define OUTPUT_ADC_EXTSYNC		(1<<OUTPUT_ADC_SYNC)

#define OUTPUT_EXTIN_CLOCK		9
#define OUTPUT_EXTIN_HANDSHAKE	8

#define INPUT_MASK		3
#define INPUT_DMA_OUT	0
#define INPUT_DSP_OUT	1
#define INPUT_EXT_IN	2
#define INPUT_ADC_IN	3

#define INPUT_DSPIN_CONNECT		7
#define INPUT_DSPIN_OUTPUT		5
#define INPUT_DSPIN_HANDSHAKE	4

#define INPUT_DMAIN				3
#define INPUT_DMAIN_DSPOUT		(0<<INPUT_DMAIN)
#define INPUT_DMAIN_ALL			(1<<INPUT_DMAIN)

#define INPUT_DMAIN_OUTPUT		1
#define INPUT_DMAIN_HANDSHAKE	0

#define INPUT_DACOUT_OUTPUT		11

#define INPUT_EXTOUT_OUTPUT		9
#define INPUT_EXTOUT_HANDSHAKE	8

#define FREQ_PREDIV_MASK		15

#define REC_TRACKS_MASK			3

#define INPUT_SOURCE_MASK		3
#define INPUT_SOURCE_ADCDAC			(1<<0)
#define INPUT_SOURCE_MULTIPLEXER	(1<<1)

#define ADC_INPUT_MASK			3
#define ADC_INPUT_RIGHT			(1<<0)
#define ADC_INPUT_RIGHT_PSG		(1<<0)
#define ADC_INPUT_RIGHT_MIC		(0<<0)
#define ADC_INPUT_LEFT			(1<<1)
#define ADC_INPUT_LEFT_PSG		(1<<1)
#define ADC_INPUT_LEFT_MIC		(0<<1)

#define GAIN_MASK	15
#define GAIN_LEFT	4
#define GAIN_RIGHT	0

#define ATTEN_MASK	15
#define ATTEN_LEFT	4
#define ATTEN_RIGHT	0

#define GPIO_DIR_MASK			7
#define GPIO_PIN_INPUT	0
#define GPIO_PIN_OUTPUT	1

#define GPIO_DATA_MASK			7

/*--- Variables ---*/

#if DEBUG
extern "C" {
	static const char *const freq_names[4] = { "25.175 MHz", "External", "32 MHz", "Undefined" };
	static const char *const out_names[4] = { "DMA output", "DSP output", "Ext. input", "ADC input" };
};
#endif

/*--- Constructor/destructor of class ---*/

CROSSBAR::CROSSBAR(memptr addr, uint32 size) : BASE_IO(addr, size)
{
	D(bug("crossbar: interface created at 0x%06x", getHWoffset()));

	reset();
}

CROSSBAR::~CROSSBAR()
{
	D(bug("crossbar: interface destroyed at 0x%06x", getHWoffset()));
	reset();
}

/*--- Public functions ---*/

void CROSSBAR::reset()
{
	input = output = extfreqdiv = intfreqdiv = rec_tracks = in_source =
		adc_input = gain = atten = gpio_dir = gpio_data = 0;
	D(bug("crossbar: reset"));
}

uae_u8 CROSSBAR::handleRead(uaecptr addr)
{
	uae_u8 value=0;

	switch(addr-getHWoffset()) {
		case 0x00:
			value = (output>>8) & 0xff;
			break;
		case 0x01:
			value = output & 0xff;
			break;
		case 0x02:
			value = (input>>8) & 0xff;
			break;
		case 0x03:
			value = input & 0xff;
			break;
		case 0x04:
			value = extfreqdiv;
			break;
		case 0x05:
			value = intfreqdiv;
			break;
		case 0x06:
			value = rec_tracks & REC_TRACKS_MASK;
			break;
		case 0x07:
			value = in_source & INPUT_SOURCE_MASK;
			break;
		case 0x08:
			value = adc_input & ADC_INPUT_MASK;
			break;
		case 0x09:
			value = gain;
			break;
		case 0x0a:
			value = atten;
			break;
		case 0x11:
			value = gpio_dir & GPIO_DIR_MASK;
			break;
		case 0x13:
			value = gpio_data & GPIO_DATA_MASK;
			break;
	}

	return value;
}

void CROSSBAR::handleWrite(uaecptr addr, uae_u8 value)
{
	switch(addr-getHWoffset()) {
		case 0x00:
			output &= 0x00ff;
			output |= value<<8;
			break;
		case 0x01:
			output &= 0xff00;
			output |= value;
			break;
		case 0x02:
			input &= 0x00ff;
			input |= value<<8;
			break;
		case 0x03:
			input &= 0xff00;
			input |= value;
			break;
		case 0x04:
			extfreqdiv = value & FREQ_PREDIV_MASK;
			break;
		case 0x05:
			intfreqdiv = value & FREQ_PREDIV_MASK;
			getAUDIODMA()->updateMode();
			break;
		case 0x06:
			rec_tracks = value & REC_TRACKS_MASK;
			break;
		case 0x07:
			in_source = value & INPUT_SOURCE_MASK;
			break;
		case 0x08:
			adc_input = value & ADC_INPUT_MASK;
			break;
		case 0x09:
			gain = value & GAIN_MASK;
			break;
		case 0x0a:
			atten = value & ATTEN_MASK;
			break;
		case 0x11:
			gpio_dir = value & GPIO_DIR_MASK;
			break;
		case 0x13:
			gpio_data = value & GPIO_DATA_MASK;
			break;
	}

#if DEBUG
	switch(addr-getHWoffset()) {
		case 0x00:
		case 0x01:
			D(bug("crossbar: output=0x%04x", output));
			D(bug("crossbar:  DSP out: connect=%d, clock=%s, handshake=%d",
				(output>>OUTPUT_DSP_CONNECT) & 1,
				freq_names[(output>>OUTPUT_DSP_CLOCK) & CLOCK_MASK],
				(output>>OUTPUT_DSP_HANDSHAKE) & 1
			));
			D(bug("crossbar:  DMA out: connect=%s, clock=%s, handshake=%d",
				(output>>OUTPUT_DMA_INPUT) & 1 ? "All" : "DMA in",
				freq_names[(OUTPUT_DMA_CLOCK>>1) & CLOCK_MASK],
				(output>>OUTPUT_DMA_HANDSHAKE) & 1
			));
			D(bug("crossbar:  Ext. input: clock=%s, handshake=%d",
				freq_names[(output>>OUTPUT_EXTIN_CLOCK) & CLOCK_MASK],
				(output>>OUTPUT_EXTIN_HANDSHAKE) & 1
			));
			D(bug("crossbar:  ADC input: %s",
				(output>>OUTPUT_ADC_SYNC) & 1 ? "External sync" : "Internal sync"
			));
			break;
		case 0x02:
		case 0x03:
			D(bug("crossbar: input=0x%04x", input));
			D(bug("crossbar:  DSP in: connect=%d, output=%s, handshake=%d",
				(input>>INPUT_DSPIN_CONNECT) & 1,
				out_names[(input>>INPUT_DSPIN_OUTPUT) & INPUT_MASK],
				(input>>INPUT_DSPIN_HANDSHAKE) & 1
			));
			D(bug("crossbar:  DMA in: connect=%s, clock=%s, handshake=%d",
				(input>>INPUT_DMAIN) & 1 ? "All" : "DSP out",
				out_names[(input>>INPUT_DMAIN_OUTPUT) & INPUT_MASK],
				(input>>INPUT_DMAIN_HANDSHAKE) & 1
			));
			D(bug("crossbar:  Ext. output: clock=%s, handshake=%d",
				out_names[(input>>INPUT_EXTOUT_OUTPUT) & INPUT_MASK],
				(input>>INPUT_EXTOUT_HANDSHAKE) & 1
			));
			D(bug("crossbar:  DAC output: %s",
				out_names[(input>>INPUT_DACOUT_OUTPUT) & INPUT_MASK]
			));
			break;
		case 0x04:
			D(bug("crossbar: extfreqdiv=0x%02x", extfreqdiv));
			if (extfreqdiv==0) {
				D(bug("crossbar:  STe compatible mode"));
			}
			break;
		case 0x05:
			D(bug("crossbar: intfreqdiv=0x%02x", intfreqdiv));
			if (intfreqdiv==0) {
				D(bug("crossbar:  STe compatible mode"));
			}
			break;
		case 0x06:
			D(bug("crossbar: rec_tracks=0x%02x", rec_tracks));
			D(bug("crossbar:  Record %d tracks", rec_tracks+1));
			break;
		case 0x07:
			D(bug("crossbar: in_source=0x%02x", in_source));
			if (in_source & INPUT_SOURCE_ADCDAC) {
				D(bug("crossbar:  ADC/DAC"));
			}
			if (in_source & INPUT_SOURCE_MULTIPLEXER) {
				D(bug("crossbar:  Multiplexer"));
			}
			break;
		case 0x08:
			D(bug("crossbar: adc_input=0x%02x", adc_input));
			if ((adc_input & ADC_INPUT_RIGHT)==ADC_INPUT_RIGHT_PSG) {
				D(bug("crossbar:  Right channel PSG"));
			} else {
				D(bug("crossbar:  Right channel mic"));
			}
			if ((adc_input & ADC_INPUT_LEFT)==ADC_INPUT_LEFT_PSG) {
				D(bug("crossbar:  Left channel PSG"));
			} else {
				D(bug("crossbar:  Left channel mic"));
			}
			break;
		case 0x09:
			D(bug("crossbar: gain=0x%02x", gain));
			D(bug("crossbar:  Left=%d, Right=%d", (gain>>GAIN_LEFT) & GAIN_MASK, (gain>>GAIN_RIGHT) & GAIN_MASK));
			break;
		case 0x0a:
			D(bug("crossbar: attenuation=0x%02x", atten));
			D(bug("crossbar:  Left=%d, Right=%d", (atten>>ATTEN_LEFT) & ATTEN_MASK, (atten>>ATTEN_RIGHT) & ATTEN_MASK));
			break;
		case 0x11:
			D(bug("crossbar: gpio_dir=0x%02x", gpio_dir));
			for (int i=0;i<3; i++) {
				D(bug("crossbar:  Pin %d: %s", i, ((gpio_dir & (1<<i))==(GPIO_PIN_OUTPUT<<i)) ? "Output" : "Input"));
			}
			break;
		case 0x13:
			D(bug("crossbar: gpio_data=0x%02x", gpio_data));
			break;
	}
#endif
}

int CROSSBAR::getIntFreq(void)
{
	switch((output>>OUTPUT_DMA_CLOCK) & CLOCK_MASK) {
		case 0:
			return CLK_25M;	/* Falcon clock generator */
		case 2:
			return CLK_32M;
	}

	return CLK_44K;	/* 44.1 KHz clock generator */
}

int CROSSBAR::getIntPrediv(void)
{
	return intfreqdiv;
}
