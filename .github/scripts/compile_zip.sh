#!/bin/env bash

if [ -z $GITHUB_WORKSPACE ]; then
	echo "This script should only run on GitHub action!"
	exit 1
fi

# Make sure we're on right directory
cd $GITHUB_WORKSPACE

# Put critical files and folders here
need_integrity=(
	"./module/system/bin"
	"./module/libs"
	"./module/META-INF"
	"./module/service.sh"
	"./module/toast.apk"
	"./module/module.prop"
	"./module/encore_logo.png"
)

# Version info
version="$(cat ./version)"
version_code=$(git rev-list HEAD --count)
gitsha1=$(git rev-parse --short HEAD)

# Compile Gamelist
bash ./gamelist_compile.sh

# Copy module files
cp -r ./website/docs/public/android-crhome.png ./module/encore_logo.png
cp -r ./src/libs ./module
cp -r ./src/scripts/* ./module/system/bin
cp LICENSE ./module

# Parse version info to module prop
sed -i "s/version=.*/version=$version (GIT@$gitsha1)/" ./module/module.prop
sed -i "s/versionCode=.*/versionCode=$version_code/" ./module/module.prop
zipName="encore-$version-$version_code_$gitsha1.zip" && echo "zipName=$zipName" >>$GITHUB_OUTPUT

# Generate sha256sum for integrity checkup
for file in ${need_integrity[@]}; do
	bash .github/scripts/gen_sha256sum.sh "$file"
done

# Zip the file
cd ./module && zip -r9 ../$zipName * -x *placeholder* *.map
