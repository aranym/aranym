/*
 *	Input handling
 */

#ifndef _INPUT_H
#define _INPUT_H

#include <SDL.h>

void InputInit();
void hideMouse(bool hide);
bool grabMouse(bool grab);
void grabTheMouse();
void releaseTheMouse();
bool check_event();
extern void QuitEmulator();

extern SDL_Joystick *sdl_joystick;

#endif /* _INPUT_H */
