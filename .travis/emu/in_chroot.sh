#!/bin/bash
if [ -z "$BINTRAY_API_KEY" ]
then
	echo "error: BINTRAY_API_KEY is undefined" >&2
fi
export ar=no
uname -a
env
cd "/home/travis/build/${TRAVIS_REPO_SLUG}"
echo "LC_ALL=en_US.UTF-8" >> /etc/environment
echo "en_US.UTF-8 UTF-8" >> /etc/locale.gen
echo "LANG=en_US.UTF-8" > /etc/locale.conf
locale-gen en_US.UTF-8
. ./.travis/install_ssh_id.sh
unset SSH_ID
. ./.travis/install_prerequisities.sh
. ./.travis/setup_env.sh
. ./.travis/build.sh
. ./.travis/deploy.sh
