/**
 * Ethernet Win32-Tap driver
 *
 * Standa (c) 2004
 *
 * GPL
 */

#ifndef _ETHERNET_CYGWIN_H
#define _ETHERNET_CYGWIN_H

#include "ethernet.h"
#include <w32api/windows.h>

class WinTapEthernetHandler : public ETHERNETDriver::Handler {
	OVERLAPPED read_overlapped;
	OVERLAPPED write_overlapped;
	HANDLE device_handle;
	char *device;
	char *iface;

	int device_total_in;
	int device_total_out;

public:
	WinTapEthernetHandler();

	virtual bool open( const char *mode );
	virtual bool close();
	virtual int recv(uint8 *buf, int len);
	virtual int send(const uint8 *buf, int len);
};

#define ETHERNET_HANDLER_CLASSNAME WinTapEthernetHandler

#endif // _ETHERNET_CYGWIN_H
