#pragma once

#include "findkey.h"
#include "teddy/compile.h"
#include "teddy/verification/metadata.h"

#include <stddef.h>

void print_compilation_stats(
    const teddy::CompilationMetadata& teddy_metadata,
    const teddy::VerifierCompilationMetadata& metadata);

void print_teddy_runtime_stats(const findkey_teddy_stats& teddy_stats,
                               size_t data_len);
