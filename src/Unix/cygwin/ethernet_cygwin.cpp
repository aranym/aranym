/*
    device.c -- Interaction with Windows tap driver in a Cygwin environment
    Copyright (C) 2002-2004 Ivo Timmermans <ivo@tinc-vpn.org>,
                  2002-2004 Guus Sliepen <guus@tinc-vpn.org>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "cpu_emulation.h"
#include "main.h"
#include "ethernet_cygwin.h"

#define DEBUG 0
#include "debug.h"

#include <winioctl.h>

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include <errno.h>

#define MTU 1500

//=============
// TAP IOCTLs
//=============

#define TAP_CONTROL_CODE(request,method) \
  CTL_CODE (FILE_DEVICE_UNKNOWN, request, method, FILE_ANY_ACCESS)

#define TAP_IOCTL_GET_MAC               TAP_CONTROL_CODE (1, METHOD_BUFFERED)
#define TAP_IOCTL_GET_VERSION           TAP_CONTROL_CODE (2, METHOD_BUFFERED)
#define TAP_IOCTL_GET_MTU               TAP_CONTROL_CODE (3, METHOD_BUFFERED)
#define TAP_IOCTL_GET_INFO              TAP_CONTROL_CODE (4, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_POINT_TO_POINT TAP_CONTROL_CODE (5, METHOD_BUFFERED)
#define TAP_IOCTL_SET_MEDIA_STATUS      TAP_CONTROL_CODE (6, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_MASQ      TAP_CONTROL_CODE (7, METHOD_BUFFERED)
#define TAP_IOCTL_GET_LOG_LINE          TAP_CONTROL_CODE (8, METHOD_BUFFERED)
#define TAP_IOCTL_CONFIG_DHCP_SET_OPT   TAP_CONTROL_CODE (9, METHOD_BUFFERED)

//=================
// Registry keys
//=================

#define ADAPTER_KEY "SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}"

#define NETWORK_CONNECTIONS_KEY "SYSTEM\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}"

//======================
// Filesystem prefixes
//======================

#define USERMODEDEVICEDIR "\\\\.\\Global\\"
#define SYSDEVICEDIR      "\\Device\\"
#define USERDEVICEDIR     "\\DosDevices\\Global\\"
#define TAPSUFFIX         ".tap"

//=========================================================
// TAP_COMPONENT_ID -- This string defines the TAP driver
// type -- different component IDs can reside in the system
// simultaneously.
//=========================================================

#define TAP_COMPONENT_ID "tap0801"

static char *winerror(int err) {
	static char buf[1024], *newline;

	if (!FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	        NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, sizeof(buf), NULL)) {
		strncpy(buf, "(unable to format errormessage)", sizeof(buf));
	}

	if((newline = strchr(buf, '\r')))
		*newline = '\0';

	return buf;
}


WinTapEthernetHandler::WinTapEthernetHandler(int eth_idx)
	: ETHERNETDriver::Handler(eth_idx)
{
	device_handle = INVALID_HANDLE_VALUE;
	device = NULL;
	iface = NULL;

	device_total_in = 0;
	device_total_out = 0;
}

WinTapEthernetHandler::~WinTapEthernetHandler()
{
	close();
}

bool is_tap_win32_dev(const char *guid)
{
	HKEY netcard_key;
	LONG status;
	DWORD len;
	int i = 0;

	status = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		ADAPTER_KEY,
		0,
		KEY_READ,
		&netcard_key);

	if (status != ERROR_SUCCESS) {
		D(bug("WinTap: Error opening registry key: %s", ADAPTER_KEY));
		return false;
	}

	while (true)
	{
		char enum_name[256];
		char unit_string[256];
		HKEY unit_key;
		char component_id_string[] = "ComponentId";
		char component_id[256];
		char net_cfg_instance_id_string[] = "NetCfgInstanceId";
		char net_cfg_instance_id[256];
		DWORD data_type;

		len = sizeof (enum_name);
		status = RegEnumKeyEx(
			netcard_key,
			i,
			enum_name,
			&len,
			NULL,
			NULL,
			NULL,
			NULL);

		if (status == ERROR_NO_MORE_ITEMS)
			break;
		else if (status != ERROR_SUCCESS) {
			D(bug("WinTap: Error enumerating registry subkeys of key: %s",
				ADAPTER_KEY));
			return false;
		}
	
		snprintf (unit_string, sizeof(unit_string), "%s\\%s",
			  ADAPTER_KEY, enum_name);

		status = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			unit_string,
			0,
			KEY_READ,
			&unit_key);

		if (status != ERROR_SUCCESS) {
			D(bug("WinTap: Error opening registry key: %s", unit_string)); 
			return false;
		}
		else
		{
			len = sizeof (component_id);
			status = RegQueryValueEx(
				unit_key,
				component_id_string,
				NULL,
				&data_type,
				(BYTE*)component_id,
				&len);

			if (!(status != ERROR_SUCCESS || data_type != REG_SZ)) {
				len = sizeof (net_cfg_instance_id);
				status = RegQueryValueEx(
					unit_key,
					net_cfg_instance_id_string,
					NULL,
					&data_type,
					(BYTE*)net_cfg_instance_id,
					&len);

				if (status == ERROR_SUCCESS && data_type == REG_SZ)
				{
					if (!strcmp (net_cfg_instance_id, guid))
					{
						RegCloseKey (unit_key);
						RegCloseKey (netcard_key);
						return true;
					}
				}
			}
			RegCloseKey (unit_key);
		}
		++i;
	}

	RegCloseKey (netcard_key);
	return false;
}


int get_device_guid(
	char *name,
	int name_size,
	char *actual_name,
	int actual_name_size)
{
	LONG status;
	HKEY control_net_key;
	DWORD len;
	int i = 0;
	int stop = 0;

	status = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		NETWORK_CONNECTIONS_KEY,
		0,
		KEY_READ,
		&control_net_key);

	if (status != ERROR_SUCCESS) {
		D(bug("WinTap: Error opening registry key: %s", NETWORK_CONNECTIONS_KEY));
		return -1;
	}

	while (!stop)
	{
		char enum_name[256];
		char connection_string[256];
		HKEY connection_key;
		char name_data[256];
		DWORD name_type;
		const char name_string[] = "Name";

		len = sizeof (enum_name);
		status = RegEnumKeyEx(
			control_net_key,
			i,
			enum_name,
			&len,
			NULL,
			NULL,
			NULL,
			NULL);

		if (status == ERROR_NO_MORE_ITEMS)
			break;
		else if (status != ERROR_SUCCESS) {
			D(bug("WinTap: Error enumerating registry subkeys of key: %s",
			       NETWORK_CONNECTIONS_KEY));
			return -1;
		}

		snprintf(connection_string, 
			 sizeof(connection_string),
			 "%s\\%s\\Connection",
			 NETWORK_CONNECTIONS_KEY, enum_name);

		status = RegOpenKeyEx(
			HKEY_LOCAL_MACHINE,
			connection_string,
			0,
			KEY_READ,
			&connection_key);
		
		if (status == ERROR_SUCCESS) {
			len = sizeof (name_data);
			status = RegQueryValueEx(
				connection_key,
				name_string,
				NULL,
				&name_type,
				(BYTE*)name_data,
				&len);

			if (status != ERROR_SUCCESS || name_type != REG_SZ) {
				D(bug("WinTap: Error opening registry key: %s\\%s\\%s",
				       NETWORK_CONNECTIONS_KEY, connection_string, name_string));
			        return -1;
			}
			else {
				if (is_tap_win32_dev(enum_name)) {
					D(bug("WinTap: found TAP device named \"%s\" ~ \"%s\"", name_data, actual_name));

					snprintf(name, name_size, "%s", enum_name);
					if (actual_name) {
						if (strcmp(actual_name, "") != 0) {
							if (strcmp(name_data, actual_name) != 0) {
								RegCloseKey (connection_key);
								++i;
								continue;
							}
						}
						else {
							snprintf(actual_name, actual_name_size, "%s", name_data);
						}
					}
					stop = 1;
				}
			}

			RegCloseKey (connection_key);
		}
		++i;
	}

	RegCloseKey (control_net_key);

	if (stop == 0)
		return -1;

	return 0;
}

bool WinTapEthernetHandler::open()
{
	char *type = bx_options.ethernet[ethX].type;
	char device_path[256];
	char device_guid[0x100];
	char name_buffer[0x100];

	close();

	if (strcmp(type, "none") == 0 || strlen(type) == 0)
	{
		return false;
	}

	if ( strlen(bx_options.ethernet[ethX].tunnel) == 0) {
		D(bug("WinTap(%d): tunnel name undefined", ethX));
		return false;
	}

	safe_strncpy(name_buffer, bx_options.ethernet[ethX].tunnel, sizeof(name_buffer));

 	if ( get_device_guid(device_guid, sizeof(device_guid), name_buffer, sizeof(name_buffer)) < 0 ) {
		panicbug("WinTap: ERROR: Could not find Windows tap device: %s", winerror(GetLastError()));
		return false;
	}

	/*
	 * Open Windows TAP-Win32 adapter
	 */
	snprintf (device_path, sizeof(device_path), "%s%s%s",
		  USERMODEDEVICEDIR,
		  device_guid,
		  TAPSUFFIX);

	device_handle = CreateFile(device_path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_SYSTEM | FILE_FLAG_OVERLAPPED, 0);

	if (device_handle == INVALID_HANDLE_VALUE) {
		panicbug("WinTap: ERROR: Could not open (%s) Windows tap device: %s", device_path, winerror(GetLastError()));
		return false;
	}
	device = strdup(device_path);
	
	read_overlapped.Offset = 0; 
	read_overlapped.OffsetHigh = 0; 
	read_overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	write_overlapped.Offset = 0; 
	write_overlapped.OffsetHigh = 0; 
	write_overlapped.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	D(bug("WinTap: tap device open %s [handle=%p]", device_path, (void*)device_handle));
	return true;
}

void WinTapEthernetHandler::close()
{
	if (device_handle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(device_handle);
		device_handle = INVALID_HANDLE_VALUE;
	}
	free(device);
	device = NULL;
}

int WinTapEthernetHandler::recv(uint8 *buf, int len)
{
	DWORD lenin;

	D(bug("WinTap: Read packet from %s", device));

	BOOL result = ReadFile(device_handle, buf, len, &lenin, &read_overlapped);
	if (!result) { 
		DWORD error = GetLastError();
		switch (error)
		{ 
		case ERROR_IO_PENDING: 
			WaitForSingleObject(read_overlapped.hEvent, INFINITE); 
			result = GetOverlappedResult( device_handle, &read_overlapped, &lenin, 0 );
			if ( result ) break;
			/* fallthrough */
		default:
			D(bug("WinTap: Error while reading from %s: %s", device, winerror(GetLastError())));
			return -1;
		}
	}

	device_total_in += lenin;
	D(bug("WinTap: Read packet done (len %d)", (int)lenin));
	return lenin;
}

int WinTapEthernetHandler::send(const uint8 *buf, int len)
{
	DWORD lenout;

	D(bug("WinTap: Writing packet of %d bytes to %s", len, device));

	BOOL result = WriteFile (device_handle, buf, len, &lenout, &write_overlapped);
	if (!result) { 
		DWORD error = GetLastError();
		switch (error)
		{ 
		case ERROR_IO_PENDING: 
			WaitForSingleObject(write_overlapped.hEvent, INFINITE);
			break;
		default:
			D(bug("WinTap: Error while writing to %s: %s", device, winerror(GetLastError())));
			return -1;
		} 
	}

	device_total_out += lenout;
	D(bug("WinTap: Writing packet done"));
	return lenout;
}
