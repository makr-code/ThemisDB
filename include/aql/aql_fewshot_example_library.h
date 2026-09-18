/**
 * @file aql_fewshot_example_library.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class IEmbeddingProvider {
public:
    /**
     * @brief IEmbedding Provider.
     * @return Return value.
     */
    virtual ~IEmbeddingProvider() = default;

    /**
     * @brief Embed.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    virtual std::vector<float> embed(const std::string& text) = 0;
};

// ============================================================================
// Domain enum
// ============================================================================

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

class AQLFewShotExampleLibrary {
public:
    AQLFewShotExampleLibrary();
    ~AQLFewShotExampleLibrary() = default;

    // =========================================================================
    // Registration
    // =========================================================================

    /**
     * @brief Register Example.
     * @param[in] example Input parameter.
     */
    void registerExample(const AQLFewShotExample& example);

    // =========================================================================
    // Lookup
    // =========================================================================

    /**
     * @brief All.
     * @return Return value.
     */
    const std::vector<AQLFewShotExample>& all() const;

    /**
     * @brief Find By Domain.
     * @param[in] domain Input parameter.
     * @return Return value.
     */
    std::vector<AQLFewShotExample> findByDomain(AQLExampleDomain domain) const;

    /**
     * @brief Find By Tag.
     * @param[in] tag Input parameter.
     * @return Return value.
     */
    std::vector<AQLFewShotExample> findByTag(const std::string& tag) const;

    /**
     * @brief Find By Id.
     * @param[in] id Input parameter.
     * @return Pointer to the result.
     */
    const AQLFewShotExample* findById(const std::string& id) const;

    // =========================================================================
    // Relevance-ranked retrieval
    // =========================================================================

    std::vector<AQLFewShotExample> findRelevant(
        const std::string& nl_query,
        std::size_t n = 3,
        std::optional<AQLExampleDomain> domain = std::nullopt
    ) const;

    // =========================================================================
    // Prompt formatting
    // =========================================================================

    /**
     * @brief Format For Prompt.
     * @param[in] examples Input parameter.
     * @return Return value.
     */
    static std::string formatForPrompt(
        const std::vector<AQLFewShotExample>& examples
    );

    std::string buildPromptSection(
        const std::string& nl_query,
        std::size_t n = 3,
        std::optional<AQLExampleDomain> domain = std::nullopt
    ) const;

    // =========================================================================
    // Stats
    // =========================================================================

    /**
     * @brief Size.
     * @return Return value.
     */
    std::size_t size() const;

    // =========================================================================
    // Semantic ranking support
    // =========================================================================

    /**
     * @brief Set Embedding Provider.
     * @param[in,out] provider Input/output parameter.
     */
    void setEmbeddingProvider(IEmbeddingProvider* provider);

    /**
     * @brief Rebuild Embedding Index.
     */
    void rebuildEmbeddingIndex();

private:
    std::vector<AQLFewShotExample>          examples_;
    std::unordered_map<std::string, std::size_t> index_by_id_;

    IEmbeddingProvider* embedding_provider_ = nullptr;

    mutable std::vector<std::vector<float>> embedding_cache_;

    /**
     * @brief Register Builtins.
     */
    void registerBuiltins_();

    /**
     * @brief Compute Relevance.
     * @param[in] query Input parameter.
     * @param[in] example Input parameter.
     * @return Return value.
     */
    static double computeRelevance_(
        const std::string& query,
        const AQLFewShotExample& example
    );

    /**
     * @brief Compute Relevance Semantic.
     * @param[in] query_embedding Input parameter.
     * @param[in] example_index Input parameter.
     * @return Return value.
     */
    double computeRelevanceSemantic_(
        const std::vector<float>& query_embedding,
        std::size_t example_index
    ) const;

    /**
     * @brief Ensure Embedding.
     * @param[in] idx Input parameter.
     * @return True when the operation succeeds.
     */
    bool ensureEmbedding_(std::size_t idx) const;

    /**
     * @brief Cosine Similarity.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    static double cosineSimilarity_(
        const std::vector<float>& a,
        const std::vector<float>& b
    );
};

} // namespace aql
} // namespace themis
