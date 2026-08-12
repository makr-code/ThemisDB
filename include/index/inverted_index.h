/**
 * @file inverted_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.26
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: MIT
// Copyright (c) 2024-2026 ThemisDB Contributors
#pragma once

#include "storage/rocksdb_wrapper.h"
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace themis {

/**
 * @brief Standalone inverted index for full-text search.
 *
 * Provides tokenisation, BM25-scored search, exact phrase search, and
 * fuzzy (Levenshtein) search over a RocksDB backend.
 *
 * Key schema (compatible with SecondaryIndexManager fulltext keys):
 *   ftidxmeta:<table>:<column>          – index configuration (JSON)
 *   ftidx:<table>:<column>:<token>:<pk> – posting entry
 *   fttf:<table>:<column>:<token>:<pk>  – term frequency (uint32 as string)
 *   ftdlen:<table>:<column>:<pk>        – document length (uint32 as string)
 *
 * Thread-safety: a single InvertedIndex instance is **not** thread-safe.
 * Use one instance per thread or guard with an external mutex.
 */
class InvertedIndex {
public:
    // -----------------------------------------------------------------------
    // Supporting types
    // -----------------------------------------------------------------------

    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return {false, std::move(msg)}; }
    };

    /** Configuration stored alongside the index metadata. */
    struct Config {
        bool stemming_enabled  = false;
        std::string language   = "none"; ///< "en", "de", or "none"
        bool stopwords_enabled = false;
        std::vector<std::string> stopwords; ///< custom stop-word list (lowercase)
        bool normalize_umlauts = false;     ///< de: ä→a, ö→o, ü→u, ß→ss
    };

    /** A single search hit returned by search / searchPhrase / searchFuzzy. */
    struct SearchResult {
        std::string pk;
        double score = 0.0;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    explicit InvertedIndex(RocksDBWrapper& db);

    // -----------------------------------------------------------------------
    // Index lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Create (or overwrite) the index metadata for table/column.
     * @return Status indicating success or failure.
     */
    Status create(std::string_view table, std::string_view column);

    /**
     * @brief Create (or overwrite) the index metadata with custom configuration.
     * @param table Table name.
     * @param column Column name.
     * @param config Configuration settings for the index.
     * @return Status indicating success or failure.
     */
    Status create(std::string_view table, std::string_view column,
                  Config config);

    /**
     * @brief Remove the index metadata key.
     *
     * Posting data is NOT purged here. Call deindex() for each document
     * before dropping to avoid orphaned keys.
     * @return Status indicating success or failure.
     */
    Status drop(std::string_view table, std::string_view column);

    /**
     * @brief Check if the index metadata key exists.
     * @return True if the index exists, false otherwise.
     */
    bool exists(std::string_view table, std::string_view column) const;

    /**
     * @brief Retrieve the stored configuration for the index.
     * @return Configuration if the index exists, nullopt if the index does not exist.
     */
    std::optional<Config> getConfig(std::string_view table,
                                    std::string_view column) const;

    // -----------------------------------------------------------------------
    // Document indexing
    // -----------------------------------------------------------------------

    /**
     * @brief Index a document field.
     *
     * Tokenises @p text and writes posting, TF, and document-length entries
     * for (@p table, @p column, @p pk).  Any previously indexed entries for
     * the same pk are removed first (upsert semantics).
     *
     * @return Error if the index does not exist.
     */
    Status index(std::string_view table, std::string_view column,
                 std::string_view pk, std::string_view text);

    /**
     * @brief Remove a document's posting entries.
     *
     * The @p text parameter is retained for API backward compatibility but is
     * **no longer used** internally.  Removal is driven entirely by the
     * `ftrev:<table>:<column>:<pk>` reverse-index key written by index().
     * Callers may pass an empty string.
     *
     * @return Error if the index does not exist.
     */
    Status deindex(std::string_view table, std::string_view column,
                   std::string_view pk, std::string_view text);

    // -----------------------------------------------------------------------
    // Search
    // -----------------------------------------------------------------------

    /**
     * @brief BM25-scored full-text search.
     *
     * All query tokens must appear in a document for it to be returned
     * (AND semantics).  Results are sorted by descending BM25 score.
     * @return Pair containing Status and vector of search results sorted by score.
     */
    std::pair<Status, std::vector<SearchResult>> search(
        std::string_view table, std::string_view column,
        std::string_view query, size_t limit = 1000) const;

    /**
     * @brief Exact phrase search.
     *
     * Candidates must contain all phrase tokens (AND), then the original
     * field text is checked for the verbatim lowercased phrase substring.
     * @return Pair containing Status and vector of phrase search results.
     */
    std::pair<Status, std::vector<SearchResult>> searchPhrase(
        std::string_view table, std::string_view column,
        std::string_view phrase, size_t limit = 1000) const;

    /**
     * @brief Fuzzy search using Levenshtein edit distance.
     *
     * @param table Table name.
     * @param column Column name.
     * @param query Query text.
     * @param maxDistance Maximum edit distance (0 = exact, default 2).
     * @param limit Maximum number of results to return.
     * @return Pair containing Status and vector of fuzzy search results.
     */
    std::pair<Status, std::vector<SearchResult>> searchFuzzy(
        std::string_view table, std::string_view column,
        std::string_view query, int maxDistance = 2,
        size_t limit = 1000) const;

    // -----------------------------------------------------------------------
    // Tokenisation utilities (public for reuse / testing)
    // -----------------------------------------------------------------------

    /**
     * @brief Tokenize text with basic whitespace and punctuation splitting.
     *
     * Performs whitespace/punctuation split and lowercase conversion.
     * No stemming or stop-word removal.
     * @param text Text to tokenize.
     * @return Vector of lowercase tokens.
     */
    static std::vector<std::string> tokenize(std::string_view text);

    /**
     * @brief Tokenize text with optional normalisation, stop-word removal, and stemming.
     *
     * Applies configuration-specific transformations including normalization,
     * stop-word removal, and language-specific stemming.
     * @param text Text to tokenize.
     * @param config Configuration settings for tokenization.
     * @return Vector of processed tokens.
     */
    static std::vector<std::string> tokenize(std::string_view text,
                                             const Config& config);

    // -----------------------------------------------------------------------
    // Key-schema helpers (static; same prefixes as SecondaryIndexManager)
    // -----------------------------------------------------------------------

    static std::string makeMetaKey(std::string_view table,
                                   std::string_view column);
    static std::string makeIndexKey(std::string_view table,
                                    std::string_view column,
                                    std::string_view token,
                                    std::string_view pk);
    static std::string makeIndexPrefix(std::string_view table,
                                       std::string_view column,
                                       std::string_view token);
    static std::string makeTFKey(std::string_view table,
                                 std::string_view column,
                                 std::string_view token,
                                 std::string_view pk);
    static std::string makeDocLenKey(std::string_view table,
                                     std::string_view column,
                                     std::string_view pk);

    /// Reverse-index key: stores the set of tokens currently indexed for a pk.
    /// Key: ftrev:<table>:<column>:<pk>  →  JSON array of token strings.
    static std::string makeRevKey(std::string_view table,
                                  std::string_view column,
                                  std::string_view pk);

private:
    RocksDBWrapper& db_;

    // Internal BM25 computation shared by search().
    std::pair<Status, std::vector<SearchResult>> computeBM25_(
        std::string_view table, std::string_view column,
        std::string_view query, size_t limit) const;

    // Remove all posting data for a given pk using the stored reverse-index key.
    void removePostings_(std::string_view table, std::string_view column,
                         std::string_view pk);
};

} // namespace themis

