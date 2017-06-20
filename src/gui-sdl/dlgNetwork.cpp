/*
 * dlgNetwork.cpp - Network dialog 
 *
 * Copyright (c) 2007 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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

#include "sysdeps.h"
#include "sdlgui.h"
#include "tools.h" // safe_strncpy
#include "dlgNetwork.h"

static char eth0_tunnel[16];
static char eth0_host_ip[16];
static char eth0_atari_ip[16];
static char eth0_netmask[16];
static char eth0_mac_addr[18];
static char eth1_tunnel[16];
static char eth1_host_ip[16];
static char eth1_atari_ip[16];
static char eth1_netmask[16];
static char eth1_mac_addr[18];

#define SDLGUI_INCLUDE_NETWORKDLG
#include "sdlgui.sdl"

DlgNetwork::DlgNetwork(SGOBJ *dlg)
	: Dialog(dlg)
{
	bx_ethernet_options_t *eth0 = &bx_options.ethernet[0];
	bx_ethernet_options_t *eth1 = &bx_options.ethernet[1];

	// init
	dlg[ETH0_TYP_NONE].state = 0;
	dlg[ETH0_TYP_PTP].state = 0;
	dlg[ETH0_TYP_BRIDGE].state = 0;
	if (strlen(eth0->type) == 0)
		dlg[ETH0_TYP_NONE].state = SG_SELECTED;
	else if (strcasecmp(eth0->type, "bridge") == 0)
		dlg[ETH0_TYP_BRIDGE].state = SG_SELECTED;
	else
		dlg[ETH0_TYP_PTP].state = SG_SELECTED;
	safe_strncpy(eth0_tunnel, eth0->tunnel, sizeof(eth0_tunnel));
	safe_strncpy(eth0_host_ip, eth0->ip_host, sizeof(eth0_host_ip));
	safe_strncpy(eth0_atari_ip, eth0->ip_atari, sizeof(eth0_atari_ip));
	safe_strncpy(eth0_netmask, eth0->netmask, sizeof(eth0_netmask));
	safe_strncpy(eth0_mac_addr, eth0->mac_addr, sizeof(eth0_mac_addr));

	dlg[ETH1_TYP_NONE].state = 0;
	dlg[ETH1_TYP_PTP].state = 0;
	dlg[ETH1_TYP_BRIDGE].state = 0;
	if (strlen(eth1->type) == 0)
		dlg[ETH1_TYP_NONE].state = SG_SELECTED;
	else if (strcasecmp(eth1->type, "bridge") == 0)
		dlg[ETH1_TYP_BRIDGE].state = SG_SELECTED;
	else
		dlg[ETH1_TYP_PTP].state = SG_SELECTED;
	safe_strncpy(eth1_tunnel, eth1->tunnel, sizeof(eth1_tunnel));
	safe_strncpy(eth1_host_ip, eth1->ip_host, sizeof(eth1_host_ip));
	safe_strncpy(eth1_atari_ip, eth1->ip_atari, sizeof(eth1_atari_ip));
	safe_strncpy(eth1_netmask, eth1->netmask, sizeof(eth1_netmask));
	safe_strncpy(eth1_mac_addr, eth1->mac_addr, sizeof(eth1_mac_addr));
}

DlgNetwork::~DlgNetwork()
{
}

int DlgNetwork::processDialog(void)
{
	int retval = Dialog::GUI_CONTINUE;

	switch(return_obj) {
		case APPLY:
			confirm();
			/* fall through */
		case CANCEL:
			retval = Dialog::GUI_CLOSE;
			break;
	}

	return retval;
}

void DlgNetwork::confirm(void)
{
	bx_ethernet_options_t *eth0 = &bx_options.ethernet[0];
	bx_ethernet_options_t *eth1 = &bx_options.ethernet[1];

	if (dlg[ETH0_TYP_NONE].state & SG_SELECTED) {
		*eth0->type = '\0';
	}
	else {
		bool eth0_bridge = (dlg[ETH0_TYP_BRIDGE].state & SG_SELECTED);
		safe_strncpy(eth0->type, eth0_bridge ? "bridge" : "p-t-p", sizeof(eth0->type));
	}
	safe_strncpy(eth0->tunnel, eth0_tunnel, sizeof(eth0->tunnel));
	safe_strncpy(eth0->ip_host, eth0_host_ip, sizeof(eth0->ip_host));
	safe_strncpy(eth0->ip_atari, eth0_atari_ip, sizeof(eth0->ip_atari));
	safe_strncpy(eth0->netmask, eth0_netmask, sizeof(eth0->netmask));
	safe_strncpy(eth0->mac_addr, eth0_mac_addr, sizeof(eth0->mac_addr));

	if (dlg[ETH1_TYP_NONE].state & SG_SELECTED) {
		*eth1->type = '\0';
	}
	else {
		bool eth1_bridge = (dlg[ETH1_TYP_BRIDGE].state & SG_SELECTED);
		safe_strncpy(eth1->type, eth1_bridge ? "bridge" : "p-t-p", sizeof(eth1->type));
	}
	safe_strncpy(eth1->tunnel, eth1_tunnel, sizeof(eth1->tunnel));
	safe_strncpy(eth1->ip_host, eth1_host_ip, sizeof(eth1->ip_host));
	safe_strncpy(eth1->ip_atari, eth1_atari_ip, sizeof(eth1->ip_atari));
	safe_strncpy(eth1->netmask, eth1_netmask, sizeof(eth1->netmask));
	safe_strncpy(eth1->mac_addr, eth1_mac_addr, sizeof(eth1->mac_addr));
}

Dialog *DlgNetworkOpen(void)
{
	return new DlgNetwork(networkdlg);
}
