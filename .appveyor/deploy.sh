# This script deploys the built binaries to bintray:
# https://bintray.com/aranym/aranym-files

# Bintray needs an api key for access as password.
# This must have been set as environment variable BINTRAY_API_KEY
# in the settings page of your AppVeyor account.
# You will find the key in your profile on bintray.

if [ -z "$BINTRAY_API_KEY" ]
then
	echo "error: BINTRAY_API_KEY is undefined" >&2
	exit 1
fi

SRCDIR="${PWD}"
OUT="${TMPDIR:-/tmp}"

# variables
RELEASE_DATE=`date -u +%Y-%m-%dT%H:%M:%S`
BINTRAY_HOST=https://api.bintray.com
BINTRAY_USER="${BINTRAY_USER:-aranym}"
BINTRAY_REPO_OWNER="${BINTRAY_REPO_OWNER:-$BINTRAY_USER}" # owner and user not always the same
BINTRAY_REPO="${BINTRAY_REPO_OWNER}/${BINTRAY_REPO:-aranym-files}"

if [ "${APPVEYOR_PULL_REQUEST_NUMBER}" != "" ]
then
	BINTRAY_DIR=pullrequests
	BINTRAY_DESC="[${APPVEYOR_REPO_NAME}] Download: https://dl.bintray.com/${BINTRAY_REPO}/${BINTRAY_DIR}/${ARCHIVE}"
else
	BINTRAY_DIR=snapshots
	BINTRAY_DESC="[${PROJECT}] [${APPVEYOR_REPO_BRANCH}] Commit: https://github.com/${GITHUB_USER}/${PROJECT}/commit/${APPVEYOR_REPO_COMMIT}"
fi

# use the commit id as 'version' for bintray
BINTRAY_VERSION=$APPVEYOR_REPO_COMMIT

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
for sdlname in -sdl1 -sdl2; do
	ARCHIVE="${PROJECT_LOWER}-${ATAG}${sdlname}.zip"
	SETUP_EXE="${PROJECT_LOWER}-${ATAG}${sdlname}-setup.exe"
	echo "upload ${BINTRAY_DIR}/${ARCHIVE}"
	$CURL --upload "${ARCHIVE}" "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/${BINTRAY_DIR}/${ARCHIVE}?publish=1&override=1&explode=0"
	echo "upload ${BINTRAY_DIR}/${SETUP_EXE}"
	$CURL --upload "${SETUP_EXE}" "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/${BINTRAY_DIR}/${SETUP_EXE}?publish=1&override=1&explode=0"
done
echo ""

# publish the version
echo "publish ${BINTRAY_DIR}/${BINTRAY_VERSION}"
$CURL --data '' "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/publish?publish_wait_for_secs=-1" || exit 1
echo ""

# no need to purge old snapshots here,
# or build a source archive for releases;
# this was already done by the travis script

if $isrelease; then

	echo "celebrating new release ${VERSION}"
	BINTRAY_DIR=releases
	BINTRAY_VERSION=$VERSION

#create version:
	echo "creating version ${BINTRAY_DIR}/${BINTRAY_VERSION}"
	$CURL --data '{"name":"'"${BINTRAY_VERSION}"'","released":"'"${RELEASE_DATE}"'","desc":"'"${BINTRAY_DESC}"'","published":true}' --header 'Content-Type: application/json' "${BINTRAY_HOST}/packages/${BINTRAY_REPO}/${BINTRAY_DIR}/versions"
	echo ""
	
#upload file(s):
	for sdlname in -sdl1 -sdl2; do
		ARCHIVE="${PROJECT_LOWER}-${ATAG}${sdlname}.zip"
		SETUP_EXE="${PROJECT_LOWER}-${ATAG}${sdlname}-setup.exe"
		echo "upload ${BINTRAY_DIR}/${ARCHIVE}"
		$CURL --upload "${ARCHIVE}" "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/${VERSION}/${ARCHIVE}?publish=1?override=0?explode=0"
		echo "upload ${BINTRAY_DIR}/${SETUP_EXE}"
		$CURL --upload "${SETUP_EXE}" "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/${VERSION}/${SETUP_EXE}?publish=1?override=0?explode=0"
	done
			
# publish the version
	echo "publish ${BINTRAY_DIR}/${BINTRAY_VERSION}"
	$CURL --data '' "${BINTRAY_HOST}/content/${BINTRAY_REPO}/${BINTRAY_DIR}/${BINTRAY_VERSION}/publish?publish_wait_for_secs=-1"
	echo ""

fi
