%define name	aranym
%define ver	0.9.4beta
%define rel	1
%define copy	GPL
%define joy Petr Stehlik <pstehlik@sophics.cz>
%define group	Console/Emulators
%define realname aranym-%{ver}
%define src aranym-%{ver}.tar.gz
Summary:	32-bit Atari personal computer (Falcon030/TT030) virtual machine.
Name:		%{name}
Version:	%{ver}
Release:	%{rel}
License:	%{copy}
Packager: %{joy}
URL: http://aranym.org/
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
./configure --prefix=/usr --mandir=/usr/share/man --disable-nat-debug --enable-jit-compiler --enable-nfjpeg
make
mv aranym aranym-jit
make clean
./configure --prefix=/usr --mandir=/usr/share/man --disable-nat-debug --enable-fullmmu --enable-lilo --enable-fixed-videoram --enable-nfjpeg
make
mv aranym aranym-mmu
make clean
./configure --prefix=/usr --mandir=/usr/share/man --disable-nat-debug --enable-nfjpeg
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/share/man/man1
mkdir -p $RPM_BUILD_ROOT/usr/share/aranym
make install DESTDIR=$RPM_BUILD_ROOT
install aranym $RPM_BUILD_ROOT/usr/bin
install aranym-jit $RPM_BUILD_ROOT/usr/bin
install aranym-mmu $RPM_BUILD_ROOT/usr/bin
install aratapif $RPM_BUILD_ROOT/usr/bin

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%attr(4755,root,root) /usr/bin/aratapif
/usr/bin/aranym
/usr/bin/aranym-jit
/usr/bin/aranym-mmu
/usr/share/man/man1/aranym.1.gz
/usr/share/aranym
/usr/share/doc/aranym

%changelog
* Fri Sep 22 2006 Petr Stehlik <pstehlik@sophics.cz>
New release. Version increased. Other changes in NEWS file.

* Mon Feb 20 2006 Petr Stehlik <pstehlik@sophics.cz>
URL changed to aranym.org. Version increased. Other changes in NEWS file.

* Sun Apr 17 2005 Petr Stehlik <pstehlik@sophics.cz>
Files list fixed.

* Thu Apr 14 2005 Petr Stehlik <pstehlik@sophics.cz>
Version increased. NFJPEG enabled.

* Tue Feb 22 2005 Petr Stehlik <pstehlik@sophics.cz>
Version increased. aranymrc.example removed.

* Sun Feb 20 2005 Petr Stehlik <pstehlik@sophics.cz>
Version increased. LILO enabled in the MMU version.

* Sun Nov 07 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased.

* Tue Jul 06 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased.

* Mon Jul 05 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased. tools/createdisk/ removed. tools/arabridge added.
For other changes see the NEWS file.

* Sun Feb 15 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased. For other changes see the NEWS file.

* Sun Feb 08 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased. For other changes see the NEWS file.

* Wed Jan 07 2004 Petr Stehlik <pstehlik@sophics.cz>
Version increased. For other changes see the NEWS file.

* Sat Jan 03 2004 Petr Stehlik <pstehlik@sophics.cz>
font8.bmp removed.

* Sat Oct 04 2003 Petr Stehlik <pstehlik@sophics.cz>
Version increased. NFCDROM.BOS added.

* Fri Apr 11 2003 Petr Stehlik <pstehlik@sophics.cz>
Man dir fixed. Debug info disabled.

* Mon Apr 08 2003 Petr Stehlik <pstehlik@sophics.cz>
Various fixes for the 0.8.0. And full 68040 PMMU build added as aranym-mmu.
Also manual page added.

* Mon Mar 24 2003 Petr Stehlik <pstehlik@sophics.cz>
HostFS and network drivers added. ARATAPIF installed setuid root.
Ethernet enabled.

* Sun Mar 23 2003 Petr Stehlik <pstehlik@sophics.cz>
Version increased for the new release. See the NEWS file for details.

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
