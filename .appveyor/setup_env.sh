#!/bin/sh
# Use as: ". setup_env.sh"

export GITHUB_USER=$(echo "${APPVEYOR_REPO_NAME}" | cut -d '/' -f 1)
export BASE_RAW_URL="https://raw.githubusercontent.com/${GITHUB_USER}"
export PROJECT=$(echo "${APPVEYOR_REPO_NAME}" | cut -d '/' -f 2)
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
export CPU_TYPE

archive_tag=-cygwin-${CPU_TYPE}

export archive_tag
