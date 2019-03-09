#!/bin/bash
# Creates a custom Raspbian image and cross-compilation environment.
#
# USAGE: ./create_image.sh
#
# This script must be run as either root or sudo.
#
# After creating the chroot environment, the script specified in the
# *script* variable will be executed from within the chroot. Your
# custom system setup commands should be located here. For example,
# this file may contain commands for creating new users, configuring
# the firewall, or compiling custom code.
#
# Prior to running this script, you will need the qemu,
# qemu-user-static, and binfmt-support Debian/Ubuntu packages (or
# their equivalent on other distributions). To install them on a
# Debian/Ubuntu system, use the command:
#
#     sudo apt-get install qemu qemu-user-static binfmt-support
#
# This script has been tested in Ubuntu 16.04.4 LTS.
#
# Credit for much of this script goes to Michael Daffin:
# https://disconnected.systems/blog/custom-rpi-image-with-github-travis/
#
# Kyle M. Douglass, 2018
# https://kmdouglass.github.io
#

# Setup script error handling. See
# https://disconnected.systems/blog/another-bash-strict-mode for
# details.
sudo apt-get update
sudo apt-get install -y -qq qemu qemu-user-static binfmt-support
set -uo pipefail
trap 's=$?; echo "$0: Error on line "$LINENO": $BASH_COMMAND"; exit $s' ERR
IFS=$'\n\t'

# Ensure root.
if [[ $EUID -ne 0 ]]; then
    echo "Error: This script must be run as root or sudo."
    exit 1
fi

# User-defined variables. Adjust these to your needs.
mount="/mnt/alphapi"
host_home="${TRAVIS_BUILD_DIR}"
script="/bin/uname -a"
rpi_zip="raspbian_lite_latest.zip" #do not change
rpi_url="https://downloads.raspberrypi.org/raspbian_lite_latest"
img_size="4G"
tmp_img="tmp.img"
loop_dev="/dev/loop2"

# Create the mount directory if it does not exist.
mkdir -p "${mount}"

# Download Raspbian only if we have not already done so.
wget "${rpi_url}" -O "${rpi_zip}" -q
export orig_img_name=$(unzip -l raspbian_lite_latest.zip | grep .img | awk -F" " '{print $4}')

# Tasks to run when the shell exits for any reason, unmount the image
# and general cleanup.
cleanup() {
    [[ -f "${tmp_img}" ]] && rm "${tmp_img}"
    if [[ -d "${mount}" ]]; then
        umount "${mount}/dev/pts" || true
	umount "${mount}/dev" || true
        umount "${mount}/proc" || true
        umount "${mount}/sys" || true
        umount "${mount}/boot" || true
	umount "${mount}${host_home}" || true
        umount "${mount}" || true
        rm -r "${mount}" || true
    fi
}
trap cleanup EXIT

# Extract the image.
unzip raspbian_lite_latest.zip
export root_start_sector=$(fdisk -l $orig_img_name | grep .img2 | awk -F" " '{print $2}')
export boot_start_sector=$(fdisk -l $orig_img_name | grep .img1 | awk -F" " '{print $2}')
export sector_size=$(fdisk -l $orig_img_name | grep "Sector size" | awk -F" " '{print $4}')
echo -e "Start sector: ${root_start_sector}\nSector size: ${sector_size}"

# Increase the size of the image by first appending zeros and then
# expanding the partition.
echo "Expanding the size of the image file..."
orig_size=$(du -h ${orig_img_name} | awk -F" " '{print $1}')
diff_size=$[$(numfmt --from=iec ${img_size}) - \
	    $(numfmt --from=iec ${orig_size})]
dd if=/dev/zero bs=1 count=1 seek=${diff_size} of=${tmp_img}
cat ${tmp_img} >> ${orig_img_name}
parted ${orig_img_name} resizepart 2 100%

# Mount the root image, check it, expand it, then unmount it.
losetup --offset=$[root_start_sector * sector_size] ${loop_dev} ${orig_img_name}
e2fsck -f -y ${loop_dev}
resize2fs -f ${loop_dev}
losetup -d ${loop_dev}

# Mount the images. Mount parts of the original image by using the
# offset option and the previously-determined sectors.
[ ! -d "${mount}" ] && mkdir "${mount}"
mount -o loop,offset=$[root_start_sector * sector_size] ${orig_img_name} ${mount}
[ ! -d "${mount}/boot" ] && mkdir "${mount}/boot"
mount -o loop,offset=$[boot_start_sector * sector_size] ${orig_img_name} ${mount}/boot

# Copy the image setup script.
chmod +x "${host_home}/.travis/chtest.sh"
install -Dm755 "${host_home}/.travis/chtest.sh" "${mount}${host_home}/.travis/chtest.sh"

# Prep the chroot.
mount -t proc none ${mount}/proc
mount -t sysfs none ${mount}/sys
mount -o bind /dev ${mount}/dev
mount -o bind /dev/pts ${mount}/dev/pts

# Provide network access to the chroot.
rm ${mount}/etc/resolv.conf
cp /etc/resolv.conf ${mount}/etc/resolv.conf
cp /usr/bin/qemu-arm-static ${mount}/usr/bin/

# Mount my current home directory which contains the software to build.
echo "Mounting host home directory..."
echo "Mount point: ${mount}${host_home}"
mkdir -p "${mount}${host_home}"
mount -o bind ${host_home} ${mount}${host_home}
sudo chroot ${mount} "${host_home}/.travis/chtest.sh"
