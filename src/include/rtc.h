/*
 * rtc.h - Atari NVRAM emulation code - declaration
 *
 * Copyright (c) 2001-2008 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#ifndef _RTC_H
#define _RTC_H

#include "icio.h"

/* NVRAM keyboard setting */
typedef enum {
	COUNTRY_US,	// USA
	COUNTRY_DE,	// Germany
	COUNTRY_FR,	// France
	COUNTRY_UK,	// United Kingdom
	COUNTRY_ES,	// Spain
	COUNTRY_IT,	// Italy
	COUNTRY_SE,	// Sweden
	COUNTRY_SF,	// Switzerland (French)
	COUNTRY_SG,	// Switzerland (German)
	COUNTRY_TR,	// Turkey
	COUNTRY_FI,	// Finland
	COUNTRY_NO,	// Norway
	COUNTRY_DK,	// Denmark
	COUNTRY_SA,	// Saudi Arabia
	COUNTRY_NL,	// Holland
	COUNTRY_CZ,	// Czech Republic
	COUNTRY_HU,	// Hungary
// the following entries are from EmuTOS
	COUNTRY_SK,	// Slovak Republic
	COUNTRY_GR	// Greek
} nvram_t;

class RTC : public BASE_IO {
private:
	uint8 index;
	char nvram_filename[512];

public:
	RTC(memptr, uint32);
	virtual ~RTC();
	void reset();
	void init(void);
	bool load(void);
	bool save(void);
	virtual uint8 handleRead(memptr);
	virtual void handleWrite(memptr, uint8);

	nvram_t getNvramKeyboard(void);

private:
	void setAddr(uint8 value);
	uint8 getData();
	void setData(uint8);
	void setChecksum();
	void patch();
};

#endif // _RTC_H

/*
vim:ts=4:sw=4:
*/
