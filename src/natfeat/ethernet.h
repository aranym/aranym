/**
 * Ethernet driver
 *
 * Stan (c) 2002
 *
 * GPL
 */

#include "nf_base.h"

class ETHERNETDriver : public NF_Base
{
	int32 readPacketLength(memptr nif);
	void readPacket(memptr buffer, uint32 len);
	void sendPacket(memptr buffer, uint32 len);

	// interrupt handling
	void finishInterupt();

	// emulators handling the TAP device
	static bool startThread(void);
	static void stopThread(void);
	static int receiveFunc(void *arg);

	// the /dev/net/tun driver (TAP)
	static int tapOpenOld(char *dev);
	static int tapOpen(char *dev);

public:
	char *name() { return "ETHERNET"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);

	static bool init(void);
	static void exit(void);
};
