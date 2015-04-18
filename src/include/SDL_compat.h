#ifndef _SDL_COMPAT_H
#define _SDL_COMPAT_H

/*
 * SDL 1.x/SDL2 compatibility macros.
 *
 * Newly written code should use the SDL2 names where they differ,
 * and define macros here to map them to SDL 1.x names if neccessary.
 */

#if defined(HAVE_STDINT_H)
#  define _HAVE_STDINT_H 1 /* SDL2 needs this */
#endif
#ifndef HAVE_LIBC
#  define HAVE_LIBC 1 /* SDL2 does not include system headers without it */
#endif

/*
 * workaround for SDL2/SDL_syswm.h:
 * make sure directfb++.h isn't included, because it is wrapped in extern "C"
 */
#define DIRECTFBPP_H

#include <SDL.h>

#include <SDL_version.h>
#include <SDL_endian.h>
#include <SDL_keyboard.h>
#include <SDL_audio.h>

#if defined(__CYGWIN__)
/*
 * HACK: cygwin/mingw32 mix crap
 *      if not present the SDL_putenv uses cygwin implementation however
 *       the video driver reading it uses SDL_getenv which comes from the SDL
 *       build using mingw32...
 *
 * SDL2 does not have its own putenv() any longer, but still SDL_getenv()
 * The places where SDL_putenv was used before, however,
 * are handled differently now.
 */
#undef SDL_getenv
#undef SDL_putenv
#ifdef __cplusplus
extern "C" {
#endif
extern DECLSPEC char * SDLCALL SDL_getenv(const char *name);
extern DECLSPEC int SDLCALL SDL_putenv(const char *variable);
#ifdef __cplusplus
}
#endif
#else
#ifndef SDL_putenv
#define SDL_putenv(x) putenv(x)
#endif
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)

#define SDL_CreateNamedThread(fn, name, data) SDL_CreateThread(fn, name, data)

#define WINDOW_ALPHA 0xff

#define SDL_SRCCOLORKEY SDL_TRUE

#define SDLK_IS_MODIFIER(sym) \
	((sym) == SDLK_NUMLOCKCLEAR || \
	 (sym) == SDLK_CAPSLOCK || \
	 (sym) == SDLK_SCROLLLOCK || \
	 (sym) == SDLK_RSHIFT || \
	 (sym) == SDLK_LSHIFT || \
	 (sym) == SDLK_RCTRL || \
	 (sym) == SDLK_LCTRL || \
	 (sym) == SDLK_RALT || \
	 (sym) == SDLK_LALT || \
	 (sym) == SDLK_RGUI || \
	 (sym) == SDLK_LGUI || \
	 (sym) == SDLK_MODE)

#else

#define SDL_CreateNamedThread(fn, name, data) SDL_CreateThread(fn, data)
#define SDL_SetMainReady()
#define SDL_GetWindowGrab(window) (SDL_WM_GrabInput(SDL_GRAB_QUERY) == SDL_GRAB_ON ? SDL_TRUE : SDL_FALSE)
#define SDL_SetWindowGrab(window, on) SDL_WM_GrabInput((on) ? SDL_GRAB_ON : SDL_GRAB_OFF)

typedef SDLMod SDL_Keymod;
typedef SDLKey SDL_Keycode;
typedef Uint8 SDL_Scancode;
typedef SDL_keysym SDL_Keysym;
typedef SDL_audiostatus SDL_AudioStatus;

#define SDL_GetVersion(v) *(v) = *SDL_Linked_Version()

#define KMOD_LGUI KMOD_LMETA
#define KMOD_RGUI KMOD_RMETA
#define KMOD_GUI KMOD_META

#define SDLK_KP_1 SDLK_KP1
#define SDLK_KP_2 SDLK_KP2
#define SDLK_KP_3 SDLK_KP3
#define SDLK_KP_4 SDLK_KP4
#define SDLK_KP_5 SDLK_KP5
#define SDLK_KP_6 SDLK_KP6
#define SDLK_KP_7 SDLK_KP7
#define SDLK_KP_8 SDLK_KP8
#define SDLK_KP_9 SDLK_KP9
#define SDLK_KP_0 SDLK_KP0

#define SDLK_SCROLLLOCK SDLK_SCROLLOCK
#define SDLK_PRINTSCREEN SDLK_PRINT
#define SDLK_NUMLOCKCLEAR SDLK_NUMLOCK

#define SDLK_LGUI SDLK_LMETA
#define SDLK_RGUI SDLK_RMETA
#define SDLK_CANCEL SDLK_BREAK

#define SDLK_IS_MODIFIER(sym) ((sym) >= SDLK_NUMLOCK && (sym) <= SDLK_COMPOSE)

#endif

#if !defined(SDL_VIDEO_DRIVER_WINDOWS) && (defined(SDL_VIDEO_DRIVER_WINRT) || defined(SDL_VIDEO_DRIVER_WINDIB) || defined(SDL_VIDEO_DRIVER_DDRAW) || defined(SDL_VIDEO_DRIVER_GAPI))
#  define SDL_VIDEO_DRIVER_WINDOWS 1
#endif

#endif /* _SDL_COMPAT_H */
