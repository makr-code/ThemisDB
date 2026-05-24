/*
 * ThemisDB | File: fulltext_functions.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
