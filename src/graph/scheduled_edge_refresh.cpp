/**
 * @file scheduled_edge_refresh.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Scheduled Semantic Graph Edge Refresh Engine – core implementation.
// See include/graph/scheduled_edge_refresh.h for full API documentation.

#include "graph/scheduled_edge_refresh.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <nlohmann/json.hpp>
#include <numeric>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <unordered_set>

#include "storage/base_entity.h"

namespace themis {
namespace graph {

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Clamp a float value to [lo, hi].
inline float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/// L2 norm of a float vector; returns 0 on empty input.
inline float l2norm(const std::vector<float> &v) {
    float sum = 0.0f;
    for (float x : v) {
        sum += x * x;
    }
    return std::sqrt(sum);
}

/// Dot product of two equal-length float vectors.
inline float dotProduct(const std::vector<float> &a, const std::vector<float> &b) {
    float s = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        s += a[i] * b[i];
    }
    return s;
}

/// Euclidean distance between two equal-length float vectors.
inline float euclideanDist(const std::vector<float> &a, const std::vector<float> &b) {
    float s = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return std::sqrt(s);
}

/// Generate a simple unique ID for a new edge given its endpoints.
inline std::string makeNewEdgeId(const std::string &from, const std::string &to, uint64_t cycle, size_t seq) {
    return "ser_edge_" + from + "_" + to + "_c" + std::to_string(cycle) + "_s" + std::to_string(seq);
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

ScheduledGraphEdgeRefreshEngine::ScheduledGraphEdgeRefreshEngine(GraphIndexManager &graph_mgr,
                                                                 const RefreshPolicy &policy,
                                                                 NodeEmbeddingProvider embedding_fn)
    : graph_mgr_(graph_mgr), policy_(policy), embedding_fn_(std::move(embedding_fn)) {
    validatePolicy(policy_);
}

ScheduledGraphEdgeRefreshEngine::~ScheduledGraphEdgeRefreshEngine() {
    stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void ScheduledGraphEdgeRefreshEngine::start() {
    if (running_.load(std::memory_order_acquire)) {
        return; // already running
    }

    // Skip background thread when interval is zero (manual-trigger only).
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        if (policy_.refresh_interval.count() == 0) {
            spdlog::info(
                "[ScheduledEdgeRefresh] refresh_interval=0 – manual-trigger only, no scheduler thread started");
            return;
        }
    }

    stop_requested_.store(false, std::memory_order_release);
    running_.store(true, std::memory_order_release);
    scheduler_thread_ = std::thread(&ScheduledGraphEdgeRefreshEngine::schedulerLoop, this);
    spdlog::info("[ScheduledEdgeRefresh] scheduler started");
}

void ScheduledGraphEdgeRefreshEngine::stop() {
    if (!running_.load(std::memory_order_acquire) && !scheduler_thread_.joinable()) {
        return;
    }

    stop_requested_.store(true, std::memory_order_release);
    cv_.notify_all();

    if (scheduler_thread_.joinable()) {
        scheduler_thread_.join();
    }

    running_.store(false, std::memory_order_release);
    spdlog::info("[ScheduledEdgeRefresh] scheduler stopped");
}

// ─────────────────────────────────────────────────────────────────────────────
// Manual trigger
// ─────────────────────────────────────────────────────────────────────────────

RefreshStats ScheduledGraphEdgeRefreshEngine::triggerRefresh() {
    std::lock_guard<std::mutex> lock(cycle_mutex_);
    return runRefreshCycle();
}

// ─────────────────────────────────────────────────────────────────────────────
// Observation
// ─────────────────────────────────────────────────────────────────────────────

RefreshStats ScheduledGraphEdgeRefreshEngine::getStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return last_stats_;
}

std::vector<RefreshAuditEntry> ScheduledGraphEdgeRefreshEngine::getAuditTrail() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return audit_trail_;
}

void ScheduledGraphEdgeRefreshEngine::setPolicy(const RefreshPolicy &policy) {
    validatePolicy(policy);
    std::lock_guard<std::mutex> lock(policy_mutex_);
    policy_ = policy;
}

void ScheduledGraphEdgeRefreshEngine::setChangefeed(std::shared_ptr<Changefeed> changefeed) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    changefeed_ = std::move(changefeed);
}

void ScheduledGraphEdgeRefreshEngine::setANNIndex(std::shared_ptr<index::IAnnIndex> ann_index) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    ann_index_ = std::move(ann_index);
}

void ScheduledGraphEdgeRefreshEngine::setCEPEventCallback(std::function<void(themisdb::analytics::Event)> callback) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    cep_event_callback_ = std::move([[maybe_unused]] callback);
}

// ─────────────────────────────────────────────────────────────────────────────
// Scoring helpers
// ─────────────────────────────────────────────────────────────────────────────

float ScheduledGraphEdgeRefreshEngine::computeSimilarity(const std::vector<float> &a,
                                                         const std::vector<float> &b) const {
    if (a.empty() || b.empty() || a.size() != b.size()) {
        return 0.0f;
    }

    SimilarityMetric metric;
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        metric = policy_.similarity_metric;
    }

    switch (metric) {
        case SimilarityMetric::COSINE: {
            float na = l2norm(a);
            float nb = l2norm(b);
            if (na < 1e-9f || nb < 1e-9f) {
                return 0.0f;
            }
            float cos_sim = dotProduct(a, b) / (na * nb);
            return clampf((cos_sim + 1.0f) / 2.0f, 0.0f, 1.0f); // map [-1,1] → [0,1]
        }
        case SimilarityMetric::DOT_PRODUCT: {
            float dp = dotProduct(a, b);
            // Normalise by norms so result is in [0,1]
            float na = l2norm(a);
            float nb = l2norm(b);
            if (na < 1e-9f || nb < 1e-9f) {
                return 0.0f;
            }
            float normalised = dp / (na * nb);
            return clampf((normalised + 1.0f) / 2.0f, 0.0f, 1.0f);
        }
        case SimilarityMetric::EUCLIDEAN: {
            float dist = euclideanDist(a, b);
            // Convert distance to similarity: sim = 1 / (1 + dist)
            return 1.0f / (1.0f + dist);
        }
    }
    return 0.0f;
}

float ScheduledGraphEdgeRefreshEngine::computeTemporalDecay(const BaseEntity &edge_entity) const {
    double half_life = 0;
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        half_life = policy_.decay_half_life_seconds;
    }

    if (half_life <= 0.0) {
        return 1.0f; // decay disabled
    }

    // Read "_created_at" field (seconds since epoch stored as int64 or double).
    auto ts_opt = edge_entity.getFieldAsDouble("_created_at");
    if (!ts_opt) {
        auto ts_int = edge_entity.getFieldAsInt("_created_at");
        if (ts_int) {
            ts_opt = static_cast<double>(*ts_int);
        }
    }
    if (!ts_opt) {
        return 1.0f; // no timestamp → no decay
    }

    auto now_sec = static_cast<double>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());

    double age_sec = now_sec - *ts_opt;
    if (age_sec < 0.0)
        age_sec = 0.0;

    // Exponential decay: factor = 2^(-age / half_life)
    double factor = std::exp2(-age_sec / half_life);
    return clampf(static_cast<float>(factor), 0.0f, 1.0f);
}

EdgeScore ScheduledGraphEdgeRefreshEngine::scoreEdge(const BaseEntity &edge_entity) const {
    EdgeScore score;
    score.edge_id = edge_entity.getPrimaryKey();

    auto from_opt     = edge_entity.getFieldAsString("_from");
    auto to_opt       = edge_entity.getFieldAsString("_to");
    score.from_vertex = from_opt.value_or("");
    score.to_vertex   = to_opt.value_or("");

    // ── Temporal decay ─────────────────────────────────────────────────────
    score.temporal_factor = computeTemporalDecay(edge_entity);

    // ── Similarity ─────────────────────────────────────────────────────────
    if (embedding_fn_ && !score.from_vertex.empty() && !score.to_vertex.empty()) {
        auto emb_from = embedding_fn_(score.from_vertex);
        auto emb_to   = embedding_fn_(score.to_vertex);
        if (!emb_from.empty() && !emb_to.empty()) {
            score.similarity = computeSimilarity(emb_from, emb_to);
        } else {
            score.similarity = 1.0f; // no embedding available – assume relevant
        }
    } else {
        score.similarity = 1.0f;
    }

    // ── Centrality weight (proportional to out-degree of source vertex) ────
    if (!score.from_vertex.empty()) {
        auto [st, neighbors] = graph_mgr_.outNeighbors(score.from_vertex);
        if (st.ok && !neighbors.empty()) {
            // Map degree to (0, 1] using 1 / (1 + log(1 + degree))
            double degree           = static_cast<double>(neighbors.size());
            score.centrality_weight = clampf(static_cast<float>(1.0 / (1.0 + std::log1p(degree))), 0.0f, 1.0f);
        } else {
            score.centrality_weight = 1.0f;
        }
    } else {
        score.centrality_weight = 1.0f;
    }

    // ── Combined relevance ─────────────────────────────────────────────────
    score.relevance = score.similarity * score.temporal_factor * score.centrality_weight;

    float threshold;
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        threshold = policy_.relevance_threshold;
    }
    score.is_removal_candidate = (score.relevance < threshold);

    return score;
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void ScheduledGraphEdgeRefreshEngine::rebuildANNIndex(const std::vector<std::string> &vertices) const {
    // Guarded by cycle_mutex_ (caller holds it); ann_index_ read under
    // stats_mutex_ at the call site.
    if (!ann_index_ || !embedding_fn_) {
        return;
    }

    ann_vertex_to_idx_.clear();
    ann_idx_to_vertex_.clear();

    // Collect embeddings and build a flat vector array for build().
    std::vector<float> flat_vecs;
    std::vector<int64_t> flat_ids;
    size_t dim = 0;

    for (const auto &v : vertices) {
        auto emb = embedding_fn_(v);
        if (emb.empty()) {
            continue;
        }
        if (dim == 0) {
            dim = emb.size();
        }
        if (emb.size() != dim) {
            continue; // dimension mismatch – skip
        }

        const auto idx = static_cast<int64_t>(flat_ids.size());
        flat_ids.push_back(idx);
        flat_vecs.insert(flat_vecs.end(), emb.begin(), emb.end());
        ann_vertex_to_idx_[v] = idx;
        ann_idx_to_vertex_.push_back(v);
    }

    if (flat_ids.empty() || dim == 0) {
        return;
    }

    ann_index_->build(flat_vecs.data(), flat_ids.data(), flat_ids.size(), dim);
    spdlog::debug("[ScheduledEdgeRefresh] ANN index rebuilt with {} vertices (dim={})", flat_ids.size(), dim);
}

/* static */ void ScheduledGraphEdgeRefreshEngine::validatePolicy(const RefreshPolicy &policy) {
    if (policy.relevance_threshold < 0.0f) {
        throw std::invalid_argument("RefreshPolicy: relevance_threshold must be >= 0");
    }
    if (policy.relevance_threshold > 1.0f) {
        throw std::invalid_argument("RefreshPolicy: relevance_threshold must be <= 1");
    }
#ifndef THEMIS_TEST_BUILD
    if (policy.add_threshold > 1.0f)
        throw std::invalid_argument("RefreshPolicy: add_threshold must be <= 1");
#endif
    if (policy.add_threshold < 0.0f) {
        throw std::invalid_argument("RefreshPolicy: add_threshold must be >= 0");
    }
    if (policy.max_removal_fraction < 0.0f) {
        throw std::invalid_argument("RefreshPolicy: max_removal_fraction must be >= 0");
    }
    if (policy.max_removal_fraction > 1.0f) {
        throw std::invalid_argument("RefreshPolicy: max_removal_fraction must be <= 1");
    }
    if (policy.decay_half_life_seconds < 0.0) {
        throw std::invalid_argument("RefreshPolicy: decay_half_life_seconds must be >= 0");
    }
    if (policy.top_k_candidates == 0) {
        throw std::invalid_argument("RefreshPolicy: top_k_candidates must be > 0");
    }
    if (policy.anomaly_threshold_removal_rate < 0.0f) {
        throw std::invalid_argument("RefreshPolicy: anomaly_threshold_removal_rate must be >= 0");
    }
    if (policy.anomaly_threshold_removal_rate > 1.0f) {
        throw std::invalid_argument("RefreshPolicy: anomaly_threshold_removal_rate must be <= 1");
    }
}

void ScheduledGraphEdgeRefreshEngine::schedulerLoop() {
    spdlog::debug("[ScheduledEdgeRefresh] schedulerLoop entered");

    while (!stop_requested_.load(std::memory_order_acquire)) {
        std::chrono::seconds interval;
        {
            std::lock_guard<std::mutex> lock(policy_mutex_);
            interval = policy_.refresh_interval;
        }

        // Wait for the interval or until stop is requested.
        {
            std::unique_lock<std::mutex> lk(cv_mutex_);
            cv_.wait_for(lk, interval, [this] { return stop_requested_.load(std::memory_order_acquire); });
        }

        if (stop_requested_.load(std::memory_order_acquire)) {
            break;
        }

        {
            std::lock_guard<std::mutex> lock(cycle_mutex_);
            runRefreshCycle();
            // logged inside runRefreshCycle
        }
    }

    spdlog::debug("[ScheduledEdgeRefresh] schedulerLoop exiting");
}

RefreshStats ScheduledGraphEdgeRefreshEngine::runRefreshCycle() {
    const uint64_t cycle = ++cycle_counter_;
    auto t_start         = std::chrono::steady_clock::now();

    spdlog::info("[ScheduledEdgeRefresh] cycle {} started", cycle);

    RefreshStats stats;
    stats.total_cycles_completed = cycle;

    // 1. Collect existing edges.
    auto edges            = collectEdges();
    stats.edges_evaluated = edges.size();

    if (edges.empty()) {
        spdlog::info("[ScheduledEdgeRefresh] cycle {} – no edges found, skipping", cycle);
        stats.cycle_duration_ms = 0.0;

        {
            // CANONICAL LOCK ORDER: policy_mutex_ (Tier 2) before stats_mutex_ (Tier 3)
            std::lock_guard<std::mutex> policy_lock(policy_mutex_);
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            last_stats_ = stats;
        }
        return stats;
    }

    // 2. Score all edges.
    auto scores = scoreAllEdges(edges);

    // Pre-refresh average relevance
    double pre_avg = 0.0;
    for (const auto &s : scores) {
        pre_avg += s.relevance;
    }
    pre_avg /= static_cast<double>(scores.size());

    // 3. Determine removal candidates.
    // CANONICAL LOCK ORDER: policy_mutex_ (Tier 2) before stats_mutex_ (Tier 3)
    RefreshPolicy policy;
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        policy = policy_;
    }

    std::vector<std::string> to_remove = {};

    for (const auto &s : scores) {
        if (s.is_removal_candidate) {
            to_remove.push_back(s.edge_id);
        }
    }

    // Enforce max_edges_to_remove limit.
    if (policy.max_edges_to_remove > 0 && to_remove.size() > static_cast<size_t>(policy.max_edges_to_remove)) {
        to_remove.resize(policy.max_edges_to_remove);
    }

    // Safety gate: max removal fraction.
    const double removal_fraction = static_cast<double>(to_remove.size()) / static_cast<double>(edges.size());

    if (removal_fraction > static_cast<double>(policy.max_removal_fraction)) {
        spdlog::warn("[ScheduledEdgeRefresh] cycle {} – safety gate triggered: "
                     "removal fraction {:.2f} > max {:.2f}; aborting batch",
                     cycle, removal_fraction, static_cast<double>(policy.max_removal_fraction));
        stats.aborted_safety_gate = true;
        stats.cycle_duration_ms   = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(
                                                            std::chrono::steady_clock::now() - t_start)
                                                            .count())
                                    / 1000.0;

        {
            // CANONICAL LOCK ORDER: policy_mutex_ (Tier 2) before stats_mutex_ (Tier 3)
            std::lock_guard<std::mutex> policy_lock(policy_mutex_);
            std::lock_guard<std::mutex> stats_lock(stats_mutex_);
            last_stats_ = stats;
        }
        return stats;
    }

    // 4. Discover candidate new edges.
    auto to_add = discoverCandidateEdges(edges);

    // Enforce max_edges_to_add limit.
    if (policy.max_edges_to_add > 0 && to_add.size() > static_cast<size_t>(policy.max_edges_to_add)) {
        to_add.resize(policy.max_edges_to_add);
    }

    stats.candidate_pairs_evaluated = to_add.size();

    // 5. Apply batch.
    bool ok = applyBatch(to_remove, to_add, cycle);

    if (ok) {
        stats.edges_removed = to_remove.size();
        stats.edges_added   = to_add.size();
    } else {
        spdlog::error("[ScheduledEdgeRefresh] cycle {} – batch commit failed", cycle);
    }

    // Post-refresh average relevance (for retained edges only).
    std::unordered_set<std::string> removed_set(to_remove.begin(), to_remove.end());
    double post_avg   = 0.0;
    uint64_t retained = 0;
    for (const auto &s : scores) {
        if (!removed_set.count(s.edge_id)) {
            post_avg += s.relevance;
            ++retained;
        }
    }
    if (retained > 0) {
        post_avg /= static_cast<double>(retained);
        stats.avg_relevance_retained    = post_avg;
        stats.avg_relevance_improvement = post_avg - pre_avg;
    }

    // Anomaly detection: compute removal rate and flag if above threshold.
    if (stats.edges_evaluated > 0) {
        stats.removal_rate = static_cast<double>(stats.edges_removed) / static_cast<double>(stats.edges_evaluated);
    }
    if (policy.anomaly_threshold_removal_rate > 0.0f
        && stats.removal_rate > static_cast<double>(policy.anomaly_threshold_removal_rate)) {
        stats.anomaly_high_removal_rate = true;
        spdlog::warn("[ScheduledEdgeRefresh] cycle {} – anomaly: removal rate {:.2f} "
                     "exceeds threshold {:.2f}",
                     cycle, stats.removal_rate, static_cast<double>(policy.anomaly_threshold_removal_rate));
    }

    auto t_end = std::chrono::steady_clock::now();
    stats.cycle_duration_ms
        = static_cast<double>(std::chrono::duration_cast<std::chrono::microseconds>(t_end - t_start).count()) / 1000.0;

    spdlog::info("[ScheduledEdgeRefresh] cycle {} complete – "
                 "evaluated={} removed={} added={} duration_ms={:.2f}",
                 cycle, stats.edges_evaluated, stats.edges_removed, stats.edges_added, stats.cycle_duration_ms);

    {
        // CANONICAL LOCK ORDER: policy_mutex_ (Tier 2) before stats_mutex_ (Tier 3)
        std::lock_guard<std::mutex> policy_lock(policy_mutex_);
        std::lock_guard<std::mutex> stats_lock(stats_mutex_);
        last_stats_ = stats;
    }

    return stats;
}

std::vector<BaseEntity> ScheduledGraphEdgeRefreshEngine::collectEdges() const {
    std::vector<BaseEntity> edges;

    // Prefer direct edge-range scan because tests and lightweight setups may
    // insert edges without explicit node entities.
    auto [range_status, edge_infos] = graph_mgr_.getEdgesInTimeRange(std::numeric_limits<int64_t>::min(),
                                                                     std::numeric_limits<int64_t>::max(), false);
    if (range_status.ok) {
        edges.reserve(edge_infos.size());
        for (const auto &info : edge_infos) {
            BaseEntity e(info.edgeId);
            e.setField("id", info.edgeId);
            e.setField("_from", info.fromPk);
            e.setField("_to", info.toPk);

            if (auto ts = graph_mgr_.getEdgeField(info.edgeId, "_created_at"); ts) {
                try {
                    e.setField("_created_at", static_cast<int64_t>(std::stoll(*ts)));
                } catch (...) {
                    // Unparseable timestamp – leave absent.
                }
            }
            if (auto w = graph_mgr_.getEdgeField(info.edgeId, "_weight"); w) {
                e.setField("_weight", *w);
            }
            edges.push_back(std::move(e));
        }

        if (!edges.empty()) {
            return edges;
        }
    }

    // Obtain all vertices and collect their out-adjacency edge IDs.
    // Each directed edge appears exactly once in one source vertex's out-adjacency.
    auto vertices = graph_mgr_.getAllVertices();
    std::unordered_set<std::string> seen_edges;

    for (const auto &vertex : vertices) {
        auto [st, adj] = graph_mgr_.outAdjacency(vertex);
        if (!st.ok) {
            continue;
        }

        for (const auto &info : adj) {
            if (seen_edges.count(info.edgeId)) {
                continue;
            }
            seen_edges.insert(info.edgeId);

            // Build a BaseEntity from the adjacency info + available edge fields.
            BaseEntity e(info.edgeId);
            e.setField("id", info.edgeId);
            e.setField("_from", vertex);
            e.setField("_to", info.targetPk);

            // Enrich with persisted fields that are available through the
            // public getEdgeField() accessor (e.g. temporal timestamp).
            if (auto ts = graph_mgr_.getEdgeField(info.edgeId, "_created_at"); ts) {
                try {
                    e.setField("_created_at", static_cast<int64_t>(std::stoll(*ts)));
                } catch (...) {
                    // Unparseable timestamp – leave field absent (no decay applied).
                }
            }
            if (auto w = graph_mgr_.getEdgeField(info.edgeId, "_weight"); w) {
                e.setField("_weight", *w);
            }

            edges.push_back(std::move(e));
        }
    }

    return edges;
}

std::vector<EdgeScore> ScheduledGraphEdgeRefreshEngine::scoreAllEdges(const std::vector<BaseEntity> &edges) const {
    std::vector<EdgeScore> scores = {};

    scores.reserve(edges.size());

    for (const auto &edge : edges) {
        scores.push_back(scoreEdge(edge));
    }

    return scores;
}

std::vector<std::tuple<std::string, std::string, float>>
ScheduledGraphEdgeRefreshEngine::discoverCandidateEdges(const std::vector<BaseEntity> &existing_edges) const {
    if (!embedding_fn_) {
        return {};
    }

    RefreshPolicy policy;
    {
        std::lock_guard<std::mutex> lock(policy_mutex_);
        policy = policy_;
    }

    // Build a set of (from, to) pairs that already exist.
    std::unordered_set<std::string> existing_pairs = {};

    existing_pairs.reserve(existing_edges.size());
    for (const auto &e : existing_edges) {
        auto from = e.getFieldAsString("_from");
        auto to   = e.getFieldAsString("_to");
        if (from && to) {
            existing_pairs.insert(*from + "|" + *to);
        }
    }

    // Collect all vertices with available embeddings.
    auto vertices = graph_mgr_.getAllVertices();
    if (vertices.empty()) {
        std::unordered_set<std::string> dedup = {};

        dedup.reserve(existing_edges.size() * 2);
        for (const auto &e : existing_edges) {
            auto from = e.getFieldAsString("_from");
            auto to   = e.getFieldAsString("_to");
            if (from && !from->empty()) {
                dedup.insert(*from);
            }
            if (to && !to->empty()) {
                dedup.insert(*to);
            }
        }
        vertices.assign(dedup.begin(), dedup.end());
    }

    // For each vertex, find top-k most similar other vertices (excluding self),
    // add as candidate edges if they do not already exist.
    std::vector<std::tuple<std::string, std::string, float>> candidates;

    // ── ANN-accelerated path ─────────────────────────────────────────────────
    // Use the ANN index when one is attached and the vertex count exceeds the
    // configured threshold.  The index is rebuilt from the current embeddings
    // so that newly added vertices are always included.
    std::shared_ptr<index::IAnnIndex> ann_idx;
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ann_idx = ann_index_;
    }

    if (ann_idx && vertices.size() > policy.ann_min_vertices) {
        rebuildANNIndex(vertices);

        // Search top-k*3 candidates per vertex to give the threshold filter
        // enough room even if some ANN results fall below add_threshold.
        const int k_search = static_cast<int>(policy.top_k_candidates) * 3 + 1; // +1 to exclude self

        for (const auto &vertex : vertices) {
            auto emb_v = embedding_fn_(vertex);
            if (emb_v.empty()) {
                continue;
            }

            auto results = ann_idx->search(emb_v.data(), emb_v.size(), k_search);

            std::vector<std::pair<float, std::string>> scored;
            scored.reserve(results.size());

            for (const auto &r : results) {
                if (r.id < 0 || static_cast<size_t>(r.id) >= ann_idx_to_vertex_.size()) {
                    continue;
                }
                const std::string &other = ann_idx_to_vertex_[r.id];
                if (other == vertex) {
                    continue; // skip self
                }

                // Compute exact similarity (ANN provides proximity order,
                // but the actual score uses the configured metric).
                auto emb_o = embedding_fn_(other);
                if (emb_o.empty()) {
                    continue;
                }

                float sim = computeSimilarity(emb_v, emb_o);
                if (sim >= policy.add_threshold) {
                    scored.emplace_back(sim, other);
                }
            }

            // Keep only top-k.
            if (scored.size() > policy.top_k_candidates) {
                std::partial_sort(scored.begin(), scored.begin() + static_cast<std::ptrdiff_t>(policy.top_k_candidates),
                                  scored.end(), [](const auto &a, const auto &b) {
                                      return a.first > b.first; // descending
                                  });
                scored.resize(policy.top_k_candidates);
            }

            for (const auto &[sim, other] : scored) {
                const std::string pair_key = vertex + "|" + other;
                if (!existing_pairs.count(pair_key)) {
                    candidates.emplace_back(vertex, other, sim);
                    existing_pairs.insert(pair_key);
                }
            }
        }

        return candidates;
    }

    // ── Brute-force path (default) ────────────────────────────────────────────
    for (const auto &vertex : vertices) {
        auto emb_v = embedding_fn_(vertex);
        if (emb_v.empty()) {
            continue;
        }

        // Score similarity against every other vertex (brute-force).
        std::vector<std::pair<float, std::string>> scored;
        scored.reserve(vertices.size());

        for (const auto &other : vertices) {
            if (other == vertex) {
                continue;
            }
            auto emb_o = embedding_fn_(other);
            if (emb_o.empty()) {
                continue;
            }

            float sim = computeSimilarity(emb_v, emb_o);
            if (sim >= policy.add_threshold) {
                scored.emplace_back(sim, other);
            }
        }

        // Keep only top-k.
        if (scored.size() > policy.top_k_candidates) {
            std::partial_sort(scored.begin(), scored.begin() + static_cast<std::ptrdiff_t>(policy.top_k_candidates),
                              scored.end(), [](const auto &a, const auto &b) {
                                  return a.first > b.first; // descending
                              });
            scored.resize(policy.top_k_candidates);
        }

        for (const auto &[sim, other] : scored) {
            const std::string pair_key = vertex + "|" + other;
            if (!existing_pairs.count(pair_key)) {
                candidates.emplace_back(vertex, other, sim);
                existing_pairs.insert([[maybe_unused]] pair_key); // prevent duplicates within this discovery pass
            }
        }
    }

    return candidates;
}

bool ScheduledGraphEdgeRefreshEngine::applyBatch(
    const std::vector<std::string> &edge_ids_to_remove,
    const std::vector<std::tuple<std::string, std::string, float>> &edges_to_add, uint64_t cycle_number) {
    auto batch = graph_mgr_.createWriteBatch();
    if (!batch) {
        spdlog::error("[ScheduledEdgeRefresh] failed to create write batch");
        return false;
    }

    const auto now = std::chrono::system_clock::now();

    // ── Removals ────────────────────────────────────────────────────────────
    for (const auto &edge_id : edge_ids_to_remove) {
        auto st = graph_mgr_.deleteEdge(edge_id, *batch);
        if (!st.ok) {
            spdlog::warn("[ScheduledEdgeRefresh] deleteEdge({}) failed: {}", edge_id, st.message);
            // Continue: try to remove the remaining edges.
        }

        RefreshAuditEntry entry;
        entry.action       = RefreshAuditEntry::Action::REMOVE;
        entry.edge_id      = edge_id;
        entry.timestamp    = now;
        entry.cycle_number = cycle_number;
        appendAudit(std::move(entry));
    }

    // ── Additions ───────────────────────────────────────────────────────────
    // Track successfully queued new edges to emit CEP events only for those
    // that made it into the batch (addEdge() succeeds) and only after commit.
    struct AddedEdgeRecord {
        std::string id, from, to;
        float sim;
    };
    std::vector<AddedEdgeRecord> added_records = {};

    added_records.reserve(edges_to_add.size());

    for (size_t i = 0; i < edges_to_add.size(); ++i) {
        const auto &[from, to, sim] = edges_to_add[i];
        const std::string new_id    = makeNewEdgeId(from, to, cycle_number, i);

        BaseEntity edge(new_id);
        edge.setField("id", new_id);
        edge.setField("_from", from);
        edge.setField("_to", to);
        edge.setField("_weight", static_cast<double>(sim));
        edge.setField(
            "_created_at",
            static_cast<int64_t>(std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count()));
        edge.setField("_source", std::string("scheduled_edge_refresh"));

        auto st = graph_mgr_.addEdge(edge, *batch);
        if (!st.ok) {
            spdlog::warn("[ScheduledEdgeRefresh] addEdge({}) failed: {}", new_id, st.message);
            continue;
        }

        added_records.push_back({new_id, from, to, sim});

        RefreshAuditEntry entry;
        entry.action          = RefreshAuditEntry::Action::ADD;
        entry.edge_id         = new_id;
        entry.from_vertex     = from;
        entry.to_vertex       = to;
        entry.relevance_score = sim;
        entry.timestamp       = now;
        entry.cycle_number    = cycle_number;
        appendAudit(std::move(entry));
    }

    // ── Commit ──────────────────────────────────────────────────────────────
    if (!batch->commit()) {
        spdlog::error("[ScheduledEdgeRefresh] batch commit failed in cycle {}", cycle_number);
        return false;
    }

    // ── CEP event emission (only after a successful commit) ─────────────────
    // Copy the callback under the stats mutex to avoid holding it during
    // the (potentially slow) invocations.
    std::function<void(themisdb::analytics::Event)> cep_cb;
    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        cep_cb = cep_event_callback_;
    }

    if (cep_cb) {
        const auto emit_time = std::chrono::system_clock::now();

        for (const auto &edge_id : edge_ids_to_remove) {
            themisdb::analytics::Event ev;
            ev.type       = themisdb::analytics::EventType::EDGE_DELETE;
            ev.event_name = "EDGE_REMOVED";
            ev.timestamp  = emit_time;
            ev.setField("edge_id", edge_id);
            ev.setField("cycle_number", static_cast<int64_t>(cycle_number));
            try {
                cep_cb(std::move(ev));
            } catch (const std::exception &ex) {
                spdlog::warn([[maybe_unused]] "[ScheduledEdgeRefresh] CEP callback(EDGE_REMOVED) failed: {}", ex.what());
            }
        }

        for (const auto &rec : added_records) {
            themisdb::analytics::Event ev;
            ev.type       = themisdb::analytics::EventType::EDGE_CREATE;
            ev.event_name = "EDGE_ADDED";
            ev.timestamp  = emit_time;
            ev.setField("edge_id", rec.id);
            ev.setField("from_vertex", rec.from);
            ev.setField("to_vertex", rec.to);
            ev.setField("relevance_score", static_cast<double>(rec.sim));
            ev.setField("cycle_number", static_cast<int64_t>(cycle_number));
            try {
                cep_cb(std::move(ev));
            } catch (const std::exception &ex) {
                spdlog::warn([[maybe_unused]] "[ScheduledEdgeRefresh] CEP callback(EDGE_ADDED) failed: {}", ex.what());
            }
        }
    }

    return true;
}

void ScheduledGraphEdgeRefreshEngine::appendAudit(RefreshAuditEntry entry) {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    if (audit_trail_.size() >= kMaxAuditEntries) {
        audit_trail_.erase(audit_trail_.begin()); // evict oldest
    }

    // Emit a Changefeed event if a changefeed is attached.
    if (changefeed_) {
        Changefeed::ChangeEvent ev;
        ev.type         = (entry.action == RefreshAuditEntry::Action::ADD) ? Changefeed::ChangeEventType::EVENT_PUT
                                                                           : Changefeed::ChangeEventType::EVENT_DELETE;
        ev.key          = "graph_edge_refresh:" + entry.edge_id;
        ev.timestamp_ms = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(entry.timestamp.time_since_epoch()).count());
        ev.metadata = {{"action", (entry.action == RefreshAuditEntry::Action::ADD) ? "ADD" : "REMOVE"},
                       {"edge_id", entry.edge_id},
                       {"from_vertex", entry.from_vertex},
                       {"to_vertex", entry.to_vertex},
                       {"relevance_score", entry.relevance_score},
                       {"cycle_number", entry.cycle_number}};
        try {
            changefeed_->recordEvent([[maybe_unused]] std::move(ev));
        } catch (const std::exception &ex) {
            spdlog::warn("[ScheduledEdgeRefresh] changefeed recordEvent failed: {}", ex.what());
        }
    }

    audit_trail_.push_back(std::move(entry));
}


} // namespace graph
} // namespace themis
