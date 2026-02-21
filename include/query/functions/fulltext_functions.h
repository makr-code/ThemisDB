/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            fulltext_functions.h                               ║
  Version:         0.0.21                                             ║
  Last Modified:   2026-02-21 19:20:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     63                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file fulltext_functions.h
 * @brief Fulltext Search Functions for ThemisDB AQL - Registration Interface
 * 
 * This header declares the registration function for fulltext search capabilities.
 * The implementations are in fulltext_functions.cpp.
 */

#pragma once

namespace themis {
namespace query {
namespace functions {

// Forward declaration
class FunctionRegistry;

/**
 * @brief Register all fulltext search functions with the function registry
 * 
 * Registers the following functions:
 * - FULLTEXT:         BM25-scored full-text search (wired to SecondaryIndexManager::scanFulltextWithScores)
 * - PHRASE:           Exact phrase matching (wired to SecondaryIndexManager::scanFulltextPhrase)
 * - FUZZY:            Fuzzy matching with Levenshtein distance (wired to SecondaryIndexManager::scanFulltextFuzzy)
 * - HIGHLIGHT:        Wrap query terms in source text with configurable HTML/markup tags
 * - FULLTEXT_SNIPPET: Extract and highlight a context window around the best query-term cluster
 * - NGRAM_MATCH:      N-gram similarity calculation (fully implemented)
 * - TOKENS:           Text tokenization (fully implemented)
 * - SOUNDEX:          Soundex phonetic encoding (fully implemented)
 * - METAPHONE:        Metaphone phonetic encoding (fully implemented)
 * - DOUBLE_METAPHONE: Double Metaphone encoding (fully implemented)
 * 
 * @param registry The function registry to register functions with
 */
void registerFulltextFunctions(FunctionRegistry& registry);

} // namespace functions
} // namespace query
} // namespace themis
