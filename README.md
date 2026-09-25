# JSON Key Finder

A C++20 research implementation of multi-pattern JSON key matching.
The project adapts the algorithm described in [Teddy: An Efficient SIMD-based Literal Matching Engine for Scalable Deep Packet Inspection](https://doi.org/10.1145/3472456.3473512) to scan JSON data for a set of object keys.
It also includes several alternative grouping, suffix, and verification strategies for comparison.

## High-level Overview

The Teddy algorithm is used as a prefilter for candidate matches.
Exact verification is then applied only to the candidates produced by the prefilter.

### 1. Compiling input keys

The Teddy compiler extracts suffixes of a fixed length from the keys, then groups _similar_ (similarity defined by each concrete grouping strategy and score model) suffixes into at most 8 Teddy groups.

The groups are then compiled into high- and low-nibble transition tables.
Each bit corresponds to one Teddy group.
A cleared bit indicates that the nibble occurs in that group at the given suffix position.

### 2. Candidate matching

The matcher scans the JSON input and looks up transition masks for the high and low nibbles of each byte.
The resulting masks identify potential candidate matches.
A candidate may be a type-I false positive caused by combining independently stored nibbles or characters from different suffixes.
It may also contain an exact suffix while failing full-key or JSON-context verification, which is classified as a type-II false positive.
Therefore, exact verification is needed.

### 3. Exact verification

A candidate is accepted only when it:

- ends at a valid, unescaped closing quote;
- is followed by optional whitespace and a colon;
- has a valid opening quote; and
- exactly matches one of the requested keys.

## Requirements

- A POSIX-compatible system
- CMake 3.16 or newer
- A C++20 compiler
- zlib development files (`zlib1g-dev` or `zlib-devel`)
- xxHash development files (`libxxhash-dev` or `xxhash-devel`)
- GoogleTest when building tests

`FIND_KEY_NATIVE` is enabled by default, which adds `-march=native`.
The SIMD matcher (`teddy`) is built when the compiler accepts `-mssse3`;
otherwise, use the scalar `teddy_baseline` matcher.

## Build

To build the project:

```bash
chmod +x clean_build.sh
./clean_build.sh
```

The build produces these executables:

- `findkey` - scan one JSON file;
- `bench_matrix` - run configurable benchmark and statistics matrices;
- `gen_keys` - generate key lists from a JSON file; and
- `reverse_keys` - reverse JSON keys and/or a newline-delimited key file, useful for prefix-matching experiments.

## Command-line usage

Run the executable with `--help` for the current input requirements, matcher
choices, Teddy configuration strategies, defaults, and output options:

```bash
./build/findkey --help
```

Information on each configuration is to be found in [`include/findkey.h`](include/findkey.h).

Quick start example:

```bash
./build/findkey \
  --keys keys/test.txt \
  --data data/file.json \
  --algo teddy \
  --print-positions
```

## Public C API

Although the implementation is C++20, its public entry points have a C ABI, which can be found in [`include/findkey.h`](include/findkey.h).

The main function signature is:

```c
size_t findkey(const uint8_t* data,
               size_t len,
               const uint8_t* const* keys,
               const size_t* key_lens,
               size_t num_keys,
               enum findkey_algo algo,
               const struct findkey_teddy_config* teddy_config,
               struct findkey_result* out_results,
               size_t max_out_positions,
               int* out_status,
               struct findkey_timing* out_timing);
```

The statistics-only entry point is:

```c
size_t findkey_with_stats(const uint8_t* data,
                          size_t len,
                          const uint8_t* const* keys,
                          const size_t* key_lens,
                          size_t num_keys,
                          const struct findkey_teddy_config* teddy_config,
                          struct findkey_teddy_stats* teddy_stats,
                          int* out_status);
```

Include the public header to obtain the ABI declarations, enums, result structures, configuration structures, and status codes:

```c
#include "findkey.h"
```

A minimal call looks like this:

```c
struct findkey_result results[1024];
struct findkey_timing timing = {0};
int status = FINDKEY_OK;

size_t total = findkey(
    json_bytes,
    json_length,
    key_pointers,
    key_lengths,
    key_count,
    TEDDY,
    NULL, // default Teddy configuration
    results,
    1024,
    &status,
    &timing);
```

`findkey()` returns the total number of matches, but only at most `max_out_positions` entries are written.

`findkey_with_stats()` runs the Teddy baseline and returns the total match count together with a `findkey_teddy_stats` structure, which includes matching information.

## Running tests

```bash
chmod +x run_tests.sh
./run_tests.sh
```

## Other tools

```bash
./build/bench_matrix --help
./build/gen_keys --help
./build/reverse_keys --help
```

Note that `bench_matrix` configuration is often rather large. Add `--dry-run` before running a matrix.

## JSON and key semantics

This project performs lightweight raw-byte JSON scanning; it is not a complete JSON parser.

  - Keys are matched using their spelling in the JSON source.
  Escape sequences are not decoded.
  For example, a key written as `"a\u0062"` must be supplied as the raw text `a\u0062`, not as `ab`.
  - Escaped quotes and backslashes are handled when locating string boundaries.
  - Between a key's closing quote and colon, only the four JSON whitespace bytes defined by RFC 8259 are accepted: space, horizontal tab, line feed, and carriage return.
  - Duplicate requested keys produce only one match per occurrence.

These semantics apply consistently to the scalar, Teddy baseline, and SIMD matchers.
