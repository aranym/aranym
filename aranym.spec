# generic defines used by all distributions.
#
%define ver			0.9.12

#
#
%define	myrelease		1
%define mybuild			1
%define _rel			%{myrelease}.%{mybuild}

# define the package groups. If they all followed the same naming convention,
# these would be the same. They don't, and so they aren't :(
#
%define	suse_group		System/Emulators/Others
%define	mandriva_group		Console/Emulators
%define	fedora_group		Console/Emulators

# defaults
#
%define group			Console/Emulators
%define	rel			%{_rel}

%define	my_suse			0
%define	my_mandriva		0
%define	my_fedora		0
%define	my_centos		0


%if 0%{?suse_version:1}%{?sles_version:1}
%define	my_suse			1
%endif

# if present, use %distversion to find out which Mandriva version is being built
#
%if 0%{?distversion:1}
%if 0%{?!mandriva_version:1}
%define	mandriva_version	%(echo $[%{distversion}/10])
%endif

%endif

%if 0%{?mandriva_version:1}
%define	my_mandriva		1
%define my_vendor		mandriva
%endif

# if present, decode %dist to find out which OS package is being built on
#
%if 0%{?dist:1}

# Centos or Fedora
#
%define	my_which_os		%(i=%{dist} ; if [ "${i::3}" == ".fc" ] ; then echo "1" ; else echo "0" ; fi )

%if %{my_which_os}

%if 0%{?!fedora_version:1}
%define fedora_version		%(i=%{dist} ; echo "${i:3}" )
%endif

%else

%if 0%{?!centos_version:1}
%define centos_version		%(i=%{dist} ; echo "${i:3}00" )
%endif

%endif

%endif

%if 0%{?fedora_version:1}
%define	my_fedora		1
%define my_vendor		fedora
%endif

%if 0%{?centos_version:1}
%define	my_centos		1
%define my_vendor		centos
%endif


%if %{my_suse}

%if %{suse_version}
%define	rel			%{myrelease}.suse%(echo $[%suse_version/10]).%{mybuild}
%else
%define	rel			%{myrelease}.sles%{sles_version}.%{mybuild}
%endif

%define	group			%{suse_group}

%endif


# building on a Mandriva/Mandrake Linux system.
#
# this should create a release that conforms to the Mandriva naming conventions.
#
%if %{my_mandriva}

%define rel			%{myrelease}.mdv%{mandriva_version}.%{mybuild}

%define group			%{mandriva_group}

%endif


# building on a Fedora Core Linux system.
#
# this should create a release that conforms to the Fedora naming conventions.
#
%if %{my_fedora}

%if 0%{?!fedora_version:1}
%define	fedora_version		%(i="%dist" ; echo "${i:3}")
%endif

%if 0%{?!dist:1}
%define	dist			.fc%{fedora_version}
%endif

%define	rel			%{myrelease}%{dist}.%{mybuild}
%define	group			%{fedora_group}

%endif


# building on a Centos Linux system.
#
# this should create a release that conforms to the Centos naming conventions.
#
%if %{my_centos}

%if 0%{?!centos_version:1}
%define	centos_version		%(i="%dist" ; echo "${i:3}")
%endif

%if 0%{?!dist:1}
%define	dist			.el%{centos_version}
%endif

%define	rel			%{myrelease}%{dist}.%{mybuild}
%define	group			%{fedora_group}

%endif


# move the icons into a standardised location
#
%if 0%{?!_icondir:1}
%define	_icondir		%{_datadir}/icons/hicolor/
%endif


# ensure where RPM thinks the docs should be matches reality
# gets around SUSE using %{_prefix}/share/doc/packages and
# fedora using %{_defaultdocdir}
#
%define	_docdir			%{_prefix}/share/doc

%if %{my_suse}
Requires:			libSDL-1_2-0 >= 1.2.10
Requires:			libSDL_image-1_2-0 >= 1.2.5
Requires:			zlib >= 1.2.3
Requires:			libmpfr4 >= 3.0.0
Requires:			libusb-1_0-0 >= 1.0.0
BuildRequires:			libSDL-devel >= 1.2.10
BuildRequires:			libSDL_image-devel >= 1.2.5
BuildRequires:			zlib-devel >= 1.2.3
BuildRequires:			mpfr-devel >= 3.0.0
BuildRequires:			libusb-1_0-devel >= 1.0.0
BuildRequires:			update-desktop-files
BuildRequires:			make
%endif

%if %{my_mandriva}
Requires:			libSDL >= 1.2.10
Requires:			libSDL_image >= 1.2.5
Requires:			zlib >= 1.2.3
Requires:			libmpfr >= 3.0.0
Requires:			libusb1 >= 1.0.0
BuildRequires:			libSDL-devel >= 1.2.10
BuildRequires:			libSDL_image-devel >= 1.2.5
BuildRequires:			zlib-devel >= 1.2.3
BuildRequires:			libmpfr-devel >= 3.0.0
BuildRequires:			libusb1-devel >= 1.0.0
%endif

%if %{my_fedora}
Requires:			SDL >= 1.2.10
Requires:			SDL_image >= 1.2.5
Requires:			zlib >= 1.2.3
Requires:			mpfr >= 3.0.0
Requires:			libusb1 >= 1.0.0
BuildRequires:			SDL-devel >= 1.2.10
BuildRequires:			SDL_image-devel >= 1.2.5
BuildRequires:			zlib-devel >= 1.2.3
BuildRequires:			mpfr-devel >= 3.0.0
BuildRequires:			libusb1-devel >= 1.0.0
%endif


# Now for the meat of the spec file
#
Name:			aranym
Version:		%{ver}
Release:		%{rel}
License:		GPLv2
Summary:		32-bit Atari personal computer (similar to Falcon030 but better) virtual machine
URL:			http://aranym.org/
Group:			%{group}
Source0:		http://prdownloads.sourceforge.net/aranym/%{name}_%{version}.orig.tar.gz
BuildRoot:		%{_tmppath}/%{name}-%{version}-%{release}-buildroot
Requires:		hicolor-icon-theme
BuildRequires:		hicolor-icon-theme
BuildRequires:		desktop-file-utils
BuildRequires:		gcc-c++
#Patch0:			aranym-0.9.7beta-desktop.patch


%description
ARAnyM is a software only TOS clone - a virtual machine that allows you
to run TOS, EmuTOS, FreeMiNT, MagiC and Linux-m68k operating systems
and their applications.

Authors:
Ctirad Fertr, Milan Jurik, Standa Opichal, Petr Stehlik, Johan Klockars,
Didier MEQUIGNON, Patrice Mandin and others (see AUTHORS for a full list).


%prep
%setup -q
#%%patch0


%build
# JIT only works on i586
#
%ifarch %ix86
%configure --enable-jit-compiler --enable-usbhost
%{__make} depend
%{__make}
%{__mv} aranym aranym-jit
%{__make} clean
%endif

%configure --enable-fullmmu --enable-lilo --enable-usbhost
%{__make} depend
%{__make}
%{__mv} aranym aranym-mmu
%{__make} clean

%configure --enable-usbhost
%{__make} depend
%{__make}


%install
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}/%{_mandir}/man1
mkdir -p %{buildroot}/%{_datadir}/aranym
make install DESTDIR=%{buildroot}
install -m 755 aranym-mmu %{buildroot}%{_bindir}

# should be 4755 but build fails due to SUID bit set
#
install -m 755 aratapif %{buildroot}%{_bindir}

# JIT only works on i586
#
%ifarch %ix86
install -m 755 aranym-jit %{buildroot}%{_bindir}
%endif


# add a desktop menu entry
#
mkdir -p %{buildroot}/%{_datadir}/applications
mkdir -p %{buildroot}/%{_icondir}/32x32/apps
mkdir -p %{buildroot}/%{_icondir}/48x48/apps
install -m 644 contrib/icon-32.png %{buildroot}/%{_icondir}/32x32/apps/aranym.png
install -m 644 contrib/icon-48.png %{buildroot}/%{_icondir}/48x48/apps/aranym.png

%if %{my_suse}
install -m 644 contrib/%{name}.desktop %{buildroot}/%{_datadir}/applications/%{name}.desktop
%suse_update_desktop_file -i %{name}
%endif

%if %{my_fedora}%{my_mandriva}
install -m 644 contrib/%{name}.desktop %{buildroot}%{_datadir}/applications/%{my_vendor}-%{name}.desktop
desktop-file-install                                    \
 --delete-original                                      \
 --vendor %{my_vendor}                                  \
 --dir %{buildroot}%{_datadir}/applications             \
%if %{my_fedora}
 --add-category X-Fedora                                \
%endif
 --add-category System                                  \
 --add-category Emulator                                \
 %{buildroot}/%{_datadir}/applications/%{my_vendor}-%{name}.desktop
%endif

%ifarch %ix86

%if %{my_suse}
sed -e "s/Exec=aranym/Exec=aranym-jit/g" \
 -e "s/Name=ARAnyM/Name=ARAnyM-jit/g" <%{buildroot}/%{_datadir}/applications/%{name}.desktop >%{buildroot}/%{_datadir}/applications/%{name}-jit.desktop
%endif

%if %{my_fedora}%{my_mandriva}
sed -e "s/Exec=aranym/Exec=aranym-jit/g" \
 -e "s/Name=ARAnyM/Name=ARAnyM-jit/g" <%{buildroot}/%{_datadir}/applications/%{my_vendor}-%{name}.desktop \
 >%{buildroot}/%{_datadir}/applications/%{my_vendor}-%{name}-jit.desktop
%endif
%else
%{__rm} %{buildroot}/%{_mandir}/man1/%{name}-jit.1*
%endif


%if %{my_suse}
sed -e "s/Exec=aranym/Exec=aranym-mmu/g" \
 -e "s/Name=ARAnyM/Name=ARAnyM-mmu/g" <%{buildroot}/%{_datadir}/applications/%{name}.desktop \
 >%{buildroot}/%{_datadir}/applications/%{name}-mmu.desktop
%endif

%if %{my_fedora}%{my_mandriva}
sed -e "s/Exec=aranym/Exec=aranym-mmu/g" \
 -e "s/Name=ARAnyM/Name=ARAnyM-mmu/g" <%{buildroot}/%{_datadir}/applications/%{my_vendor}-%{name}.desktop \
 >%{buildroot}/%{_datadir}/applications/%{my_vendor}-%{name}-mmu.desktop
%endif


%if %{my_mandriva}
%post
%update_menus
%endif


%if %{my_mandriva}
%postun
%clean_menus
%endif


%clean
%{__rm} -rf %{buildroot}
%{__rm} -rf %{_tmppath}/%{name}-%{version}-%{release}-buildroot


%files
%defattr(-,root,root)
%{_mandir}/man1/%{name}.1*
%{_mandir}/man1/aratapif.1*
%{_datadir}/%{name}
%{_docdir}/%{name}
%{_icondir}/32x32/apps/%{name}.png
%{_icondir}/48x48/apps/%{name}.png
%{_datadir}/applications/*%{name}.desktop
%{_bindir}/aratapif
%{_bindir}/aranym


# JIT only works on i586
#
%ifarch %ix86
%{_bindir}/aranym-jit
%{_mandir}/man1/%{name}-jit.1*
%{_datadir}/applications/*%{name}-jit.desktop
%endif


%defattr(-,root,root)
%{_bindir}/aranym-mmu
%{_mandir}/man1/%{name}-mmu.1*
%{_datadir}/applications/*%{name}-mmu.desktop


%changelog
* Sat Mar 17 2012 Petr Stehlik <pstehlik@sophics.cz> 0.9.12
New ARAnyM release.
New FPU emulation for MMU mode (using MPFR)
New Native Features enabled (PCI, USB)
New dependencies (zlib, mpfr, liusb)

* Wed May 26 2010 Petr Stehlik <pstehlik@sophics.cz> 0.9.10
New ARAnyM release.
Icons moved to icons dir.
Source archive filename follows Debian convention.

* Sat Sep 05 2009 Petr Stehlik <pstehlik@sophics.cz> 0.9.9
New ARAnyM release.

* Sat Apr 25 2009 Petr Stehlik <pstehlik@sophics.cz>
New ARAnyM release.

* Sat Nov 08 2008 Petr Stehlik <pstehlik@sophics.cz>
New ARAnyM release.

* Wed Jan 30 2008 Petr Stehlik <pstehlik@sophics.cz>
Added SDL_image to Requires:
Bumped the minimal SDL version to 1.2.10
Enabled the Requires: for mandriva (still untested)

* Tue Jan 29 2008 Petr Stehlik <pstehlik@sophics.cz>
Desktop file corrected and renamed to lowercase.
_icondir defined for Fedora. For other changes see ChangeLog.

* Mon Jan 28 2008 Petr Stehlik <pstehlik@sophics.cz>
The right icon added. Desktop file updated. Build system root changed.
New release. Version increased. Other changes in NEWS file.

* Mon Jul 09 2007 Petr Stehlik <pstehlik@sophics.cz>
New release. Version increased. Other changes in NEWS file.

* Tue Oct 11 2006 David Bolt <davjam@davjam.org>	0.9.4beta
Added an aranym.desktop file for inclusion in desktop menus.
Temporarily uses emulator.png as the menu icon.
Added bits to spec file to try and build packages for (open)SUSE, Mandriva
and Fedora Core distributions without any changes.

* Fri Sep 22 2006 Petr Stehlik <pstehlik@sophics.cz>
New release. Version increased. Other changes in NEWS file.
Thanks to David Bolt this spec file is nicely updated - does not fail
on 64bit anymore. Thanks, David!

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
