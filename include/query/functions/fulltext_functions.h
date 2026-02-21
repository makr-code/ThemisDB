/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            fulltext_functions.h                               ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     56                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
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
 * - FULLTEXT: Full-text search with scoring (placeholder, awaits SecondaryIndexManager integration)
 * - PHRASE: Exact phrase matching (placeholder, awaits SecondaryIndexManager integration)
 * - FUZZY: Fuzzy matching with Levenshtein distance (placeholder, awaits SecondaryIndexManager integration)
 * - NGRAM_MATCH: N-gram similarity calculation (fully implemented)
 * - TOKENS: Text tokenization (fully implemented)
 * - SOUNDEX: Soundex phonetic encoding (fully implemented)
 * - METAPHONE: Metaphone phonetic encoding (fully implemented)
 * - DOUBLE_METAPHONE: Double Metaphone encoding (fully implemented)
 * 
 * @param registry The function registry to register functions with
 */
void registerFulltextFunctions(FunctionRegistry& registry);

} // namespace functions
} // namespace query
} // namespace themis
