#!/usr/bin/env bash
# One-command configure + build for the RADiCAL simulation.
#
# Assumes GEANT4 is installed in a conda env (see README). Override the env name
# with:  GEANT4_ENV=myenv ./configure.sh
#
# Usage:
#   ./configure.sh          # configure + build into ./build
#   ./configure.sh clean    # remove ./build first
set -euo pipefail

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD="$HERE/build"
ENV_NAME="${GEANT4_ENV:-geant4}"

# Activate the conda env that provides geant4-config + cmake.
if ! command -v geant4-config >/dev/null 2>&1; then
  for base in "$HOME/miniforge3" "$HOME/miniconda3" \
              /opt/homebrew/Caskroom/miniforge/base /opt/conda; do
    if [ -f "$base/etc/profile.d/conda.sh" ]; then
      # shellcheck disable=SC1091
      source "$base/etc/profile.d/conda.sh"
      conda activate "$ENV_NAME"
      break
    fi
  done
fi
command -v geant4-config >/dev/null 2>&1 || {
  echo "ERROR: geant4-config not found. Activate your GEANT4 conda env, e.g.:"
  echo "       conda activate $ENV_NAME"
  exit 1
}

[ "${1:-}" = "clean" ] && rm -rf "$BUILD"

PREFIX="$(geant4-config --prefix)"
echo "Using GEANT4 $(geant4-config --version) at $PREFIX"

# Pick the conda env's expat (>=2.8.1); the macOS SDK ships an older one that
# CMake otherwise finds and rejects. Handles .dylib (macOS) and .so (Linux).
EXPAT_LIB=""
for cand in "$PREFIX/lib/libexpat.dylib" "$PREFIX/lib/libexpat.so"; do
  [ -e "$cand" ] && EXPAT_LIB="$cand" && break
done

mkdir -p "$BUILD"
cmake -S "$HERE" -B "$BUILD" \
  -DGeant4_DIR="$PREFIX/lib/cmake/Geant4" \
  -DCMAKE_PREFIX_PATH="$PREFIX" \
  -DEXPAT_INCLUDE_DIR="$PREFIX/include" \
  ${EXPAT_LIB:+-DEXPAT_LIBRARY="$EXPAT_LIB"}

cmake --build "$BUILD" -j"$(sysctl -n hw.ncpu 2>/dev/null || nproc)"

echo
echo "Built: $BUILD/radical"
echo "Run:   (cd $BUILD && ./radical run.mac)        # macros are copied into build/"
