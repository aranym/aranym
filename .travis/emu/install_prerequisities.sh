#!/bin/bash
sudo apt-get update
sudo apt-get install -y -qq \
	curl \
	wget \
	git \
	zsync \
	xz-utils \
	libjson-perl \
	libwww-perl
if ! ( echo $is | grep -q deploy ); then
	sudo apt-get update
	if ( echo $arch_build | grep -q i386 ); then
		sudo apt-get install build-essential sbuild debootstrap debhelper schroot ubuntu-dev-tools moreutils piuparts -y -qq
		chmod +x .travis/emu/i386_chroot.sh
		sudo adduser $USER sbuild
		echo "/home                 /home  none  rw,bind  0  0" | sudo tee -a /etc/schroot/sbuild/fstab
	fi

	echo $arch_build

	sudo apt-get install -y -qq \
		qemu \
		qemu-user-static \
		binfmt-support
	chmod +x .travis/emu/armhf_chroot.sh



fi