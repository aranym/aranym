/**
 * Ethernet driver
 *
 * Standa, Petr (c) 2002-2005
 *
 * GPL
 */

#ifndef _ETHERNET_H
#define _ETHERNET_H

#include "nf_base.h"

#define MAX_ETH		4
#define MAX_PACKET_SIZE	1514

class ETHERNETDriver : public NF_Base
{
public:
	class Handler {
	public:
		ssize_t packet_length;
		uint8 packet[MAX_PACKET_SIZE+2];
		SDL_Thread *handlingThread;	// Packet reception thread
		SDL_sem *intAck;			// Interrupt acknowledge semaphore

		Handler() {
			packet_length = 0;
			handlingThread = NULL;
			intAck = NULL;
		}

		virtual bool open( const char * ) { return false; }
		virtual bool close() { return false; }
		virtual int recv(uint8 *, int) { return 0; }
		virtual int send(const uint8 *, int) { return 0; }
		virtual ~Handler() { }
	};

private:
	Handler *handlers[MAX_ETH];
	Handler *getHandler(int ethX);

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

/*
vim:ts=4:sw=4:
*/
