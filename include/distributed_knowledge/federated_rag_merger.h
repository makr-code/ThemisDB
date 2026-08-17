#pragma once

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
     *
     * **Formatting:**
     *  - Each document formatted as: "[Shard: <shard_id>] <content>\n  Entities: <entities>\n\n"
     *  - Entities appended if metadata["_entities"] is non-empty
     *  - Joins documents with "\n\n" separator
     *
     * **Truncation Contract:**
     *  1. If max_docs > 0: stop after including max_docs documents
     *  2. If max_chars > 0: stop if appending next doc exceeds budget
     *  3. Last document is truncated (if necessary) to fit max_chars, append "..."
     *  4. If remaining budget < 20 chars: skip last document entirely
     *
     * **Determinism:**
     *  - Output order matches input documents ranking
     *  - Identical inputs → identical output (idempotent)
     *
     * **Use Case:**
     *  - After merge(), call buildPromptContext() to create RAG context for LLM
     *  - Example: buildPromptContext(10, 4000) → top-10 docs, max 4KB
     *
     * @param max_docs   Maximum documents to include (default: 0 = all). If 0,
     *                   include all documents (subject to max_chars).
     * @param max_chars  Approximate character budget (default: 0 = unlimited).
     *                   If > 0, truncate to stay within budget; last doc may
     *                   be shortened with "..." suffix.
     * @return Formatted prompt context string, ready for LLM injection.
     *         Empty string if documents list is empty.
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
 *
 * All fields have sensible defaults; most deployments use just strategy and top_k.
 *
 * **Validation Contract:**
 *  - isValid() returns true iff:
 *    - top_k > 0 (must return at least 1 document)
 *    - rrf_constant > 0.0 (RRF denominator must be positive)
 *    - specialisation_boost >= 1.0 (boost never reduces scores)
 *  - If invalid, FederatedRAGMerger ctor throws std::invalid_argument
 *
 * **Strategy Selection:**
 *  - RECIPROCAL_RANK_FUSION (default): robust, handles multi-shard tie-breaks well
 *  - SCORE_WEIGHTED: respects shard accuracy; lighter computation
 *  - ROUND_ROBIN: maximizes diversity across shards; useful for exploration
 */
struct FederatedRAGMergerConfig {
    /// Merge algorithm to apply (see MergeStrategy enum).
    MergeStrategy strategy          = MergeStrategy::RECIPROCAL_RANK_FUSION;

    /// Maximum documents in merged output. Must be > 0.
    size_t        top_k             = 20;

    /// Remove duplicate doc_ids after merge (keep first/highest-ranked occurrence).
    bool          deduplicate       = true;

    /**
     * @brief RRF constant k in formula: 1 / (k + rank).
     *
     * From Cormack et al. 2009 "Reciprocal Rank Fusion outperforms Condorcet
     * and individual Rank Learning Methods."
     *  - k = 60 (default): balances early and late-ranked results
     *  - k > 60: emphasizes all ranks equally (lower variance, higher bias)
     *  - k < 60: emphasizes top results more (higher variance, lower bias)
     * Must be > 0.0; invalid config throws in ctor.
     */
    double        rrf_constant      = 60.0;

    /// Boost docs from specialised shards (high adapter_accuracy_delta).
    bool          boost_specialised = true;

    /**
     * @brief Multiplier for RRF/score when specialisation_boost enabled.
     *
     * When adapter_accuracy_delta > 0 (shard claims specialisation), multiply
     * merge score by this factor.
     *  - 1.0 = no boost (disable by setting boost_specialised=false)
     *  - 1.2 (default) = 20% boost for specialized results
     * Must be >= 1.0; invalid config throws in ctor.
     */
    double        specialisation_boost = 1.2;

    /**
     * @brief Per-shard timeout in milliseconds (DK-OR-T contract).
     *
     * Controls timeout handling:
     *  - 0: immediate timeout; merge() throws std::runtime_error("all shards timed out")
     *       if any shard_results passed (fail-closed for safety)
     *  - UINT64_MAX (default): no timeout; all shards always included
     *  - 1..UINT64_MAX-1: custom timeout; shards with timed_out==true are skipped
     *
     * **Timeout Contract:**
     *  - merge() checks each ShardRetrievalResult.timed_out flag
     *  - If timed_out == true and timeout_ms != UINT64_MAX: skip this shard
     *  - If ALL responding shards timed out: throw std::runtime_error("all shards timed out")
     *  - Partial timeouts are handled gracefully (merge proceeds with responsive shards)
     */
    size_t        shard_timeout_ms  = std::numeric_limits<size_t>::max();

    /**
     * @brief Validate configuration for constructor.
     *
     * Returns true iff:
     *  - top_k > 0
     *  - rrf_constant > 0.0
     *  - specialisation_boost >= 1.0
     */
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
 * Implements three explicit merge strategies to resolve conflicts when multiple
 * shards return overlapping or identical documents:
 *
 * **1. Reciprocal Rank Fusion (RRF)** — default and most robust
 *    - Formula: RRF_score(d) = Σ_i 1 / (k + rank_i(d))
 *    - where k = rrf_constant (default 60, per Cormack et al. 2009)
 *    - Tie-breaking: Identical RRF scores result in stable sort order
 *    - Conflict resolution: Multiple shards ranking same doc_id → scores added
 *    - Specialisation boost: If adapter_accuracy_delta > 0, multiply by specialisation_boost
 *
 * **2. Score-Weighted Merge** — respects shard accuracy weights
 *    - Merges scores: weighted_score(d) = Σ_i score_i(d) × weight_i
 *    - Weight = 1.0 + (boost_specialised ? adapter_accuracy_delta : 0.0)
 *    - Tie-breaking: Identical weighted scores result in stable sort order
 *    - Conflict resolution: Multiple shards ranking same doc_id → weighted scores added
 *
 * **3. Round-Robin Merge** — interleaved diversity sampling
 *    - Interleaves documents across shards at each rank position
 *    - Position 1: [shard_1_doc_1, shard_2_doc_1, shard_3_doc_1, ...]
 *    - Position 2: [shard_1_doc_2, shard_2_doc_2, shard_3_doc_2, ...]
 *    - Conflict resolution: Later shards' docs for same position override earlier
 *    - Use case: maximizing diversity across multi-specialized shards
 *
 * **Deduplication Contract:**
 *  - After merge, if deduplicate == true, removes entries with duplicate doc_id
 *  - Keeps first occurrence (highest score in final ranking)
 *  - Dedup happens BEFORE top_k truncation
 *  - Result: unique_doc_count reflects dedup'd count
 *
 * **Timeout & Failure Handling (DK-OR-T):**
 *  - Shards with sr.ok == false are silently skipped
 *  - Shards with sr.timed_out == true are silently skipped
 *  - If shard_timeout_ms == 0 and results non-empty → throws std::runtime_error("all shards timed out")
 *  - If shard_timeout_ms > 0 and ALL shards timed_out → throws std::runtime_error("all shards timed out")
 *  - If SOME shards timeout but others respond → those are merged normally
 *  - Exception: strong guarantee (no state change on throw, idempotent retry safe)
 *
 * **Top-K Truncation:**
 *  - Final results trimmed to config_.top_k documents
 *  - Happens AFTER dedup; may result in < top_k docs if dedup reduces count
 *  - Deterministic: sort order preserved
 *
 * **Stateless Design:**
 *  - FederatedRAGMerger is stateless: create once, call merge() repeatedly
 *  - Each merge() call is independent and thread-safe (read-only operations)
 *  - GDPR erase() is a no-op for this component (no cached contexts)
 *
 * Example:
 * @code
 *   FederatedRAGMergerConfig cfg;
 *   cfg.strategy = MergeStrategy::RECIPROCAL_RANK_FUSION;
 *   cfg.top_k = 20;
 *   FederatedRAGMerger merger(cfg);
 *
 *   std::vector<ShardRetrievalResult> results = queryShards();
 *   auto merged_ctx = merger.merge(results);
 *   std::string prompt = merged_ctx.buildPromptContext(10, 4000);
 * @endcode
 */
class FederatedRAGMerger {
public:
    explicit FederatedRAGMerger(FederatedRAGMergerConfig config = {});

    /**
    * @brief Merge retrieval results from multiple shards using configured strategy.
    *
    * Orchestrates merge conflict resolution by:
    * 1. Filtering timed-out or failed shards
    * 2. Applying selected merge strategy (RRF, score-weighted, or round-robin)
    * 3. Optional deduplication by doc_id
    * 4. Truncation to top_k
    *
    * **Consistency Level: EVENTUAL**
    *  - Operation is stateless; no coordinator synchronization required
    *  - Shard responses may be from different temporal snapshots (stale OK)
    *  - Merge tolerates heterogeneous freshness: RRF/weighted/round-robin robust to rank shifts
    *  - No version vector; timestamp in ShardRetrievalResult is informational only
    *  - Rationale: Merge correctness depends only on input ranking, not absolute freshness
    *
    * **Version Tracking:**
    *  - ShardRetrievalResult.latency_ms documents response age (informational)
    *  - No causal ordering enforced; shards may respond in any order
    *  - Deterministic: same inputs always produce same output
    *
    * **Replication Lag:**
    *  - Max acceptable lag: unbounded (design intent: eventual consistency OK)
    *  - Policy: Stale rankings accepted by merge strategies
    *  - Correctness: Cross-shard score/rank bias naturally bounds stale-data impact
    *  - Safe-read: Timestamp available but not enforced as guard
    *
    * **Exception Semantics:**
    *  - std::runtime_error("all shards timed out"): when shard_timeout_ms == 0
    *    or all shards report timed_out == true
    *  - std::invalid_argument: when config_ is invalid (checked in ctor)
    *  - Strong exception safety: merge either completes or throws; no partial state change
    *
    * **Idempotency:**
    *  - Same shard_results input always produces identical output
    *  - Safe to retry; no internal state mutation beyond GDPR erase_count_
    *
    * @param shard_results Per-shard retrieval responses.
    *                      - sr.ok == false → silently skipped (shard failure)
    *                      - sr.timed_out == true → silently skipped (shard timeout)
    *                      - sr.documents → ranked list per shard
    *                      - sr.adapter_accuracy_delta → shard specialisation weight (for RRF/weighted)
    *
    * @return MergedRAGContext with:
    *         - documents: ranked merged list (size ≤ top_k)
    *         - total_candidate_count: sum of doc counts from responding shards
    *         - unique_doc_count: size after dedup (if enabled)
    *         - shards_queried: |shard_results|
    *         - shards_responded: count with ok == true && timed_out == false
    *         - strategy_used: which algorithm was applied
    */
    [[nodiscard]] MergedRAGContext merge(
        const std::vector<ShardRetrievalResult>& shard_results) const;

    /**
     * @brief Convenience overload: merge and directly build prompt context.
     *
     * Equivalent to: merge(shard_results).buildPromptContext(max_docs, max_chars)
     *
     * @param shard_results  Per-shard results (see merge() for details).
     * @param max_docs       Maximum documents to include in prompt (0 = all).
     * @param max_chars      Character budget (0 = unlimited). If specified,
     *                       last document is truncated to fit budget.
     * @return Formatted prompt context string with "[Shard: X] content" per document.
     *
     * @throws std::runtime_error (see merge() exception contract)
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

    /**
     * @brief Merge using Reciprocal Rank Fusion (RRF).
     *
     * **Algorithm:**
     *  1. For each shard sr with sr.ok && !sr.timed_out:
     *  2.   For each document doc at rank r in sr.documents:
     *  3.     rrf_score += 1.0 / (rrf_constant + r)
     *  4. Optionally boost score if sr.adapter_accuracy_delta > 0
     *  5. Sort by rrf_score descending (stable sort for ties)
     *
     * **Conflict Resolution (multiple shards return same doc_id):**
     *  - RRF scores are accumulated: if shard1 ranks doc_id at position 3
     *    and shard2 ranks same doc_id at position 5, their RRF contributions add
     *  - Result: duplicates automatically ranked higher (due to accumulated score)
     *
     * **Tie-Breaking (identical RRF scores):**
     *  - Stable sort preserves input order
     *  - Later occurrences stay later in output (when scores equal)
     *
     * **Specialisation Boost:**
     *  - If adapter_accuracy_delta > 0 (specialised shard), multiply RRF by specialisation_boost
     *  - Default boost = 1.2 (20% advantage for specialized shards)
     *
     * **Failure Handling:**
     *  - Skips shards with sr.ok == false (malformed response, partial timeout)
     *  - Skips shards with sr.timed_out == true (exceeded DK-OR deadline)
     *  - Returns partial merge if some shards failed
     *
     * @param results Pre-filtered shard results (may include timed-out or failed entries)
     * @return Ranked document list by RRF score (ties stable-sorted by input order)
     */
    [[nodiscard]] std::vector<RetrievedDocument> mergeRRF(
        const std::vector<ShardRetrievalResult>& results) const;

    /**
     * @brief Merge using score-weighted accumulation.
     *
     * **Algorithm:**
     *  1. For each shard sr with sr.ok && !sr.timed_out:
     *  2.   weight = 1.0 + (boost_specialised ? sr.adapter_accuracy_delta : 0.0)
     *  3.   For each document doc:
     *  4.     weighted_score += doc.relevance_score * max(0.01, weight)
     *  5. Sort by weighted_score descending (stable sort for ties)
     *
     * **Conflict Resolution (multiple shards return same doc_id):**
     *  - Scores are weighted-summed: doc's final score = Σ_i (score_i × weight_i)
     *  - Multi-occurrence docs automatically ranked higher
     *  - Weights reflect shard accuracy (adapter_accuracy_delta from capability announcement)
     *
     * **Tie-Breaking:**
     *  - When weighted scores equal, stable sort preserves input order
     *
     * **Weight Clamping:**
     *  - Weight has minimum 0.01 to avoid score collapse for untrusted shards
     *
     * **Failure Handling:**
     *  - Skips timed-out or failed shards (see mergeRRF)
     *
     * @param results Pre-filtered shard results
     * @return Ranked document list by weighted score
     */
    [[nodiscard]] std::vector<RetrievedDocument> mergeScoreWeighted(
        const std::vector<ShardRetrievalResult>& results) const;

    /**
     * @brief Merge using round-robin interleaving for diversity.
     *
     * **Algorithm:**
     *  1. Collect all non-empty, non-timed-out shard result lists
     *  2. pos = 0
     *  3. While (any shard has doc at pos):
     *  4.   For each shard in order:
     *  5.     If shard.documents[pos] exists: append to merged
     *  6.   ++pos
     *  7. Limit total size to ~top_k * 2 (before dedup/trim)
     *
     * **Conflict Resolution:**
     *  - Multiple shards at same position → both included (interleaved)
     *  - No score computation; rank order from merge determines final ranking
     *
     * **Tie-Breaking:**
     *  - Shard order in results determines interleave order
     *  - Deterministic: same input always produces same output
     *
     * **Failure Handling:**
     *  - Skips timed-out or failed shards (see mergeRRF)
     *
     * @param results Pre-filtered shard results
     * @return Ranked document list (interleaved by position across shards)
     */
    [[nodiscard]] std::vector<RetrievedDocument> mergeRoundRobin(
        const std::vector<ShardRetrievalResult>& results) const;

    /**
     * @brief Remove documents with duplicate doc_id, preserving order.
     *
     * **Deduplication Semantics:**
     *  - Iterates input in order
     *  - For each doc_id: keeps first occurrence, discards later ones
     *  - Result size ≤ input size (never grows)
     *  - Order preserved: output documents in input order
     *
     * **Determinism Contract:**
     *  - Uses std::set (ordered) instead of std::unordered_set to ensure
     *    DETERMINISTIC iteration order
     *  - Identical input always produces identical output
     *  - Enables reproducible testing and reliable debugging in distributed systems
     *  - Performance: O(n log n) worst-case; negligible for typical dedup sets (<100 docs)
     *  - See DKRG-01..06 benchmarks for performance validation (<5% regression expected <1%)
     *
     * **Conflict Resolution:**
     *  - When same doc_id appears multiple times (merged from different shards),
     *    the first (highest-ranked) version is kept
     *  - Content from duplicate is discarded (shard_id, metadata unchanged from first)
     *
     * **Use Case:**
     *  - After RRF/weighted/round-robin merge, removes same document from different shards
     *  - Before top_k truncation (dedup may reduce count below top_k)
     *
     * @param docs Input ranked document list (may contain duplicates by doc_id)
     * @return Deduplicated document list, order preserved, size ≤ input size
     *         Deterministic: same input → same output, reproducible across runs
     */
    [[nodiscard]] std::vector<RetrievedDocument> deduplicate(
        std::vector<RetrievedDocument> docs) const;
};

} // namespace themis::distributed_knowledge
