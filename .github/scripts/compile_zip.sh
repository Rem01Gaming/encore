#!/bin/env bash

if [ -z $GITHUB_WORKSPACE ]; then
	echo "This script should only run on GitHub action!" >&2
	exit 1
fi

# Make sure we're on right directory
cd $GITHUB_WORKSPACE

# Put critical files and folders here
need_integrity=(
	"module/system/bin"
	"module/libs"
	"module/META-INF"
	"module/service.sh"
	"module/uninstall.sh"
	"module/action.sh"
	"module/toast.apk"
	"module/module.prop"
	"module/encore_logo.png"
	"module/gamelist.txt"
)

# Version info
version="$(cat version)"
version_code="$(git rev-list HEAD --count)"
release_code="$(git rev-list HEAD --count)-$(git rev-parse --short HEAD)-release"
sed -i "s/version=.*/version=$version ($release_code)/" module/module.prop
sed -i "s/versionCode=.*/versionCode=$version_code/" module/module.prop

# Compile Gamelist
bash gamelist_compile.sh

# Copy module files
cp -r ./src/libs module
cp -r ./src/scripts/* module/system/bin
cp LICENSE ./module

# Parse version info to module prop
zipName="encore-$version-$release_code.zip"
echo "zipName=$zipName" >>$GITHUB_OUTPUT

# Generate sha256sum for integrity checkup
for file in ${need_integrity[@]}; do
	bash .github/scripts/gen_sha256sum.sh "$file"
done

# Zip the file
cd ./module
zip -r9 ../$zipName * -x *placeholder* *.map
zip -z ../$zipName <<EOF
$version-$release_code
Build Date $(date +"%a %b %d %H:%M:%S %Z %Y")
EOF
