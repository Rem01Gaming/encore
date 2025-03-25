#!/bin/env bash

generate_checksum() {
	sha256sum "$1" | awk '{print $1}' >"$1.sha256"
	echo "Generated checksum for: $1"
}

find "$1" -type f | while IFS= read -r file; do
	generate_checksum "$file"
done
