#!/usr/bin/env sh
# Verifies native Nova bytecode self-hosting; independent of Python.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
native="$here/bin/nova-native"
[ -x "$native" ] || { echo 'Build Nova first: sh build.sh' >&2; exit 2; }
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
compiler="$here/compiler/novac.nova"
"$native" run "$compiler" --max-steps 15000000 -- "$compiler" "$tmp/novac-a.nbc"
"$native" exec "$tmp/novac-a.nbc" --max-steps 15000000 -- "$compiler" "$tmp/novac-b.nbc"
cmp "$tmp/novac-a.nbc" "$tmp/novac-b.nbc"
"$native" exec "$tmp/novac-b.nbc" --max-steps 3000000 -- "$here/examples/bootstrap/objects_modules.nova" "$tmp/demo.nbc"
"$native" run "$here/examples/bootstrap/objects_modules.nova" > "$tmp/reference.txt"
"$native" exec "$tmp/demo.nbc" > "$tmp/compiled.txt"
cmp "$tmp/reference.txt" "$tmp/compiled.txt"
echo 'PASS: stage-1 compiler compiles itself byte-identically and compiles an object/module sample.'
