#ifndef _BASE_IO_h
#define _BASE_IO_h

class BASE_IO
{
protected:
	memptr hw_offset;
	uint32 hw_size;

	memptr getHWoffset() { return hw_offset; }
	uint16 getHWsize()   { return hw_size; }

public:
	BASE_IO(memptr addr, uint32 size) {
		hw_offset = addr;
		hw_size = size;
	}
	virtual void reset() {};
	virtual ~BASE_IO() {};
	virtual bool isMyHWRegister(memptr addr) { return (addr >= hw_offset && addr < (hw_offset + hw_size)); }

	virtual uae_u8 handleRead(memptr addr);
	virtual uae_u16 handleReadW(memptr addr);
	virtual uae_u32 handleReadL(memptr addr);
	virtual void handleWrite(memptr addr, uint8 value);
	virtual void handleWriteW(memptr addr, uint16 value);
	virtual void handleWriteL(memptr addr, uint32 value);
};
#endif //_BASE_IO_h
