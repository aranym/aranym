#ifndef _XHDI_H
#define _XHDI_H
#include "nf_base.h"
#include "parameters.h"

class XHDIDriver : public NF_Base
{
private:
	bx_disk_options_t *dev2disk(uint16 major, uint16 minor);
	void byteSwapBuf(uint8 *buf, int size);
	int32 XHReadWrite(uint16 major, uint16 minor, uint16 rwflag,
				uint32 recno, uint16 count, memptr buf);
	int32 XHGetCapacity(uint16 major, uint16 minor,
				memptr blocks, memptr blocksize);

public:
	char *name() { return "XHDI"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);
};

#endif /* _XHDI_H */
