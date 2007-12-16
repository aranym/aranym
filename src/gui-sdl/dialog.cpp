/*
	dialog.cpp - base class for a dialog

	Copyright (C) 2007 ARAnyM developer team

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
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "dialog.h"

Dialog::Dialog(SGOBJ *new_dlg)
	: dlg(new_dlg), return_obj(-1)
{
	/* Init cursor position in dialog */
	cursor.object = SDLGui_FindEditField(dlg, -1, SG_FIRST_EDITFIELD);
	cursor.position = (cursor.object != -1) ? strlen(dlg[cursor.object].txt) : 0;
	cursor.blink_counter = SDL_GetTicks();
	cursor.blink_state = true;
}

Dialog::~Dialog()
{
	SDLGui_DeselectButtons(dlg);
}

SGOBJ *Dialog::getDialog(void)
{
	return dlg;
}

cursor_state *Dialog::getCursor(void)
{
	return &cursor;
}

void Dialog::init(void)
{
	/*SDLGui_DrawDialog(dlg);*/

	return_obj = -1;
}

int Dialog::processDialog(void)
{
	/* Close current dialog if any object clicked/selected */
	if (return_obj>=0) {
		return GUI_CLOSE;
	}

	return GUI_CONTINUE;
}

void Dialog::mouseClick(const SDL_Event &event, int gui_x, int gui_y)
{
	int clicked_obj;
	int original_state = 0;

	int x = event.button.x - gui_x;
	int y = event.button.y - gui_y;

	clicked_obj = SDLGui_FindObj(dlg, x, y);

	if (clicked_obj >= 0) {
		original_state = dlg[clicked_obj].state;
		SDLGui_UpdateObjState(dlg, clicked_obj, original_state, x, y);

		if (dlg[clicked_obj].flags & SG_TOUCHEXIT) {
			return_obj = clicked_obj;
			clicked_obj = -1;
		}
	}

	if ((event.type == SDL_MOUSEBUTTONUP) && (clicked_obj>=0)) {
		if (SDLGui_UpdateObjState(dlg, clicked_obj, original_state, x, y)) {
			// true if mouse button is released over clicked object.
			// If applicable, the object has been selected by
			// SDLGui_UpdateObjState(). Let's do additional handling here.

			// Exit if object is an SG_EXIT one.
			if (dlg[clicked_obj].flags & SG_EXIT)
				return_obj = clicked_obj;

			// If it's a SG_RADIO object, deselect other objects in his group.
			if (dlg[clicked_obj].flags & SG_RADIO)
				SDLGui_SelectRadioObject(dlg, clicked_obj);

			if (dlg[clicked_obj].type == SGEDITFIELD)
				SDLGui_ClickEditField(dlg, &cursor, clicked_obj, x);
		}
	}

	// No special event occured.
	// Update object state according to mouse coordinates.
	SDLGui_UpdateObjState(dlg, clicked_obj, original_state, x, y);
}

void Dialog::keyPress(const SDL_Event &event)
{
	if (event.type != SDL_KEYDOWN) {
		return;
	}

	int obj;
	int keysym = event.key.keysym.sym;
	SDLMod mod = event.key.keysym.mod;

	if (cursor.object != -1) {
		switch(keysym) {
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				break;

			case SDLK_BACKSPACE:
				if (cursor.position > 0) {
					memmove(&dlg[cursor.object].txt[cursor.position-1],
						&dlg[cursor.object].txt[cursor.position],
						strlen(&dlg[cursor.object].txt[cursor.position])+1);
					cursor.position--;
				}
				break;

			case SDLK_DELETE:
				if(cursor.position < (int)strlen(dlg[cursor.object].txt)) {
					memmove(&dlg[cursor.object].txt[cursor.position],
						&dlg[cursor.object].txt[cursor.position+1],
						strlen(&dlg[cursor.object].txt[cursor.position+1])+1);
				}
				break;

			case SDLK_LEFT:
				if (cursor.position > 0)
					cursor.position--;
				break;

			case SDLK_RIGHT:
				if (cursor.position < (int)strlen(dlg[cursor.object].txt))
					cursor.position++;
				break;

			case SDLK_DOWN:
				SDLGui_MoveCursor(dlg, &cursor, SG_NEXT_EDITFIELD);
				break;

			case SDLK_UP:
				SDLGui_MoveCursor(dlg, &cursor, SG_PREVIOUS_EDITFIELD);
				break;

			case SDLK_TAB:
				SDLGui_MoveCursor(dlg, &cursor,
					mod & KMOD_SHIFT ? SG_PREVIOUS_EDITFIELD : SG_NEXT_EDITFIELD);
				break;

			case SDLK_HOME:
				if (mod & KMOD_CTRL)
					SDLGui_MoveCursor(dlg, &cursor, SG_FIRST_EDITFIELD);
				else
					cursor.position = 0;
				break;

			case SDLK_END:
				if (mod & KMOD_CTRL)
					SDLGui_MoveCursor(dlg, &cursor, SG_LAST_EDITFIELD);
				else
					cursor.position = strlen(dlg[cursor.object].txt);
				break;

			default:
				if ((keysym >= SDLK_KP0) && (keysym <= SDLK_KP9)) {
					// map numpad numbers to normal numbers
					keysym -= (SDLK_KP0 - SDLK_0);
				}
				/* If it is a "good" key then insert it into the text field */
				if ((keysym >= SDLK_SPACE) && (keysym < SDLK_KP0)) {
					if (strlen(dlg[cursor.object].txt) < dlg[cursor.object].w) {
						memmove(&dlg[cursor.object].txt[cursor.position+1],
							&dlg[cursor.object].txt[cursor.position],
							strlen(&dlg[cursor.object].txt[cursor.position])+1);
						dlg[cursor.object].txt[cursor.position] =
							(mod & KMOD_SHIFT) ? toupper(keysym) : keysym;
						cursor.position++;
					}
				}
				break;
		}
	}

	switch(keysym) {
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			obj = SDLGui_FindDefaultObj(dlg);
			if (obj >= 0) {
				dlg[obj].state ^= SG_SELECTED;
				SDLGui_DrawObject(dlg, obj);
				SDLGui_RefreshObj(dlg, obj);
				if (dlg[obj].flags & (SG_EXIT | SG_TOUCHEXIT)) {
					return_obj = obj;
				}
			}
			break;
		default:
			break;
	}

	// Force cursor display. Should ease text input.
	cursor.blink_state = true;
	cursor.blink_counter = SDL_GetTicks();
}

void Dialog::idle(void)
{
}

void Dialog::processResult(void)
{
}

