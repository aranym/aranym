#ifndef _XHDI_H
#define _XHDI_H
#include "parameters.h"

class XHDIDriver
{
private:
	bx_disk_options_t *dev2disk(uint16 major, uint16 minor);
	void byteSwapBuf(uint8 *buf, int size);
public:
	uint32 dispatch(uint16 fncode, memptr stack);
	uint32 dispatch(uint32 *params);
	uint32 XHReadWrite(uint16 major, uint16 minor, uint16 rwflag,
				uint32 recno, uint16 count, memptr buf);
	uint32 XHGetCapacity(uint16 major, uint16 minor,
				memptr blocks, memptr blocksize);
};

#endif /* _XHDI_H */
