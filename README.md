[![Build Status](https://travis-ci.org/aranym/aranym.svg?branch=master)](https://travis-ci.org/aranym/aranym)
[![Build status](https://ci.appveyor.com/api/projects/status/buvngw1mdtdo28ri/branch/master?svg=true)](https://ci.appveyor.com/project/th-otto/aranym/branch/master)

Latest snapshot: [![Download](https://api.bintray.com/packages/aranym/aranym-files/snapshots/images/download.svg) ](https://bintray.com/aranym/aranym-files/snapshots/_latestVersion#files)
Latest release: [![Download](https://api.bintray.com/packages/aranym/aranym-files/releases/images/download.svg) ](https://bintray.com/aranym/aranym-files/releases/_latestVersion#files)

      ARAnyM (Atari Running on Any Machine)
      version 1.1.0 released on 2019/04/14


# License

    ARAnyM is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    ARAnyM is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with ARAnyM; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA



# What is this?

ARAnyM is a multiplatform virtual machine (a software layer, or an emulator)
for running Atari ST/TT/Falcon operating systems and applications on almost
any hardware with many host operating systems.
The reason for writing ARAnyM is to provide Atari power users with
faster and better machines. The ultimate goal is to create a new platform
where TOS/GEM applications could continue to live forever.

# Features:

   - 68040 CPU (including MMU040)
   - 68040 and 68881/2 FPU
   - 14 MB ST-RAM and up to 3824 MB (configurable) of FastRAM
   - VIDEL, Blitter, MFP, SCC, ACIA, IKBD for highest possible compatibility
   - Sound (compatible with Atari Falcon DMA Sound)
   - Atari floppy DD/HD for connecting floppy image or real floppy drive
   - two IDE channels for connecting disk images, harddrives or CD-ROMs
   - extended keyboard and mouse support (including mouse wheel)
   - direct access to host file system via BetaDOS and MiNT xfs drivers
   - networking using ethernet emulation with a driver for MiNT-Net
   - TOS 4.04, EmuTOS, MagiC or Linux-m68k as the boot operating system
   - runs with FreeMiNT, MagiC, and any other operating system that runs
     also on real Atari computer.
   - Native CD-ROM access (under Linux, other OS: audio CD only), without
     scsi/ide/whatsoever emulation
   - USB and PCI NatFeat support for developing Atari drivers on ARAnyM

Important: ARAnyM is not finished. Consider this to be a beta version
and a work-in-progress. Sorry for incomplete and confusing documentation.
Any help with programming/documentation/porting is very welcome!


# Installing

See [INSTALL](INSTALL) file and https://github.com/aranym/aranym/wiki

You can also visit https://repology.org/metapackage/aranym/versions to check
whether your distribution has a pre-compiled version already.


# Configuring and running

See https://github.com/aranym/aranym/wiki


# Keyboard shortcuts

### Keys not found on a PC keyboard:

- Atari Help key - mapped on the F11
- Atari Undo key - mapped on the F12

### Keys not found on Atari keyboard:

- Page Up key    - mapped as Shift+Arrow Up (usual combination in GEM apps)
- Page Down key  - mapped as Shift+Arrow Down (usual combo in GEM apps)

### Special keys (most can be re-defined in the SETUP):

- Pause/Break    - invoke SETUP GUI (ARAnyM on-the-fly configuration)
- Shift+Pause    - QUIT ARAnyM
- Ctrl+Pause     - REBOOT ARAnyM
- Alt+Pause      - enter integrated debugger (only if started with "-D")

- PrintScreen    - create a screenshot in actual directory

- Alt+Ctrl+Shift+Esc - release the keyboard/mouse input focus so you can use
                 your keyboard and mouse in other host applications.
                 Can be changed to other key combo in the Settings.
                 Middle mouse does the same job.

- Scroll Lock    - switch between windowed and fullscreen mode


# Troubleshooting

If your Microsoft IntelliMouse doesn't work correctly on Linux framebuffer
console you may want to set the SDL_MOUSEDEV_IMPS2 environment variable to 1.

Example for bash: export SDL_MOUSEDEV_IMPS2=1; aranym


# More information

Read the [NEWS](NEWS) file for user visible changes.

Read [ChangeLog](ChangeLog) for internal changes.
Look at [TODO](TODO) if you want to help us.

Join our mailing list for ARAnyM users - the WEB interface for the list
is at https://groups.google.com/forum/#!forum/aranym

Visit https://github.com/aranym/ for latest information and source code.

Help us improve the Wiki documentation at https://github.com/aranym/aranym/wiki

The "Power Without The Price" is back!    https://aranym.github.io/
