#!/bin/bash

# ***********************************
# * Plex86 Empty Disk Image Utility *
# *                                 *
# *               by                *
# *                                 *
# *          Eric Laberge           *
# *                                 *
# *          v. 20001125            *
# ***********************************

MBR_DATA="mbrdata"

# *** Standard CHS, limit 504MB ***

let "MAX_C = 1024"
let "MAX_H = 16"
let "MAX_S = 63"

let "B_PER_S = 512"
let "MAX_SIZE = $MAX_C * $MAX_H * $MAX_S * $B_PER_S"

# $1 : File, $2 : Size

if [ -z $1 ] || [ -z $2 ] || [ `echo $2 | grep [^0-9] | wc -l` -gt 0 ]
then
  echo "Usage:"
  echo "$0 file size"
  echo "  file : File name"
  echo "  size : Requested size (MB)"
  exit 1
fi

let "REQ_SIZE = $2 * 1024 * 1024"

if [ $REQ_SIZE -gt $MAX_SIZE ]
then
  echo "Requested size is too big!"
  exit -2
fi

let "SIZE = $REQ_SIZE / $B_PER_S"
let "C = $SIZE / ($MAX_H * $MAX_S)"
let "H = $MAX_H"
let "S = $MAX_S"
let "BLOCKS = $C * $H * $S"
let "SIZE = $BLOCKS * $B_PER_S"

echo "Disk Geometry:"
echo "C: $C"
echo "H: $H"
echo "S: $S"
echo "Total size: $SIZE bytes"

dd if=/dev/zero of=$1 bs=$B_PER_S count=$BLOCKS 2> /dev/null

if [ -e $MBR_DATA ]
then
  dd if=$MBR_DATA of=$1 bs=$B_PER_S count=1 conv=notrunc 2> /dev/null
  echo "Wrote master boot record from file $MBR_DATA"
fi

exit 0


