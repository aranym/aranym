/*
 *	Input handling
 */

#ifndef _INPUT_H
#define _INPUT_H

#include <SDL.h>

#define HOTKEYS_MOD_MASK	(KMOD_SHIFT | KMOD_CTRL | KMOD_ALT | KMOD_META)

void InputInit();
void InputReset();
void InputExit();
bool grabMouse(bool grab);
void check_event();

#ifdef SDL_GUI
void open_GUI(void);
void close_GUI(void);
#endif

extern SDL_Joystick *sdl_joystick;

#endif /* _INPUT_H */
