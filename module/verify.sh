TMPDIR_FOR_VERIFY="$TMPDIR/.vunzip"
mkdir "$TMPDIR_FOR_VERIFY"

abort_verify() {
	ui_print "*********************************************************"
	ui_print "! $1"
	ui_print "! Installation aborted. The module may be corrupted."
	ui_print "! Please re-download and try again."
	abort "*********************************************************"
}

# extract <zip> <file> <target dir> <junk paths>
extract() {
	zip=$1
	file=$2
	dir=$3
	junk_paths=$4
	[ -z "$junk_paths" ] && junk_paths=false
	opts="-o"
	[ "$junk_paths" = true ] && opts="-oj"

	if [ "$junk_paths" = true ]; then
		file_path="$dir/$(basename "$file")"
		hash_path="$TMPDIR_FOR_VERIFY/$(basename "$file").sha256"
	else
		file_path="$dir/$file"
		hash_path="$TMPDIR_FOR_VERIFY/$file.sha256"
	fi

	unzip $opts "$zip" "$file" -d "$dir" >&2
	[ -f "$file_path" ] || abort_verify "$file does not exists"

	unzip $opts "$zip" "$file.sha256" -d "$TMPDIR_FOR_VERIFY" >&2
	[ -f "$hash_path" ] || abort_verify "Missing checksum for $file"

	(echo "$(<"$hash_path")  $file_path" | sha256sum -c -s -) || abort_verify "Checksum mismatch for $file"
	ui_print "- Verified $file" >&1

	unset file_path
	unset hash_path
}

file="META-INF/com/google/android/update-binary"
file_path="$TMPDIR_FOR_VERIFY/$file"
hash_path="$file_path.sha256"
unzip -o "$ZIPFILE" "META-INF/com/google/android/*" -d "$TMPDIR_FOR_VERIFY" >&2
[ -f "$file_path" ] || abort_verify "$file does not exists"
if [ -f "$hash_path" ]; then
	(echo "$(<"$hash_path")  $file_path" | sha256sum -c -s -) || abort_verify "Checksum mismatch for $file"
	ui_print "- Verified $file" >&1
else
	ui_print "- Download from Magisk app"
fi
