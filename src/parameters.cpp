/*
 * parameters.cpp - parameter init/load/save code
 *
 * Copyright (c) 2001-2010 ARAnyM developer team (see AUTHORS)
 *
 * Authors:
 *  MJ		Milan Jurik
 *  Joy		Petr Stehlik
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
#include "parameters.h"
#include "tools.h"		// for safe_strncpy()
#include "host.h"
#include "host_filesys.h"
#include "cfgopts.h"
#include "natfeat/nf_base.h"
#include "rtc.h"
#include "vm_alloc.h"
#include "main.h"

#define DEBUG 0
#include "debug.h"

#include <cstdlib>

#ifdef OS_darwin
#  include <CoreFoundation/CoreFoundation.h>
#endif

#ifndef USE_JIT
# define USE_JIT 0
#endif

#ifndef USE_JIT_FPU
# define USE_JIT_FPU 0
#endif

#if defined(HW_SIGSEGV)
# define MEMORY_CHECK "sseg"
#elif defined(EXTENDED_SIGSEGV) && defined(ARAM_PAGE_CHECK)
# define MEMORY_CHECK "pagehwsp"
#elif defined(EXTENDED_SIGSEGV)
# define MEMORY_CHECK "hwsp"
#elif defined(ARAM_PAGE_CHECK)
# define MEMORY_CHECK "page"
#elif defined(NOCHECKBOUNDARY)
# define MEMORY_CHECK "none"
#else
# define MEMORY_CHECK "full"
#endif

#ifndef FULLMMU
# define FULLMMU 0
#endif

#ifndef DSP_EMULATION
# define DSP_EMULATION 0
#endif

#ifndef DSP_DISASM
# define DSP_DISASM 0
#endif

#ifndef ENABLE_OPENGL
# define ENABLE_OPENGL 0
#endif

#ifndef HOSTFS_SUPPORT
# define HOSTFS_SUPPORT 0
#endif

#ifndef USES_FPU_CORE
# define USES_FPU_CORE "<undefined>"
#endif

#ifndef PROVIDES_NATFEATS
# define PROVIDES_NATFEATS "<undefined>"
#endif

enum {
	OPT_PROBE_FIXED = 256,
	OPT_FIXEDMEM_OFFSET,
	OPT_SET_OPTION
};

static struct option const long_options[] =
{
#ifndef FixedSizeFastRAM
  {"fastram", required_argument, 0, 'F'},
#endif
  {"floppy", required_argument, 0, 'a'},
  {"resolution", required_argument, 0, 'r'},
#ifdef DEBUGGER
  {"debug", no_argument, 0, 'D'},
#endif
  {"fullscreen", no_argument, 0, 'f'},
  {"nomouse", no_argument, 0, 'N'},
  {"refresh", required_argument, 0, 'v'},
  {"monitor", required_argument, 0, 'm'},
#if HOSTFS_SUPPORT
  {"disk", required_argument, 0, 'd'},
#endif
  {"help", no_argument, 0, 'h'},
  {"version", no_argument, 0, 'V'},
  {"config", required_argument, 0, 'c'},
  {"save", no_argument, 0, 's'},
#ifdef SDL_GUI
  {"gui", no_argument, 0, 'G'},
#endif
  {"swap-ide", no_argument, 0, 'S'},
  {"emutos", no_argument, 0, 'e'},
  {"locale", required_argument, 0, 'k'},
#ifdef ENABLE_LILO
  {"lilo", no_argument, 0, 'l'},
#endif
  {"display", required_argument, 0, 'P'},
#if FIXED_ADDRESSING
  {"probe-fixed", no_argument, 0, OPT_PROBE_FIXED },
  {"fixedmem", required_argument, 0, OPT_FIXEDMEM_OFFSET },
#endif
  {"option", required_argument, 0, OPT_SET_OPTION },
  {NULL, 0, NULL, 0}
};

#define TOS_FILENAME		"ROM"
#define EMUTOS_FILENAME		"emutos-aranym.img"
#define FREEMINT_FILENAME	"mintara.prg"

#ifndef DEFAULT_SERIAL
#define DEFAULT_SERIAL "/dev/ttyS0"
#endif

char *program_name;		// set by main()

bool boot_emutos = false;
bool boot_lilo = false;
bool halt_on_reboot = false;
bool ide_swap = false;
uint32 FastRAMSize;
#if FIXED_ADDRESSING
uintptr fixed_memory_offset = FMEMORY;
#endif

static char config_file[512];

#if !defined(XIF_HOST_IP) && !defined(XIF_ATARI_IP) && !defined(XIF_NETMASK)
# define XIF_TYPE	"ptp"
# define XIF_TUNNEL	"tap0"
# define XIF_HOST_IP	"192.168.0.1"
# define XIF_ATARI_IP	"192.168.0.2"
# define XIF_NETMASK	"255.255.255.0"
# define XIF_MAC_ADDR	"00:41:45:54:48:30"		// just made up from \0AETH0
#endif

static bool saveConfigFile = false;

bx_options_t bx_options;

static bx_atadevice_options_t *diskc = &bx_options.atadevice[0][0];
static bx_atadevice_options_t *diskd = &bx_options.atadevice[0][1];

#if defined(__IRIX__)
/* IRIX doesn't have a GL library versioning system */
#define DEFAULT_OPENGL	"libGL.so"
#elif defined(__MACOSX__)
/* note: this is the Quartz version, not the X11 version */
#define DEFAULT_OPENGL	"/System/Library/Frameworks/OpenGL.framework/Libraries/libGL.dylib"
#elif defined(_WIN32) || defined(__WIN32__) || defined(__CYGWIN__)
/* note: this is the Windows version, not the cygwin version */
#define DEFAULT_OPENGL	"opengl32.dll"
#define DEFAULT_OSMESA  ""
#elif defined(__QNXNTO__)
#define DEFAULT_OPENGL	"libGL.so:libGL.so.3"
#elif defined(__OpenBSD__)
#define DEFAULT_OPENGL	"libGL.so:libGL.so.16.0:libGL.so.15.0:libGL.so.14.0"
#else
#define DEFAULT_OPENGL	"libGL.so:libGL.so.1"
#endif
#ifndef DEFAULT_OSMESA
#define DEFAULT_OSMESA	"libOSMesa.so:libOSMesa.8.so:libOSMesa.7.so:libOSMesa.6.so"
#endif

 
// configuration file 
/*************************************************************************/

static struct {
	SDL_Keycode current;
	int sdl1;
	const char *name;
} const sdl_keysyms[] = {
	{ SDLK_BACKSPACE, 8, "Backspace" },
	{ SDLK_TAB, 9, "Tab" },
	{ SDLK_CLEAR, 12, "Clear" },
	{ SDLK_RETURN, 13, "Return" },
	{ SDLK_PAUSE, 19, "Pause" },
	{ SDLK_ESCAPE, 27, "Escape" },
	{ SDLK_SPACE, 32, "Space" },

	{ SDLK_EXCLAIM, 33, "!" },
	{ SDLK_QUOTEDBL, 34, "\"" },
	{ SDLK_HASH, 35, "#" },
	{ SDLK_DOLLAR, 36, "$" },
	{ SDLK_AMPERSAND, 38, "&" },
	{ SDLK_QUOTE, 39, "'" },
	{ SDLK_LEFTPAREN, 40, "(" },
	{ SDLK_RIGHTPAREN, 41, ")" },
	{ SDLK_ASTERISK, 42, "*" },
	{ SDLK_PLUS, 43, "+" },
	{ SDLK_COMMA, 44, "," },
	{ SDLK_MINUS, 45, "-" },
	{ SDLK_PERIOD, 46, "." },
	{ SDLK_SLASH, 47, "/" },
	{ SDLK_0, 48, "0" },
	{ SDLK_1, 49, "1" },
	{ SDLK_2, 50, "2" },
	{ SDLK_3, 51, "3" },
	{ SDLK_4, 52, "4" },
	{ SDLK_5, 53, "5" },
	{ SDLK_6, 54, "6" },
	{ SDLK_7, 55, "7" },
	{ SDLK_8, 56, "8" },
	{ SDLK_9, 57, "9" },
	{ SDLK_COLON, 58, ":" },
	{ SDLK_SEMICOLON, 59, ";" },
	{ SDLK_LESS, 60, "<" },
	{ SDLK_EQUALS, 61, "=" },
	{ SDLK_GREATER, 62, ">" },
	{ SDLK_QUESTION, 63, "?" },
	{ SDLK_AT, 64, "@" },
	{ SDLK_LEFTBRACKET, 91, "[" },
	{ SDLK_BACKSLASH, 92, "\\" },
	{ SDLK_RIGHTBRACKET, 93, "]" },
	{ SDLK_CARET, 94, "^" },
	{ SDLK_UNDERSCORE, 95, "_" },
	{ SDLK_BACKQUOTE, 96, "`" },
	{ SDLK_a, 97, "a" },
	{ SDLK_b, 98, "b" },
	{ SDLK_c, 99, "c" },
	{ SDLK_d, 100, "d" },
	{ SDLK_e, 101, "e" },
	{ SDLK_f, 102, "f" },
	{ SDLK_g, 103, "g" },
	{ SDLK_h, 104, "h" },
	{ SDLK_i, 105, "i" },
	{ SDLK_j, 106, "j" },
	{ SDLK_k, 107, "k" },
	{ SDLK_l, 108, "l" },
	{ SDLK_m, 109, "m" },
	{ SDLK_n, 110, "n" },
	{ SDLK_o, 111, "o" },
	{ SDLK_p, 112, "p" },
	{ SDLK_q, 113, "q" },
	{ SDLK_r, 114, "r" },
	{ SDLK_s, 115, "s" },
	{ SDLK_t, 116, "t" },
	{ SDLK_u, 117, "u" },
	{ SDLK_v, 118, "v" },
	{ SDLK_w, 119, "w" },
	{ SDLK_x, 120, "x" },
	{ SDLK_y, 121, "y" },
	{ SDLK_z, 122, "z" },
	{ SDLK_DELETE, 127, "Delete" },
/*
	{ SDLK_WORLD_0, 160 },
	{ SDLK_WORLD_1, 161 },
	{ SDLK_WORLD_2, 162 },
	{ SDLK_WORLD_3, 163 },
	{ SDLK_WORLD_4, 164 },
	{ SDLK_WORLD_5, 165 },
	{ SDLK_WORLD_6, 166 },
	{ SDLK_WORLD_7, 167 },
	{ SDLK_WORLD_8, 168 },
	{ SDLK_WORLD_9, 169 },
	{ SDLK_WORLD_10, 170 },
	{ SDLK_WORLD_11, 171 },
	{ SDLK_WORLD_12, 172 },
	{ SDLK_WORLD_13, 173 },
	{ SDLK_WORLD_14, 174 },
	{ SDLK_WORLD_15, 175 },
	{ SDLK_WORLD_16, 176 },
	{ SDLK_WORLD_17, 177 },
	{ SDLK_WORLD_18, 178 },
	{ SDLK_WORLD_19, 179 },
	{ SDLK_WORLD_20, 180 },
	{ SDLK_WORLD_21, 181 },
	{ SDLK_WORLD_22, 182 },
	{ SDLK_WORLD_23, 183 },
	{ SDLK_WORLD_24, 184 },
	{ SDLK_WORLD_25, 185 },
	{ SDLK_WORLD_26, 186 },
	{ SDLK_WORLD_27, 187 },
	{ SDLK_WORLD_28, 188 },
	{ SDLK_WORLD_29, 189 },
	{ SDLK_WORLD_30, 190 },
	{ SDLK_WORLD_31, 191 },
	{ SDLK_WORLD_32, 192 },
	{ SDLK_WORLD_33, 193 },
	{ SDLK_WORLD_34, 194 },
	{ SDLK_WORLD_35, 195 },
	{ SDLK_WORLD_36, 196 },
	{ SDLK_WORLD_37, 197 },
	{ SDLK_WORLD_38, 198 },
	{ SDLK_WORLD_39, 199 },
	{ SDLK_WORLD_40, 200 },
	{ SDLK_WORLD_41, 201 },
	{ SDLK_WORLD_42, 202 },
	{ SDLK_WORLD_43, 203 },
	{ SDLK_WORLD_44, 204 },
	{ SDLK_WORLD_45, 205 },
	{ SDLK_WORLD_46, 206 },
	{ SDLK_WORLD_47, 207 },
	{ SDLK_WORLD_48, 208 },
	{ SDLK_WORLD_49, 209 },
	{ SDLK_WORLD_50, 210 },
	{ SDLK_WORLD_51, 211 },
	{ SDLK_WORLD_52, 212 },
	{ SDLK_WORLD_53, 213 },
	{ SDLK_WORLD_54, 214 },
	{ SDLK_WORLD_55, 215 },
	{ SDLK_WORLD_56, 216 },
	{ SDLK_WORLD_57, 217 },
	{ SDLK_WORLD_58, 218 },
	{ SDLK_WORLD_59, 219 },
	{ SDLK_WORLD_60, 220 },
	{ SDLK_WORLD_61, 221 },
	{ SDLK_WORLD_62, 222 },
	{ SDLK_WORLD_63, 223 },
	{ SDLK_WORLD_64, 224 },
	{ SDLK_WORLD_65, 225 },
	{ SDLK_WORLD_66, 226 },
	{ SDLK_WORLD_67, 227 },
	{ SDLK_WORLD_68, 228 },
	{ SDLK_WORLD_69, 229 },
	{ SDLK_WORLD_70, 230 },
	{ SDLK_WORLD_71, 231 },
	{ SDLK_WORLD_72, 232 },
	{ SDLK_WORLD_73, 233 },
	{ SDLK_WORLD_74, 234 },
	{ SDLK_WORLD_75, 235 },
	{ SDLK_WORLD_76, 236 },
	{ SDLK_WORLD_77, 237 },
	{ SDLK_WORLD_78, 238 },
	{ SDLK_WORLD_79, 239 },
	{ SDLK_WORLD_80, 240 },
	{ SDLK_WORLD_81, 241 },
	{ SDLK_WORLD_82, 242 },
	{ SDLK_WORLD_83, 243 },
	{ SDLK_WORLD_84, 244 },
	{ SDLK_WORLD_85, 245 },
	{ SDLK_WORLD_86, 246 },
	{ SDLK_WORLD_87, 247 },
	{ SDLK_WORLD_88, 248 },
	{ SDLK_WORLD_89, 249 },
	{ SDLK_WORLD_90, 250 },
	{ SDLK_WORLD_91, 251 },
	{ SDLK_WORLD_92, 252 },
	{ SDLK_WORLD_93, 253 },
	{ SDLK_WORLD_94, 254 },
	{ SDLK_WORLD_95, 255 },
*/
	{ SDLK_KP_0, 256, "Keypad 0" },
	{ SDLK_KP_1, 257, "Keypad 1" },
	{ SDLK_KP_2, 258, "Keypad 2" },
	{ SDLK_KP_3, 259, "Keypad 3" },
	{ SDLK_KP_4, 260, "Keypad 4" },
	{ SDLK_KP_5, 261, "Keypad 5" },
	{ SDLK_KP_6, 262, "Keypad 6" },
	{ SDLK_KP_7, 263, "Keypad 7" },
	{ SDLK_KP_8, 264, "Keypad 8" },
	{ SDLK_KP_9, 265, "Keypad 9" },
	{ SDLK_KP_PERIOD, 266, "Keypad ." },
	{ SDLK_KP_DIVIDE, 267, "Keypad /" },
	{ SDLK_KP_MULTIPLY, 268, "Keypad *" },
	{ SDLK_KP_MINUS, 269, "Keypad -" },
	{ SDLK_KP_PLUS, 270, "Keypad +" },
	{ SDLK_KP_ENTER, 271, "Keypad Enter" },
	{ SDLK_KP_EQUALS, 272, "Keypad =" },
	{ SDLK_UP, 273, "Up" },
	{ SDLK_DOWN, 274, "Down" },
	{ SDLK_RIGHT, 275, "Right" },
	{ SDLK_LEFT, 276, "Left" },
	{ SDLK_INSERT, 277, "Insert" },
	{ SDLK_HOME, 278, "Home" },
	{ SDLK_END, 279, "End" },
	{ SDLK_PAGEUP, 280, "PageUp" },
	{ SDLK_PAGEDOWN, 281, "PageDown" },
	{ SDLK_F1, 282, "F1" },
	{ SDLK_F2, 283, "F2" },
	{ SDLK_F3, 284, "F3" },
	{ SDLK_F4, 285, "F4" },
	{ SDLK_F5, 286, "F5" },
	{ SDLK_F6, 287, "F6" },
	{ SDLK_F7, 288, "F7" },
	{ SDLK_F8, 289, "F8" },
	{ SDLK_F9, 290, "F9" },
	{ SDLK_F10, 291, "F10" },
	{ SDLK_F11, 292, "F11" },
	{ SDLK_F12, 293, "F12" },
	{ SDLK_F13, 294, "F13" },
	{ SDLK_F14, 295, "F14" },
	{ SDLK_F15, 296, "F15" },
	{ SDLK_NUMLOCKCLEAR, 300, "Numlock" },
	{ SDLK_CAPSLOCK, 301, "CapsLock" },
	{ SDLK_SCROLLLOCK, 302, "ScrollLock" },
	{ SDLK_RSHIFT, 303, "Right Shift" },
	{ SDLK_LSHIFT, 304, "Left Shift" },
	{ SDLK_RCTRL, 305, "Right Ctrl" },
	{ SDLK_LCTRL, 306, "Left Ctrl" },
	{ SDLK_RALT, 307, "Right Alt" },
	{ SDLK_LALT, 308, "Left Alt" },
	{ SDLK_RGUI, 309, "Right GUI" },
	{ SDLK_LGUI, 310, "Left GUI" },
//	{ SDLK_LSUPER, 311 },
//	{ SDLK_RSUPER, 312 },
	{ SDLK_MODE, 313, "ModeSwitch" },
//	{ SDLK_COMPOSE, 314 },
	{ SDLK_HELP, 315, "Help" },
	{ SDLK_PRINTSCREEN, 316, "PrintScreen" },
	{ SDLK_SYSREQ, 317, "SysReq" },
	{ SDLK_CANCEL, 318, "Cancel" },
	{ SDLK_MENU, 319, "Menu" },
	{ SDLK_POWER, 320, "Power" },
//	{ SDLK_EURO, 321 },
	{ SDLK_UNDO, 322, "Undo" },

#if SDL_VERSION_ATLEAST(2, 0, 0)
	{ SDLK_APPLICATION, 0, "Application" },
	{ SDLK_F16, 0, "F16" },
	{ SDLK_F17, 0, "F17" },
	{ SDLK_F18, 0, "F18" },
	{ SDLK_F19, 0, "F19" },
	{ SDLK_F20, 0, "F20" },
	{ SDLK_F21, 0, "F21" },
	{ SDLK_F22, 0, "F22" },
	{ SDLK_F23, 0, "F23" },
	{ SDLK_F24, 0, "F24" },
	{ SDLK_EXECUTE, 0, "Execute" },
	{ SDLK_SELECT, 0, "Select" },
	{ SDLK_STOP, 0, "Stop" },
	{ SDLK_AGAIN, 0, "Again" },
	{ SDLK_CUT, 0, "Cut" },
	{ SDLK_COPY, 0, "Copy" },
	{ SDLK_PASTE, 0, "Paste" },
	{ SDLK_FIND, 0, "Find" },
	{ SDLK_MUTE, 0, "Mute" },
	{ SDLK_VOLUMEUP, 0, "VolumeUp" },
	{ SDLK_VOLUMEDOWN, 0, "VolumeDown" },
	{ SDLK_KP_COMMA, 0, "Keypad ," },
	{ SDLK_PRIOR, 0, "Prior" },
	{ SDLK_RETURN2, 0, "Return" },
	{ SDLK_SEPARATOR, 0, "Separator" },
	{ SDLK_OUT, 0, "Out" },
	{ SDLK_OPER, 0, "Oper" },
	{ SDLK_CLEARAGAIN, 0, "Clear / Again" },
	{ SDLK_CRSEL, 0, "CrSel" },
	{ SDLK_EXSEL, 0, "ExSel" },
	{ SDLK_KP_00, 0, "Keypad 00" },
	{ SDLK_KP_000, 0, "Keypad 000" },
	{ SDLK_THOUSANDSSEPARATOR, 0, "ThousandsSeparator" },
	{ SDLK_DECIMALSEPARATOR, 0, "DecimalSeparator" },
	{ SDLK_CURRENCYUNIT, 0, "CurrencyUnit" },
	{ SDLK_CURRENCYSUBUNIT, 0, "CurrencySubUnit" },
#endif
};

static const char *sdl_getkeyname(SDL_Keycode sym)
{
	for (unsigned int i = 0; i < sizeof(sdl_keysyms) / sizeof(sdl_keysyms[0]); i++)
		if (sdl_keysyms[i].current == sym)
			return sdl_keysyms[i].name;
	return SDL_GetKeyName(sym);
}

char *keysymToString(char *buffer, const bx_hotkey *keysym)
{
	*buffer = 0;
	SDL_Keymod mods = keysym->mod;
	if (mods & KMOD_LSHIFT) strcat(buffer, "LS+");
	if (mods & KMOD_RSHIFT) strcat(buffer, "RS+");
	if (mods & KMOD_LCTRL) strcat(buffer, "LC+");
	if (mods & KMOD_RCTRL) strcat(buffer, "RC+");
	if (mods & KMOD_LALT) strcat(buffer, "LA+");
	if (mods & KMOD_RALT) strcat(buffer, "RA+");
	if (mods & KMOD_LGUI) strcat(buffer, "LM+");
	if (mods & KMOD_RGUI) strcat(buffer, "RM+");
	if (mods & KMOD_MODE) strcat(buffer, "MO+");
	if (keysym->sym) {
		strcat(buffer, sdl_getkeyname(keysym->sym));
	} else {
		// mod keys only, remove last plus sign
		int len = strlen(buffer);
		if (len > 0 && buffer[len-1] == '+')
			buffer[len-1] = '\0';
	}
	return buffer;
}

bool stringToKeysym(bx_hotkey *keysym, const char *string)
{
	int sym, mod;
	if ( sscanf(string, "%i:%i", &sym, &mod) == 2)
	{
		/*
		 * old format with direct encoding; keysyms are from SDL < 2.0
		 * We must translate the SDL 1.2.x values
		 */
		for (unsigned int i = 0; i < sizeof(sdl_keysyms) / sizeof(sdl_keysyms[0]); i++)
		{
			if (sym == sdl_keysyms[i].sdl1)
			{
				keysym->mod = SDL_Keymod(mod);
				keysym->sym = sdl_keysyms[i].current;
				return true;
			}
		}
		if (mod != 0 && sym == 0)
		{
			/* modifiers only */
			keysym->mod = SDL_Keymod(mod);
			keysym->sym = SDLK_UNKNOWN;
			return true;
		}
		return false;
	}
	mod = 0;
#define MOD(s, k) \
	if (strncmp(string, s, sizeof(s) - 1) == 0 && \
		(string[sizeof(s) - 1] == '+' || string[sizeof(s) - 1] == '\0')) \
		{ mod |= k; string += sizeof(s) - 1; if (*string == '+') string++; }
	MOD("LS", KMOD_LSHIFT);
	MOD("RS", KMOD_RSHIFT);
	MOD("LC", KMOD_LCTRL);
	MOD("RC", KMOD_RCTRL);
	MOD("LA", KMOD_LALT);
	MOD("RA", KMOD_RALT);
	MOD("LM", KMOD_LGUI);
	MOD("RM", KMOD_RGUI);
	MOD("MO", KMOD_MODE);
#undef MOD
	for (unsigned int i = 0; i < sizeof(sdl_keysyms) / sizeof(sdl_keysyms[0]); i++)
	{
		if (strcmp(sdl_keysyms[i].name, string) == 0)
		{
			keysym->mod = SDL_Keymod(mod);
			keysym->sym = sdl_keysyms[i].current;
			return true;
		}
	}
	if (mod != 0)
	{
		/* modifiers only */
		keysym->mod = SDL_Keymod(mod);
		keysym->sym = SDLK_UNKNOWN;
		return true;
	}
	return false;
}

/*
 * split a pathlist into an array of strings.
 * returns a single block of malloced memory
 */
char **split_pathlist(const char *pathlist)
{
	size_t size;
	const char *p;
	size_t cnt;
	char *dst;
	char **result;
	
	if (pathlist == NULL)
		return NULL;
	
	/* count words */
	cnt = 2;
	p = pathlist;
	while (*p != '\0')
	{
		if (*p == ';' || *p == ':')
			cnt++;
		p++;
	}
	
	size = (p - pathlist) + cnt * (sizeof(char *) + 2);
	result = (char **)malloc(size);
	if (result == NULL)
		return NULL;
	
	p = pathlist;
	dst = (char *)(result + cnt);
	cnt = 0;
	p = pathlist;
	while (*p != '\0')
	{
		result[cnt] = dst;
		while (*p != '\0')
		{
			if (*p == ';' || *p == ':')
			{
				size_t len = dst - result[cnt];
				if (*p == ':' && len == 1 && isalnum(p[-1]))
				{
					*dst++ = *p++;
					continue;
				}
				break;
			}
			*dst++ = *p++;
		}
		*dst++ = '\0';
		cnt++;
		if (*p != '\0')
			p++;
	}
	result[cnt] = NULL;
	return result;
}


/*************************************************************************/
struct Config_Tag global_conf[]={
	{ "FastRAM", Int_Tag, &bx_options.fastram, 0, 0},
	{ "FixedMemoryOffset", HexLong_Tag, &bx_options.fixed_memory_offset, 0, 0}, // FIXME: implement Ptr_Tag
	{ "Floppy", Path_Tag, bx_options.floppy.path, sizeof(bx_options.floppy.path), 0},
	{ "TOS", Path_Tag, bx_options.tos.tos_path, sizeof(bx_options.tos.tos_path), 0},
	{ "EmuTOS", Path_Tag, bx_options.tos.emutos_path, sizeof(bx_options.tos.emutos_path), 0},
	{ "Bootstrap", Path_Tag, bx_options.bootstrap_path, sizeof(bx_options.bootstrap_path), 0},
	{ "BootstrapArgs", String_Tag, bx_options.bootstrap_args, sizeof(bx_options.bootstrap_args), 0},
	{ "BootDrive", Char_Tag, &bx_options.bootdrive, 0, 0},
	{ "GMTime", Bool_Tag, &bx_options.gmtime, 0, 0},
	{ "EpsEnabled", Bool_Tag, &bx_options.cpu.eps_enabled, 0, 0},
	{ "EpsMax", Int_Tag, &bx_options.cpu.eps_max, 0, 0},
	{ "SnapshotDir", Path_Tag, bx_options.snapshot_dir, sizeof(bx_options.snapshot_dir), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_global()
{
	strcpy(bx_options.tos.tos_path, TOS_FILENAME);
	strcpy(bx_options.tos.emutos_path, EMUTOS_FILENAME);
	strcpy(bx_options.bootstrap_path, FREEMINT_FILENAME);
	bx_options.gmtime = false;	// use localtime by default
	strcpy(bx_options.floppy.path, "");
#ifdef FixedSizeFastRAM
	FastRAMSize = FixedSizeFastRAM * 1024 * 1024;
#else
	FastRAMSize = 0;
#endif
#if FIXED_ADDRESSING
	bx_options.fixed_memory_offset = fixed_memory_offset;
#endif
	bx_options.cpu.eps_enabled = false;
	bx_options.cpu.eps_max = 20;
	strcpy(bx_options.snapshot_dir, "snapshots");
}

static void postload_global()
{
#ifndef FixedSizeFastRAM
	FastRAMSize = bx_options.fastram * 1024 * 1024;
#endif
#if FIXED_ADDRESSING
	fixed_memory_offset = bx_options.fixed_memory_offset;
	if (fixed_memory_offset == 0)
		fixed_memory_offset = FMEMORY;
#endif
	if (!isalpha(bx_options.bootdrive))
		bx_options.bootdrive = 0;
}

static void presave_global()
{
	bx_options.fastram = FastRAMSize / 1024 / 1024;
#if FIXED_ADDRESSING
	bx_options.fixed_memory_offset = fixed_memory_offset;
#endif
	if (bx_options.bootdrive == 0)
		bx_options.bootdrive = ' ';
}

/*************************************************************************/
struct Config_Tag startup_conf[]={
	{ "GrabMouse", Bool_Tag, &bx_options.startup.grabMouseAllowed, 0, 0},
	{ "Debugger", Bool_Tag, &bx_options.startup.debugger, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_startup()
{
  bx_options.startup.debugger = false;
  bx_options.startup.grabMouseAllowed = true;
}

static void postload_startup()
{
}

static void presave_startup()
{
}

/*************************************************************************/
struct Config_Tag jit_conf[]={
	{ "JIT", Bool_Tag, &bx_options.jit.jit, 0, 0},
	{ "JITFPU", Bool_Tag, &bx_options.jit.jitfpu, 0, 0},
	{ "JITCacheSize", Int_Tag, &bx_options.jit.jitcachesize, 0, 0},
	{ "JITLazyFlush", Int_Tag, &bx_options.jit.jitlazyflush, 0, 0},
	{ "JITBlackList", String_Tag, &bx_options.jit.jitblacklist, sizeof(bx_options.jit.jitblacklist), 0},
	{ "JITInline", Bool_Tag, &bx_options.jit.jitinline, 0, 0},
	{ "JITDebug", Bool_Tag, &bx_options.jit.jitdebug, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_jit()
{
	bx_options.jit.jit = true;
	bx_options.jit.jitfpu = true;
	bx_options.jit.jitcachesize = 8192;
	bx_options.jit.jitlazyflush = 1;
	strcpy(bx_options.jit.jitblacklist, "");
}

static void postload_jit()
{
}

static void presave_jit()
{
}

/*************************************************************************/
struct Config_Tag tos_conf[]={
	{ "Cookie_MCH", HexLong_Tag, &bx_options.tos.cookie_mch, 0, 0},
	{ "RedirConsole", Bool_Tag, &bx_options.tos.redirect_CON, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_tos()
{
  bx_options.tos.redirect_CON = false;
  bx_options.tos.cookie_mch = 0x00050000; // ARAnyM
  bx_options.tos.cookie_akp = -1;
}

static void postload_tos()
{
}

static void presave_tos()
{
}

/*************************************************************************/
struct Config_Tag video_conf[]={
	{ "FullScreen", Bool_Tag, &bx_options.video.fullscreen, 0, 0},
	{ "BootColorDepth", Byte_Tag, &bx_options.video.boot_color_depth, 0, 0},
	{ "VidelRefresh", Byte_Tag, &bx_options.video.refresh, 0, 0},
	{ "VidelMonitor", Byte_Tag, &bx_options.video.monitor, 0, 0},
	{ "SingleBlitComposing", Bool_Tag, &bx_options.video.single_blit_composing, 0, 0},
	{ "SingleBlitRefresh", Bool_Tag, &bx_options.video.single_blit_refresh, 0, 0},
	{ "WindowPos", String_Tag, bx_options.video.window_pos, sizeof(bx_options.video.window_pos), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_video()
{
  bx_options.video.fullscreen = false;		// Boot in Fullscreen
  bx_options.video.boot_color_depth = -1;	// Boot in color depth
  bx_options.video.monitor = -1;			// preserve default NVRAM monitor
  bx_options.video.refresh = 2;				// 25 Hz update
  strcpy(bx_options.video.window_pos, "");	// ARAnyM window position on screen
  bx_options.video.single_blit_composing = false;	// Use chunky screen composing
  bx_options.video.single_blit_refresh = false;	// Use chunky screen refreshing
}

static void postload_video()
{
	if (bx_options.video.refresh < 1 || bx_options.video.refresh > 200)
		bx_options.video.refresh = 2;	// default if input parameter is insane
}

static void presave_video()
{
}

/*************************************************************************/
struct Config_Tag opengl_conf[]={
	{ "Enabled", Bool_Tag, &bx_options.opengl.enabled, 0, 0},
	{ "Filtered", Bool_Tag, &bx_options.opengl.filtered, 0, 0},
	{ "Library", String_Tag, bx_options.opengl.library, sizeof(bx_options.opengl.library), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_opengl()
{
  bx_options.opengl.enabled = false;
  bx_options.opengl.filtered = false;
  strcpy(bx_options.opengl.library, DEFAULT_OPENGL);
}

static void postload_opengl()
{
#ifndef ENABLE_OPENGL
  bx_options.opengl.enabled = false;
#endif
}

static void presave_opengl()
{
}

/*************************************************************************/
#define BX_DISK_CONFIG(Disk)	struct Config_Tag Disk ## _configs[] = {	\
	{ "Present", Bool_Tag, &Disk->present, 0, 0},	\
	{ "IsCDROM", Bool_Tag, &Disk->isCDROM, 0, 0},	\
	{ "ByteSwap", Bool_Tag, &Disk->byteswap, 0, 0},	\
	{ "ReadOnly", Bool_Tag, &Disk->readonly, 0, 0},	\
	{ "Path", Path_Tag, Disk->path, sizeof(Disk->path), 0},	\
	{ "Cylinders", Int_Tag, &Disk->cylinders, 0, 0},	\
	{ "Heads", Int_Tag, &Disk->heads, 0, 0},	\
	{ "SectorsPerTrack", Int_Tag, &Disk->spt, 0, 0},	\
	{ "ModelName", String_Tag, Disk->model, sizeof(Disk->model), 0},	\
	{ NULL , Error_Tag, NULL, 0, 0 }	\
}

BX_DISK_CONFIG(diskc);
BX_DISK_CONFIG(diskd);

static void set_ide(unsigned int number, const char *dev_path, int cylinders, int heads, int spt, int byteswap, bool readonly, const char *model_name)
{
  // Autodetect ???
  if (cylinders == -1) {
    if ((cylinders = get_geometry(dev_path, geoCylinders)) == -1) {
      panicbug("Disk %s has unknown geometry.", dev_path);
      exit(EXIT_FAILURE);
    }
  }

  if (heads == -1) {
    if ((heads = get_geometry(dev_path, geoHeads)) == -1) {
      panicbug("Disk %s has unknown geometry.", dev_path);
      exit(EXIT_FAILURE);
    }
  }

  if (spt == -1) {
    if ((spt = get_geometry(dev_path, geoSpt)) == -1) {
      panicbug("Disk %s has unknown geometry.", dev_path);
      exit(EXIT_FAILURE);
    }
  }

  if (byteswap == -1) {
    if ((byteswap = get_geometry(dev_path, geoByteswap)) == -1) {
      panicbug("Disk %s has unknown geometry.", dev_path);
      exit(EXIT_FAILURE);
    }
  }

	bx_atadevice_options_t *disk;
	switch(number) {
		case 0: disk = diskc; break;
		case 1: disk = diskd; break;
		default: disk = NULL; break;
	}
  if (disk != NULL) {
    disk->present = strlen(dev_path) > 0;
    disk->isCDROM = false;
    disk->byteswap = byteswap;
    disk->readonly = readonly;
    disk->cylinders = cylinders;
    disk->heads = heads;
    disk->spt = spt;
    strcpy(disk->path, dev_path);
	safe_strncpy(disk->model, model_name, sizeof(disk->model));
  }

  if (cylinders && heads && spt) {
    D(bug("IDE%d CHS geometry: %d/%d/%d %d", number, cylinders, heads, spt, byteswap));
  }

}

static void preset_ide()
{
  set_ide(0, "", 0, 0, 0, false, false, "Master");
  set_ide(1, "", 0, 0, 0, false, false, "Slave");

  bx_options.newHardDriveSupport = true;
}

static void postload_ide()
{
}

static void presave_ide()
{
}

/*************************************************************************/
#define DISK_CONFIG(Disk)	struct Config_Tag Disk ## _configs[] = {	\
	{ "Path", Path_Tag, Disk->path, sizeof(Disk->path), 0},	\
	{ "Present", Bool_Tag, &Disk->present, 0, 0},	\
	{ "PartID", String_Tag, Disk->partID, sizeof(Disk->partID), 0},	\
	{ "ByteSwap", Bool_Tag, &Disk->byteswap, 0, 0},	\
	{ "ReadOnly", Bool_Tag, &Disk->readonly, 0, 0},	\
	{ NULL , Error_Tag, NULL, 0, 0 }	\
}

static bx_scsidevice_options_t *disk0 = &bx_options.disks[0];
static bx_scsidevice_options_t *disk1 = &bx_options.disks[1];
static bx_scsidevice_options_t *disk2 = &bx_options.disks[2];
static bx_scsidevice_options_t *disk3 = &bx_options.disks[3];
static bx_scsidevice_options_t *disk4 = &bx_options.disks[4];
static bx_scsidevice_options_t *disk5 = &bx_options.disks[5];
static bx_scsidevice_options_t *disk6 = &bx_options.disks[6];
static bx_scsidevice_options_t *disk7 = &bx_options.disks[7];

DISK_CONFIG(disk0);
DISK_CONFIG(disk1);
DISK_CONFIG(disk2);
DISK_CONFIG(disk3);
DISK_CONFIG(disk4);
DISK_CONFIG(disk5);
DISK_CONFIG(disk6);
DISK_CONFIG(disk7);

static void preset_disk()
{
	for(int i=0; i<DISKS; i++) {
		*bx_options.disks[i].path = '\0';
		bx_options.disks[i].present = false;
		strcpy(bx_options.disks[i].partID, "BGM");
		bx_options.disks[i].byteswap = false;
		bx_options.disks[i].readonly = false;
	}
}

static void postload_disk()
{
}

static void presave_disk()
{
}

/*************************************************************************/
#define HOSTFS_ENTRY(c,n) \
	{ c, Path_Tag, &bx_options.aranymfs.drive[n].configPath, sizeof(bx_options.aranymfs.drive[n].configPath), 0}

struct Config_Tag arafs_conf[]={
	{ "symlinks", String_Tag, &bx_options.aranymfs.symlinks, sizeof(bx_options.aranymfs.symlinks), 0},
	HOSTFS_ENTRY("A", 0),
	HOSTFS_ENTRY("B", 1),
	HOSTFS_ENTRY("C", 2),
	HOSTFS_ENTRY("D", 3),
	HOSTFS_ENTRY("E", 4),
	HOSTFS_ENTRY("F", 5),
	HOSTFS_ENTRY("G", 6),
	HOSTFS_ENTRY("H", 7),
	HOSTFS_ENTRY("I", 8),
	HOSTFS_ENTRY("J", 9),
	HOSTFS_ENTRY("K", 10),
	HOSTFS_ENTRY("L", 11),
	HOSTFS_ENTRY("M", 12),
	HOSTFS_ENTRY("N", 13),
	HOSTFS_ENTRY("O", 14),
	HOSTFS_ENTRY("P", 15),
	HOSTFS_ENTRY("Q", 16),
	HOSTFS_ENTRY("R", 17),
	HOSTFS_ENTRY("S", 18),
	HOSTFS_ENTRY("T", 19),
	HOSTFS_ENTRY("U", 20),
	HOSTFS_ENTRY("V", 21),
	HOSTFS_ENTRY("W", 22),
	HOSTFS_ENTRY("X", 23),
	HOSTFS_ENTRY("Y", 24),
	HOSTFS_ENTRY("Z", 25),
	HOSTFS_ENTRY("0", 26),
	HOSTFS_ENTRY("1", 27),
	HOSTFS_ENTRY("2", 28),
	HOSTFS_ENTRY("3", 29),
	HOSTFS_ENTRY("4", 30),
	HOSTFS_ENTRY("5", 31),
	{ NULL , Error_Tag, NULL, 0, 0}
};

static void preset_arafs()
{
	strcpy(bx_options.aranymfs.symlinks, "posix");
	for(int i=0; i < HOSTFS_MAX_DRIVES; i++) {
		bx_options.aranymfs.drive[i].rootPath[0] = '\0';
		bx_options.aranymfs.drive[i].configPath[0] = '\0';
		bx_options.aranymfs.drive[i].halfSensitive = true;
	}
}

static void postload_arafs()
{
	for(int i=0; i < HOSTFS_MAX_DRIVES; i++) {
		safe_strncpy(bx_options.aranymfs.drive[i].rootPath, bx_options.aranymfs.drive[i].configPath, sizeof(bx_options.aranymfs.drive[i].rootPath));
		int len = strlen(bx_options.aranymfs.drive[i].configPath);
		bx_options.aranymfs.drive[i].halfSensitive = true;
		if (len > 0) {
			char *ptrLast = bx_options.aranymfs.drive[i].rootPath + len-1;
			if (*ptrLast == ':') {
				*ptrLast = '\0';
				bx_options.aranymfs.drive[i].halfSensitive = false;
			}
#if defined(__CYGWIN__) || defined(_WIN32)
			// interpet "C:" here as the root directory of C:, not the current directory
			if (DriveFromLetter(bx_options.aranymfs.drive[i].rootPath[0]) >= 0 &&
				bx_options.aranymfs.drive[i].rootPath[1] == ':' &&
				bx_options.aranymfs.drive[i].rootPath[2] == '\0')
				strcat(bx_options.aranymfs.drive[i].rootPath, DIRSEPARATOR);
#endif
#if defined(__CYGWIN__)
			cygwin_path_to_win32(bx_options.aranymfs.drive[i].rootPath, sizeof(bx_options.aranymfs.drive[i].rootPath));
#endif
			strd2upath(bx_options.aranymfs.drive[i].rootPath, bx_options.aranymfs.drive[i].rootPath);
			len = strlen(bx_options.aranymfs.drive[i].rootPath);
			ptrLast = bx_options.aranymfs.drive[i].rootPath + len-1;
			if (*ptrLast != *DIRSEPARATOR)
				strcat(bx_options.aranymfs.drive[i].rootPath, DIRSEPARATOR);
		}
	}
}

static void presave_arafs()
{
	for(int i=0; i < HOSTFS_MAX_DRIVES; i++) {
		safe_strncpy(bx_options.aranymfs.drive[i].configPath, bx_options.aranymfs.drive[i].rootPath, sizeof(bx_options.aranymfs.drive[i].configPath));
		if ( strlen(bx_options.aranymfs.drive[i].rootPath) > 0 &&
			 !bx_options.aranymfs.drive[i].halfSensitive ) {
			// set the halfSensitive indicator
			strcat( bx_options.aranymfs.drive[i].configPath, ":" );
		}
	}
}

/*************************************************************************/
// struct Config_Tag ethernet_conf[]={
#define ETH_CONFIG(Eth)	struct Config_Tag Eth ## _conf[] = {	\
	{ "Type", String_Tag, &Eth->type, sizeof(Eth->type), 0}, \
	{ "Tunnel", String_Tag, &Eth->tunnel, sizeof(Eth->tunnel), 0}, \
	{ "HostIP", String_Tag, &Eth->ip_host, sizeof(Eth->ip_host), 0}, \
	{ "AtariIP", String_Tag, &Eth->ip_atari, sizeof(Eth->ip_atari), 0}, \
	{ "Netmask", String_Tag, &Eth->netmask, sizeof(Eth->netmask), 0}, \
	{ "MAC", String_Tag, &Eth->mac_addr, sizeof(Eth->mac_addr), 0}, \
	{ NULL , Error_Tag, NULL, 0, 0 } \
}

static bx_ethernet_options_t *eth0 = &bx_options.ethernet[0];
static bx_ethernet_options_t *eth1 = &bx_options.ethernet[1];
static bx_ethernet_options_t *eth2 = &bx_options.ethernet[2];
static bx_ethernet_options_t *eth3 = &bx_options.ethernet[3];

ETH_CONFIG(eth0);
ETH_CONFIG(eth1);
ETH_CONFIG(eth2);
ETH_CONFIG(eth3);

#define ETH(i, x) bx_options.ethernet[i].x
static void preset_ethernet()
{
	// ETH[0] with some default values
	safe_strncpy(ETH(0, type), XIF_TYPE, sizeof(ETH(0, type)));
	safe_strncpy(ETH(0, tunnel), XIF_TUNNEL, sizeof(ETH(0, tunnel)));
	safe_strncpy(ETH(0, ip_host), XIF_HOST_IP, sizeof(ETH(0, ip_host)));
	safe_strncpy(ETH(0, ip_atari), XIF_ATARI_IP, sizeof(ETH(0, ip_atari)));
	safe_strncpy(ETH(0, netmask), XIF_NETMASK, sizeof(ETH(0, netmask)));
	safe_strncpy(ETH(0, mac_addr), XIF_MAC_ADDR, sizeof(ETH(0, mac_addr)));

	// ETH[1] - ETH[MAX_ETH] are empty by default
	for(int i=1; i<MAX_ETH; i++) {
		*ETH(i, type) = '\0';
		*ETH(i, tunnel) = '\0';
		*ETH(i, ip_host) = '\0';
		*ETH(i, ip_atari) = '\0';
		*ETH(i, netmask) = '\0';
		*ETH(i, mac_addr) = '\0';
	}
}

static void postload_ethernet()
{
}

static void presave_ethernet()
{
}

/*************************************************************************/
#define LILO(x) bx_options.lilo.x

struct Config_Tag lilo_conf[]={
	{ "Kernel", Path_Tag, &LILO(kernel), sizeof(LILO(kernel)), 0},
	{ "Args", String_Tag, &LILO(args), sizeof(LILO(args)), 0},
	{ "Ramdisk", Path_Tag, &LILO(ramdisk), sizeof(LILO(ramdisk)), 0},
	{ "LoadToFastRam", Bool_Tag, &LILO(load_to_fastram), 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_lilo()
{
  safe_strncpy(LILO(kernel), "linux.bin", sizeof(LILO(kernel)));
  safe_strncpy(LILO(args), "root=/dev/ram video=atafb:vga16", sizeof(LILO(args)));
  safe_strncpy(LILO(ramdisk), "root.bin", sizeof(LILO(ramdisk)));
  LILO(load_to_fastram) = false;
}

static void postload_lilo()
{
}

static void presave_lilo()
{
}

/*************************************************************************/
#define MIDI(x) bx_options.midi.x

struct Config_Tag midi_conf[]={
	{ "Type", String_Tag, &MIDI(type), sizeof(MIDI(type)), 0},
	{ "File", Path_Tag, &MIDI(file), sizeof(MIDI(file)), 0},
	{ "Sequencer", Path_Tag, &MIDI(sequencer), sizeof(MIDI(sequencer)), 0},
	{ "Enabled", Bool_Tag, &MIDI(enabled), 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_midi() {
  safe_strncpy(MIDI(type), "none", sizeof(MIDI(type)));
  safe_strncpy(MIDI(file), "", sizeof(MIDI(file)));
  safe_strncpy(MIDI(sequencer), "/dev/sequencer", sizeof(MIDI(sequencer)));
  MIDI(enabled) = false;
}

static void postload_midi() {
	MIDI *midi = getMIDI();
	if (midi)
		midi->enable(MIDI(enabled));
}

static void presave_midi() {
}

/*************************************************************************/
#define NFCDROM_ENTRY(c,n) \
	{ c, Int_Tag, &bx_options.nfcdroms[n].physdevtohostdev, sizeof(bx_options.nfcdroms[n].physdevtohostdev), 0}

struct Config_Tag nfcdroms_conf[]={
	NFCDROM_ENTRY("A", 0),
	NFCDROM_ENTRY("B", 1),
	NFCDROM_ENTRY("C", 2),
	NFCDROM_ENTRY("D", 3),
	NFCDROM_ENTRY("E", 4),
	NFCDROM_ENTRY("F", 5),
	NFCDROM_ENTRY("G", 6),
	NFCDROM_ENTRY("H", 7),
	NFCDROM_ENTRY("I", 8),
	NFCDROM_ENTRY("J", 9),
	NFCDROM_ENTRY("K", 10),
	NFCDROM_ENTRY("L", 11),
	NFCDROM_ENTRY("M", 12),
	NFCDROM_ENTRY("N", 13),
	NFCDROM_ENTRY("O", 14),
	NFCDROM_ENTRY("P", 15),
	NFCDROM_ENTRY("Q", 16),
	NFCDROM_ENTRY("R", 17),
	NFCDROM_ENTRY("S", 18),
	NFCDROM_ENTRY("T", 19),
	NFCDROM_ENTRY("U", 20),
	NFCDROM_ENTRY("V", 21),
	NFCDROM_ENTRY("W", 22),
	NFCDROM_ENTRY("X", 23),
	NFCDROM_ENTRY("Y", 24),
	NFCDROM_ENTRY("Z", 25),
	NFCDROM_ENTRY("0", 26),
	NFCDROM_ENTRY("1", 27),
	NFCDROM_ENTRY("2", 28),
	NFCDROM_ENTRY("3", 29),
	NFCDROM_ENTRY("4", 30),
	NFCDROM_ENTRY("5", 31),
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_nfcdroms() {
	for(int i=0; i < CD_MAX_DRIVES; i++) {
		bx_options.nfcdroms[i].physdevtohostdev = -1;
	}
}

static void postload_nfcdroms() {
}

static void presave_nfcdroms() {
}

/*************************************************************************/
struct Config_Tag autozoom_conf[]={
	{ "Enabled", Bool_Tag, &bx_options.autozoom.enabled, 0, 0},
	{ "IntegerCoefs", Bool_Tag, &bx_options.autozoom.integercoefs, 0, 0},
	{ "FixedSize", Bool_Tag, &bx_options.autozoom.fixedsize, 0, 0},
	{ "Width", Int_Tag, &bx_options.autozoom.width, 0, 0},
	{ "Height", Int_Tag, &bx_options.autozoom.height, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_autozoom() {
  bx_options.autozoom.enabled = false;
  bx_options.autozoom.integercoefs = false;
  bx_options.autozoom.fixedsize = false;
  bx_options.autozoom.width = 640;
  bx_options.autozoom.height = 480;
}

static void postload_autozoom() {
}

static void presave_autozoom() {
}

/*************************************************************************/
#define OSMESA_CONF(x) bx_options.osmesa.x

struct Config_Tag osmesa_conf[]={
	{ "ChannelSize", Int_Tag, &OSMESA_CONF(channel_size), 0, 0},
	{ "LibGL", String_Tag, &OSMESA_CONF(libgl), sizeof(OSMESA_CONF(libgl)), 0},
	{ "LibOSMesa", String_Tag, &OSMESA_CONF(libosmesa), sizeof(OSMESA_CONF(libosmesa)), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_osmesa() {
	OSMESA_CONF(channel_size) = 0;
	safe_strncpy(OSMESA_CONF(libgl), DEFAULT_OPENGL, sizeof(OSMESA_CONF(libgl)));
	safe_strncpy(OSMESA_CONF(libosmesa), DEFAULT_OSMESA, sizeof(OSMESA_CONF(libosmesa)));
}

static void postload_osmesa() {
}

static void presave_osmesa() {
}

/*************************************************************************/
#define PARALLEL_CONF(x) bx_options.parallel.x

struct Config_Tag parallel_conf[]={
	{ "Type", String_Tag, &PARALLEL_CONF(type), sizeof(PARALLEL_CONF(type)), 0},
	{ "File", String_Tag, &PARALLEL_CONF(file), sizeof(PARALLEL_CONF(file)), 0},
	{ "Parport", String_Tag, &PARALLEL_CONF(parport), sizeof(PARALLEL_CONF(parport)), 0},
	{ "Enabled", Bool_Tag, &PARALLEL_CONF(enabled), 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_parallel() {
  safe_strncpy(PARALLEL_CONF(type), "file", sizeof(PARALLEL_CONF(type)));
  safe_strncpy(PARALLEL_CONF(file), "stderr", sizeof(PARALLEL_CONF(type)));
  safe_strncpy(PARALLEL_CONF(parport), "/dev/parport0", sizeof(PARALLEL_CONF(parport)));
}

static void postload_parallel() {
}

static void presave_parallel() {
}
/*************************************************************************/
#define SERIAL_CONF(x) bx_options.serial.x

struct Config_Tag serial_conf[]={
	{ "Serport", String_Tag, &SERIAL_CONF(serport), sizeof(SERIAL_CONF(serport)), 0},
	{ "Enabled", Bool_Tag, &SERIAL_CONF(enabled), 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_serial() {
  safe_strncpy(SERIAL_CONF(serport), DEFAULT_SERIAL, sizeof(SERIAL_CONF(serport)));
}

static void postload_serial() {
}

static void presave_serial() {
}

/*************************************************************************/
#define NATFEAT_CONF(x) bx_options.natfeats.x

struct Config_Tag natfeat_conf[]={
	{ "CDROM", String_Tag, &NATFEAT_CONF(cdrom_driver), sizeof(NATFEAT_CONF(cdrom_driver)), 0},
	{ "Vdi", String_Tag, &NATFEAT_CONF(vdi_driver), sizeof(NATFEAT_CONF(vdi_driver)), 0},
	{ "HOSTEXEC", Bool_Tag, &NATFEAT_CONF(hostexec_enabled), 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_natfeat() {
  safe_strncpy(NATFEAT_CONF(cdrom_driver), "sdl", sizeof(NATFEAT_CONF(cdrom_driver)));
  safe_strncpy(NATFEAT_CONF(vdi_driver), "soft", sizeof(NATFEAT_CONF(vdi_driver)));
  NATFEAT_CONF(hostexec_enabled) = false;
}

static void postload_natfeat()
{
#ifndef ENABLE_OPENGL
  safe_strncpy(NATFEAT_CONF(vdi_driver), "soft", sizeof(NATFEAT_CONF(vdi_driver)));
#endif
}

static void presave_natfeat() {
}

/*************************************************************************/
#define NFVDI_CONF(x) bx_options.nfvdi.x

struct Config_Tag nfvdi_conf[]={
	{ "UseHostMouseCursor", Bool_Tag,  &NFVDI_CONF(use_host_mouse_cursor), 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_nfvdi() {
	NFVDI_CONF(use_host_mouse_cursor) = false;
}

static void postload_nfvdi() {
}

static void presave_nfvdi() {
}

/*************************************************************************/

static char hotkeys[10][HOTKEYS_STRING_SIZE];
struct Config_Tag hotkeys_conf[]={
	{ "Setup", String_Tag, hotkeys[0], HOTKEYS_STRING_SIZE, 0},
	{ "Quit", String_Tag, hotkeys[1], HOTKEYS_STRING_SIZE, 0},
	{ "Reboot", String_Tag, hotkeys[2], HOTKEYS_STRING_SIZE, 0},
	{ "ColdReboot", String_Tag, hotkeys[3], HOTKEYS_STRING_SIZE, 0},
	{ "Ungrab", String_Tag, hotkeys[4], HOTKEYS_STRING_SIZE, 0},
	{ "Debug", String_Tag, hotkeys[5], HOTKEYS_STRING_SIZE, 0},
	{ "Screenshot", String_Tag, hotkeys[6], HOTKEYS_STRING_SIZE, 0},
	{ "Fullscreen", String_Tag, hotkeys[7], HOTKEYS_STRING_SIZE, 0},
	{ "Sound", String_Tag, hotkeys[8], HOTKEYS_STRING_SIZE, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

typedef struct { char *string; bx_hotkey *keysym; } HOTKEYS_REL;

HOTKEYS_REL hotkeys_rel[]={
	{ hotkeys[0], &bx_options.hotkeys.setup },
	{ hotkeys[1], &bx_options.hotkeys.quit },
	{ hotkeys[2], &bx_options.hotkeys.warmreboot },
	{ hotkeys[3], &bx_options.hotkeys.coldreboot },
	{ hotkeys[4], &bx_options.hotkeys.ungrab },
	{ hotkeys[5], &bx_options.hotkeys.debug },
	{ hotkeys[6], &bx_options.hotkeys.screenshot },
	{ hotkeys[7], &bx_options.hotkeys.fullscreen },
	{ hotkeys[8], &bx_options.hotkeys.sound }
};

static void preset_hotkeys()
{
	// default values
#ifdef OS_darwin
	strcpy(hotkeys[0], "LM+,");
	strcpy(hotkeys[1], "LM+q");
	strcpy(hotkeys[2], "LM+r");
	strcpy(hotkeys[3], "LS+LM+r");
	strcpy(hotkeys[4], "LM+Escape");
	strcpy(hotkeys[5], "LM+d");
	strcpy(hotkeys[6], "LM+s");
	strcpy(hotkeys[7], "LM+f");
	strcpy(hotkeys[8], "RM+s");
#else
	strcpy(hotkeys[0], "Pause");
	strcpy(hotkeys[1], "LS+Pause");
	strcpy(hotkeys[2], "LC+Pause");
	strcpy(hotkeys[3], "LS+LC+Pause");
	strcpy(hotkeys[4], "LS+LC+LA+Escape");
	strcpy(hotkeys[5], "LA+Pause");
	strcpy(hotkeys[6], "PrintScreen");
	strcpy(hotkeys[7], "ScrollLock");
	strcpy(hotkeys[8], "RA+s");
#endif
}

static void postload_hotkeys() {
	// convert from string to pair of ints
	for(uint16 i=0; i<sizeof(hotkeys_rel)/sizeof(hotkeys_rel[0]); i++) {
		stringToKeysym(hotkeys_rel[i].keysym, hotkeys_rel[i].string);
	}
}

static void presave_hotkeys() {
	// convert from pair of ints to string
	for(uint16 i=0; i<sizeof(hotkeys_rel)/sizeof(hotkeys_rel[0]); i++) {
		keysymToString(hotkeys_rel[i].string, hotkeys_rel[i].keysym);
	}
}

/*************************************************************************/
struct Config_Tag ikbd_conf[]={
	{ "WheelEiffel", Bool_Tag, &bx_options.ikbd.wheel_eiffel, 0, 0},
	{ "AltGr", Bool_Tag, &bx_options.ikbd.altgr, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_ikbd()
{
	bx_options.ikbd.wheel_eiffel = false;
	bx_options.ikbd.altgr = true;
}

static void postload_ikbd()
{
}

static void presave_ikbd()
{
}

/*************************************************************************/
struct Config_Tag audio_conf[]={
	{ "Frequency", Int_Tag, &bx_options.audio.freq, 0, 0},
	{ "Channels", Int_Tag, &bx_options.audio.chans, 0, 0},
	{ "Bits", Int_Tag, &bx_options.audio.bits, 0, 0},
	{ "Samples", Int_Tag, &bx_options.audio.samples, 0, 0},
	{ "Enabled", Bool_Tag, &bx_options.audio.enabled, 0, 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_audio() {
  bx_options.audio.freq = 22050;
  bx_options.audio.chans = 2;
  bx_options.audio.bits = 16;
  bx_options.audio.samples = 1024;
  bx_options.audio.enabled = true;
}

static void postload_audio() {
	if (host)
		host->audio.Enable(bx_options.audio.enabled);
}

static void presave_audio() {
}

/*************************************************************************/
struct Config_Tag joysticks_conf[]={
	{ "Ikbd0", Int_Tag, &bx_options.joysticks.ikbd0, 0, 0},
	{ "Ikbd1", Int_Tag, &bx_options.joysticks.ikbd1, 0, 0},
	{ "JoypadA", Int_Tag, &bx_options.joysticks.joypada, 0, 0},
	{ "JoypadAButtons", String_Tag, &bx_options.joysticks.joypada_mapping,
		sizeof(bx_options.joysticks.joypada_mapping), 0},
	{ "JoypadB", Int_Tag, &bx_options.joysticks.joypadb, 0, 0},
	{ "JoypadBButtons", String_Tag, &bx_options.joysticks.joypadb_mapping,
		sizeof(bx_options.joysticks.joypadb_mapping), 0},
	{ NULL , Error_Tag, NULL, 0, 0 }
};

static void preset_joysticks() {
	bx_options.joysticks.ikbd0 = -1;	/* This one is wired to mouse */
	bx_options.joysticks.ikbd1 = 0;
	bx_options.joysticks.joypada = -1;
	bx_options.joysticks.joypadb = -1;
	safe_strncpy(bx_options.joysticks.joypada_mapping,
		"0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16",
		sizeof(bx_options.joysticks.joypada_mapping));
	safe_strncpy(bx_options.joysticks.joypadb_mapping,
		"0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16",
		sizeof(bx_options.joysticks.joypadb_mapping));
}

static void postload_joysticks() {
}

static void presave_joysticks() {
}

/*************************************************************************/

struct Config_Tag cmdline_conf[]={
	{ "IdeSwap", Bool_Tag, &ide_swap, 0, 0 },
	{ "BootEmutos", Bool_Tag, &boot_emutos, 0, 0 },
	{ "BootLilo", Bool_Tag, &boot_lilo, 0, 0 },
	{ "HaltOnReboot", Bool_Tag, &halt_on_reboot, 0, 0 },
	{ "ConfigFile", Path_Tag, config_file, sizeof(config_file), 0 },
#ifdef SDL_GUI
	{ "StartupGUI", Bool_Tag, &startupGUI, 0, 0 },
#endif
	{ NULL , Error_Tag, NULL, 0, 0 }
};

/*************************************************************************/
static struct Config_Section const all_sections[] = {
	{ "[GLOBAL]",     global_conf,   false, preset_global, postload_global, presave_global },
	{ "[STARTUP]",    startup_conf,  false, preset_startup, postload_startup, presave_startup },
	{ "[IKBD]",       ikbd_conf,     false, preset_ikbd, postload_ikbd, presave_ikbd },
	{ "[HOTKEYS]",    hotkeys_conf,  false, preset_hotkeys, postload_hotkeys, presave_hotkeys },
	{ "[JIT]",        jit_conf,      false, preset_jit, postload_jit, presave_jit },
	{ "[VIDEO]",      video_conf,    false, preset_video, postload_video, presave_video },
	{ "[TOS]",        tos_conf,      false, preset_tos, postload_tos, presave_tos },
	{ "[IDE0]",       diskc_configs, false, preset_ide, postload_ide, presave_ide }, // beware of ide_swap
	{ "[IDE1]",       diskd_configs, false, 0, 0, 0 },
	  /* DISKS sections */
	{ "[PARTITION0]", disk0_configs, false, preset_disk, postload_disk, presave_disk },
	{ "[PARTITION1]", disk1_configs, true,  0, 0, 0 },
	{ "[PARTITION2]", disk2_configs, true,  0, 0, 0 },
	{ "[PARTITION3]", disk3_configs, true,  0, 0, 0 },
	{ "[PARTITION4]", disk4_configs, true,  0, 0, 0 },
	{ "[PARTITION5]", disk5_configs, true,  0, 0, 0 },
	{ "[PARTITION6]", disk6_configs, true,  0, 0, 0 },
	{ "[PARTITION7]", disk7_configs, true,  0, 0, 0 },
	{ "[HOSTFS]",     arafs_conf,    false, preset_arafs, postload_arafs, presave_arafs },
	{ "[OPENGL]",     opengl_conf,   false, preset_opengl, postload_opengl, presave_opengl },
	  /* MAX_ETH sections */
	{ "[ETH0]",       eth0_conf,     false, preset_ethernet, postload_ethernet, presave_ethernet },
	{ "[ETH1]",       eth1_conf,     true,  0, 0, 0 },
	{ "[ETH2]",       eth2_conf,     true,  0, 0, 0 },
	{ "[ETH3]",       eth3_conf,     true,  0, 0, 0 },
	{ "[LILO]",       lilo_conf,     false, preset_lilo, postload_lilo, presave_lilo },
	{ "[MIDI]",       midi_conf,     false, preset_midi, postload_midi, presave_midi },
	{ "[CDROMS]",     nfcdroms_conf, false, preset_nfcdroms, postload_nfcdroms, presave_nfcdroms },
	{ "[AUTOZOOM]",   autozoom_conf, false, preset_autozoom, postload_autozoom, presave_autozoom },
	{ "[NFOSMESA]",   osmesa_conf,   false, preset_osmesa, postload_osmesa, presave_osmesa },
	{ "[PARALLEL]",   parallel_conf, false, preset_parallel, postload_parallel, presave_parallel },
	{ "[SERIAL]",     serial_conf,   false, preset_serial, postload_serial, presave_serial },
	{ "[NATFEATS]",   natfeat_conf,  false, preset_natfeat, postload_natfeat, presave_natfeat },
	{ "[NFVDI]",      nfvdi_conf,    false, preset_nfvdi, postload_nfvdi, presave_nfvdi },
	{ "[AUDIO]",      audio_conf,    false, preset_audio, postload_audio, presave_audio },
	{ "[JOYSTICKS]",  joysticks_conf,false, preset_joysticks, postload_joysticks, presave_joysticks },
	{ "cmdline",      cmdline_conf,  false, 0, 0, 0 },
	{ 0, 0, false, 0, 0, 0 }
};

const struct Config_Section *getConfigSections(void)
{
	return all_sections;
}

/*************************************************************************/
static void usage (int status) {
  printf ("Usage: %s [OPTIONS]\n", program_name);
  printf ("\
Options:\n\
  -a, --floppy NAME               floppy image file NAME\n\
  -e, --emutos                    boot EmuTOS\n\
  -N, --nomouse                   don't grab mouse at startup\n\
  -f, --fullscreen                start in fullscreen\n\
  -v, --refresh <X>               VIDEL refresh rate in VBL (default 2)\n\
  -r, --resolution <X>            boot in X color depth [1,2,4,8,16]\n\
  -m, --monitor <X>               attached monitor: 0 = VGA, 1 = TV\n\
  -c, --config FILE               read different configuration file\n\
  -s, --save                      save configuration file\n\
  -S, --swap-ide                  swap IDE drives\n\
  -P <X,Y> or <center>            set window position\n\
  -k, --locale <XY>               set NVRAM keyboard layout and language\n\
      --option section:key:value  set configuration value\n\
");
#ifdef SDL_GUI
  printf("  -G, --gui                       open GUI at startup\n");
#endif
#if HOSTFS_SUPPORT
  printf("  -d, --disk CHAR:PATH[:]         HostFS mapping, e.g. d:/atari/d_drive\n");
#endif
#ifdef ENABLE_LILO
  printf("  -l, --lilo                      boot a linux kernel\n");
  printf("  -H, --halt                      linux kernel halts on reboot\n");
#endif
#ifndef FixedSizeFastRAM
  printf("  -F, --fastram SIZE              FastRAM size (in MB)\n");
#endif
#if FIXED_ADDRESSING
  printf("      --fixedmem OFFSET           use OFFSET for Atari memory (default: 0x%08x)\n", (unsigned int) fixed_memory_offset);
  printf("      --probe-fixed               try to figure out best value for above offset\n");
#endif
#ifdef DEBUGGER
  printf("  -D, --debug                     start debugger\n");
#endif
  printf ("\
\n\
  -h, --help                      display this help and exit\n\
  -V, --version                   output version information and exit\n\
");
  exit (status);
}

static void preset_cfg() {
	for (const struct Config_Section *section = all_sections; section->name; section++)
	{
		if (section->skip_if_empty)
		{
			/*
			 * This is assumed in saveSettings().
			 * Check this here because it is not called on startup.
			 */
			if (section->tags->type != String_Tag && section->tags->type != Path_Tag)
			{
				panicbug("type of first entry in config section %s not String or Path", section->name);
				abort();
			}
		}
		if (section->preset)
			section->preset();
	}
}

static void postload_cfg() {
	for (const struct Config_Section *section = all_sections; section->name; section++)
		if (section->postload)
			section->postload();
}

static void presave_cfg() {
	for (const struct Config_Section *section = all_sections; section->name; section++)
		if (section->presave)
			section->presave();
}

static void print_version(void)
{
	loadSettings(config_file);
	// infoprint("%s\n", version_string);
	infoprint("Configuration:");
	infoprint("SDL (compiled)   : %d.%d.%d", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
	SDL_version linked;
	SDL_GetVersion(&linked);
	infoprint("SDL (linked)     : %d.%d.%d", linked.major, linked.minor, linked.patch);
	infoprint("CPU JIT compiler : %s%s", USE_JIT ? "enabled" : "disabled", USE_JIT ? (bx_options.jit.jit ? " (active)" : " (inactive)") : "");
	infoprint("FPU JIT compiler : %s%s", USE_JIT_FPU ? "enabled" : "disabled", USE_JIT_FPU ? (bx_options.jit.jitfpu ? " (active)" : " (inactive)") : "");
#if FIXED_ADDRESSING
	if (fixed_memory_offset != FMEMORY)
	infoprint("Addressing mode  : fixed (0x%08x; default: 0x%08x)", (unsigned int) fixed_memory_offset, FMEMORY);
	else
	infoprint("Addressing mode  : fixed (0x%08x)", (unsigned int) fixed_memory_offset);
#else
	infoprint("Addressing mode  : %s", DIRECT_ADDRESSING ? "direct" : "normal");
#endif
	infoprint("Memory check     : %s", MEMORY_CHECK);
	infoprint("Full MMU         : %s", FULLMMU ? "enabled" : "disabled");
	infoprint("FPU              : %s", USES_FPU_CORE);
	infoprint("DSP              : %s", DSP_EMULATION ? "enabled" : "disabled");
	infoprint("DSP disassembler : %s", DSP_DISASM ? "enabled" : "disabled");
	infoprint("OpenGL support   : %s", ENABLE_OPENGL ? "enabled" : "disabled");
	infoprint("Native features  : %s", PROVIDES_NATFEATS);
}

static void early_cmdline_check(int argc, char **argv) {
	for (int c = 0; c < argc; c++) {
		char *p = argv[c];
		if (strcmp(p, "-S") == 0  || strcmp(p, "--swap-ide") == 0)
			ide_swap = true;

		else if ((strcmp(p, "-c") == 0) || (strcmp(p, "--config") == 0)) {
			if ((c + 1) < argc) {
				setConfigFile(argv[c + 1]);
			} else {
				panicbug("config switch requires one parameter");
				exit(EXIT_FAILURE);
			}
		} else if ((strcmp(p, "-h") == 0) || (strcmp(p, "--help") == 0)) {
			usage(EXIT_SUCCESS);
		} else if ((strcmp(p, "-V") == 0) || (strcmp(p, "--version") == 0)) {
			print_version();
			exit(EXIT_SUCCESS);
#if FIXED_ADDRESSING
		} else if ((strcmp(p, "--probe-fixed") == 0)) {
			vm_probe_fixed();
			exit(EXIT_SUCCESS);
#endif
		}
	}
}

static int process_cmdline(int argc, char **argv)
{
	int c;
	while ((c = getopt_long (argc, argv,
							 "a:" /* floppy image file */
							 "e"  /* boot emutos */
#ifdef ENABLE_LILO
							 "l"  /* boot lilo */
							 "H"  /* halt on reboot */
#endif
#ifdef DEBUGGER
							 "D"  /* debugger */
#endif
#ifndef FixedSizeFastRAM
							 "F:" /* FastRAM */
#endif
							 "N"  /* no mouse */
							 "f"  /* fullscreen */
							 "v:" /* VIDEL refresh */
							 "r:" /* resolution */
							 "m:" /* attached monitor */
#if HOSTFS_SUPPORT
							 "d:" /* filesystem assignment */
#endif
							 "s"  /* save config file */

							 "c:" /* path to config file */
#ifdef SDL_GUI
							 "G"  /* GUI startup */
#endif
							 "P:" /* position of the window */
							 "S"  /* swap IDE drives */
							 "k:" /* keyboard layout */
							 "h"  /* help */
							 "V"  /* version */,
							 long_options, (int *) 0)) != EOF) {
		switch (c) {
			case 'V':
			case 'h':
			case 'c':
			case 'S':
				/* processed in early_cmdline_check already */
				break;
#ifdef SDL_GUI
			case 'G':
				startupGUI = true;
				break;
#endif
#ifdef DEBUGGER
			case 'D':
				bx_options.startup.debugger = true;
				break;
#endif

			case 'e':
				boot_emutos = true;
				break;
	
			case 'f':
				bx_options.video.fullscreen = true;
				break;

#ifdef ENABLE_LILO
			case 'l':
				boot_lilo = true;
				break;

			case 'H':
				halt_on_reboot = true;
				break;
#endif

			case 'v':
				bx_options.video.refresh = atoi(optarg);
				break;
	
			case 'N':
				bx_options.startup.grabMouseAllowed = false;
				break;
	
			case 'm':
				bx_options.video.monitor = atoi(optarg);
				break;
	
			case 'a':
				if ((strlen(optarg)-1) > sizeof(bx_options.floppy.path))
					panicbug("Floppy image filename longer than %u chars.", (unsigned)sizeof(bx_options.floppy.path));
				safe_strncpy(bx_options.floppy.path, optarg, sizeof(bx_options.floppy.path));
				break;

			case 'r':
				bx_options.video.boot_color_depth = atoi(optarg);
				break;

			case 'P':
				safe_strncpy(bx_options.video.window_pos, optarg, sizeof(bx_options.video.window_pos));
				break;

#if HOSTFS_SUPPORT
			case 'd':
				if ( strlen(optarg) < 4 || optarg[1] != ':') {
					panicbug("Not enough parameters for -d");
					break;
				}
				// set the drive
				{
					int8 i = DriveFromLetter(optarg[0]);
					if (i <= 0 || i >= HOSTFS_MAX_DRIVES) {
						panicbug("Drive out of [A-Z] range for -d");
						break;
					}

					safe_strncpy( bx_options.aranymfs.drive[i].configPath, optarg+2,
								sizeof(bx_options.aranymfs.drive[i].configPath) );
					// Note: tail colon processing (case sensitivity flag) is 
					// done later by calling postload_cfg.
					// Just make sure postload_cfg is called after this.
				}
				break;
#endif

			case 's':
				saveConfigFile = true;
				break;

#ifndef FixedSizeFastRAM
			case 'F':
				bx_options.fastram = atoi(optarg);
				break;
#endif

			case 'k':
				{
					static struct {
						char name[6];
						nvram_t id;
					} const countries[] = {
						{ "us", COUNTRY_US }, { "en_US", COUNTRY_US },
						{ "de", COUNTRY_DE }, { "de_DE", COUNTRY_DE }, 
						{ "fr", COUNTRY_FR }, { "fr_FR", COUNTRY_FR },
						{ "uk", COUNTRY_UK }, { "en_GB", COUNTRY_UK }, 
						{ "es", COUNTRY_ES }, { "es_ES", COUNTRY_ES },
						{ "it", COUNTRY_IT }, { "it_IT", COUNTRY_IT },
						{ "se", COUNTRY_SE }, { "sv_SE", COUNTRY_SE },
						{ "ch", COUNTRY_SF }, { "fr_CH", COUNTRY_SF },
						{ "cd", COUNTRY_SG }, { "de_CH", COUNTRY_SG },
						{ "tr", COUNTRY_TR }, { "tr_TR", COUNTRY_TR },
						{ "fi", COUNTRY_FI }, { "fi_FI", COUNTRY_FI },
						{ "no", COUNTRY_NO }, { "no_NO", COUNTRY_NO }, 
						{ "dk", COUNTRY_DK }, { "da_DK", COUNTRY_DK },
						{ "sa", COUNTRY_SA }, { "ar_SA", COUNTRY_SA },
						{ "nl", COUNTRY_NL }, { "nl_NL", COUNTRY_NL },
						{ "cz", COUNTRY_CZ }, { "cS_CZ", COUNTRY_CZ },
						{ "hu", COUNTRY_HU }, { "hu_HU", COUNTRY_HU }, 
						{ "pl", COUNTRY_PL }, { "pl_PL", COUNTRY_PL }, 
						{ "lt", COUNTRY_PL }, { "lt_LT", COUNTRY_LT }, 
						{ "ru", COUNTRY_RU }, { "ru_RU", COUNTRY_RU }, 
						{ "ee", COUNTRY_EE }, { "et_EE", COUNTRY_EE }, 
						{ "by", COUNTRY_BY }, { "be_BY", COUNTRY_BY }, 
						{ "ua", COUNTRY_UA }, { "uk_UA", COUNTRY_UA }, 
						{ "sk", COUNTRY_SK }, { "sk_SK", COUNTRY_SK },
						{ "ro", COUNTRY_RO }, { "ro_RO", COUNTRY_RO },
						{ "bg", COUNTRY_BG }, { "bg_BG", COUNTRY_BG },
						{ "si", COUNTRY_SI }, { "sl_SI", COUNTRY_SI },
						{ "hr", COUNTRY_HR }, { "hr_HR", COUNTRY_HR },
						{ "rs", COUNTRY_RS }, { "sr_RS", COUNTRY_RS },
						{ "me", COUNTRY_ME }, { "sr_ME", COUNTRY_ME },
						{ "mk", COUNTRY_MK }, { "mk_MK", COUNTRY_MK },
						{ "gr", COUNTRY_GR }, { "el_GR", COUNTRY_GR },
						{ "lv", COUNTRY_LV }, { "lv_LV", COUNTRY_LV },
						{ "il", COUNTRY_IL }, { "he_IL", COUNTRY_IL },
						{ "za", COUNTRY_ZA }, { "af_ZA", COUNTRY_ZA }, /* ambigious language */
						{ "pt", COUNTRY_PT }, { "pt_PT", COUNTRY_PT },
						{ "be", COUNTRY_BE }, { "fr_BE", COUNTRY_BE },
						{ "jp", COUNTRY_JP }, { "ja_JP", COUNTRY_JP },
						{ "cn", COUNTRY_CN }, { "zh_CN", COUNTRY_CN },
						{ "kr", COUNTRY_KR }, { "ko_KR", COUNTRY_KR },
						{ "vn", COUNTRY_VN }, { "vi_VN", COUNTRY_VN },
						{ "in", COUNTRY_IN }, { "ar_IN", COUNTRY_IN }, /* ambigious language */
						{ "ir", COUNTRY_IR }, { "fa_IR", COUNTRY_IR },
						{ "mn", COUNTRY_MN }, { "mn_MN", COUNTRY_MN },
						{ "np", COUNTRY_NP }, { "ne_NP", COUNTRY_NP },
						{ "la", COUNTRY_LA }, { "lo_LA", COUNTRY_LA },
						{ "kh", COUNTRY_KH }, { "km_KH", COUNTRY_KH },
						{ "id", COUNTRY_ID }, { "id_ID", COUNTRY_ID },
						{ "bd", COUNTRY_BD }, { "bn_BD", COUNTRY_BD },
					};
					bx_options.tos.cookie_akp = -1;
					for(unsigned i=0; i<sizeof(countries)/sizeof(countries[0]); i++) {
						if (strcasecmp(optarg, countries[i].name) == 0) {
							i = countries[i].id;
							bx_options.tos.cookie_akp = i << 8 | i;
							break;
						}
					}
					if (bx_options.tos.cookie_akp == -1) {
						char countrystr[(sizeof(countries)/sizeof(countries[0])) * 6 + 1];
						*countrystr = '\0';
						for(unsigned i=0; i<sizeof(countries)/sizeof(countries[0]); i++) {
							strcat(countrystr, " ");
							strcat(countrystr, countries[i].name);
						}
						panicbug("Error unknown country '%s', use one of: %s", optarg, countrystr);
						exit(EXIT_FAILURE);
					}
				}
				break;
				
			case OPT_PROBE_FIXED:
				/* processed in early_cmdline_check already */
				break;

#if FIXED_ADDRESSING
			case OPT_FIXEDMEM_OFFSET:
				bx_options.fixed_memory_offset = strtoul(optarg, NULL, 0);
				break;
#endif

			case OPT_SET_OPTION:
				{
					char *section = optarg;
					char *key = strchr(section, ':');
					char *value;
					if (key == NULL || (value = strchr(key + 1, ':')) == NULL)
					{
						if (strcmp(section, "help") == 0)
						{
							listConfigValues(true);
							exit(EXIT_SUCCESS);
						} else if (strcmp(section, "list") == 0)
						{
							listConfigValues(false);
							exit(EXIT_SUCCESS);
						} else
						{
							panicbug("wrong '--option' argument: must be section:key:value");
							exit(EXIT_FAILURE);
						}
					} else
					{
						*key++ = '\0';
						*value++ = '\0';
						if (setConfigValue(section, key, value) == false)
						{
							/* exit(EXIT_FAILURE); */
							bug("cannot set [%s]%s to %s (ignored)", section, key, value);
						}
					}
				}
				break;
				
			default:
				usage (EXIT_FAILURE);
		}
	}
	return optind;
}

// append a filename to a path
char *addFilename(char *buffer, const char *file, unsigned int bufsize)
{
	int dirlen = strlen(buffer);
	if ((dirlen + 1 + strlen(file) + 1) < bufsize) {
		if (dirlen > 0) {
			char *ptrLast = buffer + dirlen - 1;
			if (*ptrLast != '/' && *ptrLast != '\\')
				strcat(buffer, DIRSEPARATOR);
		}
		strcat(buffer, file);
	}
	else {
		panicbug("addFilename(\"%s\") - buffer too small!", file);
		safe_strncpy(buffer, file, bufsize);	// at least the filename
	}
	strd2upath(buffer, buffer);
	return buffer;
}

// build a complete path to an user-specific file
char *getConfFilename(const char *file, char *buffer, unsigned int bufsize)
{
	Host::getConfFolder(buffer, bufsize);

	// Does the folder exist?
	struct stat buf;
	if (stat(buffer, &buf) == -1) {
		D(bug("Creating config folder '%s'", buffer));
		Host::makeDir(buffer);
	}

	return addFilename(buffer, file, bufsize);
}

// build a complete path to system wide data file
char *getDataFilename(const char *file, char *buffer, unsigned int bufsize)
{
	char *name = getConfFilename(file, buffer, bufsize);
	struct stat buf;
	if (stat(name, &buf) == -1)
	{
	  Host::getDataFolder(buffer, bufsize);
	  name = addFilename(buffer, file, bufsize);
	}
	return name;
}

static bool decode_ini_file(const char *rcfile)
{
	// Does the config exist?
	struct stat buf;
	if (stat(rcfile, &buf) == -1) {
		infoprint("Config file '%s' not found. It's been created with default values. Edit it to suit your needs.", rcfile);
		saveConfigFile = true;
#ifdef SDL_GUI
		startupGUI = true;
#endif
		return false;
	}

	infoprint("Using config file: '%s'", rcfile);

	bool verbose = false;

	char home_folder[1024];
	char data_folder[1024];
	Host::getHomeFolder(home_folder, sizeof(home_folder));
	Host::getDataFolder(data_folder, sizeof(data_folder));
	ConfigOptions cfgopts(rcfile, home_folder, data_folder);

	for (const struct Config_Section *section = all_sections; section->name; section++)
	{
		struct Config_Tag *conf = section->tags;
		
		if (*section->name != '[') continue;
		if (ide_swap)
		{
			if (conf == diskc_configs) conf = diskd_configs;
			else if (conf == diskd_configs) conf = diskc_configs;
		}
		cfgopts.process_config(conf, section->name, verbose);
	}
	
	return true;
}

bool loadSettings(const char *cfg_file) {
	preset_cfg();
	bool ret = decode_ini_file(cfg_file);
	postload_cfg();
	return ret;
}

bool saveSettings(const char *fs)
{
	presave_cfg();

	char home_folder[1024];
	char data_folder[1024];
	Host::getHomeFolder(home_folder, sizeof(home_folder));
	Host::getDataFolder(data_folder, sizeof(data_folder));
	ConfigOptions cfgopts(fs, home_folder, data_folder);

	bool first = true;
	for (const struct Config_Section *section = all_sections; section->name; section++, first = false)
	{
		struct Config_Tag *conf = section->tags;
		
		if (*section->name != '[') continue;
		if (ide_swap)
		{
			if (conf == diskc_configs) conf = diskd_configs;
			else if (conf == diskd_configs) conf = diskc_configs;
		}
		if (!section->skip_if_empty || strlen((const char *)conf->buf) != 0)
			if (cfgopts.update_config(conf, section->name) < 0)
			{
				if (first)
				{
					panicbug("Error while writing the '%s' config file.", fs);
					return false;
				}
			}
	}
	
	return true;
}


/*
 * set config variable by name
 * 'section_name' must be passed without brackets here
 */
bool setConfigValue(const char *section_name, const char *key, const char *value)
{
	char home_folder[1024];
	char data_folder[1024];
	Host::getHomeFolder(home_folder, sizeof(home_folder));
	Host::getDataFolder(data_folder, sizeof(data_folder));
	ConfigOptions cfgopts(config_file, home_folder, data_folder);

	int len = strlen(section_name);
	for (const struct Config_Section *section = all_sections; section->name; section++)
	{
		struct Config_Tag *conf = section->tags;
		
		if (*section->name != '[')
			continue;
		if (strncmp(section->name + 1, section_name, len) != 0 || section->name[len + 1] != ']')
			continue;
		if (ide_swap)
		{
			if (conf == diskc_configs) conf = diskd_configs;
			else if (conf == diskd_configs) conf = diskc_configs;
		}
		for (; conf->code; conf++)
		{
			if (strcmp(conf->code, key) == 0)
			{
				bool ret = cfgopts.set_config_value(conf, value);
				if (ret && section->postload)
					section->postload();
				return ret;
			}
		}
		return false;
	}
	
	return false;
}


void listConfigValues(bool type)
{
	char home_folder[1024];
	char data_folder[1024];
	Host::getHomeFolder(home_folder, sizeof(home_folder));
	Host::getDataFolder(data_folder, sizeof(data_folder));
	ConfigOptions cfgopts(config_file, home_folder, data_folder);
	
	for (const struct Config_Section *section = all_sections; section->name; section++)
	{
		const char *name = section->name;
		if (*name == '[')
			name++;
		const char *end = strchr(name, ']');
		int len = (int)(end ? (end - name) : strlen(name));
		char *section_name = strdup(name);
		section_name[len] = '\0';
		
		for (struct Config_Tag *conf = section->tags; conf->code; conf++)
		{
			if (ide_swap)
			{
				if (conf == diskc_configs) conf = diskd_configs;
				else if (conf == diskd_configs) conf = diskc_configs;
			}
			char *value = cfgopts.get_config_value(conf, type);
			printf("%s:%s:%s\n", section_name, conf->code, value ? value : "");
			
		}
		free(section_name);
	}
}


bool decode_switches(int argc, char **argv)
{
	getConfFilename(ARANYMCONFIG, config_file, sizeof(config_file));

	early_cmdline_check(argc, argv);
	preset_cfg();
	decode_ini_file(config_file);
	process_cmdline(argc, argv);
	postload_cfg();

	if (saveConfigFile) {
		D(bug("Storing configuration to file '%s'", config_file));
		if (saveSettings(config_file))
			loadSettings(config_file);
	}

	return true;
}

const char *getConfigFile() {
	return config_file;
}

void setConfigFile(const char *filename)
{
	safe_strncpy(config_file, filename, sizeof(config_file));
}
