/**
 * Ethernet TUN/TAP driver
 *
 * Standa (c) 2004
 *
 * GPL
 */

#ifndef _ETHERNET_LINUX_H
#define _ETHERNET_LINUX_H


class TunTapEthernetHandler : public ETHERNETDriver::Handler {
	int fd;

	// the /dev/net/tun driver (TAP)
	static int tapOpenOld(char *dev);
	static int tapOpen(char *dev);

public:
	TunTapEthernetHandler() fd(-1) {}

	virtual bool up( const char *mode );
	virtual bool down();
	virtual int recv(uint8 *buf, int len);
	virtual int send(const uint8 *buf, int len);
}

#define ETHERNET_HANDLER_CLASSNAME TunTapEthernetHandler

#endif // _ETHERNET_LINUX_H
