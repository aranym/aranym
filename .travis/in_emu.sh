#!/bin/bash
if [ -z "$BINTRAY_API_KEY" ]
then
	echo "error: BINTRAY_API_KEY is undefined" >&2
fi

if ( echo $arch | grep -q i386 ); then
	cat > uname << __EOF__
		#!/bin/bash	
		/bin/uname_orig \$@ | /bin/sed 's/x86_64/i386/g'
__EOF__
	mv /bin/uname /bin/uname_orig
	chmod +x ./uname
	mv ./uname /bin/uname
fi
uname -a
apt update
cd "/home/travis/build/${TRAVIS_REPO_SLUG}"
cat > ~/.sbuildrc << __EOF__
$apt_allow_unauthenticated = 1;

# Directory for writing build logs to
$log_dir=$ENV{HOME}."/ubuntu/logs";

# don't remove this, Perl needs it:
1;
__EOF__
export LANG="en_US.UTF-8"
export LANGUAGE="en_US:en"
export LC_ALL="en_US.UTF-8"
export emu=true
apt install -y locales sudo
locale-gen en_US.UTF-8

. ./.travis/install_prerequisities.sh
. ./.travis/setup_env.sh
if ! [ "$deploy" = true ]; then
. ./.travis/build.sh
fi
if ( echo $arch | grep -q i386 ) || [ "$deploy" = true ]; then # we run deploy in emu just for building snaps
. ./.travis/deploy.sh
fi
