/*
 *	Input handling
 */

#ifndef _INPUT_H
#define _INPUT_H

#include <SDL.h>

void InputInit();
void InputReset();
bool grabMouse(bool grab);
void check_event();

#ifdef SDL_GUI
bool start_GUI_thread();
void kill_GUI_thread();
#endif

extern SDL_Joystick *sdl_joystick;

#endif /* _INPUT_H */
