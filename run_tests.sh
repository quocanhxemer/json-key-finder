#!/usr/bin/env bash
set -euo pipefail

cmake -S . -B build-tests -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-tests --target find_json_key_tests -j
ctest --test-dir build-tests --output-on-failure
