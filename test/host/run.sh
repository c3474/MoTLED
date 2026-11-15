#!/usr/bin/env bash
# Build and run the pure-logic unit tests on this computer (no ESP toolchain needed).
set -euo pipefail

cd "$(dirname "$0")/../.."   # repo root

bin="$(mktemp)"
trap 'rm -f "$bin"' EXIT

"${CXX:-c++}" -std=c++17 -Wall -Wextra -Itest/host/stubs -Imain \
    test/host/main.cpp test/test_color_math.cpp -o "$bin"
"$bin"
