#!/usr/bin/env bash
# Prepare a release for tag $1 (e.g. v0.9.1) in directory $2 (default: dist):
#   - check that the tag matches project(VERSION) in CMakeLists.txt
#   - extract the version's section of CHANGELOG.md into $2/notes.md
#   - build msgpack-cpp-<version>-headers.{tar.gz,zip} and SHA256SUMS
#   - compile ci/install_consumer.cpp against the packaged headers only
set -euo pipefail

tag="${1:?usage: $0 <tag> [out-dir]}"
out="${2:-dist}"
root="$(cd "$(dirname "$0")/.." && pwd)"
version="${tag#v}"

cmake_version="$(sed -nE 's/^project\([^ ]+ VERSION ([0-9][0-9.]*).*/\1/p' "$root/CMakeLists.txt")"
if [ "$version" != "$cmake_version" ]; then
    echo "error: tag $tag does not match project(VERSION $cmake_version) in CMakeLists.txt" >&2
    exit 1
fi

mkdir -p "$out"
out="$(cd "$out" && pwd)"

# release notes: everything between "## [<version>]" and the next "## [" heading
awk -v v="$version" '
    $0 ~ "^## \\[" v "\\]" { found = 1; next }
    found && /^## \[/       { exit }
    /^\[[^]]+\]: /          { next }   # link reference definitions at the end of the file
    found                   { print }
' "$root/CHANGELOG.md" | sed -e '/./,$!d' > "$out/notes.md"
if ! grep -q '[^[:space:]]' "$out/notes.md"; then
    echo "error: no '## [$version]' section with content in CHANGELOG.md" >&2
    exit 1
fi

name="msgpack-cpp-$version"
stage="$(mktemp -d)"
trap 'rm -rf "$stage"' EXIT
mkdir "$stage/$name"
cp "$root"/{packer.h,unpacker.h,msgpack-platform.h,LICENSE,README.md,CHANGELOG.md} "$stage/$name/"

rm -f "$out/$name-headers.tar.gz" "$out/$name-headers.zip"
tar -C "$stage" -czf "$out/$name-headers.tar.gz" "$name"
(cd "$stage" && zip -qr "$out/$name-headers.zip" "$name")
(cd "$out" && sha256sum "$name-headers.tar.gz" "$name-headers.zip" > SHA256SUMS)

# smoke test: the packaged headers alone must be enough
check="$stage/check"
mkdir "$check"
tar -C "$check" -xzf "$out/$name-headers.tar.gz"
"${CXX:-g++}" -std=c++11 -Wall -Wextra -Werror -I "$check/$name" \
    "$root/ci/install_consumer.cpp" -o "$check/consumer"
"$check/consumer"

echo "release $tag prepared in $out:"
ls -1 "$out"
