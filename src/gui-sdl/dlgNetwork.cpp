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

#define BRIDGE_FILTER_NONE  0
#define BRIDGE_FILTER_MCAST 1
#define BRIDGE_FILTER_IP    2
#define BRIDGE_FILTER_MAC   3

static struct {
	bx_ethernet_options_t config;
	bool bridge;
	bool debug;
	int filter_type;
} eth[2];

#define SDLGUI_INCLUDE_NETWORKDLG
#include "sdlgui.sdl"


void DlgNetwork::init_options(int ethX, int type)
{
	eth[ethX].config = bx_options.ethernet[ethX];
	eth[ethX].debug = strstr(eth[ethX].config.type, "debug") != NULL;
	eth[ethX].bridge = strstr(eth[ethX].config.type, "bridge") != NULL;

	dlg[type + 0].state = 0; /* ETH0_TYP_NONE */
	dlg[type + 1].state = 0; /* ETH0_TYP_PTP */
	dlg[type + 2].state = 0; /* ETH0_TYP_BRIDGE */

	if (strlen(eth[ethX].config.type) == 0 || strcmp(eth[ethX].config.type, "none") == 0)
		dlg[type + 0].state = SG_SELECTED;
	else if (eth[ethX].bridge)
		dlg[type + 2].state = SG_SELECTED;
	else
		dlg[type + 1].state = SG_SELECTED;

	if (strstr(eth[ethX].config.type, "nofilter") != NULL)
		eth[ethX].filter_type = BRIDGE_FILTER_NONE;
	else if (strstr(eth[ethX].config.type, "mcast") != NULL)
		eth[ethX].filter_type = BRIDGE_FILTER_MCAST;
	else if (strstr(eth[ethX].config.type, "ip") != NULL)
		eth[ethX].filter_type = BRIDGE_FILTER_IP;
	else
		eth[ethX].filter_type = BRIDGE_FILTER_MAC;
}

DlgNetwork::DlgNetwork(SGOBJ *dlg)
	: Dialog(dlg)
{
	// init
	init_options(0, ETH0_TYP_NONE);
	init_options(1, ETH1_TYP_NONE);
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

void DlgNetwork::save_options(int ethX, int type)
{
	eth[ethX].bridge = (dlg[type + 2].state & SG_SELECTED);
	if (dlg[type + 0].state & SG_SELECTED) {
		eth[ethX].config.type[0] = '\0';
	}
	else {
		safe_strncpy(eth[ethX].config.type, eth[ethX].bridge ? "bridge" : "ptp", sizeof(eth[ethX].config.type));
		if (eth[ethX].debug)
			safe_strncat(eth[ethX].config.type, " debug", sizeof(eth[ethX].config.type));
		if (eth[ethX].bridge)
		{
			switch (eth[ethX].filter_type)
			{
			case BRIDGE_FILTER_NONE:
				safe_strncat(eth[ethX].config.type, " nofilter", sizeof(eth[ethX].config.type));
				break;
			case BRIDGE_FILTER_MCAST:
				safe_strncat(eth[ethX].config.type, " mcast", sizeof(eth[ethX].config.type));
				break;
			case BRIDGE_FILTER_IP:
				safe_strncat(eth[ethX].config.type, " ip", sizeof(eth[ethX].config.type));
				break;
			default:
			case BRIDGE_FILTER_MAC:
				break;
			}
		}
	}
	bx_options.ethernet[ethX] = eth[ethX].config;
}

void DlgNetwork::confirm(void)
{
	save_options(0, ETH0_TYP_NONE);
	save_options(1, ETH1_TYP_NONE);
}

Dialog *DlgNetworkOpen(void)
{
	return new DlgNetwork(networkdlg);
}
