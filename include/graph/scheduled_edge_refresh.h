/**
 * @file scheduled_edge_refresh.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Similarity metric used to score candidate edges.
 *
 * DE: Ähnlichkeitsmetrik für die Bewertung von Kanten.
 */
enum class SimilarityMetric {
    COSINE,      ///< Cosine similarity between node embedding vectors
    DOT_PRODUCT, ///< Dot-product similarity
    EUCLIDEAN    ///< Negative Euclidean distance (higher = more similar)
};

/**
 * @brief Configuration policy controlling how and when edges are refreshed.
 *
 * All thresholds and limits are validated at engine construction time.
 * Invalid (out-of-range) values cause the constructor to throw
 * std::invalid_argument.
 *
 * DE: Konfigurationsrichtlinie für den Refresh-Zyklus.
 */
struct RefreshPolicy {
    // ── Scheduling ────────────────────────────────────────────────────────────
    /// Refresh interval. Zero disables automatic scheduling (manual trigger only).
    std::chrono::seconds refresh_interval{3600};

    // ── Similarity ────────────────────────────────────────────────────────────
    /// Metric used to compare node embedding vectors.
    SimilarityMetric similarity_metric{SimilarityMetric::COSINE};

    /// Minimum similarity score [0, 1] required to keep or create an edge.
    /// Edges with score < relevance_threshold are candidates for removal.
    float relevance_threshold{0.5f};

    /// Minimum similarity score [0, 1] for a new candidate edge to be added.
    float add_threshold{0.7f};

    /// Top-k candidate edges to consider per node during new-edge discovery.
    uint32_t top_k_candidates{10};

    // ── Temporal decay ────────────────────────────────────────────────────────
    /// Half-life for temporal relevance decay in seconds (0 = no decay).
    double decay_half_life_seconds{86400.0}; // 24 h default

    // ── Safety gates ──────────────────────────────────────────────────────────
    /// Maximum fraction [0, 1] of existing edges that may be removed in a
    /// single refresh cycle.  If the computed removal count exceeds this
    /// fraction the entire batch is aborted (rollback).
    float max_removal_fraction{0.10f};

    /// Maximum number of edges that may be added per refresh cycle (0 = unlimited).
    uint32_t max_edges_to_add{1000};

    /// Maximum number of edges that may be removed per refresh cycle (0 = unlimited).
    uint32_t max_edges_to_remove{500};

    // ── Graph scope ───────────────────────────────────────────────────────────
    /// If non-empty, restrict refresh to this graph ID.
    std::string graph_id;

    // ── Anomaly detection ─────────────────────────────────────────────────────
    /// Removal rate fraction [0, 1] above which a cycle is flagged as anomalous.
    /// 0 = anomaly detection disabled.
    /// DE: Entfernungsrate, ab der ein Zyklus als anomal markiert wird.
    float anomaly_threshold_removal_rate{0.0f};

    // ── ANN acceleration ──────────────────────────────────────────────────────
    /// Minimum number of vertices in the graph for the engine to use an
    /// attached ANN index instead of the O(V²) brute-force similarity scan
    /// during candidate edge discovery.  Set to 0 to always use ANN when
    /// an index has been attached via setANNIndex().
    /// DE: Mindestanzahl an Knoten für ANN-beschleunigte Kandidatenerkennung.
    size_t ann_min_vertices{10000};

    RefreshPolicy() = default;
};

// ─────────────────────────────────────────────────────────────────────────────
// EdgeScore
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Relevance score for a single edge.
 *
 * The final relevance is a weighted combination of similarity, temporal decay,
 * and centrality:
 *   relevance = similarity * temporal_factor * centrality_weight
 *
 * DE: Relevanzbewertung für eine einzelne Kante.
 */
struct EdgeScore {
    std::string edge_id;
    std::string from_vertex;
    std::string to_vertex;

    /// Raw similarity score in [0, 1].
    float similarity{0.0f};

    /// Temporal decay factor in (0, 1] (1.0 = no decay).
    float temporal_factor{1.0f};

    /// Centrality-based weight in (0, 1] (proportional to vertex degree).
    float centrality_weight{1.0f};

    /// Combined relevance score (similarity * temporal_factor * centrality_weight).
    float relevance{0.0f};

    /// Whether this edge is a candidate for removal (relevance < threshold).
    bool is_removal_candidate{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// RefreshStats
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Statistics collected during a single refresh cycle.
 *
 * DE: Statistiken eines einzelnen Refresh-Zyklus.
 */
struct RefreshStats {
    /// Number of existing edges evaluated.
    uint64_t edges_evaluated{0};

    /// Number of edges removed in this cycle.
    uint64_t edges_removed{0};

    /// Number of edges added in this cycle.
    uint64_t edges_added{0};

    /// Number of candidate pairs evaluated for new edges.
    uint64_t candidate_pairs_evaluated{0};

    /// Average relevance score of retained edges (after refresh).
    double avg_relevance_retained{0.0};

    /// Average relevance improvement relative to the pre-refresh average.
    double avg_relevance_improvement{0.0};

    /// Wall-clock time for this cycle in milliseconds.
    double cycle_duration_ms{0.0};

    /// Whether the cycle was aborted due to a safety gate violation.
    bool aborted_safety_gate{false};

    /// Total number of completed refresh cycles since engine start.
    uint64_t total_cycles_completed{0};

    // ── Anomaly detection metrics ─────────────────────────────────────────────

    /// Fraction of evaluated edges that were removed [0, 1].
    /// removal_rate = edges_removed / edges_evaluated (0 when edges_evaluated == 0).
    /// DE: Anteil entfernter Kanten an der Gesamtzahl bewerteter Kanten.
    double removal_rate{0.0};

    /// True when removal_rate exceeded policy.anomaly_threshold_removal_rate
    /// and anomaly detection was enabled (threshold > 0).
    /// DE: Gibt an, ob eine anomal hohe Entfernungsrate erkannt wurde.
    bool anomaly_high_removal_rate{false};
};

// ─────────────────────────────────────────────────────────────────────────────
// RefreshAuditEntry
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Immutable record of a single edge mutation in the audit trail.
 *
 * DE: Unveränderlicher Eintrag in der Prüfspur für eine Kantenmutation.
 */
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

/**
 * @brief Callback interface for retrieving node embedding vectors.
 *
 * The engine calls this function for every node it needs to score.  The caller
 * must supply or wire up an implementation that looks up the appropriate
 * embedding from the acceleration module, GNN index, or an in-memory cache.
 *
 * Returns an empty vector if no embedding is available for @p node_id; in that
 * case the node is skipped during similarity scoring.
 *
 * DE: Callback-Schnittstelle zum Abrufen von Knoten-Einbettungsvektoren.
 */
using NodeEmbeddingProvider =
    std::function<std::vector<float>(const std::string& node_id)>;

// ─────────────────────────────────────────────────────────────────────────────
// ScheduledGraphEdgeRefreshEngine
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Automatically refreshes graph edges based on semantic similarity.
 *
 * ## Overview
 *
 * The engine runs a background scheduler that wakes up at the configured
 * `refresh_interval`.  Each cycle:
 *
 *  1. Enumerates all edges (optionally scoped to a single graph).
 *  2. Scores each edge using a combination of vector similarity (cosine /
 *     dot-product / Euclidean), temporal decay, and centrality.
 *  3. Marks low-scoring edges as removal candidates.
 *  4. Discovers top-k new candidate edges per node using embedding similarity.
 *  5. Validates against safety gates (max removal fraction, max add/remove
 *     counts).  Aborts with rollback if gates would be violated.
 *  6. Applies all removals and additions in a single ACID batch transaction.
 *  7. Appends every mutation to the in-memory audit trail.
 *  8. Updates aggregate metrics (RefreshStats).
 *
 * ## Thread Safety
 *
 * - `triggerRefresh()` and `getStats()` / `getAuditTrail()` may be called
 *   concurrently from any thread.
 * - The background scheduler thread is started by `start()` and stopped by
 *   `stop()`.  Do not destroy the engine while the scheduler is running.
 *
 * ## Example
 *
 * @code
 *   RefreshPolicy policy;
 *   policy.refresh_interval    = std::chrono::seconds(300);
 *   policy.relevance_threshold = 0.4f;
 *   policy.add_threshold       = 0.75f;
 *   policy.max_removal_fraction = 0.05f;
 *
 *   ScheduledGraphEdgeRefreshEngine engine(graph_mgr, policy, embedding_fn);
 *   engine.start();
 *
 *   // … later …
 *   auto stats = engine.getStats();
 *   engine.stop();
 * @endcode
 *
 * DE: Automatische semantische Kantenerneuerung mit konfigurierbarer Richtlinie,
 * ACID-Transaktionen und Sicherheitssperren.
 */
class ScheduledGraphEdgeRefreshEngine {
public:
    // ── Construction / destruction ────────────────────────────────────────────

    /**
     * @brief Construct the engine.
     *
     * @param graph_mgr        Reference to the graph index (must outlive this engine).
     * @param policy           Refresh configuration.
     * @param embedding_fn     Callback that returns the embedding vector for a node.
     *                         May be nullptr; in that case similarity scoring is
     *                         skipped and only temporal-decay scores are used.
     *
     * @throws std::invalid_argument if policy parameters are out of range.
     *
     * DE: Erstellt die Engine mit der angegebenen Richtlinie.
     */
    explicit ScheduledGraphEdgeRefreshEngine(
        GraphIndexManager& graph_mgr,
        const RefreshPolicy& policy,
        NodeEmbeddingProvider embedding_fn = nullptr);

    /**
     * @brief Destructor. Calls stop() if the scheduler is still running.
     */
    ~ScheduledGraphEdgeRefreshEngine();

    // Non-copyable, movable
    ScheduledGraphEdgeRefreshEngine(const ScheduledGraphEdgeRefreshEngine&) = delete;
    ScheduledGraphEdgeRefreshEngine& operator=(const ScheduledGraphEdgeRefreshEngine&) = delete;
    ScheduledGraphEdgeRefreshEngine(ScheduledGraphEdgeRefreshEngine&&) = delete;
    ScheduledGraphEdgeRefreshEngine& operator=(ScheduledGraphEdgeRefreshEngine&&) = delete;

    // ── Lifecycle ─────────────────────────────────────────────────────────────

    /**
     * @brief Start the background scheduler thread.
     *
     * If `policy.refresh_interval` is zero, no background thread is started;
     * only manual triggers via `triggerRefresh()` are available.
     *
     * Calling start() on an already-running engine is a no-op.
     *
     * DE: Startet den Hintergrund-Scheduler-Thread.
     */
    void start();

    /**
     * @brief Stop the background scheduler and wait for it to exit.
     *
     * Any in-progress refresh cycle is allowed to complete before the thread
     * joins.  Safe to call multiple times.
     *
     * DE: Stoppt den Scheduler und wartet auf das Ende des Threads.
     */
    void stop();

    // ── Manual trigger ────────────────────────────────────────────────────────

    /**
     * @brief Trigger an immediate refresh cycle (runs synchronously).
     *
     * Can be called from any thread, including while the scheduler is running
     * (the internal mutex prevents overlapping executions).
     *
     * @return RefreshStats for this cycle.
     *
     * DE: Löst sofort einen Refresh-Zyklus aus (synchron).
     */
    RefreshStats triggerRefresh();

    // ── Observation ───────────────────────────────────────────────────────────

    /**
     * @brief Return statistics for the most recently completed refresh cycle.
     *
     * DE: Gibt Statistiken des letzten abgeschlossenen Zyklus zurück.
     */
    RefreshStats getStats() const;

    /**
     * @brief Return a snapshot of the complete audit trail.
     *
     * The audit trail contains one entry per edge mutation (ADD or REMOVE)
     * ordered by event time.  The trail is bounded by `max_audit_entries`
     * (oldest entries are evicted when the limit is reached).
     *
     * DE: Gibt eine Momentaufnahme des vollständigen Prüfpfads zurück.
     */
    std::vector<RefreshAuditEntry> getAuditTrail() const;

    /**
     * @brief Return the currently active refresh policy.
     *
     * DE: Gibt die aktive Refresh-Richtlinie zurück.
     */
    const RefreshPolicy& getPolicy() const { return policy_; }

    /**
     * @brief Update the refresh policy at runtime.
     *
     * The new policy takes effect on the next scheduled or manually triggered
     * refresh cycle.  The running scheduler is not interrupted.
     *
     * @throws std::invalid_argument if the new policy is invalid.
     *
     * DE: Aktualisiert die Richtlinie zur Laufzeit.
     */
    void setPolicy(const RefreshPolicy& policy);

    /**
     * @brief Attach a Changefeed instance for persistent event logging.
     *
     * When set, every edge mutation (ADD / REMOVE) emitted during a refresh
     * cycle will additionally be recorded as a Changefeed::ChangeEvent using
     * the key "graph_edge_refresh:<edge_id>".  Events carry the action, cycle
     * number, and relevance score in the metadata JSON.
     *
     * Set to nullptr to detach an existing Changefeed.
     *
     * Thread-safe: may be called before or after start().
     *
     * DE: Registriert einen Changefeed für dauerhafte Ereignisprotokollierung.
     */
    void setChangefeed(std::shared_ptr<Changefeed> changefeed);

    /**
     * @brief Attach an ANN index for accelerated candidate edge discovery.
     *
     * When set and the vertex count during a refresh cycle exceeds
     * `policy.ann_min_vertices`, the engine calls `ann_index->build()` with
     * the current vertex embeddings and then uses `ann_index->search()` to
     * find top-k nearest neighbours per vertex in O(V·log V) instead of the
     * default O(V²) brute-force scan.
     *
     * The engine rebuilds the index at the start of each qualifying cycle.
     * Set to nullptr to detach and revert to brute-force discovery.
     *
     * Thread-safe: may be called before or after start().
     *
     * DE: Registriert einen ANN-Index für beschleunigte Kantenkandidaten-Suche.
     */
    void setANNIndex(std::shared_ptr<index::IAnnIndex> ann_index);

    /**
     * @brief Register a callback for real-time CEP event emission.
     *
     * The callback is invoked after every successful batch commit, once for
     * each edge mutation:
     *   - Additions  → EventType::EDGE_CREATE, event_name = "EDGE_ADDED"
     *   - Removals   → EventType::EDGE_DELETE, event_name = "EDGE_REMOVED"
     *
     * Each event carries the fields: edge_id, from_vertex (additions only),
     * to_vertex (additions only), relevance_score (additions only), and
     * cycle_number.
     *
     * No events are emitted when a cycle is aborted by the safety gate or
     * when the batch commit fails.
     *
     * Pass an empty function to detach an existing callback.
     *
     * Typical production usage:
     * @code
     *   engine.setCEPEventCallback([](themisdb::analytics::Event ev) {
     *       themisdb::analytics::CEPEngine::getInstance().submitEvent(std::move(ev));
     *   });
     * @endcode
     *
     * Thread-safe: may be called before or after start().
     *
     * DE: Registriert einen Callback für CEP-Echtzeit-Kantenmutationsereignisse.
     */
    void setCEPEventCallback(
        std::function<void(themisdb::analytics::Event)> callback);

    // ── Scoring helpers (exposed for testability) ─────────────────────────────

    /**
     * @brief Compute the similarity score between two embedding vectors.
     *
     * Uses the metric configured in the current policy.
     *
     * @param a  First embedding vector (must be non-empty and same length as b).
     * @param b  Second embedding vector.
     * @return   Similarity in [0, 1] (or 0.0 on dimension mismatch / empty input).
     *
     * DE: Berechnet die Ähnlichkeit zwischen zwei Einbettungsvektoren.
     */
    float computeSimilarity(const std::vector<float>& a,
                            const std::vector<float>& b) const;

    /**
     * @brief Compute the temporal decay factor for an edge.
     *
     * Applies exponential decay:  factor = 2^(-age / half_life)
     * where age is derived from the edge's "_created_at" field (seconds since
     * epoch).  Returns 1.0 if no timestamp is available or half_life is zero.
     *
     * @param edge_entity  Entity representing the edge.
     * @return             Decay factor in (0, 1].
     *
     * DE: Berechnet den zeitlichen Verfallsfaktor einer Kante.
     */
    float computeTemporalDecay(const BaseEntity& edge_entity) const;

    /**
     * @brief Score a single edge and populate an EdgeScore record.
     *
     * @param edge_entity  The edge to score.
     * @return             Populated EdgeScore.
     *
     * DE: Bewertet eine einzelne Kante und gibt ein EdgeScore-Objekt zurück.
     */
    EdgeScore scoreEdge(const BaseEntity& edge_entity) const;

private:
    // ── Internal helpers ──────────────────────────────────────────────────────

    /// Validate policy values and throw std::invalid_argument if invalid.
    static void validatePolicy(const RefreshPolicy& policy);

    /// Background scheduler loop – runs in scheduler_thread_.
    void schedulerLoop();

    /// Core refresh logic. Called by triggerRefresh() and schedulerLoop().
    RefreshStats runRefreshCycle();

    /// Collect all edge entities for the configured graph scope.
    std::vector<BaseEntity> collectEdges() const;

    /// Score all collected edges and return sorted score list.
    std::vector<EdgeScore> scoreAllEdges(
        const std::vector<BaseEntity>& edges) const;

    /// Discover candidate new edges via top-k similarity search.
    /// Returns (from, to, score) triples for edges not already present.
    std::vector<std::tuple<std::string, std::string, float>>
    discoverCandidateEdges(
        const std::vector<BaseEntity>& existing_edges) const;

    /// Apply all removals and additions in a single ACID batch.
    /// Returns false if the batch commit fails.
    bool applyBatch(const std::vector<std::string>& edge_ids_to_remove,
                    const std::vector<std::tuple<std::string, std::string, float>>& edges_to_add,
                    uint64_t cycle_number);

    /// Append an entry to the audit trail (evicts oldest if at capacity).
    void appendAudit(RefreshAuditEntry entry);

    /// (Re-)build the ANN index from the current vertex set and their
    /// embeddings.  Populates ann_vertex_to_idx_ and ann_idx_to_vertex_.
    /// No-op when ann_index_ is nullptr or embedding_fn_ is not set.
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

    /// Maximum number of audit entries kept in memory.
    static constexpr size_t kMaxAuditEntries = 10000;
};

} // namespace graph
} // namespace themis
