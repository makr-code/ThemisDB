/**
 * @file i_history_compressor.h
 * @brief Compatibility wrapper for historical include path changes.
 *
 * Some translation units include this header as "aql/i_history_compressor.h"
 * while the canonical interface currently lives under "llm/i_history_compressor.h".
 * This wrapper forwards the real definition to avoid include-path breakage
 * during incremental refactors.
 */

#pragma once

#include "llm/i_history_compressor.h"

// Preserve original namespace placement (the concrete interface declares
// types in `themis::aql`) so no additional declarations are required here.
