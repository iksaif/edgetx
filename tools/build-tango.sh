#!/usr/bin/env bash
# Build helper for the TBS Tango II EdgeTX port.
#
# Sets up the Python venv, points PATH at the ARM GNU toolchain cask, and
# runs `make firmware` (or any target passed as $1) from the build dir.
#
# Usage:
#   tools/build-tango.sh                 # default: make firmware
#   tools/build-tango.sh clean firmware  # multiple targets ok
#   tools/build-tango.sh configure       # re-run cmake only
#
# Runs from repo root regardless of cwd.

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="$ROOT/build"
VENV="$ROOT/.venv"
ARM_BIN="/Applications/ArmGNUToolchain/15.2.rel1/arm-none-eabi/bin"

if [[ ! -d "$VENV" ]]; then
  echo "[build-tango] creating venv at $VENV" >&2
  python3 -m venv "$VENV"
  # shellcheck disable=SC1091
  source "$VENV/bin/activate"
  pip install --quiet --upgrade pip
  pip install --quiet Pillow lz4 pydantic jinja2 clang
else
  # shellcheck disable=SC1091
  source "$VENV/bin/activate"
fi

if [[ ! -x "$ARM_BIN/arm-none-eabi-gcc" ]]; then
  echo "[build-tango] ARM GNU toolchain not found at $ARM_BIN" >&2
  echo "[build-tango] install with: brew install --cask gcc-arm-embedded" >&2
  exit 1
fi
export PATH="$ARM_BIN:$PATH"

mkdir -p "$BUILD"
cd "$BUILD"

# Re-configure if CMakeCache is missing or we were asked to.
if [[ "${1:-}" == "configure" ]]; then
  shift
  cmake -DPCB=TANGO -DCOMPANION=OFF -DCPACK=OFF -DUSE_UNSUPPORTED_TOOLCHAIN=YES ..
fi
if [[ ! -f "$BUILD/CMakeCache.txt" ]]; then
  cmake -DPCB=TANGO -DCOMPANION=OFF -DCPACK=OFF -DUSE_UNSUPPORTED_TOOLCHAIN=YES ..
fi

# `--errors` (default): run `make firmware` and filter to the interesting
# lines (errors, link failures, undefined references, and successful .elf).
# Any other args are passed straight through to make.
if [[ $# -eq 0 || "${1:-}" == "--errors" ]]; then
  [[ "${1:-}" == "--errors" ]] && shift
  # PIPESTATUS ensures we keep make's exit code, not grep's.
  set -o pipefail
  make firmware "$@" 2>&1 | grep -E 'error:|Linking |undefined reference|firmware\.elf|\.elf$' || true
  exit ${PIPESTATUS[0]}
fi

exec make "$@"
