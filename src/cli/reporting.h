#pragma once

#include "findkey.h"
#include "teddy/teddy_compile.h"

#include <stddef.h>

void print_teddy_compilation_stats(const TeddyCompilationMetadata& metadata);

void print_teddy_runtime_stats(const findkey_teddy_stats& teddy_stats,
                               size_t data_len);
