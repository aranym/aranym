#!/bin/bash
# This script deploys the built binaries to bintray:
# https://bintray.com/aranym/aranym-files

# Bintray needs an api key for access as password.
# This must have been set as environment variable BINTRAY_API_KEY
# in the settings page of your travis account.
# You will find the key in your profile on bintray.

if [ -z "$BINTRAY_API_KEY" ]
then
	echo "error: BINTRAY_API_KEY is undefined" >&2
	exit 1
fi

if [ -z "$SNAP_TOKEN" ]
then
	echo "error: SNAP_TOKEN is undefined" >&2
	exit 1
fi
if [ -z "$ARCHIVE" ]
then
	if [ -z "$typec" ]; then
		export ARCHIVE="${PROJECT_LOWER}-${ATAG}.tar.xz"
	else
		export ARCHIVE="${PROJECT_LOWER}-${CPU_TYPE}-${TRAVIS_COMMIT}-${typec}.tar.xz"
	fi
fi
export SRCDIR="${TRAVIS_BUILD_DIR}"
# variables
RELEASE_DATE=`date -u +%Y-%m-%dT%H:%M:%S`
BINTRAY_HOST=https://api.bintray.com
BINTRAY_USER="${BINTRAY_USER:-aranym}"
BINTRAY_REPO_OWNER="${BINTRAY_REPO_OWNER:-$BINTRAY_USER}" # owner and user not always the same
BINTRAY_REPO_NAME="${BINTRAY_REPO_NAME:-aranym-files}"
BINTRAY_REPO="${BINTRAY_REPO_OWNER}/${BINTRAY_REPO_NAME}"
SNAP_STORE_NAME="${SNAP_STORE_NAME:-aranym}"

function bined {
	cd ${SRCDIR}
	OUT="${SRCDIR}/.travis/out"
	# dirty hack for removing premisions
	# see https://github.com/aranym/aranym/issues/12#issuecomment-473660193
	mkdir bined
	cp "$OUT/${ARCHIVE}" bined
	cd bined
	tar xf ${ARCHIVE}
	rm ${ARCHIVE}
	sudo chmod -Rf 777 .
	tar cvfJ "../bined.tar.xz" .
	cd ${SRCDIR}
}

function snap_install {
	if ( echo $arch | grep -q i386 ); then
		mv /bin/uname_orig /bin/uname
	fi
	apt install -y \
      	curl \
      	jq \
      	squashfs-tools\
		snapd
	case "$CPU_TYPE" in
		x86_64)
			snap_cpu=amd64
		;;
		i386)
			snap_cpu=i386
		;;
		armhf)
			snap_cpu=armhf
		;;
		aarch)
			snap_cpu=arm64
		;;
		*)
			echo "Wrong arch in deploy for snap"
		;;
	esac
	export SNAP_ARCH="$snap_cpu"
	export PATH="/snap/bin:$PATH"
	export SNAP="/snap/snapcraft/current"
	export SNAP_NAME="snapcraft"
	export SNAP_VERSION="$(awk '/^version:/{print $2}' $SNAP/meta/snap.yaml)"
	# Grab the core snap (for backwards compatibility) from the stable channel and
	# unpack it in the proper place.
	curl -L $(curl -H 'X-Ubuntu-Series: 16' -H "X-Ubuntu-Architecture: ${SNAP_ARCH}" 'https://api.snapcraft.io/api/v1/snaps/details/core' | jq '.download_url' -r) --output core.snap
	mkdir -p /snap/core
	unsquashfs -d /snap/core/current core.snap
	# Grab the core18 snap (which snapcraft uses as a base) from the stable channel
	# and unpack it in the proper place
	curl -L $(curl -H 'X-Ubuntu-Series: 16' -H "X-Ubuntu-Architecture: ${SNAP_ARCH}" 'https://api.snapcraft.io/api/v1/snaps/details/core18' | jq '.download_url' -r) --output core18.snap
	mkdir -p /snap/core18
	unsquashfs -d /snap/core18/current core18.snap
	# Grab the snapcraft snap from the edge channel and unpack it in the proper
	# place.
	curl -L $(curl -H 'X-Ubuntu-Series: 16' -H "X-Ubuntu-Architecture: ${SNAP_ARCH}" 'https://api.snapcraft.io/api/v1/snaps/details/snapcraft?channel=stable' | jq '.download_url' -r) --output snapcraft.snap
	mkdir -p /snap/snapcraft
	unsquashfs -d /snap/snapcraft/current snapcraft.snap
	# Create a snapcraft runner
	mkdir -p /snap/bin
	echo "#!/bin/sh" > /snap/bin/snapcraft
	snap_version="$(awk '/^version:/{print $2}' /snap/snapcraft/current/meta/snap.yaml)" && echo "export SNAP_VERSION=\"$snap_version\"" >> /snap/bin/snapcraft
	echo 'exec "$SNAP/usr/bin/python3" "$SNAP/bin/snapcraft" "$@"' >> /snap/bin/snapcraft
	chmod +x /snap/bin/snapcraft
}

function snap_build {
	cd ${SRCDIR}
	case "$CPU_TYPE" in
		x86_64)
			snap_cpu=amd64
		;;
		i386)
			snap_cpu=i386
		;;
		armhf)
			snap_cpu=armhf
		;;
		aarch)
			snap_cpu=arm64
			sed -i '64,90d' snap/snapcraft.yaml # no jit in aarch64
		;;
		*)
			echo "Wrong arch in deploy for snap"
		;;
	esac
	sed -i "0,/aranym/ s/aranym/${SNAP_STORE_NAME}/" snap/snapcraft.yaml
	sed -i "0,/version:/ s/.*version.*/version: $VERSION/" snap/snapcraft.yaml
	snapcraft --destructive-mode --target-arch=$snap_cpu
}

function snap_push {
	cd ${SRCDIR}
	case "$CPU_TYPE" in
		x86_64)
			snap_cpu=amd64
		;;
		i386)
			snap_cpu=i386
		;;
		armhf)
			snap_cpu=armhf
		;;
		aarch)
			snap_cpu=arm64
		;;
		*)
			echo "Wrong arch in deploy for snap"
		;;
	esac
	echo "$SNAP_TOKEN" | snapcraft login --with -
	snapcraft push --release=edge "${SNAP_STORE_NAME}_${VERSION}_${snap_cpu}.snap"
	if $isrelease; then
		echo "Stable release on Snap"
		export revision=$(snapcraft status $SNAP_STORE_NAME --arch $snap_cpu | grep "edge" | awk '{print $NF}')
		snapcraft release $SNAP_STORE_NAME $revision stable
	fi
}

function normal_deploy {
	#SRCDIR="${PWD}"
	OUT="${SRCDIR}/.travis/out"

	if [ "${TRAVIS_PULL_REQUEST}" != "false" ];
		then
			BINTRAY_DIR=pullrequests
			BINTRAY_DESC="[${TRAVIS_REPO_SLUG}] Download: https://dl.bintray.com/${BINTRAY_REPO}/${BINTRAY_DIR}/${ARCHIVE}"
		else
			BINTRAY_DIR=snapshots
			BINTRAY_DESC="[${PROJECT}] [${TRAVIS_BRANCH}] Commit: https://github.com/${GITHUB_USER}/${PROJECT}/commit/${TRAVIS_COMMIT}"
	fi

	# use the commit id as 'version' for bintray
	BINTRAY_VERSION=$TRAVIS_COMMIT

	echo "Deploying $ARCHIVE to ${BINTRAY_HOST}/${BINTRAY_REPO}"
	echo "See result at https://bintray.com/${BINTRAY_REPO}/${BINTRAY_DIR}#files"

	# See https://bintray.com/docs/api for a description of the REST API
	# in their terminology:
	# - :subject is the owner of the account (aranym in our case)
	# - :repo is aranym-files
	# - :package either snapshots, releases or pullrequests
	# - for snapshot builds, the commit id is used as version number

	CURL="curl --silent -u ${BINTRAY_USER}:${BINTRAY_API_KEY} -H Accept:application/json -w \n"

	cd "$OUT"

	#create version:
	echo "creating version ${BINTRAY_DIR}/${BINTRAY_VERSION}"
	# do not fail if the version exists;
	# it might have been created already by other CI scripts
	$CURL --data '{"name":"'"${BINTRAY_VERSION}"'","released":"'"${RELEASE_DATE}"'","desc":"'"${BINTRAY_DESC}"'","published":true}' --header 'Content-Type: application/json' "${BINTRAY_HOST}/packages/${BINTRAY_REPO}/${BINTRAY_DIR}/versions"
	echo ""

	#upload file(s):
	echo "upload ${BINTRAY_DIR}/${ARCHIVE}"
	$CURL --upload "${ARCHIVE}" "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/${BINTRAY_DIR}/${ARCHIVE}?publish=1&override=1&explode=0"
	for file in `ls *.AppImage* 2>/dev/null`; do
		echo "upload ${BINTRAY_DIR}/${file}"
		$CURL --upload "${file}" "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/${BINTRAY_DIR}/${file}?publish=1&override=1&explode=0" || exit 1
	done
	echo ""

	# publish the version
	echo "publish ${BINTRAY_DIR}/${BINTRAY_VERSION}"
	$CURL --data '' "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/publish?publish_wait_for_secs=-1" || exit 1
	echo ""

	# purge old snapshots
	if ( echo $arch | grep -q amd64 ); then
		perl "$SRCDIR/.travis/purge-snapshots.pl"
	fi

	if $isrelease; then

		echo "celebrating new release ${VERSION}"
		BINTRAY_DIR=releases
		BINTRAY_VERSION=$VERSION

	#create version:
		echo "creating version ${BINTRAY_DIR}/${BINTRAY_VERSION}"
		$CURL --data '{"name":"'"${BINTRAY_VERSION}"'","released":"'"${RELEASE_DATE}"'","desc":"'"${BINTRAY_DESC}"'","published":true}' --header 'Content-Type: application/json' "${BINTRAY_HOST}/packages/${BINTRAY_REPO}/${BINTRAY_DIR}/versions"
		echo ""
		
	#upload file(s):
		# we only need to upload the src archive once
		if ( echo $arch | grep -q amd64 ); then
			for ext in gz bz2 xz lz; do
				SRCARCHIVE="${PROJECT_LOWER}-${VERSION}.tar.${ext}"
				if test -f "${SRCARCHIVE}"; then
					echo "upload ${BINTRAY_DIR}/${VERSION}/${SRCARCHIVE}"
					$CURL --upload "${SRCARCHIVE}" "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/${VERSION}/${PROJECT_LOWER}-${VERSION}.orig.tar.${ext}?publish=1&override=0&explode=0"
					echo ""
				fi
			done
		fi
		echo "upload ${BINTRAY_DIR}/${ARCHIVE}"
		$CURL --upload "${ARCHIVE}" "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/${VERSION}/${ARCHIVE}?publish=1?override=0?explode=0"
		
		# publish the version
		echo "publish ${BINTRAY_DIR}/${BINTRAY_VERSION}"
		$CURL --data '' "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/publish?publish_wait_for_secs=-1"
		echo ""
	fi
}

function cache_deploy {
	SRCDIR="${PWD}"
	OUT="${SRCDIR}/.travis/out"

	BINTRAY_DIR=temp
	BINTRAY_DESC="!!!!! _DO NOT USE IT_ !!!!! [${TRAVIS_REPO_SLUG}] [${PROJECT}] [${TRAVIS_BRANCH}]"


	# use the commit id as 'version' for bintray
	BINTRAY_VERSION=$TRAVIS_COMMIT

	echo "Deploying $ARCHIVE to ${BINTRAY_HOST}/${BINTRAY_REPO}"
	echo "See result at ${BINTRAY_HOST}/${BINTRAY_REPO}/${BINTRAY_DIR}#files"

	# See https://bintray.com/docs/api for a description of the REST API
	# in their terminology:
	# - :subject is the owner of the account (aranym in our case)
	# - :repo is aranym-files
	# - :package either snapshots, releases or pullrequests
	# - for snapshot builds, the commit id is used as version number

	CURL="curl --silent -u ${BINTRAY_USER}:${BINTRAY_API_KEY} -H Accept:application/json -w \n"

	cd "$OUT"

	#create version:
	echo "creating version ${BINTRAY_DIR}/${BINTRAY_VERSION}"
	# do not fail if the version exists;
	# it might have been created already by other CI scripts
	$CURL --data '{"name":"'"${BINTRAY_VERSION}"'","released":"'"${RELEASE_DATE}"'","desc":"'"${BINTRAY_DESC}"'","published":true}' --header 'Content-Type: application/json' "${BINTRAY_HOST}/packages/${BINTRAY_REPO}/${BINTRAY_DIR}/versions"
	echo ""

	#upload file(s):
	echo "upload ${BINTRAY_DIR}/${ARCHIVE}"
	$CURL --upload "${ARCHIVE}" "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/${BINTRAY_DIR}/${ARCHIVE}?publish=1&override=1&explode=0"
	echo ""

	# publish the version
	echo "publish ${BINTRAY_DIR}/${BINTRAY_VERSION}"
	$CURL --data '' "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/publish?publish_wait_for_secs=-1" || exit 1
	echo ""
}

function get_cache {
	SRCDIR="${PWD}"
	OUT="${SRCDIR}/.travis/out"
	TMP="${SRCDIR}/.travis/tmp"
	mkdir -p $OUT
	mkdir -p $TMP
	(
		cd "${TMP}"
		# firstly merge builds
		for build_type in jit mmu nor; do
			echo "get $build_type";
			TARCHIVE="${PROJECT_LOWER}-${CPU_TYPE}-${TRAVIS_COMMIT}-${build_type}.tar.xz"
			echo "url: https://dl.bintray.com/${BINTRAY_REPO}/temp/${TARCHIVE}"
			curl -L "https://dl.bintray.com/${BINTRAY_REPO}/temp/${TARCHIVE}" -o ${TARCHIVE}
			tar xf ${TARCHIVE}
			rm ${TARCHIVE}
		done;
	)
	# and package all builds.
	export ARCHIVE="${PROJECT_LOWER}-${ATAG}.tar.xz"
	(
		cd "${TMP}"
		sudo chown root "usr/bin/aratapif"
		sudo chgrp root "usr/bin/aratapif"
		sudo chmod 4755 "usr/bin/aratapif"
		tar cvfJ "${OUT}/${ARCHIVE}" .
	)
}

# build snap in emu
# Check if it is deploy or build job
if ! [ "$deploy" = true ]; then # build job
	case "$TRAVIS_OS_NAME" in
		linux) # if linux
		case "$arch" in
			arm*)
				echo "------------ cache ------------"
				cache_deploy
			;;
			i386)
				echo "------------ normal ------------"
				normal_deploy
				bined
				snap_install
				snap_build
				snap_push
			;;
			*)
			echo "------------ normal ------------"
				normal_deploy
				bined
				snap_build
				snap_push
			;;
		esac
		;;
		osx) # directly push
			normal_deploy
		;;
		*)
			echo "wrong TRAVIS_OS_NAME"
			exit 1;
		;;
	esac
else # deploy job
	echo "------------ deploy ------------"
	get_cache
	normal_deploy
	bined
	snap_install
	snap_build
	snap_push
fi
