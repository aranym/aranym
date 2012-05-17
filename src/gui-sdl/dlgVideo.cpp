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

enum VIDEODLG {
	box_main,
	text_fullscreen,
	FULLSCREEN,
	WINDOW,
	text_frameskip,
	FRAMESKIP_0,
	FRAMESKIP_1,
	FRAMESKIP_2,
	FRAMESKIP_5,
	FRAMESKIP_10,
	FRAMESKIP_50,
	text_monitor,
	MONITOR_NV,
	MONITOR_VGA,
	MONITOR_TV,
	text_colordepth,
	COLORDEPTH_NV,
	COLORDEPTH_1,
	COLORDEPTH_2,
	COLORDEPTH_4,
	COLORDEPTH_8,
	COLORDEPTH_16,
	text_autozoom,
	AUTOZOOM_ON,
	AZ_INTEGER,
	AZ_FIXEDSIZE,
	SINGLE_BLIT_COMPOSING,
	SINGLE_BLIT_REFRESH,
	RES_640,
	RES_800,
	RES_1024,
	RES_1280,
	RES_CUSTOM,
	text_rw,
	RES_WIDTH,
	text_rh,
	RES_HEIGHT,
	APPLY,
	CANCEL
};

// static char window_pos[10];
static char video_width[5];
static char video_height[5];

static SGOBJ videodlg[] =
{
	{ SGBOX, SG_BACKGROUND, 0, 0,0, 40,24, NULL },
	{ SGTEXT, 0, 0, 2,2, 7,1, "ARAnyM:" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 10,2, 10,1, "Fullscreen" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 24,2, 11,1, "in a Window" },
	{ SGTEXT, 0, 0, 2,4, 6,1, "VIDEL:" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 2,5, 6,1, "50 FPS" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 2,6, 6,1, "25 FPS" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 2,7, 6,1, "17 FPS" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 2,8, 6,1, "10 FPS" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 2,9, 6,1, " 5 FPS" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 2,10,6,1, " 1 FPS" },
	{ SGTEXT, 0, 0, 13, 4, 8, 1, "Monitor:"},
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 13,5, 7,1, "<NVRAM>" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 13,6, 3,1, "VGA" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 13,7, 2,1, "TV" },
	{ SGTEXT, 0, 0, 25,4, 12,1, "Boot Depth:" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 25,5, 7,1, "<NVRAM>" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 25,6, 8,1, "2 colors" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 25,7, 8,1, "4 colors" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 25,8, 9,1, "16 colors" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 25,9, 10,1, "256 colors" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 25,10, 9,1, "TrueColor" },
	{ SGTEXT, 0, 0, 2,13, 17,1, "Video output:" },
	{ SGCHECKBOX, SG_SELECTABLE, 0, 2,14, 16,1, "Autozoom enabled" },
	{ SGCHECKBOX, SG_SELECTABLE, 0, 2,15, 16,1, "Integer coefs" },
	{ SGCHECKBOX, SG_SELECTABLE, 0, 2,16, 16,1, "Fixed size" },
	{ SGCHECKBOX, SG_SELECTABLE, 0, 2,17, 19,1, "SingleBlitComposing" },
	{ SGCHECKBOX, SG_SELECTABLE, 0, 2,18, 16,1, "SingleBlitRefresh" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 23,14, 9,1, "640x480" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 23,15, 9,1, "800x600" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 23,16, 10,1, "1024x768" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 23,17, 11,1, "1280x1024" },
	{ SGCHECKBOX, SG_SELECTABLE|SG_RADIO, 0, 23,18, 10,1, "Custom" },
	{ SGTEXT, 0, 0, 26,19, 7,1, "Width: " },
	{ SGEDITFIELD, 0, 0, 33,19, sizeof(video_width)-1,1, video_width },
	{ SGTEXT, 0, 0, 26,20, 7,1, "Height:" },
	{ SGEDITFIELD, 0, 0, 33,20, sizeof(video_height)-1,1, video_height },
//	{ SGEDITFIELD, 0, 0, 12, 10, 9, 1, window_pos},
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT|SG_DEFAULT, 0, 6,22, 8,1, "Apply" },
	{ SGBUTTON, SG_SELECTABLE|SG_EXIT, 0, 27,22, 8,1, "Cancel" },
	{ -1, 0, 0, 0,0, 0,0, NULL }
};

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
