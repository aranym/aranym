#ifndef _XHDI_H
#define _XHDI_H
#include "nf_base.h"
#include "parameters.h"

typedef memptr wmemptr;
typedef memptr lmemptr;

class XHDIDriver : public NF_Base
{
private:
	bx_atadevice_options_t *dev2disk(uint16 major, uint16 minor);
	void byteSwapBuf(uint8 *buf, int size);

protected:
	int32 XHDrvMap();
	int32 XHInqDriver(uint16 bios_device, memptr name, memptr version,
				memptr company, wmemptr ahdi_version, wmemptr maxIPL);
	int32 XHReadWrite(uint16 major, uint16 minor, uint16 rwflag,
				uint32 recno, uint16 count, memptr buf);
	int32 XHInqTarget2(uint16 major, uint16 minor, lmemptr blocksize,
				lmemptr device_flags, memptr product_name, uint16 stringlen);
	int32 XHInqDev2(uint16 bios_device, wmemptr major, wmemptr minor,
				lmemptr start_sector, memptr bpb, lmemptr blocks,
				memptr partid);
	int32 XHGetCapacity(uint16 major, uint16 minor,
				lmemptr blocks, lmemptr blocksize);

public:
	char *name() { return "XHDI"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);
};

#endif /* _XHDI_H */
