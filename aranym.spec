%define name	aranym
%define ver	0.1.6
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
and others (see CREDITS for a full list)

%prep
rm -rf %{realname}

%setup -n %{realname}/src/Unix
./autogen.sh || echo "Autogen put out its usual complaint, ignored!"
#%patch -p1

%build
./configure
make
make clean
done
touch aranym

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/local/bin
mkdir -p $RPM_BUILD_ROOT/usr/local/man/man1
make install PREFIX=$RPM_BUILD_ROOT/usr/local
install aranym $RPM_BUILD_ROOT/usr/local/bin

%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root)
%doc ../../doc ../../README ../../COPYING
/usr/local/bin/aranym
/usr/local/man/man1/aranym.1

%changelog
* Sun Apr 14 2002 Petr Stehlik <pstehlik@sophics.cz>
First (working) version
