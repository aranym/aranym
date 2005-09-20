/**
 * Ethernet driver
 *
 * Stan (c) 2002
 *
 * GPL
 */

#ifndef _ETHERNET_H
#define _ETHERNET_H

#include "nf_base.h"

class ETHERNETDriver : public NF_Base
{
public:
	class Handler {
	public:
		virtual bool open( const char *mode ) = 0;
		virtual bool close() = 0;
		virtual int recv(uint8 *buf, int len) = 0;
		virtual int send(const uint8 *buf, int len) = 0;
		virtual ~Handler() { }
	};

private:
	Handler *handler;

	int32 readPacketLength(int ethX);
	void readPacket(int ethX, memptr buffer, uint32 len);
	void sendPacket(int ethX, memptr buffer, uint32 len);

	bool init();
	void exit();

	// emulators handling the TAP device
	bool startThread(int ethX);
	void stopThread(int ethX);
	static int receiveFunc(void *arg);

protected:
	typedef enum {HOST_IP, ATARI_IP, NETMASK} GET_PAR;
	int get_params(GET_PAR which);

public:
	ETHERNETDriver();
	~ETHERNETDriver();
	void reset();
	char *name() { return "ETHERNET"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);
};

#endif // _ETHERNET_H
