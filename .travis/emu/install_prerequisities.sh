#!/bin/bash
sudo apt-get update
sudo apt-get install -y -qq \
	curl \
	git \
	zsync \
	xz-utils \
	libjson-perl \
	libwww-perl
if ! ( echo $is | grep -q deploy ); then
sudo apt-get update
sudo apt-get install -y -qq \
    qemu \
    qemu-user-static \
    binfmt-support
chmod +x .travis/emu/chroot.sh
fi