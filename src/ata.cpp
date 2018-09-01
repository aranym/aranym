/////////////////////////////////////////////////////////////////////////
//
//  Copyright (C) 2002  MandrakeSoft S.A.
//
//    MandrakeSoft S.A.
//    43, rue d'Aboukir
//    75002 Paris - France
//    http://www.linux-mandrake.com/
//    http://www.mandrakesoft.com/
//
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2 of the License, or (at your option) any later version.
//
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

// Useful docs:
// AT Attachment with Packet Interface
// working draft by T13 at www.t13.org

#include "sysdeps.h"
#include "emu_bochs.h"
#include "parameters.h"
#include "ata.h"
#include "hardware.h"
#define DEBUG 0
#include "debug.h"



#define INDEX_PULSE_CYCLE 10

#define ATA_DMA 0
#define PACKET_SIZE 12

bx_hard_drive_c bx_hard_drive;

#ifdef BX_USE_HD_SMF
#define this (&bx_hard_drive)
#endif


static unsigned max_multiple_sectors = 0;	// was 0x3f
static unsigned curr_multiple_sectors = 0;	// was 0x3f

// some packet handling macros
#define EXTRACT_FIELD(arr,byte,start,num_bits) (((arr)[(byte)] >> (start)) & ((1 << (num_bits)) - 1))
#define get_packet_field(c,b,s,n) (EXTRACT_FIELD((BX_SELECTED_CONTROLLER((c)).buffer),(b),(s),(n)))
#define get_packet_byte(c,b) (BX_SELECTED_CONTROLLER((c)).buffer[(b)])
#define get_packet_word(c,b) (((uint16)BX_SELECTED_CONTROLLER((c)).buffer[(b)] << 8) | BX_SELECTED_CONTROLLER((c)).buffer[(b)+1])


#define BX_CONTROLLER(c,a) (BX_HD_THIS channels[(c)].drives[(a)]).controller
#define BX_DRIVE(c,a) (BX_HD_THIS channels[(c)].drives[(a)])

#define BX_DRIVE_IS_PRESENT(c,a) (BX_HD_THIS channels[(c)].drives[(a)].device_type != IDE_NONE)
#define BX_DRIVE_IS_HD(c,a) (BX_DRIVE(c, a).device_type == IDE_DISK)
#define BX_DRIVE_IS_CD(c,a) (BX_DRIVE(c, a).device_type == IDE_CDROM)

#define BX_MASTER_IS_PRESENT(c) BX_DRIVE_IS_PRESENT((c),0)
#define BX_SLAVE_IS_PRESENT(c) BX_DRIVE_IS_PRESENT((c),1)
#define BX_ANY_IS_PRESENT(c) (BX_MASTER_IS_PRESENT(c) || BX_SLAVE_IS_PRESENT(c))

#define BX_SELECTED_CONTROLLER(c) (BX_CONTROLLER((c),BX_HD_THIS channels[(c)].drive_select))
#define BX_SELECTED_DRIVE(c) (BX_DRIVE((c),BX_HD_THIS channels[(c)].drive_select))
#define BX_MASTER_SELECTED(c) (!BX_HD_THIS channels[(c)].drive_select)
#define BX_SLAVE_SELECTED(c)  (BX_HD_THIS channels[(c)].drive_select)

#define BX_SELECTED_IS_PRESENT(c) (BX_DRIVE_IS_PRESENT((c),BX_SLAVE_SELECTED((c))))
#define BX_SELECTED_IS_HD(c) (BX_DRIVE_IS_HD((c),BX_SLAVE_SELECTED((c))))
#define BX_SELECTED_IS_CD(c) (BX_DRIVE_IS_CD((c),BX_SLAVE_SELECTED((c))))

#define BX_SELECTED_MODEL(c) (BX_SELECTED_DRIVE(c).model_no)
#define BX_SELECTED_TYPE_STRING(channel) ((BX_SELECTED_IS_CD(channel)) ? "CD-ROM" : "DISK")

#define WRITE_FEATURES(c,a) do { uint8 _a = a; BX_CONTROLLER((c),0).features = _a; BX_CONTROLLER((c),1).features = _a; } while(0)
#define WRITE_SECTOR_COUNT(c,a) do { uint8 _a = a; BX_CONTROLLER((c),0).sector_count = _a; BX_CONTROLLER((c),1).sector_count = _a; } while(0)
#define WRITE_SECTOR_NUMBER(c,a) do { uint8 _a = a; BX_CONTROLLER((c),0).sector_no = _a; BX_CONTROLLER((c),1).sector_no = _a; } while(0)
#define WRITE_CYLINDER_LOW(c,a) do { uint8 _a = a; BX_CONTROLLER((c),0).cylinder_no = (BX_CONTROLLER((c),0).cylinder_no & 0xff00) | _a; BX_CONTROLLER((c),1).cylinder_no = (BX_CONTROLLER((c),1).cylinder_no & 0xff00) | _a; } while(0)
#define WRITE_CYLINDER_HIGH(c,a) do { uint16 _a = a; BX_CONTROLLER((c),0).cylinder_no = (_a << 8) | (BX_CONTROLLER((c),0).cylinder_no & 0xff); BX_CONTROLLER((c),1).cylinder_no = (_a << 8) | (BX_CONTROLLER((c),1).cylinder_no & 0xff); } while(0)
#define WRITE_HEAD_NO(c,a) do { uint8 _a = a; BX_CONTROLLER((c),0).head_no = _a; BX_CONTROLLER((c),1).head_no = _a; } while(0)
#define WRITE_LBA_MODE(c,a) do { uint8 _a = a; BX_CONTROLLER((c),0).lba_mode = _a; BX_CONTROLLER((c),1).lba_mode = _a; } while(0)


//static unsigned im_here = 0;

bx_hard_drive_c::bx_hard_drive_c(void)
{
	for (Bit8u channel = 0; channel < BX_MAX_ATA_CHANNEL; channel++)
	{
		channels[channel].drives[0].hard_drive = new default_image_t();
		channels[channel].drives[1].hard_drive = new default_image_t();
	}
}

bx_hard_drive_c::~bx_hard_drive_c(void)
{
	for (Bit8u channel = 0; channel < BX_MAX_ATA_CHANNEL; channel++)
	{
		if (channels[channel].drives[0].hard_drive != NULL)	/* DT 17.12.2001 21:55 */
		{
			delete channels[channel].drives[0].hard_drive;

			channels[channel].drives[0].hard_drive = NULL;
		}
		if (channels[channel].drives[1].hard_drive != NULL)
		{
			delete channels[channel].drives[1].hard_drive;

			channels[channel].drives[1].hard_drive = NULL;	/* DT 17.12.2001 21:56 */
		}
	}
}




Bit32u fcha2io(Bit32u address)
{
	switch (address)
	{
	case 0xf00000:						/* data */
		return 0x00;
	case 0xf00005:						/* error register */
		return 0x01;
	case 0xf00009:						/* sector count */
		return 0x02;
	case 0xf0000d:						/* sector number */
		return 0x03;
	case 0xf00011:						/* cylinder low */
		return 0x04;
	case 0xf00015:						/* cylinder high */
		return 0x05;
	case 0xf00019:						/* drive & head */
		return 0x06;
	case 0xf0001d:						/* status/command */
		return 0x07;
	case 0xf00039:						/* alternate status/data output */
		return 0x16;
	default:
		return 0xffffffff;
	}
}

Bit32u nila2io(Bit32u /*address */ )
{
	return 0xffffffff;
}

void bx_hard_drive_c::init(void)
{
	Bit8u channel;

	for (channel = 0; channel < BX_MAX_ATA_CHANNEL; channel++)
	{
		switch (channel)
		{
		case 0:
			BX_HD_THIS channels[channel].addr2io = fcha2io;
			break;
		default:
			BX_HD_THIS channels[channel].addr2io = nila2io;
			break;
		}
		BX_HD_THIS channels[channel].drive_select = 0;
	}

	for (channel = 0; channel < BX_MAX_ATA_CHANNEL; channel++)
	{
		for (Bit8u device = 0; device < 2; device++)
		{
			// Initialize controller state, even if device is not present
			BX_CONTROLLER(channel, device).status.busy = 0;
			BX_CONTROLLER(channel, device).status.drive_ready = 1;
			BX_CONTROLLER(channel, device).status.write_fault = 0;
			BX_CONTROLLER(channel, device).status.seek_complete = 1;
			BX_CONTROLLER(channel, device).status.drq = 0;
			BX_CONTROLLER(channel, device).status.corrected_data = 0;
			BX_CONTROLLER(channel, device).status.index_pulse = 0;
			BX_CONTROLLER(channel, device).status.index_pulse_count = 0;
			BX_CONTROLLER(channel, device).status.err = 0;

			BX_CONTROLLER(channel, device).error_register = 0x01;	// diagnostic code: no error
			BX_CONTROLLER(channel, device).head_no = 0;
			BX_CONTROLLER(channel, device).sector_count = 1;
			BX_CONTROLLER(channel, device).sector_no = 1;
			BX_CONTROLLER(channel, device).cylinder_no = 0;
			BX_CONTROLLER(channel, device).current_command = 0x00;
			BX_CONTROLLER(channel, device).buffer_index = 0;

			BX_CONTROLLER(channel, device).control.reset = 0;
			BX_CONTROLLER(channel, device).control.disable_irq = 0;
			BX_CONTROLLER(channel, device).reset_in_progress = 0;

			BX_CONTROLLER(channel, device).sectors_per_block = 0x80;
			BX_CONTROLLER(channel, device).lba_mode = 0;

			BX_CONTROLLER(channel, device).features = 0;

			// If not present
			BX_HD_THIS channels[channel].drives[device].device_type = IDE_NONE;

			if (!bx_options.atadevice[channel][device].present)
			{
				continue;
			}
			// Make model string
			strncpy((char *) BX_HD_THIS channels[channel].drives[device].model_no,
					bx_options.atadevice[channel][device].model, 40);
			while (strlen((char *) BX_HD_THIS channels[channel].drives[device].model_no) < 40)
			{
				strcat((char *) BX_HD_THIS channels[channel].drives[device].model_no, " ");
			}

			if (bx_options.atadevice[channel][device].type == IDE_DISK /*BX_ATA_DEVICE_DISK */ )
			{
				D(bug("Hard-Disk on target %d/%d", channel, device));
				BX_HD_THIS channels[channel].drives[device].device_type = IDE_DISK;
				BX_HD_THIS channels[channel].drives[device].hard_drive->cylinders =
					bx_options.atadevice[channel][device].cylinders;
				BX_HD_THIS channels[channel].drives[device].hard_drive->heads =
					bx_options.atadevice[channel][device].heads;
				BX_HD_THIS channels[channel].drives[device].hard_drive->sectors =
					bx_options.atadevice[channel][device].spt;
				BX_HD_THIS channels[channel].drives[device].hard_drive->byteswap =
					bx_options.atadevice[channel][device].byteswap;
				// BX_HD_THIS channels[channel].drives[device].hard_drive->readonly   = bx_options.atadevice[channel][device].readonly;

				/* open hard drive image file */
				if ((BX_HD_THIS channels[channel].drives[device].hard_drive->
					 open(bx_options.atadevice[channel][device].path,
						  bx_options.atadevice[channel][device].readonly)) < 0)
				{
					panicbug("could not open hard drive image file '%s'", bx_options.atadevice[channel][device].path);
				}
				D(bug("HD on ata%d-%d: '%s', CHS %u/%u/%u, swap %d", channel, device,
					bx_options.atadevice[channel][device].path,
					BX_DRIVE(channel, device).hard_drive->cylinders,
					BX_DRIVE(channel, device).hard_drive->heads,
					BX_DRIVE(channel, device).hard_drive->sectors,
					BX_DRIVE(channel, device).hard_drive->byteswap));
			} else if (bx_options.atadevice[channel][device].type == IDE_CDROM /* BX_ATA_DEVICE_CDROM */ )
			{
				D(bug("CDROM on target %d/%d", channel, device));
				BX_HD_THIS channels[channel].drives[device].device_type = IDE_CDROM;
				BX_HD_THIS channels[channel].drives[device].cdrom.locked = 0;
				BX_HD_THIS channels[channel].drives[device].sense.sense_key = SENSE_NONE;
				BX_HD_THIS channels[channel].drives[device].sense.asc = 0;
				BX_HD_THIS channels[channel].drives[device].sense.ascq = 0;

				// Check bit fields
				BX_CONTROLLER(channel, device).sector_count = 0;
				BX_CONTROLLER(channel, device).interrupt_reason.c_d = 1;
				if (BX_CONTROLLER(channel, device).sector_count != 0x01)
					panicbug("interrupt reason bit field error");

				BX_CONTROLLER(channel, device).sector_count = 0;
				BX_CONTROLLER(channel, device).interrupt_reason.i_o = 1;
				if (BX_CONTROLLER(channel, device).sector_count != 0x02)
					panicbug("interrupt reason bit field error");

				BX_CONTROLLER(channel, device).sector_count = 0;
				BX_CONTROLLER(channel, device).interrupt_reason.rel = 1;
				if (BX_CONTROLLER(channel, device).sector_count != 0x04)
					panicbug("interrupt reason bit field error");

				BX_CONTROLLER(channel, device).sector_count = 0;
				BX_CONTROLLER(channel, device).interrupt_reason.tag = 3;
				if (BX_CONTROLLER(channel, device).sector_count != 0x18)
					panicbug("interrupt reason bit field error");
				BX_CONTROLLER(channel, device).sector_count = 0;

				// allocate low level driver
#ifdef LOWLEVEL_CDROM
				BX_HD_THIS channels[channel].drives[device].cdrom.cd =
					new LOWLEVEL_CDROM(bx_options.atadevice[channel][device].path);
				D2(bug("CD on ata%d-%d: '%s'", channel, device, bx_options.atadevice[channel][device].path));

				if (bx_options.atadevice[channel][device].status == BX_INSERTED)
				{
					if (BX_HD_THIS channels[channel].drives[device].cdrom.cd->insert_cdrom())
					{
						D2(bug("Media present in CD-ROM drive"));
						BX_HD_THIS channels[channel].drives[device].cdrom.ready = 1;
						BX_HD_THIS channels[channel].drives[device].cdrom.capacity =
							BX_HD_THIS channels[channel].drives[device].cdrom.cd->capacity();
					} else
					{
						D2(bug("Could not locate CD-ROM, continuing with media not present"));
						BX_HD_THIS channels[channel].drives[device].cdrom.ready = 0;

						bx_options.atadevice[channel][device].status = BX_EJECTED;
					}
				} else
				{
#endif
					D2(bug("Media not present in CD-ROM drive"));
					BX_HD_THIS channels[channel].drives[device].cdrom.ready = 0;

#ifdef LOWLEVEL_CDROM
				}
#endif
			}
		}
	}
}

void bx_hard_drive_c::reset(unsigned /*type */ )
{
	getMFP()->setGPIPbit(0x20, 0x20);	// lower the interrupt
}


#define GOTO_RETURN_VALUE  if(io_len==4){\
                             goto return_value32;\
                             }\
                           else if(io_len==2){\
                             value16=(Bit16u)value32;\
                             goto return_value16;\
                             }\
                           else{\
                             value8=(Bit8u)value32;\
                             goto return_value8;\
                             }


  // static IO port read callback handler
  // redirects to non-static class handler to avoid virtual functions

Bit32u bx_hard_drive_c::read_handler(void *this_ptr, Bit32u address, unsigned io_len)
{
#ifndef BX_USE_HD_SMF
	bx_hard_drive_c *class_ptr = (bx_hard_drive_c *) this_ptr;

	return (class_ptr->read(address, io_len));
}


Bit32u bx_hard_drive_c::read(Bit32u address, unsigned io_len)
{
#else
	UNUSED(this_ptr);
#endif // !BX_USE_HD_SMF
	Bit8u value8;
	Bit16u value16;
	Bit32u value32;

	Bit8u channel = BX_MAX_ATA_CHANNEL;
	Bit32u port = 0xffffffff;

	for (channel = 0; channel < BX_MAX_ATA_CHANNEL; channel++)
	{
		if ((port = BX_HD_THIS channels[channel].addr2io(address)) != 0xffffffff)
			break;
	}

	if (channel == BX_MAX_ATA_CHANNEL)
	{
		D(panicbug("Unable to find ATA channel, ioport=0x%08x", address));
		return 0;
	}

	if (io_len > 1 && port != 0x00)
	{
		D(panicbug("non-byte IO read to 0x%08x", (unsigned) address));
	}

	switch (port)
	{
	case 0x00:							// hard disk data (16bit) (f00000)
		if (BX_SELECTED_CONTROLLER(channel).status.drq == 0)
		{
			D(bug("IO read(0x%08x) with drq == 0: last command was %02xh", address,
				  (unsigned) BX_SELECTED_CONTROLLER(channel).current_command));
			return 0;
		}
		D(bug("IO read(0x%08x): current command is %02xh", address,
			  (unsigned) BX_SELECTED_CONTROLLER(channel).current_command));
		switch (BX_SELECTED_CONTROLLER(channel).current_command)
		{
		case 0x20:						// READ SECTORS, with retries
		case 0x21:						// READ SECTORS, without retries
			if (io_len == 1)
			{
				panicbug("byte IO read from 0x%08x", (unsigned) address);
			}
			if (BX_SELECTED_CONTROLLER(channel).buffer_index >= 512)
			{
				D(bug("IO read(0x%08x): buffer_index >= 512", address));
			}
#ifdef BX_SupportRepeatSpeedups
			if (BX_HD_THIS devices->bulkIOQuantumsRequested)
			{
				unsigned transferLen, uantumsMax;

				quantumsMax = (512 - BX_SELECTED_CONTROLLER(channel).buffer_index) / io_len;
				if (quantumsMax == 0)
					D(panicbug("IO read(0x%08x): not enough space for read", address));
				BX_HD_THIS devices->bulkIOQuantumsTransferred = BX_HD_THIS devices->bulkIOQuantumsRequested;

				if (quantumsMax < BX_HD_THIS devices->bulkIOQuantumsTransferred)
					BX_HD_THIS devices->bulkIOQuantumsTransferred = quantumsMax;

				transferLen = io_len * BX_HD_THIS devices->bulkIOQuantumsTransferred;
				memcpy((Bit8u *) BX_HD_THIS devices->bulkIOHostAddr,
					   &BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index],
					   transferLen);
				BX_HD_THIS devices->bulkIOHostAddr += transferLen;

				BX_SELECTED_CONTROLLER(channel).buffer_index += transferLen;
				value32 = 0;			// Value returned not important;
			} else
#endif
			{
				value32 = 0L;
				bool bs = BX_SELECTED_DRIVE(channel).hard_drive->byteswap;	/* FALCON disk image (byte swap) */

				switch (io_len)
				{
				case 4:
					if (bs)
					{
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 0] << 24;
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 1] << 16;
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 2] << 8;
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 3];
						// value32 = ((value32 >> 16) & 0x0000ffff) | ((value32 & 0x0000ffff) << 16);	/* FALCON (word swap for long access to data register) */
					} else
					{
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 1] << 24;
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 0] << 16;
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 3] << 8;
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 2];
						// value32 = ((value32 >> 16) & 0x0000ffff) | ((value32 & 0x0000ffff) << 16);	/* FALCON (word swap for long access to data register) */
					}
					break;
				case 2:
					if (bs)
					{
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 0] << 8;
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 1];
					} else
					{
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 1] << 8;
						value32 |= BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 0];
					}
					break;
				}
				BX_SELECTED_CONTROLLER(channel).buffer_index += io_len;
			}

			// if buffer completely read
			if (BX_SELECTED_CONTROLLER(channel).buffer_index >= 512)
			{
				// update sector count, sector number, cylinder,
				// drive, head, status
				// if there are more sectors, read next one in...
				//
				BX_SELECTED_CONTROLLER(channel).buffer_index = 0;

				increment_address(channel);

				BX_SELECTED_CONTROLLER(channel).status.busy = 0;
				BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
				BX_SELECTED_CONTROLLER(channel).status.write_fault = 0;
				if (bx_options.newHardDriveSupport)
					BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
				else
					BX_SELECTED_CONTROLLER(channel).status.seek_complete = 0;
				BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;
				BX_SELECTED_CONTROLLER(channel).status.err = 0;

				if (BX_SELECTED_CONTROLLER(channel).sector_count == 0)
				{
					BX_SELECTED_CONTROLLER(channel).status.drq = 0;
				} else
				{						/* read next one into controller buffer */
					off_t logical_sector;
					off_t ret;

					BX_SELECTED_CONTROLLER(channel).status.drq = 1;
					BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;

					if (!calculate_logical_address(channel, &logical_sector))
					{
						bug("multi-sector read reached invalid sector %lu, aborting", (unsigned long) logical_sector);
						command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
						GOTO_RETURN_VALUE;
					}
					ret = BX_SELECTED_DRIVE(channel).hard_drive->lseek(logical_sector * 512, SEEK_SET);
					if (ret < 0)
					{
						bug("could not lseek() hard drive image file");
						command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
						GOTO_RETURN_VALUE;
					}
					ret =
						BX_SELECTED_DRIVE(channel).hard_drive->read((bx_ptr_t) BX_SELECTED_CONTROLLER(channel).buffer,
																	512);
					if (ret < 512)
					{
						bug("logical sector was %lu", (unsigned long) logical_sector);
						bug("could not read() hard drive image file at byte %lu", (unsigned long) logical_sector * 512);
						command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
						GOTO_RETURN_VALUE;
					}

					BX_SELECTED_CONTROLLER(channel).buffer_index = 0;
					raise_interrupt(channel);
				}
			}
			GOTO_RETURN_VALUE;
			break;

		case 0xec:						// IDENTIFY DEVICE
		case 0xa1:						// IDENTIFY PACKET DEVICE
			if (bx_options.newHardDriveSupport)
			{
				unsigned index;

				BX_SELECTED_CONTROLLER(channel).status.busy = 0;
				BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
				BX_SELECTED_CONTROLLER(channel).status.write_fault = 0;
				BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
				BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;
				BX_SELECTED_CONTROLLER(channel).status.err = 0;

				index = BX_SELECTED_CONTROLLER(channel).buffer_index;
				value32 = BX_SELECTED_CONTROLLER(channel).buffer[index];
				index++;
				if (io_len >= 2)
				{
					value32 |= (BX_SELECTED_CONTROLLER(channel).buffer[index] << 8);
					index++;
				}
				if (io_len == 4)
				{
					value32 |= (BX_SELECTED_CONTROLLER(channel).buffer[index] << 16);
					value32 |= (BX_SELECTED_CONTROLLER(channel).buffer[index + 1] << 24);
					value32 = ((value32 >> 16) & 0x0000ffff) | ((value32 & 0x0000ffff) << 16);	/* FALCON (word swap for long access to data register) */
					index += 2;
				}
				BX_SELECTED_CONTROLLER(channel).buffer_index = index;

				if (BX_SELECTED_CONTROLLER(channel).buffer_index >= 512)
				{
					BX_SELECTED_CONTROLLER(channel).status.drq = 0;
				}
				GOTO_RETURN_VALUE;
			} else
			{
				D(bug("IO read(0x%08x): current command is 0x%02x", address,
					  (unsigned) BX_SELECTED_CONTROLLER(channel).current_command));
				command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
			}
			break;

		case 0xa0:
			{							// PACKET COMMAND
				unsigned index = BX_SELECTED_CONTROLLER(channel).buffer_index;
				unsigned increment = 0;

				// Load block if necessary
				if (index >= 2048)
				{
					if (index > 2048)
					{
						D(bug("index > 2048 : 0x%x", index));
					}
					switch (BX_SELECTED_DRIVE(channel).atapi.command)
					{
					case 0x28:			// read (10)
					case 0xa8:			// read (12)
#ifdef LOWLEVEL_CDROM
						BX_SELECTED_DRIVE(channel).cdrom.cd->read_block(BX_SELECTED_CONTROLLER(channel).buffer,
																		BX_SELECTED_DRIVE(channel).cdrom.next_lba);
						BX_SELECTED_DRIVE(channel).cdrom.next_lba++;
						BX_SELECTED_DRIVE(channel).cdrom.remaining_blocks--;

						// one block transfered, start at beginnig
						index = 0;
#else
						D(panicbug("Read with no LOWLEVEL_CDROM"));
#endif
						break;

					default:			// no need to load a new block
						break;
					}
				}

				value32 = BX_SELECTED_CONTROLLER(channel).buffer[index + increment];
				increment++;
				if (io_len >= 2)
				{
					value32 |= (BX_SELECTED_CONTROLLER(channel).buffer[index + increment] << 8);
					increment++;
				}
				if (io_len == 4)
				{
					value32 |= (BX_SELECTED_CONTROLLER(channel).buffer[index + increment] << 16);
					value32 |= (BX_SELECTED_CONTROLLER(channel).buffer[index + increment + 1] << 24);
					increment += 2;
				}
				BX_SELECTED_CONTROLLER(channel).buffer_index = index + increment;
				BX_SELECTED_CONTROLLER(channel).drq_index += increment;

				if (BX_SELECTED_CONTROLLER(channel).drq_index >= (unsigned) BX_SELECTED_DRIVE(channel).atapi.drq_bytes)
				{
					BX_SELECTED_CONTROLLER(channel).status.drq = 0;
					BX_SELECTED_CONTROLLER(channel).drq_index = 0;

					BX_SELECTED_DRIVE(channel).atapi.total_bytes_remaining -=
						BX_SELECTED_DRIVE(channel).atapi.drq_bytes;

					if (BX_SELECTED_DRIVE(channel).atapi.total_bytes_remaining > 0)
					{
						// one or more blocks remaining (works only for single block commands)
						BX_SELECTED_CONTROLLER(channel).interrupt_reason.i_o = 1;
						BX_SELECTED_CONTROLLER(channel).status.busy = 0;
						BX_SELECTED_CONTROLLER(channel).status.drq = 1;
						BX_SELECTED_CONTROLLER(channel).interrupt_reason.c_d = 0;

						// set new byte count if last block
						if (BX_SELECTED_DRIVE(channel).atapi.total_bytes_remaining <
							BX_SELECTED_CONTROLLER(channel).byte_count)
						{
							BX_SELECTED_CONTROLLER(channel).byte_count =
								BX_SELECTED_DRIVE(channel).atapi.total_bytes_remaining;
						}
						BX_SELECTED_DRIVE(channel).atapi.drq_bytes = BX_SELECTED_CONTROLLER(channel).byte_count;

						raise_interrupt(channel);
					} else
					{
						// all bytes read
						BX_SELECTED_CONTROLLER(channel).interrupt_reason.i_o = 1;
						BX_SELECTED_CONTROLLER(channel).interrupt_reason.c_d = 1;
						BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
						BX_SELECTED_CONTROLLER(channel).interrupt_reason.rel = 0;
						BX_SELECTED_CONTROLLER(channel).status.busy = 0;
						BX_SELECTED_CONTROLLER(channel).status.drq = 0;
						BX_SELECTED_CONTROLLER(channel).status.err = 0;

						raise_interrupt(channel);
					}
				}
				GOTO_RETURN_VALUE;
				break;
			}

			// List all the read operations that are defined in the ATA/ATAPI spec
			// that we don't support.  Commands that are listed here will cause a
			// BX_ERROR, which is non-fatal, and the command will be aborted.
		case 0x08:
			bug("read cmd 0x08 (DEVICE RESET) not supported");
			command_aborted(channel, 0x08);
			break;
		case 0x10:
			bug("read cmd 0x10 (RECALIBRATE) not supported");
			command_aborted(channel, 0x10);
			break;
		case 0x22:
			bug("read cmd 0x22 (READ LONG) not supported");
			command_aborted(channel, 0x22);
			break;
		case 0x23:
			bug("read cmd 0x23 (READ LONG NO RETRY) not supported");
			command_aborted(channel, 0x23);
			break;
		case 0x24:
			bug("read cmd 0x24 (READ SECTORS EXT) not supported");
			command_aborted(channel, 0x24);
			break;
		case 0x25:
			bug("read cmd 0x25 (READ DMA EXT) not supported");
			command_aborted(channel, 0x25);
			break;
		case 0x26:
			bug("read cmd 0x26 (READ DMA QUEUED EXT) not supported");
			command_aborted(channel, 0x26);
			break;
		case 0x27:
			bug("read cmd 0x27 (READ NATIVE MAX ADDRESS EXT) not supported");
			command_aborted(channel, 0x27);
			break;
		case 0x29:
			bug("read cmd 0x29 (READ MULTIPLE EXT) not supported");
			command_aborted(channel, 0x29);
			break;
		case 0x2A:
			bug("read cmd 0x2A (READ STREAM DMA) not supported");
			command_aborted(channel, 0x2A);
			break;
		case 0x2B:
			bug("read cmd 0x2B (READ STREAM PIO) not supported");
			command_aborted(channel, 0x2B);
			break;
		case 0x2F:
			bug("read cmd 0x2F (READ LOG EXT) not supported");
			command_aborted(channel, 0x2F);
			break;
		case 0x30:
			bug("read cmd 0x30 (WRITE SECTORS) not supported");
			command_aborted(channel, 0x30);
			break;
		case 0x31:
			bug("read cmd 0x31 (WRITE SECTORS NO RETRY) not supported");
			command_aborted(channel, 0x31);
			break;
		case 0x32:
			bug("read cmd 0x32 (WRITE LONG) not supported");
			command_aborted(channel, 0x32);
			break;
		case 0x33:
			bug("read cmd 0x33 (WRITE LONG NO RETRY) not supported");
			command_aborted(channel, 0x33);
			break;
		case 0x34:
			bug("read cmd 0x34 (WRITE SECTORS EXT) not supported");
			command_aborted(channel, 0x34);
			break;
		case 0x35:
			bug("read cmd 0x35 (WRITE DMA EXT) not supported");
			command_aborted(channel, 0x35);
			break;
		case 0x36:
			bug("read cmd 0x36 (WRITE DMA QUEUED EXT) not supported");
			command_aborted(channel, 0x36);
			break;
		case 0x37:
			bug("read cmd 0x37 (SET MAX ADDRESS EXT) not supported");
			command_aborted(channel, 0x37);
			break;
		case 0x38:
			bug("read cmd 0x38 (CFA WRITE SECTORS W/OUT ERASE) not supported");
			command_aborted(channel, 0x38);
			break;
		case 0x39:
			bug("read cmd 0x39 (WRITE MULTIPLE EXT) not supported");
			command_aborted(channel, 0x39);
			break;
		case 0x3A:
			bug("read cmd 0x3A (WRITE STREAM DMA) not supported");
			command_aborted(channel, 0x3A);
			break;
		case 0x3B:
			bug("read cmd 0x3B (WRITE STREAM PIO) not supported");
			command_aborted(channel, 0x3B);
			break;
		case 0x3F:
			bug("read cmd 0x3F (WRITE LOG EXT) not supported");
			command_aborted(channel, 0x3F);
			break;
		case 0x40:
			bug("read cmd 0x40 (READ VERIFY SECTORS) not supported");
			command_aborted(channel, 0x40);
			break;
		case 0x41:
			bug("read cmd 0x41 (READ VERIFY SECTORS NO RETRY) not supported");
			command_aborted(channel, 0x41);
			break;
		case 0x42:
			bug("read cmd 0x42 (READ VERIFY SECTORS EXT) not supported");
			command_aborted(channel, 0x42);
			break;
		case 0x50:
			bug("read cmd 0x50 (FORMAT TRACK) not supported");
			command_aborted(channel, 0x50);
			break;
		case 0x51:
			bug("read cmd 0x51 (CONFIGURE STREAM) not supported");
			command_aborted(channel, 0x51);
			break;
		case 0x70:
			bug("read cmd 0x70 (SEEK) not supported");
			command_aborted(channel, 0x70);
			break;
		case 0x87:
			bug("read cmd 0x87 (CFA TRANSLATE SECTOR) not supported");
			command_aborted(channel, 0x87);
			break;
		case 0x90:
			bug("read cmd 0x90 (EXECUTE DEVICE DIAGNOSTIC) not supported");
			command_aborted(channel, 0x90);
			break;
		case 0x91:
			bug("read cmd 0x91 (INITIALIZE DEVICE PARAMETERS) not supported");
			command_aborted(channel, 0x91);
			break;
		case 0x92:
			bug("read cmd 0x92 (DOWNLOAD MICROCODE) not supported");
			command_aborted(channel, 0x92);
			break;
		case 0x94:
			bug("read cmd 0x94 (STANDBY IMMEDIATE) not supported");
			command_aborted(channel, 0x94);
			break;
		case 0x95:
			bug("read cmd 0x95 (IDLE IMMEDIATE) not supported");
			command_aborted(channel, 0x95);
			break;
		case 0x96:
			bug("read cmd 0x96 (STANDBY) not supported");
			command_aborted(channel, 0x96);
			break;
		case 0x97:
			bug("read cmd 0x97 (IDLE) not supported");
			command_aborted(channel, 0x97);
			break;
		case 0x98:
			bug("read cmd 0x98 (CHECK POWER MODE) not supported");
			command_aborted(channel, 0x98);
			break;
		case 0x99:
			bug("read cmd 0x99 (SLEEP) not supported");
			command_aborted(channel, 0x99);
			break;
		case 0xA2:
			bug("read cmd 0xA2 (SERVICE) not supported");
			command_aborted(channel, 0xA2);
			break;
		case 0xB0:
			bug("read cmd 0xB0 (SMART DISABLE OPERATIONS) not supported");
			command_aborted(channel, 0xB0);
			break;
		case 0xB1:
			bug("read cmd 0xB1 (DEVICE CONFIGURATION FREEZE LOCK) not supported");
			command_aborted(channel, 0xB1);
			break;
		case 0xC0:
			bug("read cmd 0xC0 (CFA ERASE SECTORS) not supported");
			command_aborted(channel, 0xC0);
			break;
		case 0xC4:
			bug("read cmd 0xC4 (READ MULTIPLE) not supported");
			command_aborted(channel, 0xC4);
			break;
		case 0xC5:
			bug("read cmd 0xC5 (WRITE MULTIPLE) not supported");
			command_aborted(channel, 0xC5);
			break;
		case 0xC6:
			bug("read cmd 0xC6 (SET MULTIPLE MODE) not supported");
			command_aborted(channel, 0xC6);
			break;
		case 0xC7:
			bug("read cmd 0xC7 (READ DMA QUEUED) not supported");
			command_aborted(channel, 0xC7);
			break;
		case 0xC8:
			bug("read cmd 0xC8 (READ DMA) not supported");
			command_aborted(channel, 0xC8);
			break;
		case 0xC9:
			bug("read cmd 0xC9 (READ DMA NO RETRY) not supported");
			command_aborted(channel, 0xC9);
			break;
		case 0xCA:
			bug("read cmd 0xCA (WRITE DMA) not supported");
			command_aborted(channel, 0xCA);
			break;
		case 0xCC:
			bug("read cmd 0xCC (WRITE DMA QUEUED) not supported");
			command_aborted(channel, 0xCC);
			break;
		case 0xCD:
			bug("read cmd 0xCD (CFA WRITE MULTIPLE W/OUT ERASE) not supported");
			command_aborted(channel, 0xCD);
			break;
		case 0xD1:
			bug("read cmd 0xD1 (CHECK MEDIA CARD TYPE) not supported");
			command_aborted(channel, 0xD1);
			break;
		case 0xDA:
			bug("read cmd 0xDA (GET MEDIA STATUS) not supported");
			command_aborted(channel, 0xDA);
			break;
		case 0xDE:
			bug("read cmd 0xDE (MEDIA LOCK) not supported");
			command_aborted(channel, 0xDE);
			break;
		case 0xDF:
			bug("read cmd 0xDF (MEDIA UNLOCK) not supported");
			command_aborted(channel, 0xDF);
			break;
		case 0xE0:
			bug("read cmd 0xE0 (STANDBY IMMEDIATE) not supported");
			command_aborted(channel, 0xE0);
			break;
		case 0xE1:
			bug("read cmd 0xE1 (IDLE IMMEDIATE) not supported");
			command_aborted(channel, 0xE1);
			break;
		case 0xE2:
			bug("read cmd 0xE2 (STANDBY) not supported");
			command_aborted(channel, 0xE2);
			break;
		case 0xE3:
			bug("read cmd 0xE3 (IDLE) not supported");
			command_aborted(channel, 0xE3);
			break;
		case 0xE4:
			bug("read cmd 0xE4 (READ BUFFER) not supported");
			command_aborted(channel, 0xE4);
			break;
		case 0xE5:
			bug("read cmd 0xE5 (CHECK POWER MODE) not supported");
			command_aborted(channel, 0xE5);
			break;
		case 0xE6:
			bug("read cmd 0xE6 (SLEEP) not supported");
			command_aborted(channel, 0xE6);
			break;
		case 0xE7:
			bug("read cmd 0xE7 (FLUSH CACHE) not supported");
			command_aborted(channel, 0xE7);
			break;
		case 0xE8:
			bug("read cmd 0xE8 (WRITE BUFFER) not supported");
			command_aborted(channel, 0xE8);
			break;
		case 0xEA:
			bug("read cmd 0xEA (FLUSH CACHE EXT) not supported");
			command_aborted(channel, 0xEA);
			break;
		case 0xED:
			bug("read cmd 0xED (MEDIA EJECT) not supported");
			command_aborted(channel, 0xED);
			break;
		case 0xEF:
			bug("read cmd 0xEF (SET FEATURES) not supported");
			command_aborted(channel, 0xEF);
			break;
		case 0xF1:
			bug("read cmd 0xF1 (SECURITY SET PASSWORD) not supported");
			command_aborted(channel, 0xF1);
			break;
		case 0xF2:
			bug("read cmd 0xF2 (SECURITY UNLOCK) not supported");
			command_aborted(channel, 0xF2);
			break;
		case 0xF3:
			bug("read cmd 0xF3 (SECURITY ERASE PREPARE) not supported");
			command_aborted(channel, 0xF3);
			break;
		case 0xF4:
			bug("read cmd 0xF4 (SECURITY ERASE UNIT) not supported");
			command_aborted(channel, 0xF4);
			break;
		case 0xF5:
			bug("read cmd 0xF5 (SECURITY FREEZE LOCK) not supported");
			command_aborted(channel, 0xF5);
			break;
		case 0xF6:
			bug("read cmd 0xF6 (SECURITY DISABLE PASSWORD) not supported");
			command_aborted(channel, 0xF6);
			break;
		case 0xF8:
			bug("read cmd 0xF8 (READ NATIVE MAX ADDRESS) not supported");
			command_aborted(channel, 0xF8);
			break;
		case 0xF9:
			bug("read cmd 0xF9 (SET MAX ADDRESS) not supported");
			command_aborted(channel, 0xF9);
			break;

		default:
			panicbug("IO read(0x%08x): current command is 0x%02x", address,
					 (unsigned) BX_SELECTED_CONTROLLER(channel).current_command);
			command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
		}
		break;

	case 0x01:							// hard disk error register (f00005)
		BX_SELECTED_CONTROLLER(channel).status.err = 0;
		value8 = (!BX_SELECTED_IS_PRESENT(channel)) ? 0 : BX_SELECTED_CONTROLLER(channel).error_register;
		goto return_value8;
		break;
	case 0x02:							// hard disk sector count / interrupt reason (f00009)
		value8 = (!BX_SELECTED_IS_PRESENT(channel)) ? 0 : BX_SELECTED_CONTROLLER(channel).sector_count;
		goto return_value8;
		break;
	case 0x03:							// sector number (f0000d)
		value8 = (!BX_SELECTED_IS_PRESENT(channel)) ? 0 : BX_SELECTED_CONTROLLER(channel).sector_no;
		goto return_value8;
	case 0x04:							// cylinder low (f00011)
		// -- WARNING : On real hardware the controller registers are shared between drives. 
		// So we must respond even if the select device is not present. Some OS uses this fact 
		// to detect the disks.... minix2 for example
		value8 = (!BX_ANY_IS_PRESENT(channel)) ? 0 : (BX_SELECTED_CONTROLLER(channel).cylinder_no & 0x00ff);
		goto return_value8;
	case 0x05:							// cylinder high (f00015)
		// -- WARNING : On real hardware the controller registers are shared between drives. 
		// So we must respond even if the select device is not present. Some OS uses this fact 
		// to detect the disks.... minix2 for example
		value8 = (!BX_ANY_IS_PRESENT(channel)) ? 0 : BX_SELECTED_CONTROLLER(channel).cylinder_no >> 8;
		goto return_value8;

	case 0x06:							// hard disk drive and head register (f00019)
		// b7 Extended data field for ECC
		// b6/b5: Used to be sector size.  00=256,01=512,10=1024,11=128
		//   Since 512 was always used, bit 6 was taken to mean LBA mode:
		//     b6 1=LBA mode, 0=CHS mode
		//     b5 1
		// b4: DRV
		// b3..0 HD3..HD0
		value8 = (1 << 7) | ((BX_SELECTED_CONTROLLER(channel).lba_mode > 0) << 6) | (1 << 5) |	// 01b = 512 sector size
			(BX_HD_THIS channels[channel].drive_select << 4) | (BX_SELECTED_CONTROLLER(channel).head_no << 0);
		goto return_value8;
		break;
//BX_CONTROLLER(channel,0).lba_mode

	case 0x07:							// Hard Disk Status (f0001d)
	case 0x16:							// Hard Disk Alternate Status (f00039)
		if (!BX_ANY_IS_PRESENT(channel))
		{
			// (mch) Just return zero for these registers
			value8 = 0;
		} else
		{
			value8 = ((BX_SELECTED_CONTROLLER(channel).status.busy << 7) |
					  (BX_SELECTED_CONTROLLER(channel).status.drive_ready << 6) |
					  (BX_SELECTED_CONTROLLER(channel).status.write_fault << 5) |
					  (BX_SELECTED_CONTROLLER(channel).status.seek_complete << 4) |
					  (BX_SELECTED_CONTROLLER(channel).status.drq << 3) |
					  (BX_SELECTED_CONTROLLER(channel).status.corrected_data << 2) |
					  (BX_SELECTED_CONTROLLER(channel).status.index_pulse << 1) |
					  (BX_SELECTED_CONTROLLER(channel).status.err));
			BX_SELECTED_CONTROLLER(channel).status.index_pulse_count++;
			BX_SELECTED_CONTROLLER(channel).status.index_pulse = 0;
			if (BX_SELECTED_CONTROLLER(channel).status.index_pulse_count >= INDEX_PULSE_CYCLE)
			{
				BX_SELECTED_CONTROLLER(channel).status.index_pulse = 1;
				BX_SELECTED_CONTROLLER(channel).status.index_pulse_count = 0;
			}
		}
		if (port == 0x07)
		{
			getMFP()->setGPIPbit(0x20, 0x20);	// lower the interrupt
		}
		goto return_value8;
		break;

	case 0x17:							// Hard Disk Address Register
		// Obsolete and unsupported register.  Not driven by hard
		// disk controller.  Report all 1's.  If floppy controller
		// is handling this address, it will call this function
		// set/clear D7 (the only bit it handles), then return
		// the combined value
		value8 = 0xff;
		goto return_value8;
		break;

	default:
		switch (address)
		{
		case 0xf00008:
		case 0xf0000c:
		case 0xf00018:
		case 0xf00038:
			/*
			 * silently ignore off-by-1 addresses for secnum/seccount/head/status;
			 * they might be used to detect twisted interface cables
			 * (since EmuTOS 0.9.8)
			 */
			break;
		default:
			panicbug("hard drive: io read from address 0x%08x unsupported", (unsigned) address);
			break;
		}
		return 0;
	}

	panicbug("hard drive: shouldnt get here! PC=%08x", showPC());
	return (0);

  return_value32:
	D(bug("32-bit read from %04x = %08x {%s}", (unsigned) address, value32, BX_SELECTED_TYPE_STRING(channel)));
	return value32;

  return_value16:
	D(bug("16-bit read from %04x = %04x {%s}", (unsigned) address, value16, BX_SELECTED_TYPE_STRING(channel)));
	return value16;

  return_value8:
	D(bug("8-bit read from %04x = %02x {%s}", (unsigned) address, value8, BX_SELECTED_TYPE_STRING(channel)));
	return value8;
}


  // static IO port write callback handler
  // redirects to non-static class handler to avoid virtual functions

void bx_hard_drive_c::write_handler(void *this_ptr, Bit32u address, Bit32u value, unsigned io_len)
{
#ifndef BX_USE_HD_SMF
	bx_hard_drive_c *class_ptr = (bx_hard_drive_c *) this_ptr;

	class_ptr->write(address, value, io_len);
}

void bx_hard_drive_c::write(Bit32u address, Bit32u value, unsigned io_len)
{
#else
	UNUSED(this_ptr);
#endif // !BX_USE_HD_SMF
	off_t logical_sector;
	off_t ret;
	bool prev_control_reset;

	Bit8u channel = BX_MAX_ATA_CHANNEL;
	Bit32u port = 0xffffffff;

	for (channel = 0; channel < BX_MAX_ATA_CHANNEL; channel++)
	{
		if ((port = BX_HD_THIS channels[channel].addr2io(address)) != 0xffffffff)
			break;
	}

	if (channel == BX_MAX_ATA_CHANNEL)
	{
		D(panicbug("Unable to find ATA channel, ioport=0x%04x", address));
		return;
	}

	if (io_len > 1 && port != 0x00)
	{
		panicbug("non-byte IO write to %04x", (unsigned) address);
	}

	D(bug("IO write to %04x = %02x", (unsigned) address, (unsigned) value));

	switch (port)
	{
	case 0x00:							// hard disk data (16bit) (f00000)
		if (io_len == 1)
		{
			panicbug("byte IO read from %04x", (unsigned) address);
		}
		switch (BX_SELECTED_CONTROLLER(channel).current_command)
		{
		case 0x30:						// WRITE SECTORS
			if (BX_SELECTED_CONTROLLER(channel).buffer_index >= 512)
				panicbug("IO write(0x%08x): buffer_index >= 512", address);

#ifdef BX_SupportRepeatSpeedups
			if (BX_HD_THIS devices->bulkIOQuantumsRequested)
			{
				unsigned transferLen, quantumsMax;

				quantumsMax = (512 - BX_SELECTED_CONTROLLER(channel).buffer_index) / io_len;
				if (quantumsMax == 0)
					BX_PANIC(("IO write(1f0): not enough space for write"));
				BX_HD_THIS devices->bulkIOQuantumsTransferred = BX_HD_THIS devices->bulkIOQuantumsRequested;

				if (quantumsMax < BX_HD_THIS devices->bulkIOQuantumsTransferred)
					BX_HD_THIS devices->bulkIOQuantumsTransferred = quantumsMax;

				transferLen = io_len * BX_HD_THIS devices->bulkIOQuantumsTransferred;
				memcpy(&BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index],
					   (Bit8u *) BX_HD_THIS devices->bulkIOHostAddr, transferLen);
				BX_HD_THIS devices->bulkIOHostAddr += transferLen;

				BX_SELECTED_CONTROLLER(channel).buffer_index += transferLen;
			} else
#endif
			{
				bool bs = BX_SELECTED_DRIVE(channel).hard_drive->byteswap;	/* FALCON disk image (byte swap) */

				switch (io_len)
				{
				case 4:
					// value = ((value >> 16) & 0x0000ffff) | ((value & 0x0000ffff) << 16);	/* FALCON (word swap for long access to data register) */
					if (bs)
					{
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 0] = (Bit8u) (value >> 24);
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 1] = (Bit8u) (value >> 16);
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 2] = (Bit8u) (value >> 8);
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 3] = (Bit8u) value;
					} else
					{
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 1] = (Bit8u) (value >> 24);
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 0] = (Bit8u) (value >> 16);
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 3] = (Bit8u) (value >> 8);
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 2] = (Bit8u) value;
					}
					break;
				case 2:
					if (bs)
					{
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 0] = (Bit8u) (value >> 8);
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 1] = (Bit8u) value;
					} else
					{
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 1] = (Bit8u) (value >> 8);
						BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 0] = (Bit8u) value;
					}
					break;
				}
				BX_SELECTED_CONTROLLER(channel).buffer_index += io_len;
			}

			/* if buffer completely writtten */
			if (BX_SELECTED_CONTROLLER(channel).buffer_index >= 512)
			{
				off_t logical_sector;
				off_t ret;

				if (!calculate_logical_address(channel, &logical_sector))
				{
					bug("write reached invalid sector %lu, aborting", (unsigned long) logical_sector);
					command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
					return;
				}
				ret = BX_SELECTED_DRIVE(channel).hard_drive->lseek(logical_sector * 512, SEEK_SET);
				if (ret < 0)
				{
					bug("could not lseek() hard drive image file at byte %lu", (unsigned long) logical_sector * 512);
					command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
					return;
				}
				ret =
					BX_SELECTED_DRIVE(channel).hard_drive->write((bx_ptr_t) BX_SELECTED_CONTROLLER(channel).buffer,
																 512);
				if (ret < 512)
				{
					bug("could not write() hard drive image file at byte %lu", (unsigned long) logical_sector * 512);
					command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
					return;
				}

				BX_SELECTED_CONTROLLER(channel).buffer_index = 0;

				/* update sector count, sector number, cylinder,
				 * drive, head, status
				 * if there are more sectors, read next one in...
				 */

				increment_address(channel);

				/* When the write is complete, controller clears the DRQ bit and
				 * sets the BSY bit.
				 * If at least one more sector is to be written, controller sets DRQ bit,
				 * clears BSY bit, and issues IRQ
				 */

				if (BX_SELECTED_CONTROLLER(channel).sector_count != 0)
				{
					BX_SELECTED_CONTROLLER(channel).status.busy = 0;
					BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
					BX_SELECTED_CONTROLLER(channel).status.drq = 1;
					BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;
					BX_SELECTED_CONTROLLER(channel).status.err = 0;
				} else
				{						/* no more sectors to write */
					BX_SELECTED_CONTROLLER(channel).status.busy = 0;
					BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
					BX_SELECTED_CONTROLLER(channel).status.drq = 0;
					BX_SELECTED_CONTROLLER(channel).status.err = 0;
					BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;
				}
				raise_interrupt(channel);
			}
			break;

		case 0xa0:						// PACKET
			if (BX_SELECTED_CONTROLLER(channel).buffer_index >= PACKET_SIZE)
				panicbug("IO write(0x%08x): buffer_index >= PACKET_SIZE", address);
			BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index] = value;
			BX_SELECTED_CONTROLLER(channel).buffer[BX_SELECTED_CONTROLLER(channel).buffer_index + 1] = (value >> 8);
			BX_SELECTED_CONTROLLER(channel).buffer_index += 2;

			/* if packet completely writtten */
			if (BX_SELECTED_CONTROLLER(channel).buffer_index >= PACKET_SIZE)
			{
				// complete command received
				Bit8u atapi_command = BX_SELECTED_CONTROLLER(channel).buffer[0];

				switch (atapi_command)
				{
				case 0x00:				// test unit ready
					if (BX_SELECTED_DRIVE(channel).cdrom.ready)
					{
						atapi_cmd_nop(channel);
					} else
					{
						atapi_cmd_error(channel, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
					}
					raise_interrupt(channel);
					break;

				case 0x03:
					{					// request sense
						int alloc_length = BX_SELECTED_CONTROLLER(channel).buffer[4];

						init_send_atapi_command(channel, atapi_command, 18, alloc_length);

						// sense data
						BX_SELECTED_CONTROLLER(channel).buffer[0] = 0x70 | (1 << 7);
						BX_SELECTED_CONTROLLER(channel).buffer[1] = 0;
						BX_SELECTED_CONTROLLER(channel).buffer[2] = BX_SELECTED_DRIVE(channel).sense.sense_key;
						BX_SELECTED_CONTROLLER(channel).buffer[3] = BX_SELECTED_DRIVE(channel).sense.information.arr[0];
						BX_SELECTED_CONTROLLER(channel).buffer[4] = BX_SELECTED_DRIVE(channel).sense.information.arr[1];
						BX_SELECTED_CONTROLLER(channel).buffer[5] = BX_SELECTED_DRIVE(channel).sense.information.arr[2];
						BX_SELECTED_CONTROLLER(channel).buffer[6] = BX_SELECTED_DRIVE(channel).sense.information.arr[3];
						BX_SELECTED_CONTROLLER(channel).buffer[7] = 17 - 7;
						BX_SELECTED_CONTROLLER(channel).buffer[8] =
							BX_SELECTED_DRIVE(channel).sense.specific_inf.arr[0];
						BX_SELECTED_CONTROLLER(channel).buffer[9] =
							BX_SELECTED_DRIVE(channel).sense.specific_inf.arr[1];
						BX_SELECTED_CONTROLLER(channel).buffer[10] =
							BX_SELECTED_DRIVE(channel).sense.specific_inf.arr[2];
						BX_SELECTED_CONTROLLER(channel).buffer[11] =
							BX_SELECTED_DRIVE(channel).sense.specific_inf.arr[3];
						BX_SELECTED_CONTROLLER(channel).buffer[12] = BX_SELECTED_DRIVE(channel).sense.asc;
						BX_SELECTED_CONTROLLER(channel).buffer[13] = BX_SELECTED_DRIVE(channel).sense.ascq;
						BX_SELECTED_CONTROLLER(channel).buffer[14] = BX_SELECTED_DRIVE(channel).sense.fruc;
						BX_SELECTED_CONTROLLER(channel).buffer[15] = BX_SELECTED_DRIVE(channel).sense.key_spec.arr[0];
						BX_SELECTED_CONTROLLER(channel).buffer[16] = BX_SELECTED_DRIVE(channel).sense.key_spec.arr[1];
						BX_SELECTED_CONTROLLER(channel).buffer[17] = BX_SELECTED_DRIVE(channel).sense.key_spec.arr[2];

						ready_to_send_atapi(channel);
					}
					break;

				case 0x1b:
					{					// start stop unit
						//bool Immed = (BX_SELECTED_CONTROLLER(channel).buffer[1] >> 0) & 1;
						bool LoEj = (BX_SELECTED_CONTROLLER(channel).buffer[4] >> 1) & 1;
						bool Start = (BX_SELECTED_CONTROLLER(channel).buffer[4] >> 0) & 1;

						if (!LoEj && !Start)
						{				// stop the disc
							panicbug("Stop disc not implemented");
							atapi_cmd_nop(channel);
							raise_interrupt(channel);
						} else if (!LoEj && Start)
						{				// start (spin up) the disc
#ifdef LOWLEVEL_CDROM
							BX_SELECTED_DRIVE(channel).cdrom.cd->start_cdrom();
#endif
							panicbug("FIXME: ATAPI start disc not reading TOC");
							atapi_cmd_nop(channel);
							raise_interrupt(channel);
						} else if (LoEj && !Start)
						{				// Eject the disc
							atapi_cmd_nop(channel);

							if (BX_SELECTED_DRIVE(channel).cdrom.ready)
							{
#ifdef LOWLEVEL_CDROM
								BX_SELECTED_DRIVE(channel).cdrom.cd->eject_cdrom();
#endif
								BX_SELECTED_DRIVE(channel).cdrom.ready = 0;
								bx_options.atadevice[channel][BX_SLAVE_SELECTED(channel)].status = BX_EJECTED;
							}
							raise_interrupt(channel);
						} else
						{				// Load the disc
							// My guess is that this command only closes the tray, that's a no-op for us
							atapi_cmd_nop(channel);
							raise_interrupt(channel);
						}
					}
					break;

				case 0xbd:
					{					// mechanism status
						uint16 alloc_length = read_16bit(BX_SELECTED_CONTROLLER(channel).buffer + 8);

						if (alloc_length == 0)
							panicbug("Zero allocation length to MECHANISM STATUS not impl.");

						init_send_atapi_command(channel, atapi_command, 8, alloc_length);

						BX_SELECTED_CONTROLLER(channel).buffer[0] = 0;	// reserved for non changers
						BX_SELECTED_CONTROLLER(channel).buffer[1] = 0;	// reserved for non changers

						BX_SELECTED_CONTROLLER(channel).buffer[2] = 0;	// Current LBA (TODO!)
						BX_SELECTED_CONTROLLER(channel).buffer[3] = 0;	// Current LBA (TODO!)
						BX_SELECTED_CONTROLLER(channel).buffer[4] = 0;	// Current LBA (TODO!)

						BX_SELECTED_CONTROLLER(channel).buffer[5] = 1;	// one slot

						BX_SELECTED_CONTROLLER(channel).buffer[6] = 0;	// slot table length
						BX_SELECTED_CONTROLLER(channel).buffer[7] = 0;	// slot table length

						ready_to_send_atapi(channel);
					}
					break;

				case 0x5a:
					{					// mode sense
						uint16 alloc_length = read_16bit(BX_SELECTED_CONTROLLER(channel).buffer + 7);

						Bit8u PC = BX_SELECTED_CONTROLLER(channel).buffer[2] >> 6;
						Bit8u PageCode = BX_SELECTED_CONTROLLER(channel).buffer[2] & 0x3f;

						switch (PC)
						{
						case 0x0:		// current values
							switch (PageCode)
							{
							case 0x01:	// error recovery
								init_send_atapi_command(channel, atapi_command, sizeof(error_recovery_t) + 8,
														alloc_length);

								init_mode_sense_single(channel,
													   &BX_SELECTED_DRIVE(channel).cdrom.current.error_recovery,
													   sizeof(error_recovery_t));
								ready_to_send_atapi(channel);
								break;

							case 0x2a:	// CD-ROM capabilities & mech. status
								init_send_atapi_command(channel, atapi_command, 28, alloc_length);
								init_mode_sense_single(channel, &BX_SELECTED_CONTROLLER(channel).buffer[8], 28);
								BX_SELECTED_CONTROLLER(channel).buffer[8] = 0x2a;
								BX_SELECTED_CONTROLLER(channel).buffer[9] = 0x12;
								BX_SELECTED_CONTROLLER(channel).buffer[10] = 0x00;
								BX_SELECTED_CONTROLLER(channel).buffer[11] = 0x00;
								// Multisession, Mode 2 Form 2, Mode 2 Form 1
								BX_SELECTED_CONTROLLER(channel).buffer[12] = 0x70;
								BX_SELECTED_CONTROLLER(channel).buffer[13] = (3 << 5);
								BX_SELECTED_CONTROLLER(channel).buffer[14] = (unsigned char) (1 |
																							  (BX_SELECTED_DRIVE
																							   (channel).cdrom.
																							   locked ? (1 << 1) : 0) |
																							  (1 << 3) | (1 << 5));
								BX_SELECTED_CONTROLLER(channel).buffer[15] = 0x00;
								BX_SELECTED_CONTROLLER(channel).buffer[16] = (706 >> 8) & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[17] = 706 & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[18] = 0;
								BX_SELECTED_CONTROLLER(channel).buffer[19] = 2;
								BX_SELECTED_CONTROLLER(channel).buffer[20] = (512 >> 8) & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[21] = 512 & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[22] = (706 >> 8) & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[23] = 706 & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[24] = 0;
								BX_SELECTED_CONTROLLER(channel).buffer[25] = 0;
								BX_SELECTED_CONTROLLER(channel).buffer[26] = 0;
								BX_SELECTED_CONTROLLER(channel).buffer[27] = 0;
								ready_to_send_atapi(channel);
								break;

							case 0x0d:	// CD-ROM
							case 0x0e:	// CD-ROM audio control
							case 0x3f:	// all
								panicbug("cdrom: MODE SENSE (curr), code=%x", PageCode);
								atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
								raise_interrupt(channel);
								break;

							default:
								// not implemeted by this device
								bug("cdrom: MODE SENSE PC=%x, PageCode=%x," " not implemented by device", PC, PageCode);
								atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
								raise_interrupt(channel);
								break;
							}
							break;

						case 0x1:		// changeable values
							switch (PageCode)
							{
							case 0x01:	// error recovery
							case 0x0d:	// CD-ROM
							case 0x0e:	// CD-ROM audio control
							case 0x2a:	// CD-ROM capabilities & mech. status
							case 0x3f:	// all
								panicbug("cdrom: MODE SENSE (chg), code=%x", PageCode);
								atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
								raise_interrupt(channel);
								break;

							default:
								// not implemeted by this device
								bug("cdrom: MODE SENSE PC=%x, PageCode=%x," " not implemented by device", PC, PageCode);
								atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
								raise_interrupt(channel);
								break;
							}
							break;

						case 0x2:		// default values
							switch (PageCode)
							{
							case 0x2a:	// CD-ROM capabilities & mech. status, copied from current values
								init_send_atapi_command(channel, atapi_command, 28, alloc_length);
								init_mode_sense_single(channel, &BX_SELECTED_CONTROLLER(channel).buffer[8], 28);
								BX_SELECTED_CONTROLLER(channel).buffer[8] = 0x2a;
								BX_SELECTED_CONTROLLER(channel).buffer[9] = 0x12;
								BX_SELECTED_CONTROLLER(channel).buffer[10] = 0x00;
								BX_SELECTED_CONTROLLER(channel).buffer[11] = 0x00;
								// Multisession, Mode 2 Form 2, Mode 2 Form 1
								BX_SELECTED_CONTROLLER(channel).buffer[12] = 0x70;
								BX_SELECTED_CONTROLLER(channel).buffer[13] = (3 << 5);
								BX_SELECTED_CONTROLLER(channel).buffer[14] = (unsigned char) (1 |
																							  (BX_SELECTED_DRIVE
																							   (channel).cdrom.
																							   locked ? (1 << 1) : 0) |
																							  (1 << 3) | (1 << 5));
								BX_SELECTED_CONTROLLER(channel).buffer[15] = 0x00;
								BX_SELECTED_CONTROLLER(channel).buffer[16] = (706 >> 8) & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[17] = 706 & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[18] = 0;
								BX_SELECTED_CONTROLLER(channel).buffer[19] = 2;
								BX_SELECTED_CONTROLLER(channel).buffer[20] = (512 >> 8) & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[21] = 512 & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[22] = (706 >> 8) & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[23] = 706 & 0xff;
								BX_SELECTED_CONTROLLER(channel).buffer[24] = 0;
								BX_SELECTED_CONTROLLER(channel).buffer[25] = 0;
								BX_SELECTED_CONTROLLER(channel).buffer[26] = 0;
								BX_SELECTED_CONTROLLER(channel).buffer[27] = 0;
								ready_to_send_atapi(channel);
								break;
							case 0x01:	// error recovery
							case 0x0d:	// CD-ROM
							case 0x0e:	// CD-ROM audio control
							case 0x3f:	// all
								panicbug("cdrom: MODE SENSE (dflt), code=%x", PageCode);
								break;

							default:
								// not implemeted by this device
								bug("cdrom: MODE SENSE PC=%x, PageCode=%x," " not implemented by device", PC, PageCode);
								atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
								raise_interrupt(channel);
								break;
							}
							break;

						case 0x3:		// saved values not implemented
							atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_SAVING_PARAMETERS_NOT_SUPPORTED);
							raise_interrupt(channel);
							break;

						default:
							panicbug("Should not get here!");
							break;
						}
					}
					break;

				case 0x12:
					{					// inquiry
						uint8 alloc_length = BX_SELECTED_CONTROLLER(channel).buffer[4];

						init_send_atapi_command(channel, atapi_command, 36, alloc_length);

						BX_SELECTED_CONTROLLER(channel).buffer[0] = 0x05;	// CD-ROM
						BX_SELECTED_CONTROLLER(channel).buffer[1] = 0x80;	// Removable
						BX_SELECTED_CONTROLLER(channel).buffer[2] = 0x00;	// ISO, ECMA, ANSI version
						BX_SELECTED_CONTROLLER(channel).buffer[3] = 0x21;	// ATAPI-2, as specified
						BX_SELECTED_CONTROLLER(channel).buffer[4] = 31;	// additional length (total 36)
						BX_SELECTED_CONTROLLER(channel).buffer[5] = 0x00;	// reserved
						BX_SELECTED_CONTROLLER(channel).buffer[6] = 0x00;	// reserved
						BX_SELECTED_CONTROLLER(channel).buffer[7] = 0x00;	// reserved

						// Vendor ID
						const char *vendor_id = "VTAB    ";
						int i;

						for (i = 0; i < 8; i++)
							BX_SELECTED_CONTROLLER(channel).buffer[8 + i] = vendor_id[i];

						// Product ID
#ifdef WORDS_BIGENDIAN
						const char *product_id = "Turbo CD-ROM    ";
#else
						const char *product_id = "rbTuCDo OM-R    ";
#endif
						for (i = 0; i < 16; i++)
							BX_SELECTED_CONTROLLER(channel).buffer[16 + i] = product_id[i];

						// Product Revision level
						const char *rev_level = "1.0 ";

						for (i = 0; i < 4; i++)
							BX_SELECTED_CONTROLLER(channel).buffer[32 + i] = rev_level[i];

						ready_to_send_atapi(channel);
					}
					break;

				case 0x25:
					{					// read cd-rom capacity
						// no allocation length???
						init_send_atapi_command(channel, atapi_command, 8, 8);

						if (BX_SELECTED_DRIVE(channel).cdrom.ready)
						{
							uint32 capacity = BX_SELECTED_DRIVE(channel).cdrom.capacity;

							BX_SELECTED_CONTROLLER(channel).buffer[0] = (capacity >> 24) & 0xff;
							BX_SELECTED_CONTROLLER(channel).buffer[1] = (capacity >> 16) & 0xff;
							BX_SELECTED_CONTROLLER(channel).buffer[2] = (capacity >> 8) & 0xff;
							BX_SELECTED_CONTROLLER(channel).buffer[3] = (capacity >> 0) & 0xff;
							BX_SELECTED_CONTROLLER(channel).buffer[4] = (2048 >> 24) & 0xff;
							BX_SELECTED_CONTROLLER(channel).buffer[5] = (2048 >> 16) & 0xff;
							BX_SELECTED_CONTROLLER(channel).buffer[6] = (2048 >> 8) & 0xff;
							BX_SELECTED_CONTROLLER(channel).buffer[7] = (2048 >> 0) & 0xff;
							ready_to_send_atapi(channel);
						} else
						{
							atapi_cmd_error(channel, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
							raise_interrupt(channel);
						}
					}
					break;

				case 0xbe:
					{					// read cd
						if (BX_SELECTED_DRIVE(channel).cdrom.ready)
						{
							panicbug("Read CD with CD present not implemented");
							atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
							raise_interrupt(channel);
						} else
						{
							atapi_cmd_error(channel, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
							raise_interrupt(channel);
						}
					}
					break;

				case 0x43:
					{					// read toc
						if (BX_SELECTED_DRIVE(channel).cdrom.ready)
						{
#ifdef LOWLEVEL_CDROM
							bool msf = (BX_SELECTED_CONTROLLER(channel).buffer[1] >> 1) & 1;
							uint8 starting_track = BX_SELECTED_CONTROLLER(channel).buffer[6];
							int toc_length;
#endif
							uint16 alloc_length = read_16bit(BX_SELECTED_CONTROLLER(channel).buffer + 7);

							uint8 format = (BX_SELECTED_CONTROLLER(channel).buffer[9] >> 6);

// Win32:  I just read the TOC using Win32's IOCTRL functions (Ben)
#if defined(WIN32)
#ifdef LOWLEVEL_CDROM
							switch (format)
							{
							case 2:
							case 3:
							case 4:
								if (msf != TRUE)
								{
									D(panicbug("READ_TOC_EX: msf not set for format %i", format));
								}
								/* fall through */
							case 0:
							case 1:
							case 5:
								if (!
									(BX_SELECTED_DRIVE(channel).cdrom.cd->
									 read_toc(BX_SELECTED_CONTROLLER(channel).buffer, &toc_length, msf, starting_track,
											  format)))
								{
									atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
									raise_interrupt(channel);
								} else
								{
									init_send_atapi_command(channel, atapi_command, toc_length, alloc_length);
									ready_to_send_atapi(channel);
								}
								break;
							default:
								panicbug("(READ TOC) Format %d not supported", format);
							}
#else
							panicbug("LOWLEVEL_CDROM not defined");
#endif
#else // WIN32
							int i;

							switch (format)
							{
							case 0:
#ifdef LOWLEVEL_CDROM
								if (!
									(BX_SELECTED_DRIVE(channel).cdrom.cd->
									 read_toc(BX_SELECTED_CONTROLLER(channel).buffer, &toc_length, msf, starting_track,
											  format)))
								{
									atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
									raise_interrupt(channel);
								} else
								{
									init_send_atapi_command(channel, atapi_command, toc_length, alloc_length);
									ready_to_send_atapi(channel);
								}
#else
								panicbug("LOWLEVEL_CDROM not defined");
#endif
								break;

							case 1:
								// multi session stuff. we ignore this and emulate a single session only
								init_send_atapi_command(channel, atapi_command, 12, alloc_length);

								BX_SELECTED_CONTROLLER(channel).buffer[0] = 0;
								BX_SELECTED_CONTROLLER(channel).buffer[1] = 0x0a;
								BX_SELECTED_CONTROLLER(channel).buffer[2] = 1;
								BX_SELECTED_CONTROLLER(channel).buffer[3] = 1;
								for (i = 0; i < 8; i++)
									BX_SELECTED_CONTROLLER(channel).buffer[4 + i] = 0;

								ready_to_send_atapi(channel);
								break;

							case 2:
							default:
								panicbug("(READ TOC) Format %d not supported", format);
								break;
							}
#endif // WIN32
						} else
						{
							atapi_cmd_error(channel, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
							raise_interrupt(channel);
						}
					}
					break;

				case 0x28:				// read (10)
				case 0xa8:				// read (12)
					{
						uint32 transfer_length;

						if (atapi_command == 0x28)
							transfer_length = read_16bit(BX_SELECTED_CONTROLLER(channel).buffer + 7);
						else
							transfer_length = read_32bit(BX_SELECTED_CONTROLLER(channel).buffer + 6);

						uint32 lba = read_32bit(BX_SELECTED_CONTROLLER(channel).buffer + 2);

						if (!BX_SELECTED_DRIVE(channel).cdrom.ready)
						{
							atapi_cmd_error(channel, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
							raise_interrupt(channel);
							break;
						}
						// Ben: see comment below
						if (lba + transfer_length > BX_SELECTED_DRIVE(channel).cdrom.capacity)
						{
							transfer_length = (BX_SELECTED_DRIVE(channel).cdrom.capacity - lba);
						}
						//if (transfer_length == 0) {
						if (transfer_length <= 0)
						{
							atapi_cmd_nop(channel);
							raise_interrupt(channel);
							break;
						}

/* Ben: I commented this out and added the three lines above.  I am not sure this is the correct thing
        to do, but it seems to work.
        FIXME: I think that if the transfer_length is more than we can transfer, we should return
        some sort of flag/error/bitrep stating so.  I haven't read the atapi specs enough to know
        what needs to be done though.

                  if (lba + transfer_length > BX_SELECTED_DRIVE(channel).cdrom.capacity) {
                    atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_LOGICAL_BLOCK_OOR);
                    raise_interrupt(channel);
                    break;
                  }
*/
						D(bug("cdrom: READ (%d) LBA=%d LEN=%d", atapi_command == 0x28 ? 10 : 12, lba, transfer_length));

						// handle command
						init_send_atapi_command(channel, atapi_command, transfer_length * 2048,
												transfer_length * 2048, true);
						BX_SELECTED_DRIVE(channel).cdrom.remaining_blocks = transfer_length;
						BX_SELECTED_DRIVE(channel).cdrom.next_lba = lba;
						ready_to_send_atapi(channel);
					}
					break;

				case 0x2b:
					{					// seek
						uint32 lba = read_32bit(BX_SELECTED_CONTROLLER(channel).buffer + 2);

						if (!BX_SELECTED_DRIVE(channel).cdrom.ready)
						{
							atapi_cmd_error(channel, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
							raise_interrupt(channel);
							break;
						}

						if (lba > BX_SELECTED_DRIVE(channel).cdrom.capacity)
						{
							atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_LOGICAL_BLOCK_OOR);
							raise_interrupt(channel);
							break;
						}
						atapi_cmd_nop(channel);
						raise_interrupt(channel);
					}
					break;

				case 0x1e:
					{					// prevent/allow medium removal
						if (BX_SELECTED_DRIVE(channel).cdrom.ready)
						{
							BX_SELECTED_DRIVE(channel).cdrom.locked = BX_SELECTED_CONTROLLER(channel).buffer[4] & 1;
							atapi_cmd_nop(channel);
						} else
						{
							atapi_cmd_error(channel, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
						}
						raise_interrupt(channel);
					}
					break;

				case 0x42:
					{					// read sub-channel
						bool msf = get_packet_field(channel, 1, 1, 1);
						bool sub_q = get_packet_field(channel, 2, 6, 1);
						uint8 data_format = get_packet_byte(channel, 3);
						uint8 track_number = get_packet_byte(channel, 6);
						uint16 alloc_length = get_packet_word(channel, 7);

						UNUSED(msf);
						UNUSED(data_format);
						UNUSED(track_number);

						if (!BX_SELECTED_DRIVE(channel).cdrom.ready)
						{
							atapi_cmd_error(channel, SENSE_NOT_READY, ASC_MEDIUM_NOT_PRESENT);
							raise_interrupt(channel);
						} else
						{
							BX_SELECTED_CONTROLLER(channel).buffer[0] = 0;
							BX_SELECTED_CONTROLLER(channel).buffer[1] = 0;	// audio not supported
							BX_SELECTED_CONTROLLER(channel).buffer[2] = 0;
							BX_SELECTED_CONTROLLER(channel).buffer[3] = 0;

							int ret_len = 4;	// header size

							if (sub_q)
							{			// !sub_q == header only
								panicbug("Read sub-channel with SubQ not implemented");
								atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
								raise_interrupt(channel);
							}

							init_send_atapi_command(channel, atapi_command, ret_len, alloc_length);
							ready_to_send_atapi(channel);
						}
					}
					break;

				case 0x51:
					{					// read disc info
						// no-op to keep the Linux CD-ROM driver happy
						atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
						raise_interrupt(channel);
					}
					break;

				case 0x55:				// mode select
				case 0xa6:				// load/unload cd
				case 0x4b:				// pause/resume
				case 0x45:				// play audio
				case 0x47:				// play audio msf
				case 0xbc:				// play cd
				case 0xb9:				// read cd msf
				case 0x44:				// read header
				case 0xba:				// scan
				case 0xbb:				// set cd speed
				case 0x4e:				// stop play/scan
				case 0x46:				// ???
				case 0x4a:				// ???
					D(bug("ATAPI command 0x%x not implemented yet", atapi_command));
					atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
					raise_interrupt(channel);
					break;
				default:
					panicbug("Unknown ATAPI command 0x%x (%d)", atapi_command, atapi_command);
					// We'd better signal the error if the user chose to continue
					atapi_cmd_error(channel, SENSE_ILLEGAL_REQUEST, ASC_INV_FIELD_IN_CMD_PACKET);
					raise_interrupt(channel);
					break;
				}
			}

			break;

		default:
			panicbug("IO write(0x%08x): current command is %02xh", address,
					 (unsigned) BX_SELECTED_CONTROLLER(channel).current_command);
		}
		break;

	case 0x01:							// hard disk write precompensation (f00005)
		WRITE_FEATURES(channel, value);
		break;

	case 0x02:							// hard disk sector count (f00009)
		WRITE_SECTOR_COUNT(channel, value);
		break;

	case 0x03:							// hard disk sector number (f0000d)
		WRITE_SECTOR_NUMBER(channel, value);
		break;

	case 0x04:							// hard disk cylinder low (f00011)
		WRITE_CYLINDER_LOW(channel, value);
		break;

	case 0x05:							// hard disk cylinder high (f00015)
		WRITE_CYLINDER_HIGH(channel, value);
		break;

	case 0x06:							// hard disk drive and head register (f00019)
		// b7 Extended data field for ECC
		// b6/b5: Used to be sector size.  00=256,01=512,10=1024,11=128
		//   Since 512 was always used, bit 6 was taken to mean LBA mode:
		//     b6 1=LBA mode, 0=CHS mode
		//     b5 1
		// b4: DRV
		// b3..0 HD3..HD0
		{
			if ((value & 0xa0) != 0xa0)
			{							// 1x1xxxxx
				D(bug("IO write 0x%08x (%02x): not 1x1xxxxxb", address, (unsigned) value));
			}
#if DEBUG
			Bit32u drvsel =
#endif
				BX_HD_THIS channels[channel].drive_select = (value >> 4) & 0x01;

			WRITE_HEAD_NO(channel, value & 0xf);
			if (BX_SELECTED_CONTROLLER(channel).lba_mode == 0 && ((value >> 6) & 1) == 1)
			{
				D(bug("enabling LBA mode"));
			}
			WRITE_LBA_MODE(channel, (value >> 6) & 1);
			if (!BX_SELECTED_IS_PRESENT(channel))
			{
				D(panicbug("device set to %d which does not exist", drvsel));
				BX_SELECTED_CONTROLLER(channel).error_register = 0x04;	// aborted
				BX_SELECTED_CONTROLLER(channel).status.err = 1;
			}
			break;
		}

	case 0x07:							// hard disk command (f0001d)
		// (mch) Writes to the command register with drive_select != 0
		// are ignored if no secondary device is present
		if ((BX_SLAVE_SELECTED(channel)) && (!BX_SLAVE_IS_PRESENT(channel)))
			break;
		// Writes to the command register clear the IRQ
		getMFP()->setGPIPbit(0x20, 0x20);	// lower the interrupt

		if (BX_SELECTED_CONTROLLER(channel).status.busy)
			panicbug("hard disk: command sent, controller BUSY");
		if ((value & 0xf0) == 0x10)
			value = 0x10;
		switch (value)
		{

		case 0x10:						// CALIBRATE DRIVE
			if (!BX_SELECTED_IS_HD(channel))
			{
				D(bug("calibrate drive issued to non-disk"));
			}
			// FIXME Maybe we should signal an error in case of cdrom
			// if (!BX_SELECTED_IS_PRESENT(channel) || !BX_SELECTED_IS_HD(channel))
			if (!BX_SELECTED_IS_PRESENT(channel))
			{
				BX_SELECTED_CONTROLLER(channel).error_register = 0x02;	// Track 0 not found
				BX_SELECTED_CONTROLLER(channel).status.busy = 0;
				BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
				BX_SELECTED_CONTROLLER(channel).status.seek_complete = 0;
				BX_SELECTED_CONTROLLER(channel).status.drq = 0;
				BX_SELECTED_CONTROLLER(channel).status.err = 1;
				raise_interrupt(channel);
				break;
			}

			/* move head to cylinder 0, issue IRQ */
			BX_SELECTED_CONTROLLER(channel).error_register = 0;
			BX_SELECTED_CONTROLLER(channel).cylinder_no = 0;
			BX_SELECTED_CONTROLLER(channel).status.busy = 0;
			BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
			BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
			BX_SELECTED_CONTROLLER(channel).status.drq = 0;
			BX_SELECTED_CONTROLLER(channel).status.err = 0;
			raise_interrupt(channel);
			break;

		case 0x20:						// READ MULTIPLE SECTORS, with retries
		case 0x21:						// READ MULTIPLE SECTORS, without retries
			/* update sector_no, always points to current sector
			 * after each sector is read to buffer, DRQ bit set and issue IRQ
			 * if interrupt handler transfers all data words into main memory,
			 * and more sectors to read, then set BSY bit again, clear DRQ and
			 * read next sector into buffer
			 * sector count of 0 means 256 sectors
			 */

			if (!BX_SELECTED_IS_HD(channel))
			{
				bug("read multiple issued to non-disk");
				command_aborted(channel, value);
				break;
			}

			BX_SELECTED_CONTROLLER(channel).current_command = value;

			// Lose98 accesses 0/0/0 in CHS mode
			if (!BX_SELECTED_CONTROLLER(channel).lba_mode &&
				!BX_SELECTED_CONTROLLER(channel).head_no &&
				!BX_SELECTED_CONTROLLER(channel).cylinder_no && !BX_SELECTED_CONTROLLER(channel).sector_no)
			{
				command_aborted(channel, value);
				break;
			}

			if (!calculate_logical_address(channel, &logical_sector))
			{
				bug("initial read from sector %lu out of bounds, aborting", (unsigned long) logical_sector);
				command_aborted(channel, value);
				break;
			}
			ret = BX_SELECTED_DRIVE(channel).hard_drive->lseek(logical_sector * 512, SEEK_SET);
			if (ret < 0)
			{
				bug("could not lseek() hard drive image file, aborting");
				command_aborted(channel, value);
				break;
			}
			ret = BX_SELECTED_DRIVE(channel).hard_drive->read((bx_ptr_t) BX_SELECTED_CONTROLLER(channel).buffer, 512);
			if (ret < 512)
			{
				bug("logical sector was %lu", (unsigned long) logical_sector);
				bug("could not read() hard drive image file at byte %lu", (unsigned long) logical_sector * 512);
				command_aborted(channel, value);
				break;
			}

			BX_SELECTED_CONTROLLER(channel).error_register = 0;
			BX_SELECTED_CONTROLLER(channel).status.busy = 0;
			BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
			BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
			BX_SELECTED_CONTROLLER(channel).status.drq = 1;
			BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;
			BX_SELECTED_CONTROLLER(channel).status.err = 0;
			BX_SELECTED_CONTROLLER(channel).buffer_index = 0;
			raise_interrupt(channel);
			break;

		case 0x30:						/* WRITE SECTORS, with retries */
			/* update sector_no, always points to current sector
			 * after each sector is read to buffer, DRQ bit set and issue IRQ
			 * if interrupt handler transfers all data words into main memory,
			 * and more sectors to read, then set BSY bit again, clear DRQ and
			 * read next sector into buffer
			 * sector count of 0 means 256 sectors
			 */

			if (!BX_SELECTED_IS_HD(channel))
				panicbug("write multiple issued to non-disk");

			if (BX_SELECTED_CONTROLLER(channel).status.busy)
			{
				panicbug("write command: BSY bit set");
			}
			BX_SELECTED_CONTROLLER(channel).current_command = value;

			// implicit seek done :^)
			BX_SELECTED_CONTROLLER(channel).error_register = 0;
			BX_SELECTED_CONTROLLER(channel).status.busy = 0;
			// BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
			BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
			BX_SELECTED_CONTROLLER(channel).status.drq = 1;
			BX_SELECTED_CONTROLLER(channel).status.err = 0;
			BX_SELECTED_CONTROLLER(channel).buffer_index = 0;
			break;

		case 0x90:						// EXECUTE DEVICE DIAGNOSTIC
			if (BX_SELECTED_CONTROLLER(channel).status.busy)
			{
				panicbug("diagnostic command: BSY bit set");
			}
			if (!BX_SELECTED_IS_HD(channel))
				panicbug("drive diagnostics issued to non-disk");
			BX_SELECTED_CONTROLLER(channel).error_register = 0x81;	// Drive 1 failed, no error on drive 0
			// BX_SELECTED_CONTROLLER(channel).status.busy = 0; // not needed
			BX_SELECTED_CONTROLLER(channel).status.drq = 0;
			BX_SELECTED_CONTROLLER(channel).status.err = 0;
			break;

		case 0x91:						// INITIALIZE DRIVE PARAMETERS
			if (BX_SELECTED_CONTROLLER(channel).status.busy)
			{
				panicbug("init drive parameters command: BSY bit set");
			}
			if (!BX_SELECTED_IS_HD(channel))
				panicbug("initialize drive parameters issued to non-disk");
			// sets logical geometry of specified drive
			D(bug("init drive params: sec=%u, drive sel=%u, head=%u",
				  (unsigned) BX_SELECTED_CONTROLLER(channel).sector_count,
				  (unsigned) BX_HD_THIS channels[channel].drive_select,
				  (unsigned) BX_SELECTED_CONTROLLER(channel).head_no));
			if (!BX_SELECTED_IS_PRESENT(channel))
			{
				panicbug("init drive params: disk ata%d-%d not present", channel, BX_SLAVE_SELECTED(channel));
				//BX_SELECTED_CONTROLLER(channel).error_register = 0x12;
				BX_SELECTED_CONTROLLER(channel).status.busy = 0;
				BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
				BX_SELECTED_CONTROLLER(channel).status.drq = 0;
				BX_SELECTED_CONTROLLER(channel).status.err = 0;
				raise_interrupt(channel);
				break;
			}
			if (BX_SELECTED_CONTROLLER(channel).sector_count != BX_SELECTED_DRIVE(channel).hard_drive->sectors)
				panicbug("init drive params: sector count doesnt match %d != %d",
						 BX_SELECTED_CONTROLLER(channel).sector_count, BX_SELECTED_DRIVE(channel).hard_drive->sectors);
			if (BX_SELECTED_CONTROLLER(channel).head_no != (BX_SELECTED_DRIVE(channel).hard_drive->heads - 1))
				panicbug("init drive params: head number doesn't match %d != %d",
						 BX_SELECTED_CONTROLLER(channel).head_no, BX_SELECTED_DRIVE(channel).hard_drive->heads - 1);
			BX_SELECTED_CONTROLLER(channel).status.busy = 0;
			BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
			BX_SELECTED_CONTROLLER(channel).status.drq = 0;
			BX_SELECTED_CONTROLLER(channel).status.err = 0;
			raise_interrupt(channel);
			break;

		case 0xec:						// IDENTIFY DEVICE
			if (bx_options.newHardDriveSupport)
			{
				if (!BX_SELECTED_IS_PRESENT(channel))
				{
					D(bug("disk ata%d-%d not present, aborting", channel, BX_SLAVE_SELECTED(channel)));
					command_aborted(channel, value);
					break;
				}
				if (BX_SELECTED_IS_CD(channel))
				{
					BX_SELECTED_CONTROLLER(channel).head_no = 0;
					BX_SELECTED_CONTROLLER(channel).sector_count = 1;
					BX_SELECTED_CONTROLLER(channel).sector_no = 1;
					BX_SELECTED_CONTROLLER(channel).cylinder_no = 0xeb14;
					command_aborted(channel, 0xec);
				} else
				{
					BX_SELECTED_CONTROLLER(channel).current_command = value;
					BX_SELECTED_CONTROLLER(channel).error_register = 0;

					// See ATA/ATAPI-4, 8.12
					BX_SELECTED_CONTROLLER(channel).status.busy = 0;
					BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
					BX_SELECTED_CONTROLLER(channel).status.write_fault = 0;
					BX_SELECTED_CONTROLLER(channel).status.drq = 1;
					BX_SELECTED_CONTROLLER(channel).status.err = 0;

					BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
					BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;

					BX_SELECTED_CONTROLLER(channel).buffer_index = 0;
					raise_interrupt(channel);
					identify_drive(channel);
				}
			} else
			{
				D(bug("sent IDENTIFY DEVICE (0xec) to old hard drive"));
				command_aborted(channel, value);
			}
			break;

		case 0xef:						// SET FEATURES
			switch (BX_SELECTED_CONTROLLER(channel).features)
			{
			case 0x02:					// Enable and
			case 0x82:					//  Disable write cache.
			case 0xAA:					// Enable and
			case 0x55:					//  Disable look-ahead cache.
			case 0xCC:					// Enable and
			case 0x66:					//  Disable reverting to power-on default
			case 0x03:					// Set Transfer Mode
				D(bug("SET FEATURES subcommand not supported by disk."));
				command_aborted(channel, value);
				break;

			default:
				panicbug("SET FEATURES with unknown subcommand: 0x%02x",
						 (unsigned) BX_SELECTED_CONTROLLER(channel).features);
				// We'd better signal the error if the user chose to continue
				command_aborted(channel, value);
			}
			break;

		case 0x40:						// READ VERIFY SECTORS
			if (bx_options.newHardDriveSupport)
			{
				if (!BX_SELECTED_IS_HD(channel))
					panicbug("read verify issued to non-disk");
				BX_SELECTED_CONTROLLER(channel).status.busy = 0;
				BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
				BX_SELECTED_CONTROLLER(channel).status.drq = 0;
				BX_SELECTED_CONTROLLER(channel).status.err = 0;
				raise_interrupt(channel);
			} else
			{
				command_aborted(channel, value);
			}
			break;

		case 0xc6:						// SET MULTIPLE MODE (mch)
			if (BX_SELECTED_CONTROLLER(channel).sector_count != 128 &&
				BX_SELECTED_CONTROLLER(channel).sector_count != 64 &&
				BX_SELECTED_CONTROLLER(channel).sector_count != 32 &&
				BX_SELECTED_CONTROLLER(channel).sector_count != 16 &&
				BX_SELECTED_CONTROLLER(channel).sector_count != 8 &&
				BX_SELECTED_CONTROLLER(channel).sector_count != 4 && BX_SELECTED_CONTROLLER(channel).sector_count != 2)
				command_aborted(channel, value);

			if (!BX_SELECTED_IS_HD(channel))
				panicbug("set multiple mode issued to non-disk");

			BX_SELECTED_CONTROLLER(channel).sectors_per_block = BX_SELECTED_CONTROLLER(channel).sector_count;
			BX_SELECTED_CONTROLLER(channel).status.busy = 0;
			BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
			BX_SELECTED_CONTROLLER(channel).status.write_fault = 0;
			BX_SELECTED_CONTROLLER(channel).status.drq = 0;
			BX_SELECTED_CONTROLLER(channel).status.err = 0;
			break;

			// ATAPI commands
		case 0xa1:						// IDENTIFY PACKET DEVICE
			if (BX_SELECTED_IS_CD(channel))
			{
				BX_SELECTED_CONTROLLER(channel).current_command = value;
				BX_SELECTED_CONTROLLER(channel).error_register = 0;

				BX_SELECTED_CONTROLLER(channel).status.busy = 0;
				BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
				BX_SELECTED_CONTROLLER(channel).status.write_fault = 0;
				BX_SELECTED_CONTROLLER(channel).status.drq = 1;
				BX_SELECTED_CONTROLLER(channel).status.err = 0;

				BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
				BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;

				BX_SELECTED_CONTROLLER(channel).buffer_index = 0;
				raise_interrupt(channel);
				identify_ATAPI_drive(channel);
			} else
			{
				command_aborted(channel, 0xa1);
			}
			break;

		case 0x08:						// DEVICE RESET (atapi)
			if (BX_SELECTED_IS_CD(channel))
			{
				BX_SELECTED_CONTROLLER(channel).status.busy = 1;
				BX_SELECTED_CONTROLLER(channel).error_register &= ~(1 << 7);

				// device signature
				BX_SELECTED_CONTROLLER(channel).head_no = 0;
				BX_SELECTED_CONTROLLER(channel).sector_count = 1;
				BX_SELECTED_CONTROLLER(channel).sector_no = 1;
				BX_SELECTED_CONTROLLER(channel).cylinder_no = 0xeb14;

				BX_SELECTED_CONTROLLER(channel).status.write_fault = 0;
				BX_SELECTED_CONTROLLER(channel).status.drq = 0;
				BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;
				BX_SELECTED_CONTROLLER(channel).status.err = 0;

				BX_SELECTED_CONTROLLER(channel).status.busy = 0;

			} else
			{
				D(bug("ATAPI Device Reset on non-cd device"));
				command_aborted(channel, 0x08);
			}
			break;

		case 0xa0:						// SEND PACKET (atapi)
			if (BX_SELECTED_IS_CD(channel))
			{
				// PACKET
				if (BX_SELECTED_CONTROLLER(channel).features & (1 << 0))
					panicbug("PACKET-DMA not supported");
				if (BX_SELECTED_CONTROLLER(channel).features & (1 << 1))
					panicbug("PACKET-overlapped not supported");

				// We're already ready!
				BX_SELECTED_CONTROLLER(channel).sector_count = 1;
				BX_SELECTED_CONTROLLER(channel).status.busy = 0;
				BX_SELECTED_CONTROLLER(channel).status.write_fault = 0;
				// serv bit??
				BX_SELECTED_CONTROLLER(channel).status.drq = 1;
				BX_SELECTED_CONTROLLER(channel).status.err = 0;

				// NOTE: no interrupt here
				BX_SELECTED_CONTROLLER(channel).current_command = value;
				BX_SELECTED_CONTROLLER(channel).buffer_index = 0;
			} else
			{
				command_aborted(channel, 0xa0);
			}
			break;

		case 0xa2:						// SERVICE (atapi), optional
			if (BX_SELECTED_IS_CD(channel))
			{
				panicbug("ATAPI SERVICE not implemented");
			} else
			{
				command_aborted(channel, 0xa2);
			}
			break;

			// power management
		case 0xe5:						// CHECK POWER MODE
			BX_SELECTED_CONTROLLER(channel).status.busy = 0;
			BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
			BX_SELECTED_CONTROLLER(channel).status.write_fault = 0;
			BX_SELECTED_CONTROLLER(channel).status.drq = 0;
			BX_SELECTED_CONTROLLER(channel).status.err = 0;
			BX_SELECTED_CONTROLLER(channel).sector_count = 0xff;	// Active or Idle mode
			raise_interrupt(channel);
			break;

		case 0x70:						// SEEK (cgs)
			if (BX_SELECTED_IS_HD(channel))
			{
				D(bug("write cmd 0x70 (SEEK) executing"));
				if (!calculate_logical_address(channel, &logical_sector))
				{
					panicbug("initial seek to sector %lu out of bounds, aborting", (unsigned long) logical_sector);
					command_aborted(channel, value);
					break;
				}
				BX_SELECTED_CONTROLLER(channel).error_register = 0;
				BX_SELECTED_CONTROLLER(channel).status.busy = 0;
				BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
				BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
				BX_SELECTED_CONTROLLER(channel).status.drq = 0;
				BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;
				BX_SELECTED_CONTROLLER(channel).status.err = 0;
				BX_SELECTED_CONTROLLER(channel).buffer_index = 0;
				D(bug("s[0].controller.control.disable_irq = %02x",
				   (BX_HD_THIS channels[channel].drives[0]).controller.control.disable_irq));
				D(bug("s[1].controller.control.disable_irq = %02x",
				   (BX_HD_THIS channels[channel].drives[1]).controller.control.disable_irq));
				D(bug("SEEK completed.  error_register = %02x", BX_SELECTED_CONTROLLER(channel).error_register));
				raise_interrupt(channel);
				D(bug("SEEK interrupt completed"));
			} else
			{
				panicbug("write cmd 0x70 (SEEK) not supported for non-disk");
				command_aborted(channel, 0x70);
			}
			break;

		case 0xC8:						// READ DMA
			if (ATA_DMA)
			{
				BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
				BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
				BX_SELECTED_CONTROLLER(channel).status.drq = 1;
				BX_SELECTED_CONTROLLER(channel).current_command = value;
			} else
			{
				D(panicbug("write cmd 0xC8 (READ DMA) not supported"));
				command_aborted(channel, 0xC8);
			}
			break;

		case 0xCA:						// WRITE DMA
			if (ATA_DMA)
			{
				BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
				BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
				BX_SELECTED_CONTROLLER(channel).status.drq = 1;
				BX_SELECTED_CONTROLLER(channel).current_command = value;
			} else
			{
				D(panicbug("write cmd 0xCA (WRITE DMA) not supported"));
				command_aborted(channel, 0xCA);
			}
			break;


			// List all the write operations that are defined in the ATA/ATAPI spec
			// that we don't support.  Commands that are listed here will cause a
			// BX_ERROR, which is non-fatal, and the command will be aborted.
		case 0x22:
			bug("write cmd 0x22 (READ LONG) not supported");
			command_aborted(channel, 0x22);
			break;
		case 0x23:
			bug("write cmd 0x23 (READ LONG NO RETRY) not supported");
			command_aborted(channel, 0x23);
			break;
		case 0x24:
			bug("write cmd 0x24 (READ SECTORS EXT) not supported");
			command_aborted(channel, 0x24);
			break;
		case 0x25:
			bug("write cmd 0x25 (READ DMA EXT) not supported");
			command_aborted(channel, 0x25);
			break;
		case 0x26:
			bug("write cmd 0x26 (READ DMA QUEUED EXT) not supported");
			command_aborted(channel, 0x26);
			break;
		case 0x27:
			bug("write cmd 0x27 (READ NATIVE MAX ADDRESS EXT) not supported");
			command_aborted(channel, 0x27);
			break;
		case 0x29:
			bug("write cmd 0x29 (READ MULTIPLE EXT) not supported");
			command_aborted(channel, 0x29);
			break;
		case 0x2A:
			bug("write cmd 0x2A (READ STREAM DMA) not supported");
			command_aborted(channel, 0x2A);
			break;
		case 0x2B:
			bug("write cmd 0x2B (READ STREAM PIO) not supported");
			command_aborted(channel, 0x2B);
			break;
		case 0x2F:
			bug("write cmd 0x2F (READ LOG EXT) not supported");
			command_aborted(channel, 0x2F);
			break;
		case 0x31:
			bug("write cmd 0x31 (WRITE SECTORS NO RETRY) not supported");
			command_aborted(channel, 0x31);
			break;
		case 0x32:
			bug("write cmd 0x32 (WRITE LONG) not supported");
			command_aborted(channel, 0x32);
			break;
		case 0x33:
			bug("write cmd 0x33 (WRITE LONG NO RETRY) not supported");
			command_aborted(channel, 0x33);
			break;
		case 0x34:
			bug("write cmd 0x34 (WRITE SECTORS EXT) not supported");
			command_aborted(channel, 0x34);
			break;
		case 0x35:
			bug("write cmd 0x35 (WRITE DMA EXT) not supported");
			command_aborted(channel, 0x35);
			break;
		case 0x36:
			bug("write cmd 0x36 (WRITE DMA QUEUED EXT) not supported");
			command_aborted(channel, 0x36);
			break;
		case 0x37:
			bug("write cmd 0x37 (SET MAX ADDRESS EXT) not supported");
			command_aborted(channel, 0x37);
			break;
		case 0x38:
			bug("write cmd 0x38 (CFA WRITE SECTORS W/OUT ERASE) not supported");
			command_aborted(channel, 0x38);
			break;
		case 0x39:
			bug("write cmd 0x39 (WRITE MULTIPLE EXT) not supported");
			command_aborted(channel, 0x39);
			break;
		case 0x3A:
			bug("write cmd 0x3A (WRITE STREAM DMA) not supported");
			command_aborted(channel, 0x3A);
			break;
		case 0x3B:
			bug("write cmd 0x3B (WRITE STREAM PIO) not supported");
			command_aborted(channel, 0x3B);
			break;
		case 0x3F:
			bug("write cmd 0x3F (WRITE LOG EXT) not supported");
			command_aborted(channel, 0x3F);
			break;
		case 0x41:
			bug("write cmd 0x41 (READ VERIFY SECTORS NO RETRY) not supported");
			command_aborted(channel, 0x41);
			break;
		case 0x42:
			bug("write cmd 0x42 (READ VERIFY SECTORS EXT) not supported");
			command_aborted(channel, 0x42);
			break;
		case 0x50:
			bug("write cmd 0x50 (FORMAT TRACK) not supported");
			command_aborted(channel, 0x50);
			break;
		case 0x51:
			bug("write cmd 0x51 (CONFIGURE STREAM) not supported");
			command_aborted(channel, 0x51);
			break;
		case 0x87:
			bug("write cmd 0x87 (CFA TRANSLATE SECTOR) not supported");
			command_aborted(channel, 0x87);
			break;
		case 0x92:
			bug("write cmd 0x92 (DOWNLOAD MICROCODE) not supported");
			command_aborted(channel, 0x92);
			break;
		case 0x94:
			bug("write cmd 0x94 (STANDBY IMMEDIATE) not supported");
			command_aborted(channel, 0x94);
			break;
		case 0x95:
			bug("write cmd 0x95 (IDLE IMMEDIATE) not supported");
			command_aborted(channel, 0x95);
			break;
		case 0x96:
			bug("write cmd 0x96 (STANDBY) not supported");
			command_aborted(channel, 0x96);
			break;
		case 0x97:
			bug("write cmd 0x97 (IDLE) not supported");
			command_aborted(channel, 0x97);
			break;
		case 0x98:
			bug("write cmd 0x98 (CHECK POWER MODE) not supported");
			command_aborted(channel, 0x98);
			break;
		case 0x99:
			bug("write cmd 0x99 (SLEEP) not supported");
			command_aborted(channel, 0x99);
			break;
		case 0xB0:
			bug("write cmd 0xB0 (SMART commands) not supported");
			command_aborted(channel, 0xB0);
			break;
		case 0xB1:
			bug("write cmd 0xB1 (DEVICE CONFIGURATION commands) not supported");
			command_aborted(channel, 0xB1);
			break;
		case 0xC0:
			bug("write cmd 0xC0 (CFA ERASE SECTORS) not supported");
			command_aborted(channel, 0xC0);
			break;
		case 0xC4:
			bug("write cmd 0xC4 (READ MULTIPLE) not supported");
			command_aborted(channel, 0xC4);
			break;
		case 0xC5:
			bug("write cmd 0xC5 (WRITE MULTIPLE) not supported");
			command_aborted(channel, 0xC5);
			break;
		case 0xC7:
			bug("write cmd 0xC7 (READ DMA QUEUED) not supported");
			command_aborted(channel, 0xC7);
			break;
		case 0xC9:
			bug("write cmd 0xC9 (READ DMA NO RETRY) not supported");
			command_aborted(channel, 0xC9);
			break;
		case 0xCC:
			bug("write cmd 0xCC (WRITE DMA QUEUED) not supported");
			command_aborted(channel, 0xCC);
			break;
		case 0xCD:
			bug("write cmd 0xCD (CFA WRITE MULTIPLE W/OUT ERASE) not supported");
			command_aborted(channel, 0xCD);
			break;
		case 0xD1:
			bug("write cmd 0xD1 (CHECK MEDIA CARD TYPE) not supported");
			command_aborted(channel, 0xD1);
			break;
		case 0xDA:
			bug("write cmd 0xDA (GET MEDIA STATUS) not supported");
			command_aborted(channel, 0xDA);
			break;
		case 0xDE:
			bug("write cmd 0xDE (MEDIA LOCK) not supported");
			command_aborted(channel, 0xDE);
			break;
		case 0xDF:
			bug("write cmd 0xDF (MEDIA UNLOCK) not supported");
			command_aborted(channel, 0xDF);
			break;
		case 0xE0:
			bug("write cmd 0xE0 (STANDBY IMMEDIATE) not supported");
			command_aborted(channel, 0xE0);
			break;
		case 0xE1:
			bug("write cmd 0xE1 (IDLE IMMEDIATE) not supported");
			command_aborted(channel, 0xE1);
			break;
		case 0xE2:
			bug("write cmd 0xE2 (STANDBY) not supported");
			command_aborted(channel, 0xE2);
			break;
		case 0xE3:
			bug("write cmd 0xE3 (IDLE) not supported");
			command_aborted(channel, 0xE3);
			break;
		case 0xE4:
			bug("write cmd 0xE4 (READ BUFFER) not supported");
			command_aborted(channel, 0xE4);
			break;
		case 0xE6:
			bug("write cmd 0xE6 (SLEEP) not supported");
			command_aborted(channel, 0xE6);
			break;
		case 0xE7:
			bug("write cmd 0xE7 (FLUSH CACHE) not supported");
			command_aborted(channel, 0xE7);
			break;
		case 0xE8:
			bug("write cmd 0xE8 (WRITE BUFFER) not supported");
			command_aborted(channel, 0xE8);
			break;
		case 0xEA:
			bug("write cmd 0xEA (FLUSH CACHE EXT) not supported");
			command_aborted(channel, 0xEA);
			break;
		case 0xED:
			bug("write cmd 0xED (MEDIA EJECT) not supported");
			command_aborted(channel, 0xED);
			break;
		case 0xF1:
			bug("write cmd 0xF1 (SECURITY SET PASSWORD) not supported");
			command_aborted(channel, 0xF1);
			break;
		case 0xF2:
			bug("write cmd 0xF2 (SECURITY UNLOCK) not supported");
			command_aborted(channel, 0xF2);
			break;
		case 0xF3:
			bug("write cmd 0xF3 (SECURITY ERASE PREPARE) not supported");
			command_aborted(channel, 0xF3);
			break;
		case 0xF4:
			bug("write cmd 0xF4 (SECURITY ERASE UNIT) not supported");
			command_aborted(channel, 0xF4);
			break;
		case 0xF5:
			bug("write cmd 0xF5 (SECURITY FREEZE LOCK) not supported");
			command_aborted(channel, 0xF5);
			break;
		case 0xF6:
			bug("write cmd 0xF6 (SECURITY DISABLE PASSWORD) not supported");
			command_aborted(channel, 0xF6);
			break;
		case 0xF8:
			bug("write cmd 0xF8 (READ NATIVE MAX ADDRESS) not supported");
			command_aborted(channel, 0xF8);
			break;
		case 0xF9:
			bug("write cmd 0xF9 (SET MAX ADDRESS) not supported");
			command_aborted(channel, 0xF9);
			break;

		default:
			panicbug("IO write(1f7h): command 0x%02x", (unsigned) value);
			// if user foolishly decides to continue, abort the command
			// so that the software knows the drive didn't understand it.
			command_aborted(channel, value);
		}
		break;

	case 0x16:							// hard disk adapter control (f00039)
		// (mch) Even if device 1 was selected, a write to this register
		// goes to device 0 (if device 1 is absent)

		prev_control_reset = BX_SELECTED_CONTROLLER(channel).control.reset;
		BX_HD_THIS channels[channel].drives[0].controller.control.reset = value & 0x04;
		BX_HD_THIS channels[channel].drives[1].controller.control.reset = value & 0x04;

		// CGS: was: BX_SELECTED_CONTROLLER(channel).control.disable_irq    = value & 0x02;
		BX_HD_THIS channels[channel].drives[0].controller.control.disable_irq = value & 0x02;
		BX_HD_THIS channels[channel].drives[1].controller.control.disable_irq = value & 0x02;

		D(bug("adpater control reg: reset controller = %d",
			  (unsigned) (BX_SELECTED_CONTROLLER(channel).control.reset) ? 1 : 0));
		D(bug("adpater control reg: disable_irq(X) = %d",
			  (unsigned) (BX_SELECTED_CONTROLLER(channel).control.disable_irq) ? 1 : 0));

		if (!prev_control_reset && BX_SELECTED_CONTROLLER(channel).control.reset)
		{
			// transition from 0 to 1 causes all drives to reset
			BX_DEBUG(("hard drive: RESET"));

			// (mch) Set BSY, drive not ready
			for (int id = 0; id < 2; id++)
			{
				BX_CONTROLLER(channel, id).status.busy = 1;
				BX_CONTROLLER(channel, id).status.drive_ready = 0;
				BX_CONTROLLER(channel, id).reset_in_progress = 1;

				BX_CONTROLLER(channel, id).status.write_fault = 0;
				BX_CONTROLLER(channel, id).status.seek_complete = 1;
				BX_CONTROLLER(channel, id).status.drq = 0;
				BX_CONTROLLER(channel, id).status.corrected_data = 0;
				BX_CONTROLLER(channel, id).status.err = 0;

				BX_CONTROLLER(channel, id).error_register = 0x01;	// diagnostic code: no error

				BX_CONTROLLER(channel, id).current_command = 0x00;
				BX_CONTROLLER(channel, id).buffer_index = 0;

				BX_CONTROLLER(channel, id).sectors_per_block = 0x80;
				BX_CONTROLLER(channel, id).lba_mode = 0;

				BX_CONTROLLER(channel, id).control.disable_irq = 0;
				getMFP()->setGPIPbit(0x20, 0x20);	// lower the interrupt
			}
		} else if (BX_SELECTED_CONTROLLER(channel).reset_in_progress && !BX_SELECTED_CONTROLLER(channel).control.reset)
		{
			// Clear BSY and DRDY
			D(bug("Reset complete {%s}", BX_SELECTED_TYPE_STRING(channel)));
			for (int id = 0; id < 2; id++)
			{
				BX_CONTROLLER(channel, id).status.busy = 0;
				BX_CONTROLLER(channel, id).status.drive_ready = 1;
				BX_CONTROLLER(channel, id).reset_in_progress = 0;

				// Device signature
				if (BX_DRIVE_IS_HD(channel, id))
				{
					BX_CONTROLLER(channel, id).head_no = 0;
					BX_CONTROLLER(channel, id).sector_count = 1;
					BX_CONTROLLER(channel, id).sector_no = 1;
					BX_CONTROLLER(channel, id).cylinder_no = 0;
				} else
				{
					BX_CONTROLLER(channel, id).head_no = 0;
					BX_CONTROLLER(channel, id).sector_count = 1;
					BX_CONTROLLER(channel, id).sector_no = 1;
					BX_CONTROLLER(channel, id).cylinder_no = 0xeb14;
				}
			}
		}
		D(bug("s[0].controller.control.disable_irq = %02x",
		   (BX_HD_THIS channels[channel].drives[0]).controller.control.disable_irq));
		D(bug("s[1].controller.control.disable_irq = %02x",
		   (BX_HD_THIS channels[channel].drives[1]).controller.control.disable_irq));
		break;

	default:
		switch (address)
		{
		case 0xf00008:
		case 0xf0000c:
		case 0xf00018:
		case 0xf00038:
			/*
			 * silently ignore off-by-1 addresses for secnum/seccount/head/status;
			 * they might be used to detect twisted interface cables
			 * (since EmuTOS 0.9.8)
			 */
			break;
		default:
			panicbug("hard drive: io write to address 0x%08x = %02x", (unsigned) address, (unsigned) value);
			break;
		}
	}
}

void bx_hard_drive_c::close_harddrive(void)
{
	for (Bit8u channel = 0; channel < BX_MAX_ATA_CHANNEL; channel++)
	{
		BX_HD_THIS channels[channel].drives[0].hard_drive->close();
		BX_HD_THIS channels[channel].drives[1].hard_drive->close();
	}
}


bool bx_hard_drive_c::calculate_logical_address(Bit8u channel, off_t * sector)
{
	off_t logical_sector;

	if (BX_SELECTED_CONTROLLER(channel).lba_mode)
	{
		logical_sector = ((Bit32u) BX_SELECTED_CONTROLLER(channel).head_no) << 24 |
			((Bit32u) BX_SELECTED_CONTROLLER(channel).cylinder_no) << 8 |
			(Bit32u) BX_SELECTED_CONTROLLER(channel).sector_no;
	} else
		logical_sector =
			((uint32) BX_SELECTED_CONTROLLER(channel).cylinder_no * BX_SELECTED_DRIVE(channel).hard_drive->heads *
			 BX_SELECTED_DRIVE(channel).hard_drive->sectors) +
			(Bit32u) (BX_SELECTED_CONTROLLER(channel).head_no * BX_SELECTED_DRIVE(channel).hard_drive->sectors) +
			(BX_SELECTED_CONTROLLER(channel).sector_no - 1);

	Bit32u sector_count =
		(Bit32u) BX_SELECTED_DRIVE(channel).hard_drive->cylinders *
		(Bit32u) BX_SELECTED_DRIVE(channel).hard_drive->heads * (Bit32u) BX_SELECTED_DRIVE(channel).hard_drive->sectors;

	*sector = logical_sector;
	if (logical_sector >= (off_t) sector_count)
	{
		bug("calc_log_addr: out of bounds");
		return false;
	}
	return true;
}

void bx_hard_drive_c::increment_address(Bit8u channel)
{
	BX_SELECTED_CONTROLLER(channel).sector_count--;

	if (BX_SELECTED_CONTROLLER(channel).lba_mode)
	{
		off_t current_address;

		calculate_logical_address(channel, &current_address);
		current_address++;
		BX_SELECTED_CONTROLLER(channel).head_no = (Bit8u) ((current_address >> 24) & 0xf);
		BX_SELECTED_CONTROLLER(channel).cylinder_no = (Bit16u) ((current_address >> 8) & 0xffff);
		BX_SELECTED_CONTROLLER(channel).sector_no = (Bit8u) ((current_address) & 0xff);
	} else
	{
		BX_SELECTED_CONTROLLER(channel).sector_no++;
		if (BX_SELECTED_CONTROLLER(channel).sector_no > BX_SELECTED_DRIVE(channel).hard_drive->sectors
			|| BX_SELECTED_CONTROLLER(channel).sector_no == 0)
		{
			BX_SELECTED_CONTROLLER(channel).sector_no = 1;
			BX_SELECTED_CONTROLLER(channel).head_no++;
			if (BX_SELECTED_CONTROLLER(channel).head_no >= BX_SELECTED_DRIVE(channel).hard_drive->heads)
			{
				BX_SELECTED_CONTROLLER(channel).head_no = 0;
				BX_SELECTED_CONTROLLER(channel).cylinder_no++;
				if (BX_SELECTED_CONTROLLER(channel).cylinder_no >= BX_SELECTED_DRIVE(channel).hard_drive->cylinders)
					BX_SELECTED_CONTROLLER(channel).cylinder_no = BX_SELECTED_DRIVE(channel).hard_drive->cylinders - 1;
			}
		}
	}
}

void bx_hard_drive_c::identify_ATAPI_drive(Bit8u channel)
{
	unsigned i;

	BX_SELECTED_DRIVE(channel).id_drive[0] = (2 << 14) | (5 << 8) | (1 << 7) | (2 << 5) | (0 << 0);	// Removable CDROM, 50us response, 12 byte packets

	for (i = 1; i <= 9; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	const char *serial_number = " VT00001\0\0\0\0\0\0\0\0\0\0\0\0";

	for (i = 0; i < 10; i++)
	{
		BX_SELECTED_DRIVE(channel).id_drive[10 + i] = (serial_number[i * 2] << 8) | serial_number[i * 2 + 1];
	}

	for (i = 20; i <= 22; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	const char *firmware = "ALPHA1  ";

	for (i = 0; i < strlen(firmware) / 2; i++)
	{
		BX_SELECTED_DRIVE(channel).id_drive[23 + i] = (firmware[i * 2] << 8) | firmware[i * 2 + 1];
	}
	for (i = 0; i < strlen((char *) BX_SELECTED_MODEL(channel)) / 2; i++)
		BX_SELECTED_DRIVE(channel).id_drive[27 + i] = (BX_SELECTED_MODEL(channel)[i * 2] << 8) |
			BX_SELECTED_MODEL(channel)[i * 2 + 1];

	BX_SELECTED_DRIVE(channel).id_drive[47] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[48] = 1;	// 32 bits access

	BX_SELECTED_DRIVE(channel).id_drive[49] = (1 << 9);	// LBA supported

	BX_SELECTED_DRIVE(channel).id_drive[50] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[51] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[52] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[53] = 3;	// words 64-70, 54-58 valid

	for (i = 54; i <= 62; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	// copied from CFA540A
	BX_SELECTED_DRIVE(channel).id_drive[63] = 0x0103;	// variable (DMA stuff)
	BX_SELECTED_DRIVE(channel).id_drive[64] = 0x0001;	// PIO
	BX_SELECTED_DRIVE(channel).id_drive[65] = 0x00b4;
	BX_SELECTED_DRIVE(channel).id_drive[66] = 0x00b4;
	BX_SELECTED_DRIVE(channel).id_drive[67] = 0x012c;
	BX_SELECTED_DRIVE(channel).id_drive[68] = 0x00b4;

	BX_SELECTED_DRIVE(channel).id_drive[69] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[70] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[71] = 30;	// faked
	BX_SELECTED_DRIVE(channel).id_drive[72] = 30;	// faked
	BX_SELECTED_DRIVE(channel).id_drive[73] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[74] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[75] = 0;

	for (i = 76; i <= 79; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[80] = 0x1e;	// supports up to ATA/ATAPI-4
	BX_SELECTED_DRIVE(channel).id_drive[81] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[82] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[83] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[84] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[85] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[86] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[87] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[88] = 0;

	for (i = 89; i <= 126; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[127] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[128] = 0;

	for (i = 129; i <= 159; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	for (i = 160; i <= 255; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	// now convert the id_drive array (native 256 word format) to
	// the controller buffer (512 bytes)
	Bit16u temp16;

	for (i = 0; i <= 255; i++)
	{
		temp16 = BX_SELECTED_DRIVE(channel).id_drive[i];
		BX_SELECTED_CONTROLLER(channel).buffer[i * 2] = temp16 & 0x00ff;
		BX_SELECTED_CONTROLLER(channel).buffer[i * 2 + 1] = temp16 >> 8;
	}
}

void bx_hard_drive_c::identify_drive(Bit8u channel)
{
	unsigned i;
	Bit32u temp32;
	Bit16u temp16;

#if defined(CONNER_CFA540A)
	BX_SELECTED_DRIVE(channel).id_drive[0] = 0x0c5a;
	BX_SELECTED_DRIVE(channel).id_drive[1] = 0x0418;
	BX_SELECTED_DRIVE(channel).id_drive[2] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[3] = BX_SELECTED_DRIVE(channel).hard_drive->heads;
	BX_SELECTED_DRIVE(channel).id_drive[4] = 0x9fb7;
	BX_SELECTED_DRIVE(channel).id_drive[5] = 0x0289;
	BX_SELECTED_DRIVE(channel).id_drive[6] = BX_SELECTED_DRIVE(channel).hard_drive->sectors;
	BX_SELECTED_DRIVE(channel).id_drive[7] = 0x0030;
	BX_SELECTED_DRIVE(channel).id_drive[8] = 0x000a;
	BX_SELECTED_DRIVE(channel).id_drive[9] = 0x0000;

	char *serial_number = " CA00GSQ\0\0\0\0\0\0\0\0\0\0\0\0";

	for (i = 0; i < 10; i++)
	{
		BX_SELECTED_DRIVE(channel).id_drive[10 + i] = (serial_number[i * 2] << 8) | serial_number[i * 2 + 1];
	}

	BX_SELECTED_DRIVE(channel).id_drive[20] = 3;
	BX_SELECTED_DRIVE(channel).id_drive[21] = 512;	// 512 Sectors = 256kB cache
	BX_SELECTED_DRIVE(channel).id_drive[22] = 4;

	char *firmware = "8FT054  ";

	for (i = 0; i < strlen(firmware) / 2; i++)
	{
		BX_SELECTED_DRIVE(channel).id_drive[23 + i] = (firmware[i * 2] << 8) | firmware[i * 2 + 1];
	}

	char *model = "Conner Peripherals 540MB - CFA540A      ";

	for (i = 0; i < strlen(model) / 2; i++)
	{
		BX_SELECTED_DRIVE(channel).id_drive[27 + i] = (model[i * 2] << 8) | model[i * 2 + 1];
	}

	BX_SELECTED_DRIVE(channel).id_drive[47] = 0x8080;	// multiple mode identification
	BX_SELECTED_DRIVE(channel).id_drive[48] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[49] = 0x0f01;

	BX_SELECTED_DRIVE(channel).id_drive[50] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[51] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[52] = 0x0002;
	BX_SELECTED_DRIVE(channel).id_drive[53] = 0x0003;
	BX_SELECTED_DRIVE(channel).id_drive[54] = 0x0418;

	BX_SELECTED_DRIVE(channel).id_drive[55] = BX_SELECTED_DRIVE(channel).hard_drive->heads;
	BX_SELECTED_DRIVE(channel).id_drive[56] = BX_SELECTED_DRIVE(channel).hard_drive->sectors;

	BX_SELECTED_DRIVE(channel).id_drive[57] = 0x1e80;
	BX_SELECTED_DRIVE(channel).id_drive[58] = 0x0010;
	BX_SELECTED_DRIVE(channel).id_drive[59] = 0x0100 | BX_SELECTED_CONTROLLER(channel).sectors_per_block;
	BX_SELECTED_DRIVE(channel).id_drive[60] = 0x20e0;
	BX_SELECTED_DRIVE(channel).id_drive[61] = 0x0010;

	BX_SELECTED_DRIVE(channel).id_drive[62] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[63] = 0x0103;	// variable (DMA stuff)
	BX_SELECTED_DRIVE(channel).id_drive[64] = 0x0001;	// PIO
	BX_SELECTED_DRIVE(channel).id_drive[65] = 0x00b4;
	BX_SELECTED_DRIVE(channel).id_drive[66] = 0x00b4;
	BX_SELECTED_DRIVE(channel).id_drive[67] = 0x012c;
	BX_SELECTED_DRIVE(channel).id_drive[68] = 0x00b4;

	for (i = 69; i <= 79; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[80] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[81] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[82] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[83] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[84] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[85] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[86] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[87] = 0;

	for (i = 88; i <= 127; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[128] = 0x0418;
	BX_SELECTED_DRIVE(channel).id_drive[129] = 0x103f;
	BX_SELECTED_DRIVE(channel).id_drive[130] = 0x0418;
	BX_SELECTED_DRIVE(channel).id_drive[131] = 0x103f;
	BX_SELECTED_DRIVE(channel).id_drive[132] = 0x0004;
	BX_SELECTED_DRIVE(channel).id_drive[133] = 0xffff;
	BX_SELECTED_DRIVE(channel).id_drive[134] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[135] = 0x5050;

	for (i = 136; i <= 144; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	BX_SELECTED_DRIVE(channel).id_drive[145] = 0x302e;
	BX_SELECTED_DRIVE(channel).id_drive[146] = 0x3245;
	BX_SELECTED_DRIVE(channel).id_drive[147] = 0x2020;
	BX_SELECTED_DRIVE(channel).id_drive[148] = 0x2020;

	for (i = 149; i <= 255; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

#else

	// Identify Drive command return values definition
	//
	// This code is rehashed from some that was donated.
	// I'm using ANSI X3.221-1994, AT Attachment Interface for Disk Drives
	// and X3T10 2008D Working Draft for ATA-3


	// Word 0: general config bit-significant info
	//   Note: bits 1-5 and 8-14 are now "Vendor specific (obsolete)"
	//   bit 15: 0=ATA device
	//           1=ATAPI device
	//   bit 14: 1=format speed tolerance gap required
	//   bit 13: 1=track offset option available
	//   bit 12: 1=data strobe offset option available
	//   bit 11: 1=rotational speed tolerance is > 0,5% (typo?)
	//   bit 10: 1=disk transfer rate > 10Mbs
	//   bit  9: 1=disk transfer rate > 5Mbs but <= 10Mbs
	//   bit  8: 1=disk transfer rate <= 5Mbs
	//   bit  7: 1=removable cartridge drive
	//   bit  6: 1=fixed drive
	//   bit  5: 1=spindle motor control option implemented
	//   bit  4: 1=head switch time > 15 usec
	//   bit  3: 1=not MFM encoded
	//   bit  2: 1=soft sectored
	//   bit  1: 1=hard sectored
	//   bit  0: 0=reserved
	BX_SELECTED_DRIVE(channel).id_drive[0] = 0x0040;

	// Word 1: number of user-addressable cylinders in
	//   default translation mode.  If the value in words 60-61
	//   exceed 16,515,072, this word shall contain 16,383.
	BX_SELECTED_DRIVE(channel).id_drive[1] = BX_SELECTED_DRIVE(channel).hard_drive->cylinders;

	// Word 2: reserved
	BX_SELECTED_DRIVE(channel).id_drive[2] = 0;

	// Word 3: number of user-addressable heads in default
	//   translation mode
	BX_SELECTED_DRIVE(channel).id_drive[3] = BX_SELECTED_DRIVE(channel).hard_drive->heads;

	// Word 4: # unformatted bytes per translated track in default xlate mode
	// Word 5: # unformatted bytes per sector in default xlated mode
	// Word 6: # user-addressable sectors per track in default xlate mode
	// Note: words 4,5 are now "Vendor specific (obsolete)"
	BX_SELECTED_DRIVE(channel).id_drive[4] = (512 * BX_SELECTED_DRIVE(channel).hard_drive->sectors);
	BX_SELECTED_DRIVE(channel).id_drive[5] = 512;
	BX_SELECTED_DRIVE(channel).id_drive[6] = BX_SELECTED_DRIVE(channel).hard_drive->sectors;

	// Word 7-9: Vendor specific
	for (i = 7; i <= 9; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	// Word 10-19: Serial number (20 ASCII characters, 0000h=not specified)
	// This field is right justified and padded with spaces (20h).
	for (i = 10; i <= 19; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	// Word 20: buffer type
	//          0000h = not specified
	//          0001h = single ported single sector buffer which is
	//                  not capable of simulataneous data xfers to/from
	//                  the host and the disk.
	//          0002h = dual ported multi-sector buffer capable of
	//                  simulatenous data xfers to/from the host and disk.
	//          0003h = dual ported mutli-sector buffer capable of
	//                  simulatenous data xfers with a read caching
	//                  capability.
	//          0004h-ffffh = reserved
	BX_SELECTED_DRIVE(channel).id_drive[20] = 3;

	// Word 21: buffer size in 512 byte increments, 0000h = not specified
	BX_SELECTED_DRIVE(channel).id_drive[21] = 512;	// 512 Sectors = 256kB cache

	// Word 22: # of ECC bytes available on read/write long cmds
	//          0000h = not specified
	BX_SELECTED_DRIVE(channel).id_drive[22] = 4;

	// Word 23..26: Firmware revision (8 ascii chars, 0000h=not specified)
	// This field is left justified and padded with spaces (20h)
	for (i = 23; i <= 26; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	// Word 27..46: Model number (40 ascii chars, 0000h=not specified)
	// This field is left justified and padded with spaces (20h)
//  for (i=27; i<=46; i++)
//    BX_SELECTED_DRIVE(channel).id_drive[i] = 0;
	for (i = 0; i < 20; i++)
	{
		BX_SELECTED_DRIVE(channel).id_drive[27 + i] = (BX_SELECTED_MODEL(channel)[i * 2] << 8) |
			BX_SELECTED_MODEL(channel)[i * 2 + 1];
	}

	// Word 47: 15-8 Vendor unique
	//           7-0 00h= read/write multiple commands not implemented
	//               xxh= maximum # of sectors that can be transferred
	//                    per interrupt on read and write multiple commands
	BX_SELECTED_DRIVE(channel).id_drive[47] = max_multiple_sectors;

	// Word 48: 0000h = cannot perform dword IO
	//          0001h = can    perform dword IO
	BX_SELECTED_DRIVE(channel).id_drive[48] = 1;

	// Word 49: Capabilities
	//   15-10: 0 = reserved
	//       9: 1 = LBA supported
	//       8: 1 = DMA supported
	//     7-0: Vendor unique
	if (ATA_DMA)
	{
		BX_SELECTED_DRIVE(channel).id_drive[49] = (1 << 9) | (1 << 8);
	} else
	{
		BX_SELECTED_DRIVE(channel).id_drive[49] = 1 << 9;
	}

	// Word 50: Reserved
	BX_SELECTED_DRIVE(channel).id_drive[50] = 0;

	// Word 51: 15-8 PIO data transfer cycle timing mode
	//           7-0 Vendor unique
	BX_SELECTED_DRIVE(channel).id_drive[51] = 0x200;

	// Word 52: 15-8 DMA data transfer cycle timing mode
	//           7-0 Vendor unique
	BX_SELECTED_DRIVE(channel).id_drive[52] = 0x200;

	// Word 53: 15-1 Reserved
	//             0 1=the fields reported in words 54-58 are valid
	//               0=the fields reported in words 54-58 may be valid
	BX_SELECTED_DRIVE(channel).id_drive[53] = 0;

	// Word 54: # of user-addressable cylinders in curr xlate mode
	// Word 55: # of user-addressable heads in curr xlate mode
	// Word 56: # of user-addressable sectors/track in curr xlate mode
	BX_SELECTED_DRIVE(channel).id_drive[54] = BX_SELECTED_DRIVE(channel).hard_drive->cylinders;
	BX_SELECTED_DRIVE(channel).id_drive[55] = BX_SELECTED_DRIVE(channel).hard_drive->heads;
	BX_SELECTED_DRIVE(channel).id_drive[56] = BX_SELECTED_DRIVE(channel).hard_drive->sectors;

	// Word 57-58: Current capacity in sectors
	// Excludes all sectors used for device specific purposes.
	temp32 =
		BX_SELECTED_DRIVE(channel).hard_drive->cylinders *
		BX_SELECTED_DRIVE(channel).hard_drive->heads * BX_SELECTED_DRIVE(channel).hard_drive->sectors;
	BX_SELECTED_DRIVE(channel).id_drive[57] = (temp32 & 0xffff);	// LSW
	BX_SELECTED_DRIVE(channel).id_drive[58] = (temp32 >> 16);	// MSW

	// Word 59: 15-9 Reserved
	//             8 1=multiple sector setting is valid
	//           7-0 current setting for number of sectors that can be
	//               transferred per interrupt on R/W multiple commands
	BX_SELECTED_DRIVE(channel).id_drive[59] = 0x0000 | curr_multiple_sectors;

	// Word 60-61:
	// If drive supports LBA Mode, these words reflect total # of user
	// addressable sectors.  This value does not depend on the current
	// drive geometry.  If the drive does not support LBA mode, these
	// words shall be set to 0.
	Bit32u num_sects =
		BX_SELECTED_DRIVE(channel).hard_drive->cylinders * BX_SELECTED_DRIVE(channel).hard_drive->heads *
		BX_SELECTED_DRIVE(channel).hard_drive->sectors;
	BX_SELECTED_DRIVE(channel).id_drive[60] = num_sects & 0xffff;	// LSW
	BX_SELECTED_DRIVE(channel).id_drive[61] = num_sects >> 16;	// MSW

	// Word 62: 15-8 single word DMA transfer mode active
	//           7-0 single word DMA transfer modes supported
	// The low order byte identifies by bit, all the Modes which are
	// supported e.g., if Mode 0 is supported bit 0 is set.
	// The high order byte contains a single bit set to indiciate
	// which mode is active.
	BX_SELECTED_DRIVE(channel).id_drive[62] = 0x0;

	// Word 63: 15-8 multiword DMA transfer mode active
	//           7-0 multiword DMA transfer modes supported
	// The low order byte identifies by bit, all the Modes which are
	// supported e.g., if Mode 0 is supported bit 0 is set.
	// The high order byte contains a single bit set to indiciate
	// which mode is active.
	if (ATA_DMA)
	{
		BX_SELECTED_DRIVE(channel).id_drive[63] = 0x07;
	} else
	{
		BX_SELECTED_DRIVE(channel).id_drive[63] = 0x0;
	}

	// Word 64-79 Reserved
	for (i = 64; i <= 79; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	// Word 80: 15-5 reserved
	//             4 supports ATA/ATAPI-4
	//             3 supports ATA-3
	//             2 supports ATA-2
	//             1 supports ATA-1
	//             0 reserved
	BX_SELECTED_DRIVE(channel).id_drive[80] = (1 << 2) | (1 << 1);

	// Word 81: Minor version number
	BX_SELECTED_DRIVE(channel).id_drive[81] = 0;

	// Word 82: 15 obsolete
	//          14 NOP command supported
	//          13 READ BUFFER command supported
	//          12 WRITE BUFFER command supported
	//          11 obsolete
	//          10 Host protected area feature set supported
	//           9 DEVICE RESET command supported
	//           8 SERVICE interrupt supported
	//           7 release interrupt supported
	//           6 look-ahead supported
	//           5 write cache supported
	//           4 supports PACKET command feature set
	//           3 supports power management feature set
	//           2 supports removable media feature set
	//           1 supports securite mode feature set
	//           0 support SMART feature set
	BX_SELECTED_DRIVE(channel).id_drive[82] = 1 << 14;
	BX_SELECTED_DRIVE(channel).id_drive[83] = 1 << 14;
	BX_SELECTED_DRIVE(channel).id_drive[84] = 1 << 14;
	BX_SELECTED_DRIVE(channel).id_drive[85] = 1 << 14;
	BX_SELECTED_DRIVE(channel).id_drive[86] = 0;
	BX_SELECTED_DRIVE(channel).id_drive[87] = 1 << 14;

	for (i = 88; i <= 127; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	// Word 128-159 Vendor unique
	for (i = 128; i <= 159; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

	// Word 160-255 Reserved
	for (i = 160; i <= 255; i++)
		BX_SELECTED_DRIVE(channel).id_drive[i] = 0;

#endif

	D(bug("Drive ID Info. initialized : %04d {%s}", 512, BX_SELECTED_TYPE_STRING(channel)));

	// now convert the id_drive array (native 256 word format) to
	// the controller buffer (512 bytes)
	for (i = 0; i <= 255; i++)
	{
		temp16 = BX_SELECTED_DRIVE(channel).id_drive[i];
		BX_SELECTED_CONTROLLER(channel).buffer[i * 2] = temp16 & 0x00ff;
		BX_SELECTED_CONTROLLER(channel).buffer[i * 2 + 1] = temp16 >> 8;
	}
}

void bx_hard_drive_c::init_send_atapi_command(Bit8u channel, Bit8u command, int req_length, int alloc_length, bool lazy)
{
	// BX_SELECTED_CONTROLLER(channel).byte_count is a union of BX_SELECTED_CONTROLLER(channel).cylinder_no;
	// lazy is used to force a data read in the buffer at the next read.

	if (BX_SELECTED_CONTROLLER(channel).byte_count == 0xffff)
		BX_SELECTED_CONTROLLER(channel).byte_count = 0xfffe;

	if ((BX_SELECTED_CONTROLLER(channel).byte_count & 1)
		&& !(alloc_length <= BX_SELECTED_CONTROLLER(channel).byte_count))
	{
		D2(bug("Odd byte count (0x%04x) to ATAPI command 0x%02x, using 0x%04x",
			   BX_SELECTED_CONTROLLER(channel).byte_count, command, BX_SELECTED_CONTROLLER(channel).byte_count - 1));
		BX_SELECTED_CONTROLLER(channel).byte_count -= 1;
	}

	if (BX_SELECTED_CONTROLLER(channel).byte_count == 0)
		panicbug(("ATAPI command with zero byte count"));

	if (alloc_length < 0)
		panicbug(("Allocation length < 0"));
	if (alloc_length == 0)
		alloc_length = BX_SELECTED_CONTROLLER(channel).byte_count;

	BX_SELECTED_CONTROLLER(channel).interrupt_reason.i_o = 1;
	BX_SELECTED_CONTROLLER(channel).interrupt_reason.c_d = 0;
	BX_SELECTED_CONTROLLER(channel).status.busy = 0;
	BX_SELECTED_CONTROLLER(channel).status.drq = 1;
	BX_SELECTED_CONTROLLER(channel).status.err = 0;

	// no bytes transfered yet
	if (lazy)
		BX_SELECTED_CONTROLLER(channel).buffer_index = 2048;
	else
		BX_SELECTED_CONTROLLER(channel).buffer_index = 0;
	BX_SELECTED_CONTROLLER(channel).drq_index = 0;

	if (BX_SELECTED_CONTROLLER(channel).byte_count > req_length)
		BX_SELECTED_CONTROLLER(channel).byte_count = req_length;

	if (BX_SELECTED_CONTROLLER(channel).byte_count > alloc_length)
		BX_SELECTED_CONTROLLER(channel).byte_count = alloc_length;

	BX_SELECTED_DRIVE(channel).atapi.command = command;
	BX_SELECTED_DRIVE(channel).atapi.drq_bytes = BX_SELECTED_CONTROLLER(channel).byte_count;
	BX_SELECTED_DRIVE(channel).atapi.total_bytes_remaining = (req_length < alloc_length) ? req_length : alloc_length;

	// if (lazy) {
	// // bias drq_bytes and total_bytes_remaining
	// BX_SELECTED_DRIVE(channel).atapi.drq_bytes += 2048;
	// BX_SELECTED_DRIVE(channel).atapi.total_bytes_remaining += 2048;
	// }
}

void bx_hard_drive_c::atapi_cmd_error(Bit8u channel, sense_t sense_key, asc_t asc)
{
	D(bug("atapi_cmd_error channel=%02x key=%02x asc=%02x", channel, sense_key, asc));

	BX_SELECTED_CONTROLLER(channel).error_register = sense_key << 4;
	BX_SELECTED_CONTROLLER(channel).interrupt_reason.i_o = 1;
	BX_SELECTED_CONTROLLER(channel).interrupt_reason.c_d = 1;
	BX_SELECTED_CONTROLLER(channel).interrupt_reason.rel = 0;
	BX_SELECTED_CONTROLLER(channel).status.busy = 0;
	BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
	BX_SELECTED_CONTROLLER(channel).status.write_fault = 0;
	BX_SELECTED_CONTROLLER(channel).status.drq = 0;
	BX_SELECTED_CONTROLLER(channel).status.err = 1;

	BX_SELECTED_DRIVE(channel).sense.sense_key = sense_key;
	BX_SELECTED_DRIVE(channel).sense.asc = asc;
	BX_SELECTED_DRIVE(channel).sense.ascq = 0;
}

void bx_hard_drive_c::atapi_cmd_nop(Bit8u channel)
{
	BX_SELECTED_CONTROLLER(channel).interrupt_reason.i_o = 1;
	BX_SELECTED_CONTROLLER(channel).interrupt_reason.c_d = 1;
	BX_SELECTED_CONTROLLER(channel).interrupt_reason.rel = 0;
	BX_SELECTED_CONTROLLER(channel).status.busy = 0;
	BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
	BX_SELECTED_CONTROLLER(channel).status.drq = 0;
	BX_SELECTED_CONTROLLER(channel).status.err = 0;
}

void bx_hard_drive_c::init_mode_sense_single(Bit8u channel, const void *src, int size)
{
	// Header
	BX_SELECTED_CONTROLLER(channel).buffer[0] = (size + 6) >> 8;
	BX_SELECTED_CONTROLLER(channel).buffer[1] = (size + 6) & 0xff;
	if (bx_options.atadevice[channel][BX_HD_THIS channels[channel].drive_select].status == BX_INSERTED)
		BX_SELECTED_CONTROLLER(channel).buffer[2] = 0x12;	// media present 120mm CD-ROM (CD-R) data/audio  door closed
	else
		BX_SELECTED_CONTROLLER(channel).buffer[2] = 0x70;	// no media present
	BX_SELECTED_CONTROLLER(channel).buffer[3] = 0;	// reserved
	BX_SELECTED_CONTROLLER(channel).buffer[4] = 0;	// reserved
	BX_SELECTED_CONTROLLER(channel).buffer[5] = 0;	// reserved
	BX_SELECTED_CONTROLLER(channel).buffer[6] = 0;	// reserved
	BX_SELECTED_CONTROLLER(channel).buffer[7] = 0;	// reserved

	// Data
	memcpy(BX_SELECTED_CONTROLLER(channel).buffer + 8, src, size);
}

void bx_hard_drive_c::ready_to_send_atapi(Bit8u channel)
{
	raise_interrupt(channel);
}

void bx_hard_drive_c::raise_interrupt(Bit8u channel)
{
	D(bug("raise_interrupt called, disable_irq = %02x", BX_SELECTED_CONTROLLER(channel).control.disable_irq));
	if (!BX_SELECTED_CONTROLLER(channel).control.disable_irq)
	{
		D(bug("raising interrupt"));
	} else
	{
		D(bug("Not raising interrupt"));
	}
	if (!BX_SELECTED_CONTROLLER(channel).control.disable_irq)
	{
		getMFP()->setGPIPbit(0x20, 0);
	}
}

void bx_hard_drive_c::command_aborted(Bit8u channel, unsigned value)
{
	DUNUSED(value);
	D(bug("aborting on command 0x%02x {%s}", value, BX_SELECTED_TYPE_STRING(channel)));
	BX_SELECTED_CONTROLLER(channel).current_command = 0;
	BX_SELECTED_CONTROLLER(channel).status.busy = 0;
	BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
	BX_SELECTED_CONTROLLER(channel).status.err = 1;
	BX_SELECTED_CONTROLLER(channel).error_register = 0x04;	// command ABORTED
	BX_SELECTED_CONTROLLER(channel).status.drq = 0;
	BX_SELECTED_CONTROLLER(channel).status.seek_complete = 0;
	BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;
	BX_SELECTED_CONTROLLER(channel).buffer_index = 0;
	raise_interrupt(channel);
}

Bit32u bx_hard_drive_c::get_device_handle(Bit8u channel, Bit8u device)
{
	D(bug("get_device_handle %d %d", channel, device));
	if ((channel < BX_MAX_ATA_CHANNEL) && (device < 2))
	{
		return ((channel * 2) + device);
	}

	return BX_MAX_ATA_CHANNEL * 2;
}

Bit32u bx_hard_drive_c::get_first_cd_handle(void)
{
	for (Bit8u channel = 0; channel < BX_MAX_ATA_CHANNEL; channel++)
	{
		if (BX_DRIVE_IS_CD(channel, 0))
			return (channel * 2);
		if (BX_DRIVE_IS_CD(channel, 1))
			return ((channel * 2) + 1);
	}
	return BX_MAX_ATA_CHANNEL * 2;
}

unsigned bx_hard_drive_c::get_cd_media_status(Bit32u handle)
{
	if (handle >= BX_MAX_ATA_CHANNEL * 2)
		return 0;

	Bit8u channel = handle / 2;
	Bit8u device = handle % 2;

	return (BX_HD_THIS channels[channel].drives[device].cdrom.ready);
}

unsigned bx_hard_drive_c::set_cd_media_status(Bit32u handle, unsigned status)
{
	if (handle >= BX_MAX_ATA_CHANNEL * 2)
		return 0;

	Bit8u channel = handle / 2;
	Bit8u device = handle % 2;

	// if setting to the current value, nothing to do
	if (status == BX_HD_THIS channels[channel].drives[device].cdrom.ready)
		return (status);
	// return 0 if no cdromd is present
	if (!BX_DRIVE_IS_CD(channel, device))
		return (0);

	if (status == 0)
	{
		// eject cdrom if not locked by guest OS
		if (BX_HD_THIS channels[channel].drives[device].cdrom.locked)
			return (1);
		else
		{
#ifdef LOWLEVEL_CDROM
			BX_HD_THIS channels[channel].drives[device].cdrom.cd->eject_cdrom();
#endif
			BX_HD_THIS channels[channel].drives[device].cdrom.ready = 0;

			bx_options.atadevice[channel][device].status = BX_EJECTED;
		}
	} else
	{
		// insert cdrom
#ifdef LOWLEVEL_CDROM
		if (BX_HD_THIS channels[channel].drives[device].cdrom.cd->
			insert_cdrom(bx_options.atadevice[channel][device].path))
		{
			BX_HD_THIS channels[channel].drives[device].cdrom.ready = 1;
			BX_HD_THIS channels[channel].drives[device].cdrom.capacity =
				BX_HD_THIS channels[channel].drives[device].cdrom.cd->capacity();
			bx_options.atadevice[channel][device].status = BX_INSERTED;
			BX_SELECTED_DRIVE(channel).sense.sense_key = SENSE_UNIT_ATTENTION;
			BX_SELECTED_DRIVE(channel).sense.asc = 0;
			BX_SELECTED_DRIVE(channel).sense.ascq = 0;
			raise_interrupt(channel);
		} else
		{
#endif
			D(bug("Could not locate CD-ROM, continuing with media not present"));
			BX_HD_THIS channels[channel].drives[device].cdrom.ready = 0;

			bx_options.atadevice[channel][device].status = BX_EJECTED;
#ifdef LOWLEVEL_CDROM
		}
#endif
	}
	return (BX_HD_THIS channels[channel].drives[device].cdrom.ready);
}

bool bx_hard_drive_c::bmdma_read_sector(Bit8u channel, Bit8u * buffer)
{
	off_t logical_sector;
	off_t ret;

	if (BX_SELECTED_CONTROLLER(channel).current_command != 0xC8)
	{
		D(panicbug("command 0xC8 (READ DMA) not active"));
		command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
		return 0;
	}
	if (!calculate_logical_address(channel, &logical_sector))
	{
		D(panicbug("BM-DMA read sector reached invalid sector %lu, aborting", (unsigned long) logical_sector));
		command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
		return 0;
	}
	ret = BX_SELECTED_DRIVE(channel).hard_drive->lseek(logical_sector * 512, SEEK_SET);
	if (ret < 0)
	{
		D(panicbug("could not lseek() hard drive image file"));
		command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
		return 0;
	}
	ret = BX_SELECTED_DRIVE(channel).hard_drive->read((bx_ptr_t) buffer, 512);
	if (ret < 512)
	{
		D(panicbug("logical sector was %lu", (unsigned long) logical_sector));
		D(panicbug("could not read() hard drive image file at byte %lu", (unsigned long) logical_sector * 512));
		command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
		return 0;
	}
	increment_address(channel);
	return 1;
}

bool bx_hard_drive_c::bmdma_write_sector(Bit8u channel, Bit8u * buffer)
{
	off_t logical_sector;
	off_t ret;

	if (BX_SELECTED_CONTROLLER(channel).current_command != 0xCA)
	{
		D(panicbug("command 0xCA (WRITE DMA) not active"));
		command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
		return 0;
	}
	if (!calculate_logical_address(channel, &logical_sector))
	{
		D(panicbug("BM-DMA read sector reached invalid sector %lu, aborting", (unsigned long) logical_sector));
		command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
		return 0;
	}
	ret = BX_SELECTED_DRIVE(channel).hard_drive->lseek(logical_sector * 512, SEEK_SET);
	if (ret < 0)
	{
		D(panicbug("could not lseek() hard drive image file"));
		command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
		return 0;
	}
	ret = BX_SELECTED_DRIVE(channel).hard_drive->write((bx_ptr_t) buffer, 512);
	if (ret < 512)
	{
		D(panicbug("could not write() hard drive image file at byte %lu", (unsigned long) logical_sector * 512));
		command_aborted(channel, BX_SELECTED_CONTROLLER(channel).current_command);
		return 0;
	}
	increment_address(channel);
	return 1;
}

void bx_hard_drive_c::bmdma_complete(Bit8u channel)
{
	BX_SELECTED_CONTROLLER(channel).status.busy = 0;
	BX_SELECTED_CONTROLLER(channel).status.drive_ready = 1;
	BX_SELECTED_CONTROLLER(channel).status.write_fault = 0;
	BX_SELECTED_CONTROLLER(channel).status.seek_complete = 1;
	BX_SELECTED_CONTROLLER(channel).status.drq = 0;
	BX_SELECTED_CONTROLLER(channel).status.corrected_data = 0;
	BX_SELECTED_CONTROLLER(channel).status.err = 0;
	raise_interrupt(channel);
}

int default_image_t::open(const char *pathname, bool readonly)
{
	int open_flags = readonly ? O_RDONLY : O_RDWR;

#ifdef O_BINARY
	open_flags |= O_BINARY;
#endif

	fd =::open(pathname, open_flags);
	if (fd < 0)
	{
		return fd;
	}

	/* look at size of image file to calculate disk geometry */
	struct stat stat_buf;
	int ret = fstat(fd, &stat_buf);

	if (ret)
	{
		panicbug("fstat() returns error!");
	}

	return fd;
}

void default_image_t::close()
{
	if (fd > -1)
	{
		::close(fd);
	}
}

off_t default_image_t::lseek(off_t offset, int whence)
{
	return::lseek(fd, offset, whence);
}

ssize_t default_image_t::read(void *buf, size_t count)
{
	return::read(fd, (char *) buf, count);
}

ssize_t default_image_t::write(const void *buf, size_t count)
{
	return::write(fd, (char *) buf, count);
}

error_recovery_t::error_recovery_t()
{
	if (sizeof(error_recovery_t) != 8)
	{
		panicbug("error_recovery_t has size != 8");
	}

	data[0] = 0x01;
	data[1] = 0x06;
	data[2] = 0x00;
	data[3] = 0x05;						// Try to recover 5 times
	data[4] = 0x00;
	data[5] = 0x00;
	data[6] = 0x00;
	data[7] = 0x00;
}

uint16 read_16bit(const uint8 * buf)
{
	return (buf[0] << 8) | buf[1];
}

uint32 read_32bit(const uint8 * buf)
{
	return (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}
