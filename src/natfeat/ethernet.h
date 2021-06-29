/*
 * ethernet.h - Ethernet Card Emulation
 *
 * Copyright (c) 2002-2005 Standa Opichal, Petr Stehlik of ARAnyM team
 * 
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _ETHERNET_H
#define _ETHERNET_H

#include "nf_base.h"
#include "parameters.h"		// defines MAX_ETH
#include <pthread.h>

#define MAX_PACKET_SIZE	9000

class ETHERNETDriver : public NF_Base
{
public:
	class Handler {
	public:
		int ethX;
		ssize_t packet_length;
		uint8 packet[MAX_PACKET_SIZE+2];
		pthread_t handlingThread;	// Packet reception thread
		pthread_cond_t intAck;			// Interrupt acknowledge semaphore
		pthread_mutex_t intLock;
		bool debug;

		Handler(int eth_idx) {
			ethX = eth_idx;
			packet_length = 0;
			handlingThread = 0;
			pthread_cond_init(&intAck, NULL);
			pthread_mutex_init(&intLock, NULL);
			debug = false;
		}
		virtual ~Handler() {
			pthread_mutex_destroy(&intLock);
			pthread_cond_destroy(&intAck);
		}
		virtual bool open() = 0;
		virtual void close() = 0;
		virtual int recv(uint8 *, int) = 0;
		virtual int send(const uint8 *, int) = 0;
	};

private:
	Handler *handlers[MAX_ETH];
	Handler *getHandler(int ethX, bool msg);
	static int pending_interrupts;

	int32 readPacketLength(int ethX);
	void readPacket(int ethX, memptr buffer, uint32 len);
	void sendPacket(int ethX, memptr buffer, uint32 len);

	bool init();
	void exit();

	// emulators handling the TAP device
	bool startThread(int ethX);
	void stopThread(Handler *handler);
	static void *receiveFunc(void *arg);

protected:
	typedef enum {HOST_IP, ATARI_IP, NETMASK} GET_PAR;
	int get_params(GET_PAR which);
	static void dump_ether_packet(const uint8_t *buf, int len);

public:
	ETHERNETDriver();
	~ETHERNETDriver();
	void reset();
	const char *name() { return "ETHERNET"; }
	bool isSuperOnly() { return true; }
	int32 dispatch(uint32 fncode);
};

#endif /* _ETHERNET_H */
