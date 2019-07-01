#!/bin/bash
#sudo adduser $USER sbuild
#exec sudo su -l $USER
cat > ~/.sbuildrc << __EOF__
$apt_allow_unauthenticated = 1;

# Directory for writing build logs to
$log_dir=$ENV{HOME}."/ubuntu/logs";

# don't remove this, Perl needs it:
1;
__EOF__
sudo mk-sbuild --arch=arm64 xenial
host_home="${TRAVIS_BUILD_DIR}"
chmod +x "${host_home}/.travis/emu/in_chroot.sh"
sudo schroot -p -c xenial-arm64 -u root -- bash "${host_home}/.travis/emu/in_chroot.sh"
