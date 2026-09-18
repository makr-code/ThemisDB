/**
 * @file workload_fingerprint_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct TenantWorkloadStats {
    std::string tenant_id;

    uint64_t query_count = 0;

    double write_ratio = 0.0;

    double avg_p99_ms = 0.0;

    uint64_t avg_rows_per_query = 0;

    uint32_t bulk_insert_count = 0;

    uint32_t window_seconds = 60;
};

// ---------------------------------------------------------------------------
// WorkloadFingerprintEngine
// ---------------------------------------------------------------------------

class WorkloadFingerprintEngine {
public:
    // ──────────────────────────────────────────────────────────────────────
    // Types
    // ──────────────────────────────────────────────────────────────────────

    enum class WorkloadPattern {
        OLTP,    ///< Many small, short transactions
        OLAP,    ///< Few, heavy analytical queries
        BATCH,   ///< Periodic bulk operations
        MIXED,   ///< No single pattern dominates
        UNKNOWN  ///< Insufficient data
    };

    struct WorkloadFingerprint {
        std::string tenant_id;
        WorkloadPattern pattern;
        std::vector<double> vector;
        double confidence;

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
     * @brief Classify the semantic intent of a query.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] stats Input parameter.
     * @return Return value.
     */
    WorkloadFingerprint classify(
        const std::string&        tenant_id,
        const TenantWorkloadStats& stats
    ) const;

    /**
     * @brief Similarity To.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    double similarityTo(
        const WorkloadFingerprint& a,
        const WorkloadFingerprint& b
    ) const;

    /**
     * @brief Pattern Name.
     * @param[in] p Input parameter.
     * @return Return value.
     */
    static std::string patternName(WorkloadPattern p);

private:
    /**
     * @brief Build Policy.
     * @param[in] pattern Input parameter.
     * @return Return value.
     */
    static WorkloadFingerprint::PolicyRecommendation buildPolicy(WorkloadPattern pattern);
};

} // namespace server
} // namespace themis
