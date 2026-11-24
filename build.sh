#!/usr/bin/env sh
set -eu
cd "$(dirname "$0")"
mkdir -p bin
if ! pkg-config --exists libcurl; then
  echo 'Missing libcurl development headers (pkg-config libcurl)' >&2
  exit 2
fi
# shellcheck disable=SC2046
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic $(pkg-config --cflags libcurl) src/nova_native.cpp -o bin/nova-native $(pkg-config --libs libcurl) ${LDFLAGS:-}
printf 'Built: %s/bin/nova-native\n' "$(pwd)"
