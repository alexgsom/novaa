#!/usr/bin/env sh
# Build a single standalone executable from Nova source + sibling .nova modules.
# Build-time: POSIX shell, C++17 compiler, od, awk, find, pkg-config/libcurl on POSIX.
# Runtime: Python-free; system libcurl/SQLite may be required on POSIX.
set -eu
if [ "$#" -ne 2 ]; then
  echo 'Usage: sh build_app.sh source.nova output_executable' >&2
  exit 2
fi
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
source_file=$1
output=$2
case "$source_file" in *.nova) ;; *) echo 'Expected .nova file' >&2; exit 2;; esac
[ -f "$source_file" ] || { echo "File not found: $source_file" >&2; exit 2; }
[ ! -d "$output" ] || { echo 'Output is a directory' >&2; exit 2; }
source_dir=$(CDPATH= cd -- "$(dirname -- "$source_file")" && pwd)
source_abs="$source_dir/$(basename -- "$source_file")"
[ "$(wc -c < "$source_abs")" -le 1048576 ] || { echo 'Main source exceeds 1 MiB' >&2; exit 2; }
mkdir -p "$(dirname -- "$output")"
tmp=$(mktemp -d)
trap 'rm -rf -- "$tmp"' EXIT HUP INT TERM
header="$tmp/nova_embedded_app.hpp"
{
  printf 'static const unsigned char NOVA_EMBEDDED_CODE[] = {\n'
  od -An -v -tu1 "$source_abs" | awk '{ for(i=1;i<=NF;++i)printf "%s,",$i; print "" }'
  printf '};\nstatic const size_t NOVA_EMBEDDED_LENGTH = sizeof(NOVA_EMBEDDED_CODE);\n'
  printf 'struct NovaEmbeddedModule { const char* name; const unsigned char* bytes; size_t length; };\n'
} > "$header"
: > "$tmp/manifest"
printf "%s\n" "$(wc -c < "$source_abs" | tr -d ' ')" > "$tmp/total_bytes"
# Filenames containing newlines are not supported by the POSIX packaging script.
# Only files below the main script's parent directory are included.
find "$source_dir" -type f -name '*.nova' | LC_ALL=C sort | while IFS= read -r file; do
  [ "$file" = "$source_abs" ] && continue
  relative=${file#"$source_dir"/}
  bytes=$(wc -c < "$file")
  total=$(cat "$tmp/total_bytes")
  total=$((total + bytes))
  [ "$total" -le 8388608 ] || { echo 'Project source exceeds 8 MiB' >&2; exit 2; }
  printf '%s\n' "$total" > "$tmp/total_bytes"
  [ "$bytes" -le 1048576 ] || { echo "Module exceeds 1 MiB: $relative" >&2; exit 2; }
  # Restrict project size: 128 files and at most 8 MiB (source + modules).
  number=$(wc -l < "$tmp/manifest" | tr -d ' ')
  [ "$number" -lt 128 ] || { echo 'Module limit (128) exceeded' >&2; exit 2; }
  {
    printf 'static const unsigned char NOVA_MOD_BYTES_%s[] = {' "$number"
    od -An -v -tu1 "$file" | awk '{for(i=1;i<=NF;++i)printf "%s,",$i; print ""}'
    printf '};\nstatic const char NOVA_MOD_PATH_%s[] = {' "$number"
    printf '%s' "$relative" | od -An -v -tu1 | awk '{for(i=1;i<=NF;++i)printf "%s,",$i; print ""}'
    printf '0};\n'
  } >> "$header"
  printf '{NOVA_MOD_PATH_%s, NOVA_MOD_BYTES_%s, sizeof(NOVA_MOD_BYTES_%s)},\n' "$number" "$number" "$number" >> "$tmp/manifest"
done
{
  printf 'static const NovaEmbeddedModule NOVA_EMBEDDED_MODULES[] = {\n'
  cat "$tmp/manifest"
  printf '{nullptr, nullptr, 0}};\n'
  count=$(wc -l < "$tmp/manifest" | tr -d ' ')
  printf 'static const size_t NOVA_EMBEDDED_MODULE_COUNT = %s;\n' "$count"
} >> "$header"
if ! pkg-config --exists libcurl; then
  echo 'libcurl development headers are required to build Nova Native on this system' >&2
  exit 2
fi
# shellcheck disable=SC2046
"${CXX:-c++}" -std=c++17 -O2 -DNOVA_EMBEDDED_APP -I"$tmp" $(pkg-config --cflags libcurl) "$here/src/nova_native.cpp" -o "$output" $(pkg-config --libs libcurl) ${LDFLAGS:-}
chmod +x "$output"
printf 'Built standalone Nova executable (with %s project modules): %s\n' "$count" "$output"
