/*
 * dlgVideo.cpp - Video dialog 
 *
 * Copyright (c) 2004-2007 Petr Stehlik of ARAnyM dev team (see AUTHORS)
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
#include "hostscreen.h"
#include "dlgVideo.h"

#define S50F	1
#define S25F	2
#define S17F	3
#define S10F	5
#define S05F	10
#define S01F	50

// static char window_pos[10];
static char video_width[5];
static char video_height[5];

#define SDLGUI_INCLUDE_VIDEODLG
#include "sdlgui.sdl"

DlgVideo::DlgVideo(SGOBJ *dlg)
	: Dialog(dlg)
{
	bx_video_options_t *video = &bx_options.video;
	bx_autozoom_options_t *autozoom = &bx_options.autozoom;

	// init
	videodlg[FULLSCREEN].state = video->fullscreen ? SG_SELECTED : 0;
	videodlg[WINDOW].state = !video->fullscreen ? SG_SELECTED : 0;

	videodlg[FRAMESKIP_0].state = 0;
	videodlg[FRAMESKIP_1].state = 0;
	videodlg[FRAMESKIP_2].state = 0;
	videodlg[FRAMESKIP_5].state = 0;
	videodlg[FRAMESKIP_10].state = 0;
	videodlg[FRAMESKIP_50].state = 0;
	switch(video->refresh)
	{
		default:
		case S50F: videodlg[FRAMESKIP_0].state = SG_SELECTED; break;
		case S25F: videodlg[FRAMESKIP_1].state = SG_SELECTED; break;
		case S17F: videodlg[FRAMESKIP_2].state = SG_SELECTED; break;
		case S10F: videodlg[FRAMESKIP_5].state = SG_SELECTED; break;
		case S05F: videodlg[FRAMESKIP_10].state = SG_SELECTED; break;
		case S01F: videodlg[FRAMESKIP_50].state = SG_SELECTED; break;
	}

	videodlg[MONITOR_NV].state = 0;
	videodlg[MONITOR_VGA].state = 0;
	videodlg[MONITOR_TV].state = 0;
	switch(video->monitor)
	{
		default:
		case -1: videodlg[MONITOR_NV].state = SG_SELECTED; break;
		case 0: videodlg[MONITOR_VGA].state = SG_SELECTED; break;
		case 1: videodlg[MONITOR_TV].state = SG_SELECTED; break;
	}

	videodlg[COLORDEPTH_NV].state = 0;
	videodlg[COLORDEPTH_1].state = 0;
	videodlg[COLORDEPTH_2].state = 0;
	videodlg[COLORDEPTH_4].state = 0;
	videodlg[COLORDEPTH_8].state = 0;
	videodlg[COLORDEPTH_16].state = 0;
	switch(video->boot_color_depth)
	{
		default:
		case -1: videodlg[COLORDEPTH_NV].state = SG_SELECTED; break;
		case 1: videodlg[COLORDEPTH_1].state = SG_SELECTED; break;
		case 2: videodlg[COLORDEPTH_2].state = SG_SELECTED; break;
		case 4: videodlg[COLORDEPTH_4].state = SG_SELECTED; break;
		case 8: videodlg[COLORDEPTH_8].state = SG_SELECTED; break;
		case 16: videodlg[COLORDEPTH_16].state = SG_SELECTED; break;
	}

	videodlg[AUTOZOOM_ON].state = autozoom->enabled ? SG_SELECTED:0;
	videodlg[AZ_INTEGER].state = autozoom->integercoefs ? SG_SELECTED:0;
	videodlg[AZ_FIXEDSIZE].state = autozoom->fixedsize ? SG_SELECTED:0;
	videodlg[SINGLE_BLIT_COMPOSING].state = video->single_blit_composing ? SG_SELECTED : 0;
	videodlg[SINGLE_BLIT_REFRESH].state = video->single_blit_refresh ? SG_SELECTED : 0;

	videodlg[RES_640].state = 0;
	videodlg[RES_800].state = 0;
	videodlg[RES_1024].state = 0;
	videodlg[RES_1280].state = 0;
	videodlg[RES_CUSTOM].state = 0;
	if (autozoom->width == 640 && autozoom->height == 480)
		videodlg[RES_640].state |= SG_SELECTED;
	else if (autozoom->width == 800 && autozoom->height == 600)
		videodlg[RES_800].state |= SG_SELECTED;
	else if (autozoom->width == 1024 && autozoom->height == 768)
		videodlg[RES_1024].state |= SG_SELECTED;
	else if (autozoom->width == 1280 && autozoom->height == 1024)
		videodlg[RES_1280].state |= SG_SELECTED;
	else {
		videodlg[RES_CUSTOM].state |= SG_SELECTED;
	}
	sprintf(video_width, "%4d", autozoom->width);
	sprintf(video_height, "%4d", autozoom->height);
}

DlgVideo::~DlgVideo()
{
}

int DlgVideo::processDialog(void)
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

void DlgVideo::confirm(void)
{
	bx_video_options_t *video = &bx_options.video;
	bx_autozoom_options_t *autozoom = &bx_options.autozoom;

	video->fullscreen = videodlg[FULLSCREEN].state & SG_SELECTED;

	video->refresh = S50F;
	if (videodlg[FRAMESKIP_1].state & SG_SELECTED)
		video->refresh = S25F;
	else if (videodlg[FRAMESKIP_2].state & SG_SELECTED)
		video->refresh = S17F;
	else if (videodlg[FRAMESKIP_5].state & SG_SELECTED)
		video->refresh = S10F;
	else if (videodlg[FRAMESKIP_10].state & SG_SELECTED)
		video->refresh = S05F;
	else if (videodlg[FRAMESKIP_50].state & SG_SELECTED)
		video->refresh = S01F;

	video->monitor = -1;
	if (videodlg[MONITOR_VGA].state & SG_SELECTED)
		video->monitor = 0;
	else if (videodlg[MONITOR_TV].state & SG_SELECTED)
		video->monitor = 1;

	video->boot_color_depth = -1;
	if (videodlg[COLORDEPTH_1].state & SG_SELECTED)
		video->boot_color_depth = 1;
	else if (videodlg[COLORDEPTH_2].state & SG_SELECTED)
		video->boot_color_depth = 2;
	else if (videodlg[COLORDEPTH_4].state & SG_SELECTED)
		video->boot_color_depth = 4;
	else if (videodlg[COLORDEPTH_8].state & SG_SELECTED)
		video->boot_color_depth = 8;
	else if (videodlg[COLORDEPTH_16].state & SG_SELECTED)
		video->boot_color_depth = 16;

	autozoom->enabled = videodlg[AUTOZOOM_ON].state & SG_SELECTED;
	autozoom->integercoefs = videodlg[AZ_INTEGER].state & SG_SELECTED;
	autozoom->fixedsize = videodlg[AZ_FIXEDSIZE].state & SG_SELECTED;

    video->single_blit_composing = videodlg[SINGLE_BLIT_COMPOSING].state & SG_SELECTED;
    video->single_blit_refresh = videodlg[SINGLE_BLIT_REFRESH].state & SG_SELECTED;

	if (videodlg[RES_640].state & SG_SELECTED) {
		autozoom->width = 640;
		autozoom->height= 480;
	}
	else if (videodlg[RES_800].state & SG_SELECTED) {
		autozoom->width = 800;
		autozoom->height= 600;
	}
	else if (videodlg[RES_1024].state & SG_SELECTED) {
		autozoom->width = 1024;
		autozoom->height= 768;
	}
	else if (videodlg[RES_1280].state & SG_SELECTED) {
		autozoom->width = 1280;
		autozoom->height= 1024;
	}
	else if (videodlg[RES_CUSTOM].state & SG_SELECTED) {
		autozoom->width = atoi(video_width);
		autozoom->height= atoi(video_height);
	}
}

Dialog *DlgVideoOpen(void)
{
	return new DlgVideo(videodlg);
}
