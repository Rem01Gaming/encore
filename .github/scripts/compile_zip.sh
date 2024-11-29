#!/bin/env bash

if [ -z $GITHUB_WORKSPACE ]; then
	echo "This script should only run on GitHub action!"
	exit 1
fi

# Make sure we're on right directory
cd $GITHUB_WORKSPACE

# Download WebUI App
wget https://github.com/5ec1cff/KsuWebUIStandalone/releases/download/v1.0/KsuWebUI-1.0-34-release.apk -O module/webui.apk

# Put critical files and folders here
need_integrity=(
	"./module/system/bin"
	"./module/libs"
	"./module/META-INF"
	"./module/service.sh"
	"./module/action.sh"
	"./module/toast.apk"
	"./module/webui.apk"
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
cp -r ./src/libs ./module
cp -r ./src/scripts/* ./module/system/bin
cp LICENSE ./module

# Parse version info to module prop
zipName="encore-$version-$version_code_$gitsha1.zip" && echo "zipName=$zipName" >>$GITHUB_OUTPUT

# Generate sha256sum for integrity checkup
for file in ${need_integrity[@]}; do
	bash .github/scripts/gen_sha256sum.sh "$file"
done

# Zip the file
cd ./module && zip -r9 ../$zipName * -x *placeholder* *.map
