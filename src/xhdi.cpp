#include "sysdeps.h"
#include "cpu_emulation.h"
#include "xhdi.h"

#define DEBUG 0
#include "debug.h"

#define SECTORSIZE	512

bx_disk_options_t *XHDIDriver::dev2disk(uint16 major, uint16 minor)
{
	if (minor != 0)
		return NULL;

	bx_disk_options_t *disk;
	switch(major) {
		case 16:	disk = &bx_options.diskc; break;
		case 17:	disk = &bx_options.diskd; break;
		default:	disk = NULL; break;
	}

	return disk;
}

void XHDIDriver::byteSwapBuf(uint8 *buf, int size)
{
	for(int i=0; i<size; i++) {
		int tmp = buf[i];
		buf[i] = buf[i+1];
		buf[++i] = tmp;
	}
}

uint32 XHDIDriver::XHReadWrite(uint16 major, uint16 minor,
				 uint16 rwflag, uint32 recno, uint16 count, memptr buf)
{
	D(bug("ARAnyM XH%s(major=%u, minor=%u, recno=%lu, count=%u, buf=$%x)",
		(rwflag & 1) ? "Write" : "Read",
		major, minor, recno, count, buf));

	bx_disk_options_t *disk = dev2disk(major, minor);
	if (disk == NULL)
		return (uint32)-15L;	// EUNDEV (unknown device)

	bool writing = (rwflag & 1);
	if (writing && !disk->xhdiWrite)
		return (uint32)-36L;	// EACCDN (access denied)

	FILE *f = fopen(disk->path, writing ? "a+b" : "rb");
	if (f != NULL) {
		int size = SECTORSIZE*count;
		uint8 *hostbuf = Atari2HostAddr(buf);
		fseek(f, recno*SECTORSIZE, SEEK_SET);
		if (writing) {
			if (! disk->byteswap)
				byteSwapBuf(hostbuf, size);
			fwrite(hostbuf, size, 1, f);
			if (! disk->byteswap)
				byteSwapBuf(hostbuf, size);
		}
		else {
			fread(hostbuf, size, 1, f);
			if (! disk->byteswap)
				byteSwapBuf(hostbuf, size);

		}
		fclose(f);
	}
	return 0;	// 0 = no error
}

uint32 XHDIDriver::XHGetCapacity(uint16 major, uint16 minor,
					 memptr blocks, memptr blocksize)
{
	D(bug("ARAnyM XHGetCapacity(major=%u, minor=%u, blocks=%lu, blocksize=%lu)", major, minor, blocks, blocksize));

	bx_disk_options_t *disk = dev2disk(major, minor);
	if (disk == NULL)
		return (uint32)-15L;	// EUNDEV (unknown device)

	struct stat buf;
	if (! stat(disk->path, &buf)) {
		long t_blocks = buf.st_size / SECTORSIZE;
		D(bug("t_blocks = %ld\n", t_blocks));
		if (blocks != 0)
			WriteAtariInt32(blocks, t_blocks);
		if (blocksize != 0)
			WriteAtariInt32(blocksize, SECTORSIZE);
		return 0;
	}
	else {
		return (uint32)-2L;		// EDRVNR (device not responding)
	}
}

uint32 XHDIDriver::dispatch(uint16 fncode, memptr stack)
{
	D(bug("ARAnyM XHDI(%u)\n", fncode));
	uint32 ret;
	switch(fncode) {
		case 10: ret = XHReadWrite(
						ReadInt16(stack),   /* UWORD major */
						ReadInt16(stack+2), /* UWORD minor */
						ReadInt16(stack+4), /* UWORD rwflag */
						ReadInt32(stack+6), /* ULONG recno */
						ReadInt16(stack+10),/* UWORD count */
						ReadInt32(stack+12) /* void *buf */
						);
				break;
		case 14: ret = XHGetCapacity(
						ReadInt16(stack),   /* UWORD major */
						ReadInt16(stack+2), /* UWORD minor */
						ReadInt32(stack+4), /* ULONG *blocks */
						ReadInt32(stack+8)  /* ULONG *blocksize */
						);
				break;
		default: ret = (uint32)-32L; // EINVFN
				break;
	}
	return ret;
}

uint32 XHDIDriver::dispatch(uint32 *params)
{
	uint32 fncode = params[0];
	D(bug("ARAnyM XHDI(%u)\n", fncode));
	uint32 ret;
	switch(fncode) {
		case 10: ret = XHReadWrite(
						params[1], /* UWORD major */
						params[2], /* UWORD minor */
						params[3], /* UWORD rwflag */
						params[4], /* ULONG recno */
						params[5], /* UWORD count */
						params[6]  /* void *buf */
						);
				break;
		case 14: ret = XHGetCapacity(
						params[1], /* UWORD major */
						params[2], /* UWORD minor */
						params[3], /* ULONG *blocks */
						params[4] /* ULONG *blocksize */
						);
				break;
		default: ret = (uint32)-32L; // EINVFN
				break;
	}
	return ret;
}
