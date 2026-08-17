/**
 * @file explainability_reason_builder.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=8; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <algorithm>
#include <chrono>
#include <functional>
#include <map>
#include <string>
#include <vector>

namespace themis {
namespace rag {

// ---------------------------------------------------------------------------
// AIDecisionRecord — lightweight input type (no RocksDB dependency)
// ---------------------------------------------------------------------------

/**
 * @brief Lightweight decision record consumed by ExplainabilityReasonBuilder.
 *
 * This struct mirrors the subset of AIDecisionAudit (ai_decision_auditor.h)
 * that is needed to build a causal explanation without pulling in the full
 * RocksDB-backed AIDecisionAuditor.
 */
struct AIDecisionRecord {
    /// Canonical decision type (e.g., "HNSW_PARAMS_UPDATED").
    std::string decision_type;

    /// Unique identifier (optional; used for deduplication in enrichAuditor).
    std::string decision_id;

    /// Arbitrary key/value parameters attached to this decision.
    std::map<std::string, std::string> parameters;

    /// Confidence score in [0.0, 1.0].
    double confidence{0.0};

    /// When this decision was made.
    std::chrono::system_clock::time_point timestamp{std::chrono::system_clock::now()};

    /// True when all guardrails and safety checks passed.
    bool guardrail_passed{true};

    /// Originating shard identifier.
    std::string shard_id;
};

// ---------------------------------------------------------------------------
// ExplainabilityReasonBuilder
// ---------------------------------------------------------------------------

/**
 * @brief Layer-9 LLM Optimization: causal explanation builder.
 *
 * Every autonomous ThemisDB decision must be explainable to the DBA in
 * natural language.  `ExplainabilityReasonBuilder` translates a lightweight
 * `AIDecisionRecord` into a structured `CausalChain` (signal → analysis →
 * decision → confidence → impact → dba_action_required) using a per-type
 * template library.
 *
 * ### Design goals
 * - **Template-based** — no LLM call, deterministic output, zero latency.
 * - **100 % coverage** — every known `decision_type` has a template; unknown
 *   types fall back to a generic template rather than crashing.
 * - **EU AI Act compliant** — each explanation explicitly states whether DBA
 *   action is required and the expected impact.
 *
 * ### Thread safety
 * `ExplainabilityReasonBuilder` is stateless after construction and safe for
 * concurrent use.
 *
 * // STUB/SIMULATION NOTE:
 * // Purpose: Template-based fallback for NL generation when no LoRA-adapted
 * //          generator is injected via setNlGeneratorFn().
 * // Activation: Always active when nl_generator_fn_ is null (default).
 * //             Replaced by the injected fn when setNlGeneratorFn() is called.
 * // Production Delta: Template text is deterministic; injected LoRA fn produces
 * //                   context-sensitive prose with richer detail.
 * // Removal Plan: Template path is retained as permanent fallback;
 * //               callers inject a real LoRA NL generator in IMPL-A2 Loop-1.
 */
class ExplainabilityReasonBuilder {
public:
    // ─── CausalChain ──────────────────────────────────────────────────────

    /**
     * @brief Structured causal explanation for a single autonomous decision.
     *
     * The chain follows the SAFE (Signal → Analysis → Fact → Effect) pattern
     * used by ThemisDB's DBA-dialog API.
     */
    struct CausalChain {
        /// What signal triggered the autonomous action.
        std::string signal;

        /// Why the signal is relevant to the system state.
        std::string analysis;

        /// What was decided / changed.
        std::string decision;

        /// How confident the system is in this decision [0.0, 1.0].
        double confidence{0.0};

        /// Expected operational impact of the decision.
        std::string impact;

        /// True when the DBA must take explicit follow-up action.
        bool dba_action_required{false};

        /// Original decision_type tag from the source AIDecisionRecord.
        std::string decision_type;
    };

    // ─── Construction ─────────────────────────────────────────────────────

    ExplainabilityReasonBuilder() = default;
    ~ExplainabilityReasonBuilder() = default;

    ExplainabilityReasonBuilder(const ExplainabilityReasonBuilder&) = default;
    ExplainabilityReasonBuilder& operator=(const ExplainabilityReasonBuilder&) = default;
    ExplainabilityReasonBuilder(ExplainabilityReasonBuilder&&) noexcept = default;
    ExplainabilityReasonBuilder& operator=(ExplainabilityReasonBuilder&&) noexcept = default;

    // ─── NL Generator injection ───────────────────────────────────────────

    /**
     * @brief Type alias for an injected natural-language generator.
     *
     * When set via setNlGeneratorFn(), `toNaturalLanguage()` delegates to
     * this function instead of the built-in template renderer.  The fn
     * receives a fully-populated CausalChain and must return a non-empty
     * string synchronously.  An empty return value falls back to the
     * template renderer.
     *
     * Planned use: inject a LoRA-adapted prose generator (IMPL-A2 Loop-1).
     */
    using NlGeneratorFn = std::function<std::string(const CausalChain&)>;

    /**
     * @brief Inject a natural-language generator.
     *
     * When `fn` is non-null, `toNaturalLanguage()` calls it and returns its
     * result.  Pass an empty (default-constructed) function to revert to the
     * built-in template renderer.
     *
     * @param fn  Callable that converts a CausalChain to a NL string.
     */
    void setNlGeneratorFn(NlGeneratorFn fn);

    // ─── Core API ─────────────────────────────────────────────────────────

    /**
     * @brief Build a causal explanation from an autonomous decision record.
     *
     * Selects the appropriate template from the built-in library, substitutes
     * parameter values, and evaluates whether DBA action is required.
     *
     * @param record  The autonomous decision to explain.
     * @return        A fully-populated CausalChain (never empty).
     */
    CausalChain build(const AIDecisionRecord& record) const;

    /**
     * @brief Render a CausalChain as human-readable natural language.
     *
     * Produces a paragraph of 50–500 words suitable for the DBA REST API
     * or CLI output.
     *
     * @param chain  A CausalChain produced by build().
     * @return       Multi-sentence English explanation.
     */
    std::string toNaturalLanguage(const CausalChain& chain) const;

    /**
     * @brief Batch-enrich a vector of decision records with causal chain text.
     *
     * For each record, calls build() + toNaturalLanguage() and stores the
     * result in `record.parameters["_explanation"]`.
     *
     * @param records  In/out vector of records to enrich.
     * @return         Number of records successfully enriched.
     */
    size_t enrichAuditor(std::vector<AIDecisionRecord>& records) const;

    /// Retrieve the template entry for a given decision_type.
    struct TemplateEntry {
        std::string signal_tmpl;
        std::string analysis_tmpl;
        std::string decision_tmpl;
        std::string impact_tmpl;
    };

private:
    NlGeneratorFn nl_generator_fn_;

    /// Evaluate whether DBA action is required for the given record.
    static bool requiresDbaAction(const AIDecisionRecord& record);

    static const TemplateEntry& getTemplate(const std::string& decision_type);
};

// ---------------------------------------------------------------------------
// FederatedAIDecisionAuditor — cross-shard timeline stub
// ---------------------------------------------------------------------------

/**
 * @brief Cross-shard autonomous-decision timeline merger.
 *
 * Accepts per-shard `AIDecisionRecord` collections and produces a
 * chronologically merged timeline for DBA inspection.  When a
 * `ShardRecordFetcher` is injected, `mergeTimeline()` also fetches
 * records from remote shards (e.g. via GossipProtocol / FederatedRAGMerger)
 * and merges them into the returned timeline.
 *
 * // STUB/SIMULATION NOTE:
 * // Purpose: In-memory fallback until DK-4 (Federated RAG Merge) wires the
 * //          real cross-shard record propagation via FederatedRAGMerger.
 * // Activation: In-memory path always active; remote-fetch path active only
 * //             when a ShardRecordFetcher is injected via setShardRecordFetcher().
 * // Production Delta: Without a fetcher, mergeTimeline() uses locally
 * //                   registered records only; remote-shard records are absent.
 * // Removal Plan: Replace in-memory shard_records_ with FederatedRAGMerger
 * //               integration in DK-4; ShardRecordFetcher becomes permanent API.
 */
class FederatedAIDecisionAuditor {
public:
    /**
     * @brief Type alias for a remote shard record fetcher.
     *
     * When injected, called by `mergeTimeline()` for each registered shard to
     * supplement locally cached records with records retrieved from the remote
     * shard via GossipProtocol or FederatedRAGMerger.
     *
     * @param shard_id  The shard identifier to fetch records for.
     * @return          Records fetched from the remote shard (may be empty).
     */
    using ShardRecordFetcher =
        std::function<std::vector<AIDecisionRecord>(const std::string& shard_id)>;

    FederatedAIDecisionAuditor() = default;
    ~FederatedAIDecisionAuditor() = default;

    /**
     * @brief Register the decision records of one shard.
     *
     * @param shard_id  Unique shard identifier.
     * @param records   Records from that shard (copied in).
     */
    void addShard(const std::string& shard_id,
                  std::vector<AIDecisionRecord> records);

    /**
     * @brief Return all records from all registered shards sorted by timestamp.
     *
     * If a `ShardRecordFetcher` has been injected, it is called for each
     * registered shard to fetch additional records from the remote node.
     * Fetched records are merged alongside locally cached records and deduplicated
     * by (shard_id, timestamp, decision_type) before sorting.
     *
     * @return Chronologically ordered decision timeline (oldest first).
     */
    std::vector<AIDecisionRecord> mergeTimeline() const;

    /// Return the number of registered shards.
    size_t shardCount() const noexcept { return shard_records_.size(); }

    /// Return the total number of locally cached records across all shards.
    size_t totalRecords() const noexcept;

    /**
     * @brief Inject a remote shard record fetcher.
     *
     * When set, `mergeTimeline()` calls this function for each registered
     * shard and merges the returned records into the timeline alongside any
     * locally cached records.  Pass an empty (default-constructed) function
     * to clear.
     *
     * @param fn  Callable that fetches remote records for a given shard_id.
     */
    void setShardRecordFetcher(ShardRecordFetcher fn);

private:
    std::map<std::string, std::vector<AIDecisionRecord>> shard_records_;
    ShardRecordFetcher shard_fetcher_;
};

} // namespace rag
} // namespace themis
