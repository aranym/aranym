#!/bin/sh
# Use as: ". setup_env.sh"
export RELEASE_DATE=`date -u +%Y-%m-%dT%H:%M:%S`
export GITHUB_USER=$(echo "${TRAVIS_REPO_SLUG}" | cut -d '/' -f 1)
export BASE_RAW_URL="https://raw.githubusercontent.com/${GITHUB_USER}"
export PROJECT=$(echo "${TRAVIS_REPO_SLUG}" | cut -d '/' -f 2)
export SHORT_ID=$(git log -n1 --format="%h")
export PROJECT_LOWER=`echo ${PROJECT} | tr '[[:upper:]]' '[[:lower:]]'`

CPU_TYPE=unknown
if echo "" | gcc -dM  -E - | grep -q __x86_64__; then
	CPU_TYPE=x86_64
fi
if echo "" | gcc -dM  -E - | grep -q __i386__; then
	CPU_TYPE=i386
fi
if echo "" | gcc -dM  -E - | grep -q "__arm.*__"; then
	CPU_TYPE=arm
fi
case "$arch" in
	amd64)
		CPU_TYPE=x86_64
	;;
	*)
		CPU_TYPE=$arch
	;;
esac
export CPU_TYPE
echo "Type is $CPU_TYPE"

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
		archive_tag=-suse${suse_version}-${CPU_TYPE}
		;;
	Debian | Ubuntu)
		VENDOR=${release_id}
		archive_tag=-${release_codename}-${CPU_TYPE}
		;;
	LinuxMint)
		VENDOR=${release_id}
		archive_tag=-linuxmint${release_version%%.*}-${CPU_TYPE}
		;;
	Mandriva | OpenMandrivaLinux)
		VENDOR=${release_id}
		archive_tag=-${release_codename}-${CPU_TYPE}
		;;
	RHEL | CentOS)
		VENDOR=${release_id}
		archive_tag=-el${release_version%%.*}-${CPU_TYPE}
		;;
	Fedora)
		VENDOR=${release_id}
		archive_tag=-fedora${release_version}-${CPU_TYPE}
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

VERSION=`sed -n -e 's/#define.*VER_MAJOR.*\([0-9][0-9]*\).*$/\1./p
s/#define.*VER_MINOR.*\([0-9][0.9]*\).*$/\1./p
s/#define.*VER_MICRO.*\([0-9][0-9]*\).*$/\1/p' src/include/version.h | tr -d '\n'`
echo $VERSION
export VERSION

tag_set() {
	isrelease=false
	ATAG=${VERSION}${archive_tag}
	tag=`git tag --points-at ${TRAVIS_COMMIT}`
	case $tag in
		ARANYM_*)
			isrelease=true
			echo yes > .isrelease
			;;
		*)
			ATAG=${VERSION}${archive_tag}-${SHORT_ID}
			;;
	esac
	export ATAG
	export isrelease
}
export -f tag_set
tag_set
export ATAG
export isrelease
case $CPU_TYPE in
	i[3456]86 | x86_64 | arm*) build_jit=true ;;
	*) build_jit=false ;;
esac
if ( echo $TRAVIS_OS_NAME | grep -q linux ); then
	if [ -z "$typec" ]; then
		export ARCHIVE="${PROJECT_LOWER}-${ATAG}.tar.xz"
	else
		export ARCHIVE="${PROJECT_LOWER}-${CPU_TYPE}-${TRAVIS_COMMIT}-${typec}.tar.xz"
	fi
fi
