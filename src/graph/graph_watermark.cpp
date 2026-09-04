/**
 * @file graph_watermark.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "graph/graph_watermark.h"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace graph {

// ─── GraphWatermark ───────────────────────────────────────────────────────

std::vector<std::string> GraphWatermark::generateWatermarkIds(const std::string &tenant_id, uint64_t seed) {
    const int k = static_cast<int>(seed % 5) + 3; // k ∈ [3, 7]
    std::vector<std::string> ids;
    ids.reserve(static_cast<size_t>(k));
    for (int i = 0; i < k; ++i) {
        std::ostringstream oss = {};
        oss << "wm_" << tenant_id << "_" << seed << "_" << i;
        ids.push_back(oss.str());
    }
    return ids;
}

bool GraphWatermark::hasCollision(const GraphSnapshot &snapshot, const std::vector<std::string> &wm_ids) {
    const std::unordered_set<std::string> existing(snapshot.node_ids.begin(), snapshot.node_ids.end());
    for (const auto &id : wm_ids) {
        if (existing.count(id)) {
            return true;
        }
    }
    return false;
}

WatermarkedSnapshot GraphWatermark::embed(const GraphSnapshot &snapshot, const std::string &tenant_id,
                                          uint64_t seed) const {
    WatermarkedSnapshot result;
    result.data = snapshot;

    if (!graph_watermarking_enabled) {
        result.fingerprint_id = "";
        return result;
    }

    // Resolve seed collisions
    uint64_t effective_seed = seed;
    std::vector<std::string> wm_ids;
    constexpr int kMaxCollisionRetries = 100;
    int attempts                       = 0;
    do {
        wm_ids = generateWatermarkIds(tenant_id, effective_seed);
        if (!hasCollision(result.data, wm_ids)) {
            break;
        }
        ++effective_seed;
        ++attempts;
    } while (attempts < kMaxCollisionRetries);

    // Add watermark nodes
    for (const auto &id : wm_ids) {
        result.data.node_ids.push_back(id);
        result.data.node_metadata[id] = "wm:tenant=" + tenant_id + ";seed=" + std::to_string(effective_seed);
    }

    // Add edges between consecutive watermark nodes (chain)
    for (size_t i = 0; i + 1 < wm_ids.size(); ++i) {
        result.data.edges.emplace_back(wm_ids[i], wm_ids[i + 1]);
    }

    // Build fingerprint_id: "fp_<tenant>_<seed>"
    result.fingerprint_id = "fp_" + tenant_id + "_" + std::to_string(effective_seed);

    return result;
}

// ─── GraphFingerprintDetector ─────────────────────────────────────────────

double GraphFingerprintDetector::jaccard(const std::vector<std::string> &a, const std::vector<std::string> &b) {
    if (a.empty() && b.empty()) {
        return 1.0;
    }
    if (a.empty() || b.empty()) {
        return 0.0;
    }

    const std::unordered_set<std::string> set_a(a.begin(), a.end());
    size_t intersection = 0;
    for (const auto &elem : b) {
        if (set_a.count(elem)) {
            ++intersection;
        }
    }
    const size_t union_size = static_cast<int>(set_a.size()) + static_cast<int>(b.size()) - intersection;
    return union_size == 0 ? 0.0 : static_cast<double>(intersection) / static_cast<double>(union_size);
}

std::optional<FingerprintMatch>
GraphFingerprintDetector::detect(const GraphSnapshot &suspect,
                                 const std::vector<RegisteredFingerprint> &fingerprints) const {
    if (fingerprints.empty()) {
        return std::nullopt;
    }

    std::optional<FingerprintMatch> best;

    for (const auto &fp : fingerprints) {
        const double sim = jaccard(suspect.node_ids, fp.watermark_node_ids);
        if (sim >= kMatchThreshold) {
            if (!best.has_value() || sim > best->confidence) {
                best = FingerprintMatch{fp.tenant_id, sim};
            }
        }
    }

    return best;
}

} // namespace graph
} // namespace themis
