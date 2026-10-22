#!/usr/bin/env bash
set -euo pipefail

root_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
toolchain="${root_dir}/cmake/toolchains/mingw-w64-x86_64.cmake"

if ! command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1; then
  echo "error: x86_64-w64-mingw32-g++ not found" >&2
  echo "install MinGW-w64, for example: sudo pacman -S mingw-w64-gcc" >&2
  exit 1
fi

run_tests=0
if [[ "${1:-}" == "--tests" ]]; then
  run_tests=1
elif [[ -n "${1:-}" ]]; then
  echo "usage: $0 [--tests]" >&2
  exit 2
fi

if [[ "${run_tests}" -eq 1 ]]; then
  if ! command -v wine >/dev/null 2>&1; then
    echo "error: wine not found; install it or build without --tests" >&2
    exit 1
  fi
  build_dir="${OTTR_WINDOWS_TEST_BUILD_DIR:-${root_dir}/build_windows_tests}"
  cmake_args=(
    -DCMAKE_BUILD_TYPE=Debug
    -DBUILD_TESTING=ON
    -DCMAKE_CROSSCOMPILING_EMULATOR=wine
  )
else
  build_dir="${OTTR_WINDOWS_BUILD_DIR:-${root_dir}/build_windows}"
  cmake_args=(
    -DCMAKE_BUILD_TYPE=Release
    -DBUILD_TESTING=OFF
  )
fi

cmake -S "${root_dir}" -B "${build_dir}" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="${toolchain}" \
  "${cmake_args[@]}"

cmake --build "${build_dir}"

if [[ "${run_tests}" -eq 1 ]]; then
  (cd "${build_dir}" && ctest --output-on-failure)
else
  echo "built: ${build_dir}/bin/ottr.exe"
fi
