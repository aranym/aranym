#!/bin/sh -e

DIR="${1-.}"

if test -f "${DIR}/.isrelease"; then
	echo '/* generated from version.h */';
	echo '#define VERSION_DATE " " RELEASE_DATE " (release)"';
elif test -d "${DIR}/.git"; then
	echo '/* generated from git log */';
	cd "${DIR}" && git log -n 1 --pretty=format:'#define VERSION_DATE " %ad (git:%h)"%n' --date=format:%Y/%m/%d;
else
	echo '/* generated from local source */';
	echo '#define VERSION_DATE " " __DATE__ " (local)"';
fi
