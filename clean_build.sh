#!/usr/bin/env bash
set -euo pipefail

rm -rf build

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build -j
