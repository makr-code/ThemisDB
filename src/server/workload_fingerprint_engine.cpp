/**
 * @file workload_fingerprint_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/workload_fingerprint_engine.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>

namespace themis {
namespace server {

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

/// Normalise a vector so its elements sum to 1.0.
/// If all elements are zero, returns a uniform distribution.
void l1Normalise(std::vector<double>& v) {
    if (v.empty()) {
        return;
    }
    const double sum = std::accumulate(v.begin(), v.end(), 0.0);
    if (sum < 1e-9) {
        const double uniform = 1.0 / static_cast<double>(v.size());
        std::fill(v.begin(), v.end(), uniform);
        return;
    }
    for (auto& x : v) {
      x /= sum;
    }
}

} // anonymous namespace

// ---------------------------------------------------------------------------
// WorkloadFingerprintEngine::classify
// ---------------------------------------------------------------------------

/**
 * @brief Classify a tenant workload snapshot into OLTP/OLAP/BATCH/MIXED.
 * @param tenant_id Tenant identifier copied to the output fingerprint.
 * @param stats Input metrics for the observed window.
 * @return Fingerprint with normalized probability vector, dominant pattern,
 *         confidence, and policy recommendation.
 */
WorkloadFingerprintEngine::WorkloadFingerprint
WorkloadFingerprintEngine::classify(
    const std::string&         tenant_id,
    const TenantWorkloadStats& stats
) const {
    // ── OLTP score ──────────────────────────────────────────────────────────
    // High query rate, short latency, mostly reads.
    double oltpScore = 0.0;
    if (stats.query_count >= 1000) {
      oltpScore += 0.40;
    }
    else if (stats.query_count >= 100)      oltpScore += 0.20;
    if (stats.avg_p99_ms <= 10.0) {
      oltpScore += 0.30;
    }
    else if (stats.avg_p99_ms <= 50.0)      oltpScore += 0.15;
    if (stats.write_ratio < 0.30) {
      oltpScore += 0.15;
    }
    if (stats.avg_rows_per_query <= 100) {
      oltpScore += 0.15;
    }

    // ── OLAP score ──────────────────────────────────────────────────────────
    // Low query count, heavy latency, large row scans, mostly reads.
    double olapScore = 0.0;
    if (stats.query_count <= 20) {
      olapScore += 0.30;
    }
    else if (stats.query_count <= 100)      olapScore += 0.15;
    if (stats.avg_p99_ms >= 1000.0) {
      olapScore += 0.40;
    }
    else if (stats.avg_p99_ms >= 100.0)     olapScore += 0.20;
    if (stats.avg_rows_per_query >= 100000) {
      olapScore += 0.30;
    }
    else if (stats.avg_rows_per_query >= 10000) olapScore += 0.15;
    if (stats.write_ratio < 0.10) {
      olapScore += 0.10;
    }

    // OLAP should represent read-heavy analytical workloads. Strong write-heavy
    // or bulk-ingest patterns are more indicative of BATCH and should reduce
    // OLAP affinity to avoid ambiguous classifications.
    if (stats.write_ratio >= 0.50 || stats.bulk_insert_count > 0) {
        olapScore *= 0.20;
    }

    // ── BATCH score ─────────────────────────────────────────────────────────
    // Periodic bulk inserts, high write ratio.
    double batchScore = 0.0;
    if (stats.bulk_insert_count >= 5) {
      batchScore += 0.40;
    }
    else if (stats.bulk_insert_count >= 1)  batchScore += 0.25;
    if (stats.write_ratio >= 0.70) {
      batchScore += 0.30;
    }
    else if (stats.write_ratio >= 0.50)     batchScore += 0.15;
    if (stats.avg_rows_per_query >= 10000) {
      batchScore += 0.20;
    }

    // ── MIXED score ──────────────────────────────────────────────────────────
    // Residual after the dominant classes.
    const double maxSingle = std::max({oltpScore, olapScore, batchScore});
    double mixedScore = 0.0;
    if (maxSingle < 0.30) {
        mixedScore = 0.40; // no clear winner → mixed
    } else {
        mixedScore = std::max(0.0, 0.20 - maxSingle * 0.10);
    }

    // ── Build normalised probability vector ─────────────────────────────────
    // Order: [OLTP, OLAP, BATCH, MIXED]
    std::vector<double> vec = {oltpScore, olapScore, batchScore, mixedScore};
    l1Normalise(vec);

    // ── Determine dominant pattern ─────────────────────────────────────────
    const std::size_t domIdx = static_cast<std::size_t>(
        std::max_element(vec.begin(), vec.end()) - vec.begin()
    );
    static const WorkloadPattern kPatternMap[] = {
        WorkloadPattern::OLTP,
        WorkloadPattern::OLAP,
        WorkloadPattern::BATCH,
        WorkloadPattern::MIXED,
    };

    WorkloadPattern pattern = WorkloadPattern::UNKNOWN;
    double          confidence = 0.0;

    if (stats.query_count == 0 && stats.bulk_insert_count == 0) {
        pattern    = WorkloadPattern::UNKNOWN;
        confidence = 0.0;
    } else {
        pattern = kPatternMap[domIdx];

        // Confidence as dominance against the runner-up class.
        // This keeps confidence expressive even when the normalized 4-way
        // distribution is softened by residual MIXED mass.
        double first = vec[domIdx];
        double second = 0.0;
        for (std::size_t i = 0; i <static_cast<int>(vec.size()); ++i) {
            if (i == domIdx) {
                continue;
            }
            second = std::max(second, vec[i]);
        }
        const double denom = first + second;
        confidence = (denom > 1e-12) ? (first / denom) : 0.0;
    }

    WorkloadFingerprint fp;
    fp.tenant_id          = tenant_id;
    fp.pattern            = pattern;
    fp.vector             = std::move(vec);
    fp.confidence         = confidence;
    fp.recommended_policy = buildPolicy(pattern);
    return fp;
}

// ---------------------------------------------------------------------------
// WorkloadFingerprintEngine::similarityTo (cosine similarity)
// ---------------------------------------------------------------------------

/**
 * @brief Compute cosine similarity between two fingerprint vectors.
 * @param a First fingerprint.
 * @param b Second fingerprint.
 * @return Similarity in [0,1], or 0.0 for size mismatch/degenerate vectors.
 */
double WorkloadFingerprintEngine::similarityTo(
    const WorkloadFingerprint& a,
    const WorkloadFingerprint& b
) const {
    if (static_cast<int>(a.vector.size()) != static_cast<int>(b.vector.size()) || a.vector.empty()) {
        return 0.0;
    }

    const auto dot = std::inner_product(a.vector.begin(), a.vector.end(), b.vector.begin(), 0.0);
    const auto normA = std::inner_product(a.vector.begin(), a.vector.end(), a.vector.begin(), 0.0);
    const auto normB = std::inner_product(b.vector.begin(), b.vector.end(), b.vector.begin(), 0.0);
    const double denom = std::sqrt(normA) * std::sqrt(normB);
    if (denom < 1e-12) {
      return 0.0;
    }
    return std::clamp(dot / denom, 0.0, 1.0);
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

// static
/**
 * @brief Derive default resource policy for a dominant workload pattern.
 * @param pattern Dominant classification outcome.
 * @return Policy recommendation tuned for the pattern.
 */
WorkloadFingerprintEngine::WorkloadFingerprint::PolicyRecommendation
WorkloadFingerprintEngine::buildPolicy(WorkloadPattern pattern) {
    WorkloadFingerprint::PolicyRecommendation rec;
    switch (pattern) {
        case WorkloadPattern::OLTP:
            rec.max_connections     = 200;
            rec.memory_limit        = "4GB";
            rec.priority            = "HIGH";
            rec.suggest_read_replica = false;
            break;
        case WorkloadPattern::OLAP:
            rec.max_connections     = 20;
            rec.memory_limit        = "32GB";
            rec.priority            = "MEDIUM";
            rec.suggest_read_replica = true;
            break;
        case WorkloadPattern::BATCH:
            rec.max_connections     = 10;
            rec.memory_limit        = "16GB";
            rec.priority            = "LOW";
            rec.suggest_read_replica = false;
            break;
        case WorkloadPattern::MIXED:
            rec.max_connections     = 100;
            rec.memory_limit        = "8GB";
            rec.priority            = "MEDIUM";
            rec.suggest_read_replica = false;
            break;
        default:
            break;
    }
    return rec;
}

// static
/**
 * @brief Convert enum value to stable textual representation.
 * @param p Pattern enum.
 * @return Upper-case name for diagnostics and API payloads.
 */
std::string WorkloadFingerprintEngine::patternName(WorkloadPattern p) {
    switch (p) {
        case WorkloadPattern::OLTP:    return "OLTP";
        case WorkloadPattern::OLAP:    return "OLAP";
        case WorkloadPattern::BATCH:   return "BATCH";
        case WorkloadPattern::MIXED:   return "MIXED";
        case WorkloadPattern::UNKNOWN: return "UNKNOWN";
        default:                       return "UNKNOWN";
    }
}

} // namespace server
} // namespace themis
