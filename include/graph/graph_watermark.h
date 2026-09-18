/**
 * @file graph_watermark.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

struct GraphSnapshot {
    std::vector<std::string>                     node_ids;    ///< All node IDs
    std::vector<std::pair<std::string,std::string>> edges;   ///< (from, to) pairs
    std::map<std::string,std::string>            node_metadata; ///< Per-node metadata
};

struct WatermarkedSnapshot {
    GraphSnapshot data;           ///< The watermarked graph
    std::string   fingerprint_id; ///< Unique fingerprint identifier for this embedding
};

struct RegisteredFingerprint {
    std::string              tenant_id;           ///< Owning tenant
    std::string              fingerprint_id;      ///< Unique fingerprint ID (matches WatermarkedSnapshot)
    std::vector<std::string> watermark_node_ids;  ///< Node IDs that constitute the watermark
};

struct FingerprintMatch {
    std::string tenant_id;  ///< Tenant whose watermark was detected
    double      confidence; ///< Jaccard similarity score [0.0, 1.0]
};

// ─── GraphWatermark ───────────────────────────────────────────────────────

class GraphWatermark {
public:
    GraphWatermark() = default;

    bool graph_watermarking_enabled = false;

    /**
     * @brief Embed.
     * @param[in] snapshot Input parameter.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] seed Input parameter.
     * @return Return value.
     */
    WatermarkedSnapshot embed(
        const GraphSnapshot& snapshot,
        const std::string&   tenant_id,
        uint64_t             seed) const;

private:
    /**
     * @brief Generate Watermark Ids.
     * @param[in] tenant_id Identifier of the tenant.
     * @param[in] seed Input parameter.
     * @return Return value.
     */
    static std::vector<std::string> generateWatermarkIds(
        const std::string& tenant_id,
        uint64_t           seed);

    /**
     * @brief Has Collision.
     * @param[in] snapshot Input parameter.
     * @param[in] wm_ids Input parameter.
     * @return True when the operation succeeds.
     */
    static bool hasCollision(
        const GraphSnapshot&            snapshot,
        const std::vector<std::string>& wm_ids);
};

// ─── GraphFingerprintDetector ─────────────────────────────────────────────

class GraphFingerprintDetector {
public:
    static constexpr double kMatchThreshold = 0.95;

    GraphFingerprintDetector() = default;

    /**
     * @brief Detect.
     * @param[in] suspect Input parameter.
     * @param[in] fingerprints Input parameter.
     * @return Return value.
     */
    std::optional<FingerprintMatch> detect(
        const GraphSnapshot&                   suspect,
        const std::vector<RegisteredFingerprint>& fingerprints) const;

private:
    /**
     * @brief Jaccard.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    static double jaccard(
        const std::vector<std::string>& a,
        const std::vector<std::string>& b);
};

} // namespace graph
} // namespace themis
