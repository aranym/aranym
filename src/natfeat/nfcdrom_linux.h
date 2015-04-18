/*
 *  NatFeat host CD-ROM access, Linux CD-ROM driver
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

#ifndef NFCDROM_LINUX_H
#define NFCDROM_LINUX_H

#include "nfcdrom.h"

class CdromDriverLinux : public CdromDriver
{
	private:
		/* for each possible opened drive */
		struct {
			char *device;
			int handle;
			dev_t cdmode;
		} cddrives[CD_MAX_DRIVES];

		bool drives_scanned;
		int numcds;
		
		uint16 AtariToLinuxIoctl(uint16 opcode);	/* Translate ioctl numbers */

		int CheckDrive(const char *drive, const char *mnttype, struct stat *stbuf);
		void AddDrive(const char *drive, struct stat *stbuf);
		void CheckMounts(const char *mtab);
		
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

		int cd_unixioctl(int drive, int request, void *buffer);

	public:
		CdromDriverLinux();
		virtual ~CdromDriverLinux();

		virtual int Count();
		virtual const char *DeviceName(int drive);
};

#endif /* NFCDROM_LINUX_H */
