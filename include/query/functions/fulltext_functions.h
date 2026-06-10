/**
 * @file fulltext_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
