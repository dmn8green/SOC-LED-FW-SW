#!/usr/bin/env bash

sed=$(which sed)
[[ "$(uname)" = "Darwin" ]] && sed=$(which gsed)

echo $sed

# parse release from rev.h
rev=$(/bin/cat main/rev.h)
major=$(echo "$rev" | grep MAJOR\  | $sed 's/#[a-zA-Z _]*MAJOR\s*\([0-9]*\)/\1/')
echo $major
minor=$(echo "$rev" | grep MINOR\  | $sed 's/#[a-zA-Z _]*MINOR\s*\([0-9]*\)/\1/')
echo $minor
build=$(echo "$rev" | grep PATCH\  | $sed 's/#[a-zA-Z _]*PATCH\s*\([0-9]*\)/\1/')
echo $build
env=$(echo "$rev" | grep ENV | $sed 's/#[a-zA-Z _]*ENV\s*\([0-9]*\)/\1/' | xargs)
echo $env
version=$major.$minor.$build-$env
echo "$version"

pkg_folder="MN8-$version"

[[ -d "$pkg_folder" ]] && rm -r "$pkg_folder"

mkdir -p "$pkg_folder"
cp build/bootloader/bootloader.bin "$pkg_folder"
cp build/partition_table/partition-table.bin "$pkg_folder"
cp build/ota_data_initial.bin "$pkg_folder"
cp build/led_strip.bin "$pkg_folder"


cat <<EOT >> "$pkg_folder/manifest"
id,filename,md5sum,flashaddress
0,bootloader.bin,$(md5sum "${pkg_folder}/bootloader.bin" | sed -r 's:\\*([^ ]*).*:\1:'),0x1000
1,partition-table.bin,$(md5sum "${pkg_folder}/partition-table.bin" | sed -r 's:\\*([^ ]*).*:\1:'),0x8000
2,ota_data_initial.bin,$(md5sum "${pkg_folder}/ota_data_initial.bin" | sed -r 's:\\*([^ ]*).*:\1:'),0xd000
3,led_strip.bin,$(md5sum "${pkg_folder}/led_strip.bin" | sed -r 's:\\*([^ ]*).*:\1:'),0x10000

EOT

zip -j ${pkg_folder}.zip ${pkg_folder}/*
