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
	int32 readPacketLength(int ethX);
	void readPacket(int ethX, memptr buffer, uint32 len);
	void sendPacket(int ethX, memptr buffer, uint32 len);

	// interrupt handling
	void finishInterupt();

	// emulators handling the TAP device
	static bool startThread(int ethX);
	static void stopThread(int ethX);
	static int receiveFunc(void *arg);

	// the /dev/net/tun driver (TAP)
	static int tapOpenOld(char *dev);
	static int tapOpen(char *dev);

protected:
	int get_params(const char *text);

public:
	char *name() { return "ETHERNET"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);

	static bool init(void);
	static void exit(void);
};
