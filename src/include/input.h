
#ifndef _INPUT_H
#define _INPUT_H
void InputInit();
void hideMouse(bool hide);
bool grabMouse(bool grab);
void grabTheMouse();
void releaseTheMouse();
bool check_event();
extern void QuitEmulator();
#endif /* _INPUT_H */
