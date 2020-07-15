#!/bin/bash
sudo apt-get update
sudo apt-get install -y -qq \
	curl \
	wget \
	git \
	zsync \
	xz-utils \
	libjson-perl \
	libwww-perl \
	lsb-release
sudo service docker stop
sudo dockerd --experimental &> /dev/null &
##docker run --rm --privileged docker/binfmt:a7996909642ee92942dcd6cff44b9b95f08dad64
docker run --rm --privileged multiarch/qemu-user-static --reset -p yes
## https://stackoverflow.com/questions/61083200/docker-poorly-formatted-environment-variable-contains-whitespaces
>.env
for var in $(compgen -v | grep -Ev '^(BASH)'); do
    var_fixed=$(printf "%s" "${!var}" | tr -d '\n' )
    echo "$var=${var_fixed}" >>.env
done
case "$arch" in
	i386)
		docker run --rm --env-file .env \
			-v "/home/travis":"/home/travis" -w "${PWD}" \
			i386/ubuntu:16.04 "${TRAVIS_BUILD_DIR}/.travis/in_emu.sh"
	;;
	armhf)
		docker run --rm --env-file .env \
			-v "/home/travis":"/home/travis" -w "${PWD}" \
			arm32v7/ubuntu:16.04 "${TRAVIS_BUILD_DIR}/.travis/in_emu.sh"
	;;
esac
sudo chmod -Rf 777 /home/travis