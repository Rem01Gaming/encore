#!/bin/env bash
# shellcheck disable=SC2035

if [ -z "$GITHUB_WORKSPACE" ]; then
	echo "This script should only run on GitHub action!" >&2
	exit 1
fi

# Make sure we're on right directory
cd "$GITHUB_WORKSPACE" || {
	echo "Unable to cd to GITHUB_WORKSPACE" >&2
	exit 1
}

# Put critical files and folders here
need_integrity=(
	"module/system/bin"
	"module/libs"
	"module/META-INF"
	"module/service.sh"
	"module/uninstall.sh"
	"module/action.sh"
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
paste -sd '|' - <"$GITHUB_WORKSPACE/gamelist.txt" >"$GITHUB_WORKSPACE/module/gamelist.txt"

# Copy module files
cp -r ./libs module
cp -r ./scripts/* module/system/bin
cp LICENSE ./module

# Remove .sh extension from scripts
find module/system/bin -maxdepth 1 -type f -name "*.sh" -exec sh -c 'mv -- "$0" "${0%.sh}"' {} \;

# Parse version info to module prop
zipName="encore-$version-$release_code.zip"
echo "zipName=$zipName" >>"$GITHUB_OUTPUT"

# Generate sha256sum for integrity checkup
for file in "${need_integrity[@]}"; do
	bash .github/scripts/gen_sha256sum.sh "$file"
done

# Zip the file
cd ./module || {
	echo "Unable to cd to ./module" >&2
	exit 1
}

zip -r9 ../"$zipName" * -x *placeholder* *.map .shellcheckrc
zip -z ../"$zipName" <<EOF
$version-$release_code
Build Date $(date +"%a %b %d %H:%M:%S %Z %Y")
EOF
