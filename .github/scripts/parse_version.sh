#/bin/env bash

version="$(cat version)"
version_code=$(git rev-list HEAD --count)
gitsha1=$(git rev-parse --short HEAD)
sed -i "s/version=.*/version=$version (GIT@$gitsha1)/" module/module.prop
sed -i "s/versionCode=.*/versionCode=$version_code/" module/module.prop
