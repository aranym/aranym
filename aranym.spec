%define name	aranym
%define ver	0.1.7
%define rel	3
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
ARAnyM is an acronym and means Atari Running on Any Machine.

Authors:
Ctirad Fertr, Milan Jurik, Standa Opichal, Petr Stehlik
and others (see AUTHORS for a full list)

%prep
rm -rf %{realname}

%setup -n %{realname}/src/Unix
./autogen.sh || echo "Autogen put out its usual complaint, ignored!"
#%patch -p1

%build
./configure
make
make clean

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/local/bin
mkdir -p $RPM_BUILD_ROOT/usr/local/share
mkdir -p $RPM_BUILD_ROOT/usr/local/share/aranym
make install DESTDIR=$RPM_BUILD_ROOT
install aranym $RPM_BUILD_ROOT/usr/local/bin

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%doc ../../doc ../../AUTHORS ../../BUGS ../../COPYING ../../FAQ ../../INSTALL ../../NEWS ../../ChangeLog ../../README ../../TODO
/usr/local/bin/aranym
/usr/local/share/aranym/atari/newdesk.inf
/usr/local/share/aranym/atari/aranymrc.example
/usr/local/share/aranym/atari/mmusetup.cnf
/usr/local/share/aranym/atari/aranymfs/aranymfs.dos.bz2
/usr/local/share/aranym/atari/aranymfs/aranym.xfs.bz2
/usr/local/share/aranym/atari/aranymfs/config.sys
/usr/local/share/aranym/createdisk/createdisk.README
/usr/local/share/aranym/createdisk/createdisk.sh
/usr/local/share/aranym/createdisk/mbrdata
/usr/local/share/aranym/atari/fvdi/aranym.sys.bz2
/usr/local/share/aranym/atari/fvdi/fvdi.prg.bz2
/usr/local/share/aranym/atari/fvdi/fvdi.sys
/usr/local/share/aranym/atari/tools/README
/usr/local/share/aranym/atari/tools/clocky.prg.bz2
/usr/local/share/aranym/atari/tools/fastram.prg.bz2
/usr/local/share/aranym/atari/tools/pc101us.kbd.bz2
/usr/local/share/aranym/atari/tools/pcpatch.prg.bz2
/usr/local/share/aranym/atari/sound/zmagxsnd.prg.bz2

%changelog
* Mon Apr 22 2002 Petr Stehlik <pstehlik@sophics.cz>
Sound driver added

* Sun Apr 14 2002 Petr Stehlik <pstehlik@sophics.cz>
First working version
