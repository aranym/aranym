/*
 *	Input handling
 */

#ifndef _INPUT_H
#define _INPUT_H

#include "SDL_compat.h"

#define HOTKEYS_MOD_MASK	(KMOD_SHIFT | KMOD_CTRL | KMOD_ALT | KMOD_GUI | KMOD_MODE)

typedef enum {
	HOTKEY_none,
	HOTKEY_setup,
	HOTKEY_quit,
	HOTKEY_warmreboot,
	HOTKEY_coldreboot,
	HOTKEY_debug,
	HOTKEY_ungrab,
	HOTKEY_fullscreen,
	HOTKEY_screenshot,
	HOTKEY_sound,
} HOTKEY;

void InputInit();
void InputReset();
void InputExit();
SDL_bool grabMouse(SDL_bool grab);
SDL_bool hideMouse(SDL_bool grab);
void check_event();

HOTKEY check_hotkey(int state, SDL_Keycode sym);

#ifdef SDL_GUI
void open_GUI(void);
void close_GUI(void);
#endif

extern SDL_Joystick *sdl_joystick;

extern SDL_Cursor *aranym_cursor;
extern SDL_Cursor *empty_cursor;

#endif /* _INPUT_H */
