/**
 * Ethernet Win32-Tap driver
 *
 * Standa (c) 2004-2005
 *
 * GPL
 */

#ifndef _ETHERNET_CYGWIN_H
#define _ETHERNET_CYGWIN_H

#include "ethernet.h"
#include "windows_ver.h"

class WinTapEthernetHandler : public ETHERNETDriver::Handler {
	OVERLAPPED read_overlapped;
	OVERLAPPED write_overlapped;
	HANDLE device_handle;
	char *device;
	char *iface;

	int device_total_in;
	int device_total_out;

public:
	WinTapEthernetHandler(int eth_idx);
	virtual ~WinTapEthernetHandler();

	virtual bool open();
	virtual void close();
	virtual int recv(uint8 *buf, int len);
	virtual int send(const uint8 *buf, int len);
};

#define ETHERNET_HANDLER_CLASSNAME WinTapEthernetHandler

#endif // _ETHERNET_CYGWIN_H
