#ifndef _BASE_IO_h
#define _BASE_IO_h

class ICio {
public:
	virtual ~ICio() {};
	virtual uae_u8 handleRead(uaecptr addr) = 0;
	virtual void handleWrite(uaecptr addr, uae_u8 value) = 0;
};

class BASE_IO : public ICio {
public:
	virtual uae_u8 handleRead(uaecptr addr);
	virtual uae_u16 handleReadW(uaecptr addr);
	virtual uae_u32 handleReadL(uaecptr addr);
	virtual void handleWrite(uaecptr addr, uae_u8 value);
	virtual void handleWriteW(uaecptr addr, uae_u16 value);
	virtual void handleWriteL(uaecptr addr, uae_u32 value);
};
#endif //_BASE_IO_h
