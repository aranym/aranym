#!/bin/sh

# ***********************************
# * Plex86 Empty Disk Image Utility *
# *                                 *
# *               by                *
# *                                 *
# *          Eric Laberge           *
# *                                 *
# *          v. 20001125            *
# ***********************************

# *** Standard CHS, limit 504MB ***

MAX_C=1024
MAX_H=16
MAX_S=63

B_PER_S=512
MAX_SIZE=`expr \( $MAX_C \* $MAX_H \* $MAX_S \* $B_PER_S \) / 1024`

# $1 : File, $2 : Size

if [ -z "$1" ] || [ -z "$2" ] || [ `echo $2 | grep [\^0-9] | wc -l` -gt 0 ]
then
  echo "Usage:"
  echo "$0 file size [mbrdata]"
  echo "  file    : File name"
  echo "  size    : Requested size (MB)"
  echo "  mbrdata : MBR contents to be used or some invalid value to force not writing it"
  exit 1
fi

if [ -n "$3" ]
then
  MBR_DATA="$3"
else
  MBR_DATA="mbrdata"
fi

REQ_SIZE=`expr $2 \* 1024`

if [ $REQ_SIZE -gt $MAX_SIZE ] && [ -f "$MBR_DATA" ]
then
  echo "Requested size is too big!"
  echo "You might want to create an empty image file without MBR:"
  echo "by using: \"$0 $1 $2 dummy\""
  exit -2
fi

SIZE=`expr \( $REQ_SIZE / $B_PER_S \) \* 1024`
C=`expr $SIZE / \( $MAX_H \* $MAX_S \)`
H=`expr $MAX_H`
S=`expr $MAX_S`
BLOCKS=`expr $C \* $H \* $S`
SIZE=`expr $BLOCKS \* $B_PER_S`

SEEKBLOCKS=`expr $BLOCKS \- 1`
dd if=/dev/zero of=$1 bs=$B_PER_S seek=$SEEKBLOCKS count=1 2> /dev/null

echo "# Disk Geometry:"
echo "# C: $C"
echo "# H: $H"
echo "# S: $S"
echo "# Total size: $SIZE bytes"

if [ -f "$MBR_DATA" ]
then
  dd if=$MBR_DATA of=$1 bs=$B_PER_S count=1 conv=notrunc 2> /dev/null
  echo "# Wrote master boot record from file $MBR_DATA"
fi
echo

echo "# You should add the following lines to your ARAnyM config:"
echo "[ID0]"
echo " Present = Yes"
echo " IsCDROM = No"
echo " Path = $1"
echo " Cylinders = $C" 
echo " Heads = $H"
echo " SectorsPerTrack = $S" 
echo " ByteSwap = No"
  
exit 0

