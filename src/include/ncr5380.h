/*
 *  Atari Falcon NCR5380 SCSI emulation
 *
 *  ARAnyM (C) 2003 Patrice Mandin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _NCR5380_H
#define _NCR5380_H

/*--- Includes ---*/

#include "icio.h"

/*--- NCR5380 class ---*/

class NCR5380 {
	private:
		uae_u8	hd_status;
		uae_u8	hd_count;

		uae_u8	hd_initiator;
		uae_u8	hd_mode;

	public:
		NCR5380(void);
		~NCR5380(void);
		void reset(void);

		uae_u8 ReadData(uae_u16 control);
		void WriteData(uae_u16 control, uae_u8 data);
};

#endif /* _NCR5380_H */
