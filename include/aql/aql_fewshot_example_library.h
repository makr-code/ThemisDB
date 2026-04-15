/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            aql_fewshot_example_library.h                      ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 07:05:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     315                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3a758b465a  2026-04-12  feat(aql): AQL module enhancements — Features 8, 10, 12, ... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file aql_fewshot_example_library.h
 * @brief Curated few-shot example library for improved NL-to-AQL accuracy.
 *
 * Provides a static registry of natural-language / AQL example pairs that
 * are injected into LLM prompts to improve translation accuracy.  Examples
 * are organized by domain (document, graph, vector, geospatial, timeseries)
 * and can be retrieved by domain or ranked by relevance to an input query.
 *
 * Integrates with LLMAQLHandler::translateNLToAQL() and the generic
 * FewShotOptimizer from llm/fewshot_optimizer.h.
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <functional>

namespace themis {
namespace aql {

// ============================================================================
// Embedding provider interface
// ============================================================================

/**
 * @brief Minimal interface for embedding a text string into a float vector.
 *
 * Implement this interface to enable semantic (cosine-similarity) few-shot
 * selection in @c AQLFewShotExampleLibrary.  Pass the implementation to
 * @c AQLFewShotExampleLibrary::setEmbeddingProvider() to activate semantic
 * ranking.
 *
 * The default (Jaccard word-overlap) ranking is used when no provider is set.
 */
class IEmbeddingProvider {
public:
    virtual ~IEmbeddingProvider() = default;

    /**
     * @brief Compute a fixed-dimensional dense embedding for @p text.
     *
     * @param text  Input text to embed (may be empty).
     * @return Normalised float vector. Length must be consistent across calls.
     *         An empty vector signals that embedding is unavailable.
     */
    virtual std::vector<float> embed(const std::string& text) = 0;
};

// ============================================================================
// Domain enum
// ============================================================================

/**
 * @brief Query domain that a few-shot example belongs to.
 */
enum class AQLExampleDomain {
    DOCUMENT,    ///< Document collection CRUD / filter / sort queries
    GRAPH,       ///< Graph traversal and path queries
    VECTOR,      ///< Approximate nearest-neighbour vector search
    GEOSPATIAL,  ///< Geospatial distance / containment queries
    TIMESERIES,  ///< Time-series range and aggregation queries
    AGGREGATION, ///< COLLECT / aggregate / window functions
    GENERAL,     ///< Generic / multi-domain examples
};

// ============================================================================
// Data types
// ============================================================================

/**
 * @brief A single natural-language → AQL example with metadata.
 */
struct AQLFewShotExample {
    std::string       id;          ///< Unique identifier (e.g., "doc_filter_city")
    std::string       nl_query;    ///< Natural-language query
    std::string       aql_query;   ///< Corresponding AQL query
    AQLExampleDomain  domain;      ///< Primary query domain
    std::string       description; ///< Short human-readable description
    std::vector<std::string> tags; ///< Searchable tags
};

// ============================================================================
// Library
// ============================================================================

/**
 * @brief Static registry of curated NL-to-AQL few-shot examples.
 *
 * All built-in examples are registered at construction time.  Custom
 * examples can be added at runtime via registerExample().
 *
 * Typical usage:
 * @code
 * AQLFewShotExampleLibrary lib;
 *
 * // Get all graph examples
 * auto graph_exs = lib.findByDomain(AQLExampleDomain::GRAPH);
 *
 * // Get the 3 most relevant examples for a query
 * auto relevant = lib.findRelevant("show users in Paris", 3);
 *
 * // Format examples for LLM prompt injection
 * std::string prompt_section = lib.formatForPrompt(relevant);
 * @endcode
 */
class AQLFewShotExampleLibrary {
public:
    /**
     * @brief Construct the library and register all built-in examples.
     */
    AQLFewShotExampleLibrary();
    ~AQLFewShotExampleLibrary() = default;

    // =========================================================================
    // Registration
    // =========================================================================

    /**
     * @brief Register a custom example.
     * @throws std::invalid_argument if the id is empty or already registered
     */
    void registerExample(const AQLFewShotExample& example);

    // =========================================================================
    // Lookup
    // =========================================================================

    /**
     * @brief Return all registered examples.
     */
    const std::vector<AQLFewShotExample>& all() const;

    /**
     * @brief Return examples belonging to @p domain.
     */
    std::vector<AQLFewShotExample> findByDomain(AQLExampleDomain domain) const;

    /**
     * @brief Find examples whose tags contain @p tag (case-insensitive).
     */
    std::vector<AQLFewShotExample> findByTag(const std::string& tag) const;

    /**
     * @brief Look up an example by its unique id.
     * @return Pointer to the example, or nullptr if not found
     */
    const AQLFewShotExample* findById(const std::string& id) const;

    // =========================================================================
    // Relevance-ranked retrieval
    // =========================================================================

    /**
     * @brief Return up to @p n examples most relevant to @p nl_query.
     *
     * Relevance is computed as Jaccard word-overlap similarity between
     * @p nl_query and each example's nl_query field.  When @p domain is
     * supplied, candidates are pre-filtered to that domain before ranking.
     *
     * @param nl_query  Input natural-language query
     * @param n         Maximum number of examples to return (default: 3)
     * @param domain    Optional domain filter
     * @return Up to @p n examples sorted by descending relevance
     */
    std::vector<AQLFewShotExample> findRelevant(
        const std::string& nl_query,
        std::size_t n = 3,
        std::optional<AQLExampleDomain> domain = std::nullopt
    ) const;

    // =========================================================================
    // Prompt formatting
    // =========================================================================

    /**
     * @brief Format a list of examples into a prompt section.
     *
     * Each example is rendered as:
     * @code
     * Natural language: <nl_query>
     * AQL: <aql_query>
     * @endcode
     * with a blank line between examples.
     *
     * @param examples  Examples to include
     * @return Multi-line string ready to be appended to a system prompt
     */
    static std::string formatForPrompt(
        const std::vector<AQLFewShotExample>& examples
    );

    /**
     * @brief Convenience: find relevant examples and return them formatted.
     *
     * Equivalent to formatForPrompt(findRelevant(nl_query, n, domain)).
     *
     * @param nl_query  Input natural-language query
     * @param n         Maximum number of examples (default: 3)
     * @param domain    Optional domain filter
     * @return Formatted few-shot prompt section (empty if no examples found)
     */
    std::string buildPromptSection(
        const std::string& nl_query,
        std::size_t n = 3,
        std::optional<AQLExampleDomain> domain = std::nullopt
    ) const;

    // =========================================================================
    // Stats
    // =========================================================================

    /**
     * @brief Return the total number of registered examples.
     */
    std::size_t size() const;

    // =========================================================================
    // Semantic ranking support
    // =========================================================================

    /**
     * @brief Attach an embedding provider for semantic (cosine-similarity) ranking.
     *
     * When a non-null provider is attached, @c findRelevant() uses cosine
     * similarity between the input query embedding and cached example embeddings
     * instead of Jaccard word-overlap.  Pass @c nullptr to revert to Jaccard.
     *
     * The caller retains ownership of @p provider; it must remain valid for the
     * lifetime of this library object.
     *
     * @note Call @c rebuildEmbeddingIndex() after changing the provider if you
     *       want the pre-computed cache to be refreshed immediately; otherwise
     *       embeddings are computed lazily on the first call to @c findRelevant().
     *
     * @param provider  Embedding provider, or nullptr to disable semantic ranking.
     */
    void setEmbeddingProvider(IEmbeddingProvider* provider);

    /**
     * @brief Pre-compute and cache embeddings for all currently registered examples.
     *
     * Calling this method is optional but useful to amortise the embedding cost
     * before the first query arrives.  It is a no-op when no provider is set.
     *
     * @note Thread-safety: this method is NOT thread-safe with respect to
     *       concurrent calls to @c findRelevant() or @c registerExample().
     *       Complete all registrations before calling @c rebuildEmbeddingIndex().
     */
    void rebuildEmbeddingIndex();

private:
    std::vector<AQLFewShotExample>          examples_;
    std::unordered_map<std::string, std::size_t> index_by_id_;

    /// Optional semantic embedding provider (null → Jaccard ranking)
    IEmbeddingProvider* embedding_provider_ = nullptr;

    /// Pre-computed embeddings: index matches examples_ ordering.
    /// Empty entries indicate that the embedding has not been computed yet.
    mutable std::vector<std::vector<float>> embedding_cache_;

    void registerBuiltins_();

    /// Lexical (Jaccard word-overlap) relevance – always available.
    static double computeRelevance_(
        const std::string& query,
        const AQLFewShotExample& example
    );

    /// Semantic (cosine-similarity) relevance – requires embedding_provider_.
    /// Returns -1.0 if the provider is unavailable or embedding fails.
    double computeRelevanceSemantic_(
        const std::vector<float>& query_embedding,
        std::size_t example_index
    ) const;

    /// Ensure embedding_cache_[idx] is populated; returns true on success.
    bool ensureEmbedding_(std::size_t idx) const;

    /// Compute cosine similarity between two equal-length vectors.
    static double cosineSimilarity_(
        const std::vector<float>& a,
        const std::vector<float>& b
    );
};

} // namespace aql
} // namespace themis
