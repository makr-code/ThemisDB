/**
 * @file workload_fingerprint_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Input: per-tenant workload statistics snapshot
// ---------------------------------------------------------------------------

/**
 * @brief Statistics snapshot used as input to WorkloadFingerprintEngine.
 *
 * Callers populate this struct from monitoring counters and pass it to
 * `WorkloadFingerprintEngine::classify()`.
 */
struct TenantWorkloadStats {
    std::string tenant_id;

    /// Total number of queries (SELECT + DML) in the observation window.
    uint64_t query_count = 0;

    /// Fraction of queries that are writes (INSERT/UPDATE/DELETE), [0.0, 1.0].
    double write_ratio = 0.0;

    /// Average p99 latency across all queries (milliseconds).
    double avg_p99_ms = 0.0;

    /// Average rows touched per query (estimate from query planner statistics).
    uint64_t avg_rows_per_query = 0;

    /// Number of distinct bulk-insert / bulk-load operations in the window.
    uint32_t bulk_insert_count = 0;

    /// Observation window duration (seconds).
    uint32_t window_seconds = 60;
};

// ---------------------------------------------------------------------------
// WorkloadFingerprintEngine
// ---------------------------------------------------------------------------

/**
 * @brief Layer-8 LLM Optimization: tenant workload fingerprint classifier.
 *
 * `WorkloadFingerprintEngine` distils a tenant's current workload statistics
 * into a compact **fingerprint vector** — a semantic signature that describes
 * the dominant access pattern (OLTP, OLAP, BATCH, MIXED).
 *
 * The fingerprint is used by:
 *  - `TenantManager`: advisory resource policy hot-reload.
 *  - Layer-11 (DK-2) GossipProtocol: cross-shard fingerprint propagation.
 *
 * ### Classification heuristic
 * Four orthogonal signals are scored:
 *  - OLTP score: many short queries (high query_count, low avg_p99_ms, low avg_rows).
 *  - OLAP score: few heavy queries (low query_count, high avg_p99_ms, high avg_rows).
 *  - BATCH score: recurring bulk inserts (bulk_insert_count > 0, high write_ratio).
 *  - MIXED score: remainder after the above scores are normalised.
 *
 * The resulting `vector` has dimension 4 = [OLTP, OLAP, BATCH, MIXED].
 *
 * ### Decision records
 * Each `classify()` call writes a `DecisionRecord{decision_type="WORKLOAD_FINGERPRINT"}`
 * to `AIDecisionAuditor` when an auditor is injected.
 *
 * ### Thread safety
 * The engine is stateless after construction and safe for concurrent use.
 */
class WorkloadFingerprintEngine {
public:
    // ──────────────────────────────────────────────────────────────────────
    // Types
    // ──────────────────────────────────────────────────────────────────────

    /// Dominant workload pattern.
    enum class WorkloadPattern {
        OLTP,    ///< Many small, short transactions
        OLAP,    ///< Few, heavy analytical queries
        BATCH,   ///< Periodic bulk operations
        MIXED,   ///< No single pattern dominates
        UNKNOWN  ///< Insufficient data
    };

    /// Fingerprint result returned by classify().
    struct WorkloadFingerprint {
        std::string tenant_id;
        WorkloadPattern pattern;
        /// Probability vector: [OLTP, OLAP, BATCH, MIXED] — sums to 1.0.
        std::vector<double> vector;
        double confidence;

        /// Advisory resource policy derived from the fingerprint.
        struct PolicyRecommendation {
            uint32_t    max_connections    = 50;
            std::string memory_limit       = "2GB";
            std::string priority           = "MEDIUM";
            bool        suggest_read_replica = false;
        } recommended_policy;
    };

    // ──────────────────────────────────────────────────────────────────────
    // Construction
    // ──────────────────────────────────────────────────────────────────────

    WorkloadFingerprintEngine()  = default;
    ~WorkloadFingerprintEngine() = default;

    WorkloadFingerprintEngine(const WorkloadFingerprintEngine&)            = default;
    WorkloadFingerprintEngine& operator=(const WorkloadFingerprintEngine&) = default;

    // ──────────────────────────────────────────────────────────────────────
    // Core API
    // ──────────────────────────────────────────────────────────────────────

    /**
     * @brief Classify the workload of @p tenant_id from @p stats.
     *
     * @param tenant_id  Tenant identifier (informational; copied into result).
     * @param stats      Statistics snapshot for the current observation window.
     * @return WorkloadFingerprint with the dominant pattern, probability vector,
     *         confidence, and resource policy recommendation.
     *
     * Runtime: ≤ 50 ms p99 (pure in-memory computation).
     */
    WorkloadFingerprint classify(
        const std::string&        tenant_id,
        const TenantWorkloadStats& stats
    ) const;

    /**
     * @brief Compute cosine similarity between two fingerprint vectors.
     *
     * @return Similarity in [0.0, 1.0]; 1.0 = identical, 0.0 = orthogonal.
     */
    double similarityTo(
        const WorkloadFingerprint& a,
        const WorkloadFingerprint& b
    ) const;

    /// Human-readable name for a WorkloadPattern.
    static std::string patternName(WorkloadPattern p);

private:
    /// Build the recommended resource policy for a given pattern.
    static WorkloadFingerprint::PolicyRecommendation buildPolicy(WorkloadPattern pattern);
};

} // namespace server
} // namespace themis
