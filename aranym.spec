%define name	aranym
%define ver	0.7.0
%define rel	2
%define copy	GPL
%define joy Petr Stehlik <pstehlik@sophics.cz>
%define group	Console/Emulators
%define realname aranym-%{ver}
%define src aranym-%{ver}.tar.gz
Summary:	32-bit Atari personal computer (Falcon030/TT030) virtual machine.
Name:		%{name}
Version:	%{ver}
Release:	%{rel}
Copyright:	%{copy}
Packager: %{joy}
URL: http://aranym.sourceforge.net/
Group:	%{group}
Source: http://prdownloads.sourceforge.net/aranym/%{src}
BuildRoot: /var/tmp/%{name}-root
#Patch: %{name}-%{ver}.patch
%description
ARAnyM is a software only TOS clone - a virtual machine that allows you
to run TOS/FreeMiNT/MagiC operating systems and TOS/GEM applications.

Authors:
Ctirad Fertr, Milan Jurik, Standa Opichal, Petr Stehlik, Johan Klockars,
Didier MEQUIGNON, Patrice Mandin and others (see AUTHORS for a full list).

%prep
rm -rf %{realname}

%setup -n %{realname}/src/Unix
./autogen.sh || echo "Autogen put out its usual complaint, ignored!"
#%patch -p1

%build
./configure --prefix=/usr --enable-jit-compiler
make
mv aranym aranym-jit
make clean
./configure --prefix=/usr
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/share/aranym
make install DESTDIR=$RPM_BUILD_ROOT
install aranym $RPM_BUILD_ROOT/usr/bin
install aranym-jit $RPM_BUILD_ROOT/usr/bin

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%doc ../../doc ../../AUTHORS ../../BUGS ../../COPYING ../../FAQ ../../INSTALL ../../NEWS ../../ChangeLog ../../README ../../TODO
/usr/bin/aranym
/usr/bin/aranym-jit
/usr/share/aranym/atari/aranymfs/aranymfs.dos.bz2
/usr/share/aranym/atari/aranymfs/aranym.xfs.bz2
/usr/share/aranym/atari/aranymfs/config.sys
/usr/share/aranym/atari/fvdi/aranym.sys.bz2
/usr/share/aranym/atari/fvdi/fvdi.prg.bz2
/usr/share/aranym/atari/fvdi/fvdi.sys
/usr/share/aranym/atari/sound/README
/usr/share/aranym/atari/sound/zmagxsnd.prg.bz2
/usr/share/aranym/atari/tools/README
/usr/share/aranym/atari/tools/clocky.prg.bz2
/usr/share/aranym/atari/tools/fastram.prg.bz2
/usr/share/aranym/atari/tools/pc101us.kbd.bz2
/usr/share/aranym/atari/tools/pcpatch.prg.bz2
/usr/share/aranym/atari/newdesk.inf
/usr/share/aranym/atari/aranymrc.example
/usr/share/aranym/atari/mmusetup.cnf
/usr/share/aranym/createdisk/createdisk.README
/usr/share/aranym/createdisk/createdisk.sh
/usr/share/aranym/createdisk/mbrdata
/usr/share/aranym/diskimage.c
/usr/share/aranym/floppy.sh
/usr/share/aranym/font8.bmp
/usr/share/aranym/etos512k.img

%changelog
* Fri Mar 07 2003 Petr Stehlik <pstehlik@sophics.cz>
Fixed paths to share/aranym folder.

* Wed Jan 29 2003 Petr Stehlik <pstehlik@sophics.cz>
New release. Updated list of provided files.

* Tue Oct 22 2002 Petr Stehlik <pstehlik@sophics.cz>
aranym-jit (JIT compiler for m68k CPU) generated.

* Sun Oct 20 2002 Petr Stehlik <pstehlik@sophics.cz>
EmuTOS image file renamed back.

* Sat Oct 12 2002 Petr Stehlik <pstehlik@sophics.cz>
EmuTOS image file renamed. Updated for new release.

* Sun Jul 21 2002 Petr Stehlik <pstehlik@sophics.cz>
SDL GUI font and EmuTOS image files added.

* Sat Jul 20 2002 Petr Stehlik <pstehlik@sophics.cz>
Version increased.

* Thu Jun 06 2002 Petr Stehlik <pstehlik@sophics.cz>
Install path changed from /usr/local to /usr

* Mon Apr 22 2002 Petr Stehlik <pstehlik@sophics.cz>
Sound driver added

* Sun Apr 14 2002 Petr Stehlik <pstehlik@sophics.cz>
First working version
