//
//  NSMenuUpdate.mm
//  MacAranym
//
//  Created by Jens Heitmann on 14.08.07.
//  Copyright 2007 __MyCompanyName__. All rights reserved.
//

#import "SDL_compat.h"
#import "SDLMain.h"
#import <sys/param.h> /* for MAXPATHLEN */
#import <unistd.h>
#import "parameters.h"

/* Menu item tag */
#define PREFERENCES_MENUITEM_TAG	1
#define QUIT_MENUITEM_TAG			2
#define REBOOT_MENUITEM_TAG			3
#define SCREENSHOT_MENUITEM_TAG		4
#define FULLSCREEN_MENUITEM_TAG		5
#define DEBUG_MENUITEM_TAG			6

/* Map SDL Modifier Mask into MAC Modifier Mask */
static unsigned int getRefreshModifier(SDL_Keymod mods) 
{
	unsigned int mask = 0;

	if (mods & (KMOD_LSHIFT|KMOD_RSHIFT)) mask |= NSShiftKeyMask;
	if (mods & (KMOD_LCTRL|KMOD_RCTRL)) mask |= NSControlKeyMask;
	if (mods & (KMOD_LALT|KMOD_RALT)) mask |= NSAlternateKeyMask;
	if (mods & (KMOD_LGUI|KMOD_RGUI)) mask |= NSCommandKeyMask;
	
	return mask;
}

/* Update key combination in a menu entry */
static void refreshMenuItem(bx_hotkey keysym, int itemTag) 
{
	NSMenu *menu = [[NSApplication sharedApplication] mainMenu];
	int numberOfItems = [menu numberOfItems];
	for (int i = 0; i < numberOfItems; i++) {
		NSMenuItem *popupmenu = [menu itemAtIndex:i];	
		NSMenuItem *menuitem = [[popupmenu submenu]itemWithTag:itemTag];
		if (menuitem != NULL) {
			[menuitem setKeyEquivalent: [NSString stringWithCString:(const char *)SDL_GetKeyName(keysym.sym) encoding: NSUTF8StringEncoding]];
			[menuitem setKeyEquivalentModifierMask: getRefreshModifier(keysym.mod)];
			[menu itemChanged:menuitem];
			return;
		}
	}
}

/* Refresh all menu items associated with hotkeys */
void refreshMenuKeys()
{		
	refreshMenuItem(bx_options.hotkeys.setup, PREFERENCES_MENUITEM_TAG);
	refreshMenuItem(bx_options.hotkeys.quit, QUIT_MENUITEM_TAG);
	refreshMenuItem(bx_options.hotkeys.warmreboot, REBOOT_MENUITEM_TAG);
	refreshMenuItem(bx_options.hotkeys.screenshot, SCREENSHOT_MENUITEM_TAG);
	refreshMenuItem(bx_options.hotkeys.fullscreen, FULLSCREEN_MENUITEM_TAG);
	refreshMenuItem(bx_options.hotkeys.debug, DEBUG_MENUITEM_TAG);
}
