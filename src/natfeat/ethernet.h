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
	typedef enum {HOST_IP, ATARI_IP, NETMASK} GET_PAR;
	int get_params(GET_PAR which);

public:
	char *name() { return "ETHERNET"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);

	bool init();
	void exit();
};
