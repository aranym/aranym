/* Joy 2001 */

#include "icio.h"

class ACIA : public ICio {
protected:
	uaecptr baseaddr;
	uae_u8 status;
	uae_u8 mode;
	uae_u8 rxdata;
	uae_u8 txdata;

public:
	ACIA(uaecptr);
	virtual uae_u8 handleRead(uaecptr addr);
	virtual void handleWrite(uaecptr addr, uae_u8 value);
	virtual uae_u8 getStatus() { return 2; };
	virtual void setMode(uae_u8 value) {};
	virtual uae_u8 getData() { return 0xa2; };
	virtual void setData(uae_u8) {};
};

#define MAXBUF 		16383
class IKBD: public ACIA {
private:
	int buffer[MAXBUF];
	int ikbd_inbuf;
	int ikbd_bufpos;

public:
	IKBD();
	virtual uae_u8 getStatus();
	virtual void setMode(uae_u8 value);
	virtual uae_u8 getData();
	virtual void setData(uae_u8);
	void ikbd_send(int value);
};

class MIDI: public ACIA {
public:
	MIDI() : ACIA(0xfffc02) {};
};
