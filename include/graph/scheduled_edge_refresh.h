/**
 * @file scheduled_edge_refresh.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "analytics/cep_engine.h"
#include "cdc/changefeed.h"
#include "index/ann_index.h"
#include "index/graph_index.h"
#include "utils/expected.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// RefreshPolicy
// ─────────────────────────────────────────────────────────────────────────────

enum class SimilarityMetric {
    COSINE,      ///< Cosine similarity between node embedding vectors
    DOT_PRODUCT, ///< Dot-product similarity
    EUCLIDEAN    ///< Negative Euclidean distance (higher = more similar)
};

struct RefreshPolicy {
    // ── Scheduling ────────────────────────────────────────────────────────────
    std::chrono::seconds refresh_interval{3600};

    // ── Similarity ────────────────────────────────────────────────────────────
    SimilarityMetric similarity_metric{SimilarityMetric::COSINE};

    float relevance_threshold{0.5f};

    float add_threshold{0.7f};

    uint32_t top_k_candidates{10};

    // ── Temporal decay ────────────────────────────────────────────────────────
    double decay_half_life_seconds{86400.0}; // 24 h default

    // ── Safety gates ──────────────────────────────────────────────────────────
    float max_removal_fraction{0.10f};

    uint32_t max_edges_to_add{1000};

    uint32_t max_edges_to_remove{500};

    // ── Graph scope ───────────────────────────────────────────────────────────
    std::string graph_id;

    // ── Anomaly detection ─────────────────────────────────────────────────────
    float anomaly_threshold_removal_rate{0.0f};

    // ── ANN acceleration ──────────────────────────────────────────────────────
    size_t ann_min_vertices{10000};

    RefreshPolicy() = default;
};

// ─────────────────────────────────────────────────────────────────────────────
// EdgeScore
// ─────────────────────────────────────────────────────────────────────────────

struct EdgeScore {
    std::string edge_id;
    std::string from_vertex;
    std::string to_vertex;

    float similarity{0.0f};

    float temporal_factor{1.0f};

    float centrality_weight{1.0f};

    float relevance{0.0f};

    bool is_removal_candidate{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// RefreshStats
// ─────────────────────────────────────────────────────────────────────────────

struct RefreshStats {
    uint64_t edges_evaluated{0};

    uint64_t edges_removed{0};

    uint64_t edges_added{0};

    uint64_t candidate_pairs_evaluated{0};

    double avg_relevance_retained{0.0};

    double avg_relevance_improvement{0.0};

    double cycle_duration_ms{0.0};

    bool aborted_safety_gate{false};

    uint64_t total_cycles_completed{0};

    // ── Anomaly detection metrics ─────────────────────────────────────────────

    double removal_rate{0.0};

    bool anomaly_high_removal_rate{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// RefreshAuditEntry
// ─────────────────────────────────────────────────────────────────────────────

struct RefreshAuditEntry {
    enum class Action { ADD, REMOVE };

    Action action;
    std::string edge_id;
    std::string from_vertex;
    std::string to_vertex;
    float relevance_score{0.0f};
    std::chrono::system_clock::time_point timestamp;
    uint64_t cycle_number{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// NodeEmbeddingProvider
// ─────────────────────────────────────────────────────────────────────────────

using NodeEmbeddingProvider =
    std::function<std::vector<float>(const std::string& node_id)>;

// ─────────────────────────────────────────────────────────────────────────────
// ScheduledGraphEdgeRefreshEngine
// ─────────────────────────────────────────────────────────────────────────────

class ScheduledGraphEdgeRefreshEngine {
public:
    // ── Construction / destruction ────────────────────────────────────────────

    explicit ScheduledGraphEdgeRefreshEngine(
        GraphIndexManager& graph_mgr,
        const RefreshPolicy& policy,
        NodeEmbeddingProvider embedding_fn = nullptr);

    ~ScheduledGraphEdgeRefreshEngine();

    // Non-copyable, movable
    ScheduledGraphEdgeRefreshEngine(const ScheduledGraphEdgeRefreshEngine&) = delete;
    ScheduledGraphEdgeRefreshEngine& operator=(const ScheduledGraphEdgeRefreshEngine&) = delete;
    ScheduledGraphEdgeRefreshEngine(ScheduledGraphEdgeRefreshEngine&&) = delete;
    ScheduledGraphEdgeRefreshEngine& operator=(ScheduledGraphEdgeRefreshEngine&&) = delete;

    /**
     * @brief ── Lifecycle ─────────────────────────────────────────────────────────────
     */

    void start();

    /**
     * @brief Stop.
     */
    void stop();

    /**
     * @brief ── Manual trigger ────────────────────────────────────────────────────────
     * @return Return value.
     */

    RefreshStats triggerRefresh();

    /**
     * @brief ── Observation ───────────────────────────────────────────────────────────
     * @return Return value.
     */

    RefreshStats getStats() const;

    /**
     * @brief Get Audit Trail.
     * @return Return value.
     */
    std::vector<RefreshAuditEntry> getAuditTrail() const;

    const RefreshPolicy& getPolicy() const { return policy_; }

    /**
     * @brief Set Policy.
     * @param[in] policy Input parameter.
     */
    void setPolicy(const RefreshPolicy& policy);

    /**
     * @brief Set Changefeed.
     * @param[in] changefeed Input parameter.
     */
    void setChangefeed(std::shared_ptr<Changefeed> changefeed);

    /**
     * @brief Set ANNIndex.
     * @param[in] ann_index Input parameter.
     */
    void setANNIndex(std::shared_ptr<index::IAnnIndex> ann_index);

    void setCEPEventCallback(
        std::function<void(themisdb::analytics::Event)> callback);

    /**
     * @brief ── Scoring helpers (exposed for testability) ─────────────────────────────
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */

    float computeSimilarity(const std::vector<float>& a,
                            const std::vector<float>& b) const;

    /**
     * @brief Compute Temporal Decay.
     * @param[in] edge_entity Input parameter.
     * @return Return value.
     */
    float computeTemporalDecay(const BaseEntity& edge_entity) const;

    /**
     * @brief Score Edge.
     * @param[in] edge_entity Input parameter.
     * @return Return value.
     */
    EdgeScore scoreEdge(const BaseEntity& edge_entity) const;

private:
    /**
     * @brief ── Internal helpers ──────────────────────────────────────────────────────
     * @param[in] policy Input parameter.
     */

    static void validatePolicy(const RefreshPolicy& policy);

    /**
     * @brief Scheduler Loop.
     */
    void schedulerLoop();

    /**
     * @brief Run Refresh Cycle.
     * @return Return value.
     */
    RefreshStats runRefreshCycle();

    /**
     * @brief Collect Edges.
     * @return Return value.
     */
    std::vector<BaseEntity> collectEdges() const;

    /**
     * @brief Score All Edges.
     * @param[in] edges Input parameter.
     * @return Return value.
     */
    std::vector<EdgeScore> scoreAllEdges(
        const std::vector<BaseEntity>& edges) const;

    std::vector<std::tuple<std::string, std::string, float>>
    discoverCandidateEdges(
        const std::vector<BaseEntity>& existing_edges) const;

    bool applyBatch(const std::vector<std::string>& edge_ids_to_remove,
                    const std::vector<std::tuple<std::string, std::string, float>>& edges_to_add,
                    uint64_t cycle_number);

    /**
     * @brief Append Audit.
     * @param[in] entry Input parameter.
     */
    void appendAudit(RefreshAuditEntry entry);

    /**
     * @brief Rebuild ANNIndex.
     * @param[in] vertices Input parameter.
     */
    void rebuildANNIndex(const std::vector<std::string>& vertices) const;

    // ── Data members ──────────────────────────────────────────────────────────

    GraphIndexManager& graph_mgr_;
    RefreshPolicy policy_;
    NodeEmbeddingProvider embedding_fn_;
    std::shared_ptr<Changefeed> changefeed_; ///< Optional – may be nullptr
    std::shared_ptr<index::IAnnIndex> ann_index_; ///< Optional ANN index
    std::function<void(themisdb::analytics::Event)> cep_event_callback_; ///< Optional CEP callback

    // Vertex ↔ integer-ID mapping built by rebuildANNIndex().
    // Protected by stats_mutex_ (rebuilt inside discoverCandidateEdges which
    // is called only from runRefreshCycle which holds cycle_mutex_).
    mutable std::unordered_map<std::string, int64_t> ann_vertex_to_idx_;
    mutable std::vector<std::string>                  ann_idx_to_vertex_;

    // ─────────────────────────────────────────────────────────────────────────
    // Lock Hierarchy (CANONICAL ORDER for deadlock prevention):
    // Tier 1 (Acquire FIRST): cycle_mutex_
    // Tier 2 (Acquire SECOND): policy_mutex_
    // Tier 3 (Acquire LAST): stats_mutex_, cv_mutex_
    //
    // CRITICAL: Always acquire locks in Tier 1 → 2 → 3 order.
    // If you need multiple locks, acquire higher tiers first.
    // NEVER reverse this order, or deadlocks will occur.
    // ─────────────────────────────────────────────────────────────────────────

    mutable std::mutex policy_mutex_;   ///< Tier 2: Protects policy_ updates
    mutable std::mutex cycle_mutex_;    ///< Tier 1: Serialises concurrent triggerRefresh calls
    mutable std::mutex stats_mutex_;    ///< Tier 3: Protects last_stats_ / audit_trail_ / changefeed_
    std::condition_variable cv_ = {};
    std::mutex cv_mutex_;               ///< Tier 3: Protects condition variable

    std::thread scheduler_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stop_requested_{false};

    RefreshStats last_stats_;
    std::vector<RefreshAuditEntry> audit_trail_;

    std::atomic<uint64_t> cycle_counter_{0};

    static constexpr size_t kMaxAuditEntries = 10000;
};

} // namespace graph
} // namespace themis
