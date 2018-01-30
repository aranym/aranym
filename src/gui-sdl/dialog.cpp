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

#include "sysdeps.h"
#include "dialog.h"
#include "input.h"

Dialog::Dialog(SGOBJ *new_dlg)
	: dlg(new_dlg), return_obj(-1), last_clicked_obj(-1), touch_exit_obj(-1)
{
	/* Init cursor position in dialog */
	cursor.object = SDLGui_FindEditField(dlg, -1, SG_FIRST_EDITFIELD);
	cursor.position = (cursor.object != -1 && dlg[cursor.object].txt) ? SDLGui_TextLen(dlg[cursor.object].txt) : 0;
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
	
	if (event.type == SDL_MOUSEBUTTONUP && touch_exit_obj != -1) {
		SDLGui_DeselectAndRedraw(dlg, touch_exit_obj);
	}
	touch_exit_obj = -1;

	clicked_obj = SDLGui_FindObj(dlg, x, y);
	if (clicked_obj<0) {
		return;
	}

	/* Read current state, to restore it if needed */
	original_state = dlg[clicked_obj].state;

	/* Memorize object on mouse button pressed event */
	if (event.type == SDL_MOUSEBUTTONDOWN) {
		/* ignore mouse wheel events reported as button presses here */
		if (!(event.button.button >= 1 && event.button.button <= 3))
			return;
		SDLGui_UpdateObjState(dlg, clicked_obj, original_state, x, y);
		last_clicked_obj = clicked_obj;
		last_state = original_state;

		/* Except for TOUCHEXIT objects which must be activated on mouse button pressed */
		if (dlg[clicked_obj].flags & SG_TOUCHEXIT) {
			/*SDLGui_UpdateObjState(dlg, clicked_obj, original_state, x, y);*/

			return_obj = clicked_obj;
			last_clicked_obj = -1;
			touch_exit_obj = clicked_obj;
		}

		return;
	}

	/* Compare with object when releasing mouse button */
	if (clicked_obj == last_clicked_obj) {
		// Exit if object is an SG_EXIT one.
		if (dlg[clicked_obj].flags & SG_EXIT) {
			/* Hum, HACK for checkbox CDROM in diskdlg */
			if (dlg[clicked_obj].type != SGCHECKBOX) {
				/* Restore original state before exiting dialog */
				SDLGui_UpdateObjState(dlg, clicked_obj, original_state, x, y);
			}
			return_obj = clicked_obj;
		}

		// If it's a SG_RADIO object, deselect other objects in his group.
		if (dlg[clicked_obj].flags & SG_RADIO)
			SDLGui_SelectRadioObject(dlg, clicked_obj);

		if (dlg[clicked_obj].type == SGEDITFIELD)
			SDLGui_ClickEditField(dlg, &cursor, clicked_obj, x);
	} else if (last_clicked_obj>=0) {
		/* We released mouse on a different object, restore state of mouse-press object */
		SDLGui_UpdateObjState(dlg, last_clicked_obj, last_state, x, y);
	}
}

void Dialog::keyPress(const SDL_Event &event)
{
	if (event.type != SDL_KEYDOWN) {
		return;
	}

	int obj;
	SDL_Keycode keysym = event.key.keysym.sym;
	int state;
	if ((event.key.keysym.mod & HOTKEYS_MOD_MASK) == 0)
		state  = SDL_GetModState(); // keysym.mod does not deliver single mod key presses for some reason
	else
		state = event.key.keysym.mod;	// May be send by SDL_PushEvent

	if (cursor.object != -1) {
		switch(keysym) {
			case SDLK_RETURN:
			case SDLK_KP_ENTER:
				break;

			case SDLK_BACKSPACE:
				if (cursor.position > 0) {
					char *txt = (char *)dlg[cursor.object].txt;
					int prev = SDLGui_ByteLen(txt, cursor.position-1);
					int curr = SDLGui_ByteLen(txt, cursor.position);
					memmove(&txt[prev], &txt[curr], strlen(&txt[curr]) + 1);
					cursor.position--;
				}
				break;

			case SDLK_DELETE:
				if(cursor.position < SDLGui_TextLen(dlg[cursor.object].txt)) {
					char *txt = (char *)dlg[cursor.object].txt;
					int curr = SDLGui_ByteLen(txt, cursor.position);
					int next = SDLGui_ByteLen(txt, cursor.position + 1);
					memmove(&txt[curr], &txt[next], strlen(&txt[next]) + 1);
				}
				break;

			case SDLK_LEFT:
				if (cursor.position > 0)
					cursor.position--;
				break;

			case SDLK_RIGHT:
				if (cursor.position < SDLGui_TextLen(dlg[cursor.object].txt))
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
					(state & KMOD_SHIFT) ? SG_PREVIOUS_EDITFIELD : SG_NEXT_EDITFIELD);
				break;

			case SDLK_HOME:
				if (state & KMOD_CTRL)
					SDLGui_MoveCursor(dlg, &cursor, SG_FIRST_EDITFIELD);
				else
					cursor.position = 0;
				break;

			case SDLK_END:
				if (state & KMOD_CTRL)
					SDLGui_MoveCursor(dlg, &cursor, SG_LAST_EDITFIELD);
				else
					cursor.position = SDLGui_TextLen(dlg[cursor.object].txt);
				break;

			default:
				// map numpad numbers to normal numbers
				switch(keysym)
				{
					case SDLK_KP_0: keysym = SDLK_0; break;
					case SDLK_KP_1: keysym = SDLK_1; break;
					case SDLK_KP_2: keysym = SDLK_2; break;
					case SDLK_KP_3: keysym = SDLK_3; break;
					case SDLK_KP_4: keysym = SDLK_4; break;
					case SDLK_KP_5: keysym = SDLK_5; break;
					case SDLK_KP_6: keysym = SDLK_6; break;
					case SDLK_KP_7: keysym = SDLK_7; break;
					case SDLK_KP_8: keysym = SDLK_8; break;
					case SDLK_KP_9: keysym = SDLK_9; break;
					default: break;
				}
				/* If it is a "good" key then insert it into the text field */
				/* FIXME: can't handle non-ascii characters here */
				if (((unsigned int)keysym >= 0x20) && ((unsigned int)keysym < 0x80)) {
					char *dlgtxt = (char *)dlg[cursor.object].txt;
					if (SDLGui_TextLen(dlgtxt) < (int)dlg[cursor.object].w) {
						int curr = SDLGui_ByteLen(dlgtxt, cursor.position);
						memmove(&dlgtxt[curr + 1], &dlgtxt[curr], strlen(&dlgtxt[curr])+1);
						dlgtxt[curr] = (state & KMOD_SHIFT) ? toupper(keysym) : keysym;
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
			{
				int masked_mod = state & HOTKEYS_MOD_MASK;
				HOTKEY hotkey = check_hotkey(masked_mod, keysym);
				handleHotkey(hotkey);
			}
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

bool Dialog::isTouchExitPressed(void)
{
	return touch_exit_obj != -1;
}
