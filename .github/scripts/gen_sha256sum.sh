#!/bin/env bash

generate_checksum() {
    if [[ "$1" != *.sha256 ]]; then
        sha256sum "$1" | awk '{print $1}' >"$1.sha256"
        echo "Generated checksum for: $1"
    fi
}

find "$1" -type f ! -name "*.sha256" | while IFS= read -r file; do
    generate_checksum "$file"
done
