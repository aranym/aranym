#ifndef _ICio_h
#define _ICio_h
class ICio {
public:
	virtual uae_u8 handleRead(uaecptr addr)  = 0;
	virtual void handleWrite(uaecptr addr, uae_u8 value) = 0;
};
#endif //_ICio_h
