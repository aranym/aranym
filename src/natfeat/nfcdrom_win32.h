/*
 *  NatFeat host CD-ROM access, Win32 CD-ROM driver
 *
 *  ARAnyM (C) 2014 ARAnyM developer team
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

#ifndef NFCDROM_WIN32_H
#define NFCDROM_WIN32_H

/*--- Includes ---*/

#include "nfcdrom.h"
#include "win32_supp.h"
#undef NOERROR /* conflicts with jpgdh.h */
#include "parameters.h"

class CdromDriverWin32 : public CdromDriver
{
	private:
		/* for each possible opened drive */
		struct {
			char *device;
			HANDLE handle;
			int usecount;
		} cddrives[CD_MAX_DRIVES];

		bool drives_scanned;
		int numcds;
		
	protected:
		int OpenDrive(memptr device);
		void CloseDrive(int drive);

		int32 cd_read(memptr device, memptr buffer, uint32 first, uint32 length);
		int32 cd_status(memptr device, memptr ext_status);
		int32 cd_ioctl(memptr device, uint16 opcode, memptr buffer);
	
		int32 cd_startaudio(memptr device, uint32 dummy, memptr buffer);
		int32 cd_stopaudio(memptr device);
		int32 cd_setsongtime(memptr device, uint32 dummy, uint32 start_msf, uint32 end_msf);
		int32 cd_gettoc(memptr device, uint32 dummy, memptr buffer);
		int32 cd_discinfo(memptr device, memptr buffer);

		int cd_winioctl(int drive, DWORD code, LPVOID in = NULL, DWORD insize = 0, LPVOID out = NULL, DWORD outsize = 0, LPDWORD returned = NULL);
		int cd_win_playtracks(int drive, unsigned char first, unsigned char last);
		
	public:
		CdromDriverWin32();
		virtual ~CdromDriverWin32();

		virtual int Count();
		virtual const char *DeviceName(int drive);
};

#endif /* NFCDROM_WIN32_H */
