/**
 * @file federated_rag_merger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include "governance/gdpr_subject_rights.h"

namespace themis::distributed_knowledge {

// ─────────────────────────────────────────────────────────────────────────────
// MergeStrategy
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Algorithm used to merge retrieval results across shards.
 */
enum class MergeStrategy {
    RECIPROCAL_RANK_FUSION, ///< Default: robust multi-shard re-ranking
    SCORE_WEIGHTED,         ///< Multiply by shard adapter accuracy_delta
    ROUND_ROBIN             ///< Interleaved diversity sampling
};

// ─────────────────────────────────────────────────────────────────────────────
// RetrievedDocument — per-shard document entry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single document retrieved by one shard.
 */
struct RetrievedDocument {
    std::string doc_id;           ///< Document identifier
    std::string content;          ///< Document text (may be truncated)
    std::string shard_id;         ///< Shard that produced this result
    double      relevance_score;  ///< Shard-local relevance score [0, 1]
    size_t      rank_in_shard;    ///< Rank within the shard result list (1-based)
    std::map<std::string, std::string> metadata; ///< Including "_entities" from enrichment

    [[nodiscard]] nlohmann::json toJson() const {
        nlohmann::json j = {{"doc_id",          doc_id},
                            {"content",         content},
                            {"shard_id",        shard_id},
                            {"relevance_score", relevance_score},
                            {"rank_in_shard",   rank_in_shard}};
        for (const auto& [k, v] : metadata) j["metadata"][k] = v;
        return j;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// ShardRetrievalResult — one shard's complete retrieval response
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Complete retrieval response from one shard.
 */
struct ShardRetrievalResult {
    std::string                  shard_id;
    std::vector<RetrievedDocument> documents;
    double   adapter_accuracy_delta = 0.0; ///< From AdapterCapabilityAnnouncement
    uint64_t latency_ms             = 0;
    bool     ok                     = true;
    bool     timed_out              = false; ///< DK-OR-T: set true when shard exceeded deadline
    std::string error_message;             ///< Non-empty when ok == false
};

// ─────────────────────────────────────────────────────────────────────────────
// MergedRAGContext — final output
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Globally merged RAG context, ready for LLM prompt injection.
 *
 * Immutable after construction.
 */
struct MergedRAGContext {
    std::vector<RetrievedDocument> documents;  ///< Ranked merged document list
    size_t   total_candidate_count = 0;        ///< Before dedup + cut-off
    size_t   unique_doc_count      = 0;        ///< After dedup
    size_t   shards_queried        = 0;
    size_t   shards_responded      = 0;
    MergeStrategy strategy_used    = MergeStrategy::RECIPROCAL_RANK_FUSION;

    /**
     * @brief Build a compact text context suitable for LLM prompt injection.
     * @param max_docs   Maximum documents to include (default: all).
     * @param max_chars  Approximate character budget (0 = unlimited).
     */
    [[nodiscard]] std::string buildPromptContext(
        size_t max_docs  = 0,
        size_t max_chars = 0) const;
};

// ─────────────────────────────────────────────────────────────────────────────
// FederatedRAGMergerConfig
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for `FederatedRAGMerger`.
 */
struct FederatedRAGMergerConfig {
    MergeStrategy strategy          = MergeStrategy::RECIPROCAL_RANK_FUSION;
    size_t        top_k             = 20;       ///< Maximum documents in merged output
    bool          deduplicate       = true;     ///< Remove identical doc_ids
    double        rrf_constant      = 60.0;     ///< RRF constant k (Cormack 2009)
    bool          boost_specialised = true;     ///< Boost docs from specialised shards
    double        specialisation_boost = 1.2;  ///< Multiplier for adaptor accuracy_delta > 0

    // DK-OR: per-shard timeout (0 = instant timeout; UINT64_MAX = no timeout)
    size_t        shard_timeout_ms  = std::numeric_limits<size_t>::max();
    ///< When 0: merge() throws "all shards timed out" immediately.
    ///< When > 0: ShardRetrievalResult entries with timed_out==true are skipped.

    [[nodiscard]] bool isValid() const {
        return top_k > 0 && rrf_constant > 0.0 && specialisation_boost >= 1.0;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// FederatedRAGMerger
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Merges retrieval results from N shards into a single ranked context.
 *
 * Stateless: create once, call `merge()` repeatedly.
 *
 * Example:
 * @code
 *   FederatedRAGMerger merger(config);
 *   auto ctx = merger.merge(shard_results);
 *   std::string prompt_fragment = ctx.buildPromptContext(10, 4000);
 * @endcode
 */
class FederatedRAGMerger {
public:
    explicit FederatedRAGMerger(FederatedRAGMergerConfig config = {});

    /**
     * @brief Merge retrieval results from multiple shards.
     *
     * @param shard_results  Per-shard retrieval responses. Failed shards
     *                       (`ok == false`) are skipped gracefully.
     * @return Merged, ranked, de-duplicated context.
     */
    [[nodiscard]] MergedRAGContext merge(
        const std::vector<ShardRetrievalResult>& shard_results) const;

    /**
     * @brief Convenience overload: merge and directly build prompt context.
     *
     * @param shard_results  Per-shard results.
     * @param max_docs       Max documents in prompt context.
     * @param max_chars      Character budget (0 = unlimited).
     * @return Formatted prompt context string.
     */
    [[nodiscard]] std::string mergeAndBuildContext(
        const std::vector<ShardRetrievalResult>& shard_results,
        size_t max_docs  = 10,
        size_t max_chars = 0) const;

    [[nodiscard]] const FederatedRAGMergerConfig& config() const { return config_; }

    // ── DK-OR: GDPR erase ────────────────────────────────────────────────────

    /**
     * @brief Clear any cached merge context (DK-OR-H-2).
     *
     * Clears internal state and increments `erase_count_`.
     */
    themis::governance::StoreErasureResult erase(
        const std::string& subject_id = "",
        themis::governance::Regulation regulation = themis::governance::Regulation::GDPR);

    [[nodiscard]] size_t eraseCount() const { return erase_count_; }

private:
    FederatedRAGMergerConfig config_;

    // DK-OR: GDPR erase count
    mutable size_t erase_count_{0};

    // Merge strategy implementations
    [[nodiscard]] std::vector<RetrievedDocument> mergeRRF(
        const std::vector<ShardRetrievalResult>& results) const;
    [[nodiscard]] std::vector<RetrievedDocument> mergeScoreWeighted(
        const std::vector<ShardRetrievalResult>& results) const;
    [[nodiscard]] std::vector<RetrievedDocument> mergeRoundRobin(
        const std::vector<ShardRetrievalResult>& results) const;

    [[nodiscard]] std::vector<RetrievedDocument> deduplicate(
        std::vector<RetrievedDocument> docs) const;
};

} // namespace themis::distributed_knowledge
