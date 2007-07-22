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

enum DLG {
	box_main,
	eth0_type,
	ETH0_TUNNEL,
	ETH0_TYP_NONE,
	ETH0_TYP_PTP,
	ETH0_TYP_BRIDGE,
	eth0_hip,
	ETH0_HIP,
	eth0_aip,
	ETH0_AIP,
	eth0_mask,
	ETH0_MASK,
	eth0_mac,
	ETH0_MAC,
	eth1_type,
	ETH1_TUNNEL,
	ETH1_TYP_NONE,
	ETH1_TYP_PTP,
	ETH1_TYP_BRIDGE,
	eth1_hip,
	ETH1_HIP,
	eth1_aip,
	ETH1_AIP,
	eth1_mask,
	ETH1_MASK,
	eth1_mac,
	ETH1_MAC,
	APPLY,
	CANCEL
};

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

static SGOBJ dlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 40,25, NULL },
	{ SGTEXT, 0, 0, 1,2, 5,1, "ETH0:" },
	{ SGEDITFIELD, 0, 0, 7,2, MIN(sizeof(eth0_tunnel)-1, 5),1, eth0_tunnel },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 14,2, 6,1, "None" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 22,2, 7,1, "P-t-p" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 31,2, 8,1, "Bridge" },
	{ SGTEXT, 0, 0, 4,4, 8,1, "Host IP:" },
	{ SGEDITFIELD, 0, 0, 14,4, sizeof(eth0_host_ip)-1,1, eth0_host_ip },
	{ SGTEXT, 0, 0, 4,6, 9,1, "Atari IP:" },
	{ SGEDITFIELD, 0, 0, 14,6, sizeof(eth0_atari_ip)-1,1, eth0_atari_ip },
	{ SGTEXT, 0, 0, 4,8, 8,1, "Netmask:" },
	{ SGEDITFIELD, 0, 0, 14,8, sizeof(eth0_netmask)-1,1, eth0_netmask },
	{ SGTEXT, 0, 0, 4,10, 9,1, "MAC addr:" },
	{ SGEDITFIELD, 0, 0, 14,10, sizeof(eth0_mac_addr)-1,1, eth0_mac_addr },
	{ SGTEXT, 0, 0, 1,12, 7,1, "ETH1:" },
	{ SGEDITFIELD, 0, 0, 7,12, MIN(sizeof(eth1_tunnel)-1, 5),1, eth1_tunnel },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 14,12, 6,1, "None" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 21,12, 8,1, "P-t-p" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 30,12, 8,1, "Bridge" },
	{ SGTEXT, 0, 0, 4,14, 8,1, "Host IP:" },
	{ SGEDITFIELD, 0, 0, 14,14, sizeof(eth1_host_ip)-1,1, eth1_host_ip },
	{ SGTEXT, 0, 0, 4,16, 9,1, "Atari IP:" },
	{ SGEDITFIELD, 0, 0, 14,16, sizeof(eth1_atari_ip)-1,1, eth1_atari_ip },
	{ SGTEXT, 0, 0, 4,18, 8,1, "Netmask:" },
	{ SGEDITFIELD, 0, 0, 14,18, sizeof(eth1_netmask)-1,1, eth1_netmask },
	{ SGTEXT, 0, 0, 4,20, 9,1, "MAC addr:" },
	{ SGEDITFIELD, 0, 0, 14,20, sizeof(eth1_mac_addr)-1,1, eth1_mac_addr },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 8,23, 8,1, "Apply" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 28,23, 8,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

void Dialog_NetworkDlg()
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

	// apply
	if (SDLGui_DoDialog(dlg) == APPLY) {
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
}

/*
vim:ts=4:sw=4:
*/
