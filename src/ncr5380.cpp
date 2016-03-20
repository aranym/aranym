/*
 *  Atari Falcon NCR5380 SCSI emulation
 *
 *  ARAnyM (C) 2003 Patrice Mandin
 *  and stuff from Linux NCR5380 scsi driver
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
 
/*--- Includes ---*/

#include "sysdeps.h"
#include "hardware.h"
#include "cpu_emulation.h"
#include "memory-uae.h"
#include "ncr5380.h"

#define DEBUG 0
#include "debug.h"

/*--- Defines ---*/

#define OUTPUT_DATA_REG	0			/* wo DATA lines on SCSI bus */

#define CURRENT_SCSI_DATA_REG	0	/* ro same */

#define INITIATOR_COMMAND_REG	1	/* rw */
#define ICR_ASSERT_RST		0x80	/* rw Set to assert RST  */
#define ICR_ARBITRATION_PROGRESS 0x40	/* ro Indicates arbitration complete */
#define ICR_TRI_STATE		0x40	/* wo Set to tri-state drivers */
#define ICR_ARBITRATION_LOST	0x20	/* ro Indicates arbitration lost */
#define ICR_DIFF_ENABLE		0x20	/* wo Set to enable diff. drivers */
#define ICR_ASSERT_ACK		0x10	/* rw ini Set to assert ACK */
#define ICR_ASSERT_BSY		0x08	/* rw Set to assert BSY */
#define ICR_ASSERT_SEL 		0x04	/* rw Set to assert SEL */
#define ICR_ASSERT_ATN		0x02	/* rw Set to assert ATN */
#define ICR_ASSERT_DATA		0x01	/* rw SCSI_DATA_REG is asserted */

#define MODE_REG			2

#define TARGET_COMMAND_REG	3

#define STATUS_REG			4		/* ro */
#define SR_RST			0x80
#define SR_BSY			0x40
#define SR_REQ			0x20
#define SR_MSG			0x10
#define SR_CD			0x08
#define SR_IO			0x04
#define SR_SEL			0x02
#define SR_DBP			0x01

#define SELECT_ENABLE_REG	4		/* wo */

#define BUS_AND_STATUS_REG	5		/* ro */

#define START_DMA_SEND_REG	5		/* wo */

#define INPUT_DATA_REG		6		/* ro */

#define START_DMA_TARGET_RECEIVE_REG	6	/* wo */

#define RESET_PARITY_INTERRUPT_REG		7	/* ro */

#define START_DMA_INITIATOR_RECEIVE_REG	7	/* wo */

/*--- Types ---*/

/*--- Constants ---*/

#if DEBUG
static const char *ncr5380_regnames[16]={
	"current scsi data",
	"initiator command",
	"mode",
	"target command",
	"status",
	"bus and status",
	"input data",
	"reset parity interrupt",

	"output data",
	"initiator command",
	"mode",
	"target command",
	"select enable",
	"start dma send",
	"start dma target receive",
	"start dma initiator receive"
};
#endif

/*--- Variables ---*/

/*--- Constructor/destructor ---*/

NCR5380::NCR5380(void)
{
	D(bug("ncr5380: interface created"));

	reset();
}

NCR5380::~NCR5380(void)
{
	D(bug("ncr5380: interface destroyed"));
}

void NCR5380::reset(void)
{
	hd_count = hd_status = hd_initiator = hd_mode = 0;

	D(bug("ncr5380: reset"));
}

uae_u8 NCR5380::ReadData(uae_u16 control)
{
	uae_u8	data;
	int regnum;

	regnum = control & 7;
	data=0;
	
	switch(regnum) {
		case CURRENT_SCSI_DATA_REG:
			break;
		case INITIATOR_COMMAND_REG:
			data = hd_initiator = ICR_ARBITRATION_PROGRESS;
			break;
		case MODE_REG:
			data = hd_mode;
			break;
		case TARGET_COMMAND_REG:
			break;
		case STATUS_REG:
			data = hd_status = SR_REQ;
			break;
		case BUS_AND_STATUS_REG:
			break;
		case INPUT_DATA_REG:
			break;
		case RESET_PARITY_INTERRUPT_REG:
			break;
	}

	D(bug("ncr5380: read %s register: value=0x%02x", ncr5380_regnames[regnum], data));
	return data;
}

void NCR5380::WriteData(uae_u16 control, uae_u8 data)
{
	int regnum;

	regnum = control & 7;
	D(bug("ncr5380: write %s register: value=0x%02x",ncr5380_regnames[regnum+8], data));

	switch(regnum) {
		case OUTPUT_DATA_REG:
			break;
		case INITIATOR_COMMAND_REG:
			hd_initiator = data;
			break;
		case MODE_REG:
			hd_mode = data;
			break;
		case TARGET_COMMAND_REG:
			break;
		case SELECT_ENABLE_REG:
			break;
		case START_DMA_SEND_REG:
			break;
		case START_DMA_TARGET_RECEIVE_REG:
			break;
		case START_DMA_INITIATOR_RECEIVE_REG:
			break;
	}
}
