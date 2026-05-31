/*
 * ThemisDB | File: graph_watermark.h | Version: 0.0.1 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 148
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

/**
 * @file graph_watermark.h
 * @brief Graph watermarking and fingerprint detection for data provenance.
 *
 * Phase 8.1: Graph Watermarking & Fingerprinting
 *
 * Embeds imperceptible watermarks into graph snapshots by adding a small
 * number of synthetic "watermark nodes" with provenance metadata. Enables
 * detection of unauthorised graph exports by computing Jaccard similarity
 * between the suspect graph and registered fingerprints.
 */

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace themis {
namespace graph {

// ─── Data structures ──────────────────────────────────────────────────────

/**
 * @brief An immutable snapshot of a graph (nodes + edges + metadata).
 */
struct GraphSnapshot {
    std::vector<std::string>                     node_ids;    ///< All node IDs
    std::vector<std::pair<std::string,std::string>> edges;   ///< (from, to) pairs
    std::map<std::string,std::string>            node_metadata; ///< Per-node metadata
};

/**
 * @brief A graph snapshot with an embedded watermark fingerprint.
 */
struct WatermarkedSnapshot {
    GraphSnapshot data;           ///< The watermarked graph
    std::string   fingerprint_id; ///< Unique fingerprint identifier for this embedding
};

/**
 * @brief A registered watermark fingerprint for a tenant.
 */
struct RegisteredFingerprint {
    std::string              tenant_id;           ///< Owning tenant
    std::string              fingerprint_id;      ///< Unique fingerprint ID (matches WatermarkedSnapshot)
    std::vector<std::string> watermark_node_ids;  ///< Node IDs that constitute the watermark
};

/**
 * @brief Result of a fingerprint detection attempt.
 */
struct FingerprintMatch {
    std::string tenant_id;  ///< Tenant whose watermark was detected
    double      confidence; ///< Jaccard similarity score [0.0, 1.0]
};

// ─── GraphWatermark ───────────────────────────────────────────────────────

/**
 * @brief Embeds watermark nodes into a graph snapshot.
 *
 * The watermark consists of k synthetic nodes (k = seed % 5 + 3) with IDs
 * like `wm_<tenant>_<seed>_<i>`. Edges are added between watermark nodes
 * and provenance metadata is set.
 *
 * Seed collision handling: if a watermark node ID already exists in the
 * snapshot, the seed is incremented and watermark generation is retried.
 */
class GraphWatermark {
public:
    GraphWatermark() = default;

    /// If false, embed() returns the input snapshot unchanged.
    bool graph_watermarking_enabled = false;

    /**
     * @brief Embed a watermark into the given graph snapshot.
     *
     * @param snapshot   Source graph snapshot (copied, not modified).
     * @param tenant_id  Tenant identifier included in watermark node IDs.
     * @param seed       Deterministic seed for watermark generation.
     * @return WatermarkedSnapshot with embedded watermark and fingerprint_id.
     */
    WatermarkedSnapshot embed(
        const GraphSnapshot& snapshot,
        const std::string&   tenant_id,
        uint64_t             seed) const;

private:
    /// Generate watermark node IDs for a given seed and tenant.
    static std::vector<std::string> generateWatermarkIds(
        const std::string& tenant_id,
        uint64_t           seed);

    /// Check for ID collision with existing snapshot nodes.
    static bool hasCollision(
        const GraphSnapshot&            snapshot,
        const std::vector<std::string>& wm_ids);
};

// ─── GraphFingerprintDetector ─────────────────────────────────────────────

/**
 * @brief Detects registered watermark fingerprints in a suspect graph.
 *
 * Uses Jaccard similarity between the suspect's node IDs and each registered
 * fingerprint's watermark node IDs. A match is reported when similarity ≥ 0.95.
 */
class GraphFingerprintDetector {
public:
    /// Minimum Jaccard similarity required to report a match.
    static constexpr double kMatchThreshold = 0.95;

    GraphFingerprintDetector() = default;

    /**
     * @brief Detect whether any registered fingerprint is present in the suspect graph.
     *
     * @param suspect      Graph snapshot to check.
     * @param fingerprints List of registered fingerprints to match against.
     * @return FingerprintMatch if similarity ≥ kMatchThreshold, std::nullopt otherwise.
     */
    std::optional<FingerprintMatch> detect(
        const GraphSnapshot&                   suspect,
        const std::vector<RegisteredFingerprint>& fingerprints) const;

private:
    /// Compute Jaccard similarity between two sets of node IDs.
    static double jaccard(
        const std::vector<std::string>& a,
        const std::vector<std::string>& b);
};

} // namespace graph
} // namespace themis
