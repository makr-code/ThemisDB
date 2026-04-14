/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            fulltext_functions.h                               ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:27:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     61                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
