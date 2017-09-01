/**
 * Ethernet TUN/TAP driver
 *
 * Standa (c) 2004-2005
 *
 * GPL
 */

#ifndef _ETHERNET_LINUX_H
#define _ETHERNET_LINUX_H

#include "ethernet.h"

class TunTapEthernetHandler : public ETHERNETDriver::Handler {
	int fd;

	// the /dev/net/tun driver (TAP)
	int tapOpenOld(char *dev);
	int tapOpen(char *dev);

public:
	TunTapEthernetHandler(int eth_idx);
	virtual ~TunTapEthernetHandler();

	virtual bool open();
	virtual void close();
	virtual int recv(uint8 *buf, int len);
	virtual int send(const uint8 *buf, int len);
};

#define ETHERNET_HANDLER_CLASSNAME TunTapEthernetHandler

#endif // _ETHERNET_LINUX_H

