Using Xcode to compile MacAranym:
---------------------------------
-Install Mac OS X developer tools (from your Mac OS X system CD/DVD or download
 latest version from http://connect.apple.com)

-Download the SDL runtime library from http://www.libsdl.org
 Copy the file "SDL.framework" to the "data" subdirectory.

-Open the Xcode project file "MacAranym.pbproj" using Xcode.

-Select one of the top level targets: "MacAranym", "MacAranym MMU" or "Make DMG"

-Select "Development" or "Deployment" build style from the popup menu.

-Press the "Build" button and wait until compilation has finished. If no errors
 occured, you should find the binary in the "build" subdirectory.


Have fun


Note to Mac OS 10.3.9 users:
----------------------------
Do not install any update to Quicktime 7 or you won't be able to compile the 
project without problems. Stick with Quicktime 6.5.2!
