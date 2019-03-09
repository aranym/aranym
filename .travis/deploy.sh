#!/bin/bash
if ! ( echo $ar | grep -q arm ); then # if arm do not exec
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

# variables
RELEASE_DATE=`date -u +%Y-%m-%dT%H:%M:%S`
BINTRAY_HOST=https://api.bintray.com
BINTRAY_USER="${BINTRAY_USER:-aranym}"
BINTRAY_REPO_OWNER="${BINTRAY_REPO_OWNER:-$BINTRAY_USER}" # owner and user not always the same
BINTRAY_REPO_NAME="${BINTRAY_REPO_NAME:-aranym-files}"
BINTRAY_REPO="${BINTRAY_REPO_OWNER}/${BINTRAY_REPO_NAME}"

function normal_deploy {
	SRCDIR="${PWD}"
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
	if test "$TRAVIS_OS_NAME" = linux; then
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
		if test "$TRAVIS_OS_NAME" = linux; then
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

function uncache_deploy {
	SRCDIR="${PWD}"
	OUT="${SRCDIR}/.travis/out"
	mkdir -p $OUT
	(
		cd "${BUILDROOT}"
		# firstly merge builds
		for build_type in jit mmu nor; do
			echo "get $build_type";
			TARCHIVE="${PROJECT_LOWER}-${TRAVIS_COMMIT}-${build_type}.tar.xz"
			echo "url: https://dl.bintray.com/${BINTRAY_REPO}/temp/${TARCHIVE}"
			curl -L "https://dl.bintray.com/${BINTRAY_REPO}/temp/${TARCHIVE}" -o ${TARCHIVE}
			tar xf ${TARCHIVE}
			rm ${TARCHIVE}
		done;
	)
	# and package all builds.
	export ARCHIVE="${PROJECT_LOWER}-${ATAG}.tar.xz"
	(
		cd "${BUILDROOT}"
		tar cvfJ "${OUT}/${ARCHIVE}" .
	)

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

	if $isrelease; then

		echo "celebrating new release ${VERSION}"
		BINTRAY_DIR=releases
		BINTRAY_VERSION=$VERSION

	#create version:
		echo "creating version ${BINTRAY_DIR}/${BINTRAY_VERSION}"
		$CURL --data '{"name":"'"${BINTRAY_VERSION}"'","released":"'"${RELEASE_DATE}"'","desc":"'"${BINTRAY_DESC}"'","published":true}' --header 'Content-Type: application/json' "${BINTRAY_HOST}/packages/${BINTRAY_REPO}/${BINTRAY_DIR}/versions"
		echo ""
		
		echo "upload ${BINTRAY_DIR}/${ARCHIVE}"
		$CURL --upload "${ARCHIVE}" "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/${VERSION}/${ARCHIVE}?publish=1?override=0?explode=0"
		
		# publish the version
		echo "publish ${BINTRAY_DIR}/${BINTRAY_VERSION}"
		$CURL --data '' "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/publish?publish_wait_for_secs=-1"
		echo ""
	fi
}

# Check if it is deploy or build job
if ! ( echo $is | grep -q deploy ); then # build job
	case "$TRAVIS_OS_NAME" in
		linux) # if linux use cache
			if ! ( echo $ar | grep -q no ); then # Except if is not arm linux
				echo "------------ normal --------------------"
				normal_deploy
			else # if arm finally use cache
				echo "------------ cache --------------------"
				cache_deploy
			fi
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
	echo "------------ deploy --------------------"
	uncache_deploy
fi
fi
