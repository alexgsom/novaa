#!/usr/bin/env sh
# Nova portable CLI: self-hosted compiler + compiler-free native packaging.
# Python is not used by this script or by the Nova runtime.
set -eu
here=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
runner="$here/bin/nova-native"
compiler="$here/compiler/novac_selfhost.nbc"
[ -x "$runner" ] || { echo 'Nova runtime missing; run sh build.sh once or install a prebuilt binary.' >&2; exit 2; }
[ "$#" -ge 1 ] || { echo 'Usage: sh nova.sh build|compile|run|inspect|unpack|doctor ...' >&2; exit 2; }
mode=$1; shift
case "$mode" in
  build|compile|doctor)
    [ -f "$compiler" ] || { echo 'Self-hosted Nova compiler bytecode missing.' >&2; exit 2; }
    ;;
esac
case "$mode" in
  build|compile)
    [ "$#" -ge 2 ] || { echo 'Usage: nova.sh build source.nova output [--force]' >&2; exit 2; }
    source=$1; output=$2; shift 2
    [ -f "$source" ] || { echo 'Input .nova file does not exist' >&2; exit 2; }
    case "$source" in *.nova) ;; *) echo 'Input must be .nova' >&2; exit 2;; esac
    force=false
    if [ "$#" -gt 0 ]; then
      if [ "$#" -eq 1 ] && [ "$1" = '--force' ]; then force=true
      else echo 'Only --force is supported after the output' >&2; exit 2; fi
    fi
    if [ "$mode" = compile ]; then
      case "$output" in *.nbc) ;; *) echo 'Compiled output must end in .nbc' >&2; exit 2;; esac
      if [ -e "$output" ] && [ "$force" = false ]; then echo 'Output exists; use --force' >&2; exit 2; fi
      "$runner" exec "$compiler" --max-steps 15000000 -- "$source" "$output"
    else
      temp=$(mktemp -d)
      trap 'rm -rf -- "$temp"' EXIT HUP INT TERM
      "$runner" exec "$compiler" --max-steps 15000000 -- "$source" "$temp/code.nbc"
      if [ "$force" = true ]; then "$runner" pack "$temp/code.nbc" "$output" --force
      else "$runner" pack "$temp/code.nbc" "$output"; fi
    fi
    ;;
  doctor)
    [ "$#" -eq 0 ] || { echo 'Usage: nova.sh doctor' >&2; exit 2; }
    temp=$(mktemp -d)
    trap 'rm -rf -- "$temp"' EXIT HUP INT TERM
    printf 'show 12\n' >"$temp/health.nova"
    "$runner" exec "$compiler" --max-steps 15000000 -- "$temp/health.nova" "$temp/health.nbc" >/dev/null
    output=$("$runner" exec "$temp/health.nbc")
    [ "$output" = '12' ] || { echo 'Nova doctor: compiler or VM produced incorrect result' >&2; exit 1; }
    "$runner" pack "$temp/health.nbc" "$temp/health" >/dev/null
    output=$("$temp/health")
    [ "$output" = '12' ] || { echo 'Nova doctor: packaged executable produced incorrect result' >&2; exit 1; }
    "$runner" inspect "$temp/health" | grep -q 'Integrity: OK' || { echo 'Nova doctor: package integrity check failed' >&2; exit 1; }
    echo 'PASS: native runtime, Nova-authored compiler, bytecode VM, and portable executable'
    ;;
  run) exec "$runner" run "$@";;
  exec) exec "$runner" exec "$@";;
  inspect) exec "$runner" inspect "$@";;
  unpack) exec "$runner" unpack "$@";;
  --help|-h|--version) exec "$runner" "$mode";;
  *) echo "Unknown Nova command: $mode" >&2; exit 2;;
esac
