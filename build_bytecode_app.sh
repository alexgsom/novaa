#!/usr/bin/env sh
# Linux developer bootstrap: compile Nova-generated .nbc into standalone executable.
# Requires C++17 and libcurl development libraries (the shared Nova C++ runtime).
set -eu
if [ "$#" -ne 2 ]; then echo 'Usage: sh build_bytecode_app.sh input.nbc output' >&2; exit 2; fi
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_file=$1
output=$2
case "$source_file" in *.nbc) ;; *) echo 'Expected .nbc file' >&2; exit 2;; esac
[ -f "$source_file" ] || { echo 'Bytecode file not found' >&2; exit 2; }
[ "$(wc -c < "$source_file")" -le 4194304 ] || { echo 'Bytecode exceeds 4 MiB' >&2; exit 2; }
mkdir -p "$(dirname -- "$output")"
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM
{
  printf 'static const unsigned char NOVA_BYTECODE_BYTES[] = {\n'
  od -An -v -tu1 "$source_file" | awk '{for(i=1;i<=NF;++i)printf "%s,",$i; print ""}'
  printf '};\nstatic const size_t NOVA_BYTECODE_LENGTH = sizeof(NOVA_BYTECODE_BYTES);\n'
} > "$tmp/nova_embedded_bytecode.hpp"
if ! pkg-config --exists libcurl; then echo 'libcurl development headers are required' >&2; exit 2; fi
# shellcheck disable=SC2046
"${CXX:-c++}" -std=c++17 -O2 -DNOVA_EMBEDDED_BYTECODE_APP -I"$tmp" $(pkg-config --cflags libcurl) "$here/src/nova_native.cpp" -o "$output" $(pkg-config --libs libcurl) ${LDFLAGS:-}
chmod +x "$output"
printf 'Built standalone Nova bytecode application: %s\n' "$output"
