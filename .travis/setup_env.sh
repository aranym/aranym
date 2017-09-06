#!/bin/sh
# Use as: ". setup_env.sh"

export GITHUB_USER=$(echo "${TRAVIS_REPO_SLUG}" | cut -d '/' -f 1)
export BASE_RAW_URL="https://raw.githubusercontent.com/${GITHUB_USER}"
export PROJECT=$(echo "${TRAVIS_REPO_SLUG}" | cut -d '/' -f 2)
export SHORT_ID=$(git log -n1 --format="%h")
export PROJECT_LOWER=${PROJECT,,}

CPU=unknown
if echo "" | gcc -dM  -E - | grep -q __x86_64__; then
	CPU=x86_64
fi
if echo "" | gcc -dM  -E - | grep -q __i386__; then
	CPU=i386
fi
if echo "" | gcc -dM  -E - | grep -q "__arm.*__"; then
	CPU=arm
fi
export CPU

case "$TRAVIS_OS_NAME" in
linux)
	release_id=`lsb_release -i | sed -e 's/.*:[ \t]*//' 2>/dev/null`
	release_codename=`lsb_release -c | sed -e 's/.*:[ \t]*//' | tr '[[:upper:]]' '[[:lower:]]' 2>/dev/null`
	release_version=`lsb_release -r | sed -e 's/.*:[ \t]*//' 2>/dev/null`
	release_description=`lsb_release -d | sed -e 's/.*:[ \t]*//' 2>/dev/null`
	if test "$release_codename" = "" -o "$release_codename" = "n/a"; then
		case "$release_description" in
		*Tumbleweed*) release_codename=tumbleweed ;;
		*) release_codename="$release_version" ;;
		esac
	fi
	suse_version=`rpm --eval '%{suse_version}' 2>/dev/null`

	case $release_id in
	openSUSE)
		VENDOR=${release_id}
		archive_tag=-suse${suse_version}-${CPU}
		;;
	Debian | Ubuntu)
		VENDOR=${release_id}
		archive_tag=-${release_codename}-${CPU}
		;;
	LinuxMint)
		VENDOR=${release_id}
		archive_tag=-linuxmint${release_version%%.*}-${CPU}
		;;
	Mandriva | OpenMandrivaLinux)
		VENDOR=${release_id}
		archive_tag=-${release_codename}-${CPU}
		;;
	RHEL | CentOS)
		VENDOR=${release_id}
		archive_tag=-el${release_version%%.*}-${CPU}
		;;
	Fedora)
		VENDOR=${release_id}
		archive_tag=-fedora${release_version}-${CPU}
		;;
	*)
		VENDOR="unknown"
		archive_tag=-`uname -s 2>/dev/null`-`uname -r 2>/dev/null`
		;;
	esac
	;;

osx)
	VENDOR=darwin
	archive_tag=-macosx-x86_64_i386
	;;

esac

if test "$suse_version" = '%{suse_version}' -o "$suse_version" = ""; then suse_version=0; fi
export suse_version

export archive_tag
