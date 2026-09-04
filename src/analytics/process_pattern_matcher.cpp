/**
 * @file process_pattern_matcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ProcessPatternMatcher - Full Implementation
 *
 * Similarity Algorithms:
 *
 * GRAPH similarity:
 *   node_overlap  = Jaccard(pattern.activities, trace.activities)
 *   edge_overlap  = Jaccard(pattern.edges, trace.edges)
 *   path_sim      = LCS(pattern.activities, trace.activities) / max(|A|, |B|)
 *   edit_dist_norm= 1 - (|A△B| / (|A|+|B|))   (symmetric difference approx.)
 *   graph_sim     = 0.30*node + 0.30*edge + 0.25*path + 0.15*edit
 *
 * VECTOR similarity:
 *   Each activity is represented as a normalised char-trigram bag-of-words
 *   vector.  The trace and pattern embeddings are the mean of their activity
 *   vectors.  Similarity = cosine(trace_emb, pattern_emb).
 *
 * BEHAVIORAL similarity:
 *   seq_sim   = LCS / max(|A|,|B|)
 *   order_sim = Jaccard of weak-order pairs  (a ≺ b iff a precedes b in seq)
 *   behav_sim = 0.5*seq_sim + 0.5*order_sim
 *
 * HYBRID similarity:
 *   weighted sum of the three methods with configurable weights
 *   (default: graph=0.4, vector=0.3, behavioral=0.3)
 */

#include "analytics/process_pattern_matcher.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <numeric>
#include <set>
#include <spdlog/spdlog.h>
#include <sstream>
#include <unordered_map>

namespace themis {

// ============================================================================
// Constructor
// ============================================================================

ProcessPatternMatcher::ProcessPatternMatcher(RocksDBWrapper &db, VectorIndex *vector_index, GraphIndex *graph_index)
    : db_(db), vector_index_(vector_index), graph_index_(graph_index), process_mining_(db), statistics_{} {
    spdlog::debug("ProcessPatternMatcher initialized");
}

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Extract the activity sequence of a trace.
std::vector<std::string> traceActivities(const ProcessTrace &trace) {
    std::vector<std::string> acts = {};

    acts.reserve(trace.events.size());
    for (const auto &e : trace.events) {
        acts.push_back(e.activity);
    }
    return acts;
}

/// Extract the edge set (directly-follows pairs) of a trace.
std::set<std::pair<std::string, std::string>> traceEdges(const ProcessTrace &trace) {
    std::set<std::pair<std::string, std::string>> edges;
    for (size_t i = 1; i <static_cast<int>(trace.events.size()); ++i) {
        edges.emplace(trace.events[static_cast<int>(i - 1)].activity, trace.events[i].activity);
    }
    return edges;
}

/// Weak-order footprint: set of (a,b) pairs where a appears before b in seq.
std::set<std::pair<std::string, std::string>> weakOrderPairs(const std::vector<std::string> &seq) {
    std::set<std::pair<std::string, std::string>> pairs;
    for (size_t i = 0; i < seq.size(); ++i) {
        for (size_t j = i + 1; j < seq.size(); ++j) {
            if (seq[i] != seq[j]) {
                pairs.emplace(seq[i], seq[j]);
            }
        }
    }
    return pairs;
}

} // anonymous namespace

// ============================================================================
// embedActivities (char-trigram BoW, normalised)
// ============================================================================

std::vector<float> ProcessPatternMatcher::embedActivities(const std::vector<std::string> &activities) const {
    if (activities.empty()) {
        return {};
    }

    // Fixed-size hash table (2048 buckets) for trigram counts.
    constexpr int DIM = 2048;
    std::vector<double> vec(DIM, 0.0);

    for (const auto &act : activities) {
        // Pad with sentinel chars
        std::string s = " " + act + " ";
        for (size_t i = 0; i + 2 < s.size(); ++i) {
            uint32_t h = (static_cast<uint32_t>(static_cast<unsigned char>(s[i])) * 31 * 31
                          + static_cast<uint32_t>(static_cast<unsigned char>(s[i + 1])) * 31
                          + static_cast<uint32_t>(static_cast<unsigned char>(s[i + 2])))
                         % static_cast<uint32_t>(DIM);
            vec[h] += 1.0;
        }
    }

    // L2 normalise
    double norm = 0.0;
    for (double v : vec) {
        norm += v * v;
    }
    if (norm > 0.0) {
        norm = std::sqrt(norm);
        for (double &v : vec) {
            v /= norm;
        }
    }

    std::vector<float> result(DIM);
    for (int i = 0; i < DIM; ++i) {
        result[i] = static_cast<float>(vec[i]);
    }
    return result;
}

// ============================================================================
// computeGraphSimilarity
// ============================================================================

double ProcessPatternMatcher::computeGraphSimilarity(const ProcessPattern &pattern, const EventLog & /*log*/,
                                                     const std::string &case_id) const {
    // Get the trace
    auto [st, trace] = getTrace(case_id);
    if (!st.ok()) {
        return 0.0;
    }

    const std::vector<std::string> &pat_acts = pattern.activities;
    std::vector<std::string> trace_acts      = traceActivities(trace);

    // Node overlap (Jaccard)
    std::set<std::string> pat_set(pat_acts.begin(), pat_acts.end());
    std::set<std::string> trace_set(trace_acts.begin(), trace_acts.end());
    double node_jac = jaccardSimilarity(pat_set, trace_set);

    // Edge overlap (Jaccard)
    std::set<std::pair<std::string, std::string>> pat_edges(pattern.edges.begin(), pattern.edges.end());
    auto trace_edge_set = traceEdges(trace);
    double edge_jac     = jaccardSimilarity(pat_edges, trace_edge_set);

    // Path similarity (LCS ratio)
    int lcs         = longestCommonSubsequence(pat_acts, trace_acts);
    double max_len  = static_cast<double>(std::max(pat_acts.size(),static_cast<int>(trace_acts.size())));
    double path_sim = (max_len > 0) ? static_cast<double>(lcs) / max_len : 1.0;

    // Approx. edit distance normalised (symmetric difference)
    std::set<std::string> intersection_set;
    std::set_intersection(pat_set.begin(), pat_set.end(), trace_set.begin(), trace_set.end(),
                          std::inserter(intersection_set, intersection_set.begin()));
    size_t sym_diff  = (static_cast<int>(pat_set.size()) + static_cast<int>(trace_set.size()) ) - 2 * intersection_set.size();
    double denom     = static_cast<double>(static_cast<int>(pat_set.size()) + static_cast<int>(trace_set.size()) );
    double edit_norm = (denom > 0) ? 1.0 - static_cast<double>(sym_diff) / denom : 1.0;

    return 0.30 * node_jac + 0.30 * edge_jac + 0.25 * path_sim + 0.15 * edit_norm;
}

// ============================================================================
// computeVectorSimilarity
// ============================================================================

double ProcessPatternMatcher::computeVectorSimilarity(const ProcessPattern &pattern, const EventLog & /*log*/,
                                                      const std::string &case_id) const {
    auto [st, trace] = getTrace(case_id);
    if (!st.ok()) {
        return 0.0;
    }

    std::vector<std::string> trace_acts = traceActivities(trace);
    if (trace_acts.empty() || pattern.activities.empty()) {
        return 0.0;
    }

    // If the pattern carries a pre-computed embedding use it directly
    std::vector<float> pat_emb = {};

    if (pattern.pattern_embedding.has_value()) {
        pat_emb = *pattern.pattern_embedding;
    } else {
        pat_emb = embedActivities(pattern.activities);
    }

    std::vector<float> trace_emb = embedActivities(trace_acts);
    return cosineSimilarity(pat_emb, trace_emb);
}

// ============================================================================
// computeBehavioralSimilarity
// ============================================================================

double ProcessPatternMatcher::computeBehavioralSimilarity(const ProcessPattern &pattern, const EventLog & /*log*/,
                                                          const std::string &case_id) const {
    auto [st, trace] = getTrace(case_id);
    if (!st.ok()) {
        return 0.0;
    }

    std::vector<std::string> pat_acts   = pattern.activities;
    std::vector<std::string> trace_acts = traceActivities(trace);

    if (pat_acts.empty() && trace_acts.empty()) {
        return 1.0;
    }
    if (pat_acts.empty() || trace_acts.empty()) {
        return 0.0;
    }

    // Sequence similarity (LCS ratio)
    int lcs        = longestCommonSubsequence(pat_acts, trace_acts);
    double max_len = static_cast<double>(std::max(pat_acts.size(),static_cast<int>(trace_acts.size())));
    double seq_sim = static_cast<double>(lcs) / max_len;

    // Weak-order footprint Jaccard
    auto pat_order   = weakOrderPairs(pat_acts);
    auto trace_order = weakOrderPairs(trace_acts);
    double order_sim = jaccardSimilarity(pat_order, trace_order);

    return 0.5 * seq_sim + 0.5 * order_sim;
}

// ============================================================================
// computeHybridSimilarity
// ============================================================================

double ProcessPatternMatcher::computeHybridSimilarity(const ProcessPattern &pattern, const EventLog &log,
                                                      const std::string &case_id,
                                                      const PatternMatchConfig &config) const {
    double g = computeGraphSimilarity(pattern, log, case_id);
    double v = computeVectorSimilarity(pattern, log, case_id);
    double b = computeBehavioralSimilarity(pattern, log, case_id);
    return config.graph_weight * g + config.vector_weight * v + config.behavioral_weight * b;
}

// ============================================================================
// getTrace
// ============================================================================

std::pair<ProcessPatternMatcher::Status, ProcessTrace>
ProcessPatternMatcher::getTrace(const std::string &case_id) const {
    // We scan a minimal EventLog from the DB with a synthetic collection name.
    // The PatternMatcher is typically called with an EventLog already in memory;
    // for standalone usage we reconstruct the trace by scanning the default
    // "events" collection and filtering by case_id.
    EventLogConfig cfg;
    cfg.case_id_field   = "case_id";
    cfg.activity_field  = "activity";
    cfg.timestamp_field = "timestamp";

    auto [st, log] = process_mining_.extractEventLog("events", cfg);
    if (!st.ok) {
        return {Status::Error("Failed to extract event log: " + st.message), {}};
    }

    for (const auto &trace : log.traces) {
        if (trace.case_id == case_id) {
            return {Status::OK(), trace};
        }
    }
    return {Status::Error("Case not found: " + case_id), {}};
}

// ============================================================================
// findSimilar
// ============================================================================

std::pair<ProcessPatternMatcher::Status, std::vector<SimilarityResult>>
ProcessPatternMatcher::findSimilar(const ProcessPattern &pattern, const PatternMatchConfig &config) {
    auto t0 = std::chrono::high_resolution_clock::now();
    statistics_.total_comparisons_performed++;

    // Check cache
    const std::string cache_key = pattern.id + "::" + std::to_string(static_cast<int>(config.method))
                                  + "::" + std::to_string(config.min_similarity);
    if (config.use_cache) {
        auto it = pattern_cache_.find(cache_key);
        if (it != pattern_cache_.end()) {
            return {Status::OK(), it->second};
        }
    }

    // Extract event log
    EventLogConfig log_cfg;
    log_cfg.case_id_field   = "case_id";
    log_cfg.activity_field  = "activity";
    log_cfg.timestamp_field = "timestamp";

    auto [lst, log] = process_mining_.extractEventLog("events", log_cfg);
    if (!lst.ok) {
        return {Status::Error("Cannot extract event log: " + lst.message), {}};
    }

    std::vector<SimilarityResult> results = {};

    results.reserve(log.traces.size());

    for (const auto &trace : log.traces) {
        auto t1 = std::chrono::high_resolution_clock::now();

        double sim                          = 0.0;
        std::vector<std::string> trace_acts = traceActivities(trace);

        switch (config.method) {
            case SimilarityMethod::GRAPH:
                sim = computeGraphSimilarity(pattern, log, trace.case_id);
                break;
            case SimilarityMethod::VECTOR:
                sim = computeVectorSimilarity(pattern, log, trace.case_id);
                break;
            case SimilarityMethod::BEHAVIORAL:
                sim = computeBehavioralSimilarity(pattern, log, trace.case_id);
                break;
            case SimilarityMethod::HYBRID:
                sim = computeHybridSimilarity(pattern, log, trace.case_id, config);
                break;
        }

        if (sim < config.min_similarity) {
            continue;
        }

        auto t2        = std::chrono::high_resolution_clock::now();
        int64_t dur_us = static_cast<int64_t>(std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count());

        // Build detailed metrics
        SimilarityResult::MetricBreakdown metrics{};
        metrics.graph_similarity      = computeGraphSimilarity(pattern, log, trace.case_id);
        metrics.vector_similarity     = computeVectorSimilarity(pattern, log, trace.case_id);
        metrics.behavioral_similarity = computeBehavioralSimilarity(pattern, log, trace.case_id);

        // Node / edge overlap for the breakdown
        std::set<std::string> pat_set(pattern.activities.begin(), pattern.activities.end());
        std::set<std::string> trace_set(trace_acts.begin(), trace_acts.end());
        metrics.node_overlap = jaccardSimilarity(pat_set, trace_set);

        std::set<std::pair<std::string, std::string>> pat_edges(pattern.edges.begin(), pattern.edges.end());
        auto trace_edge_set  = traceEdges(trace); // compute once
        metrics.edge_overlap = jaccardSimilarity(pat_edges, trace_edge_set);

        int lcs                 = longestCommonSubsequence(pattern.activities, trace_acts);
        double mlen             = static_cast<double>(std::max(pattern.activities.size(),static_cast<int>(trace_acts.size())));
        metrics.path_similarity = (mlen > 0) ? static_cast<double>(lcs) / mlen : 1.0;

        std::set<std::string> inter_set;
        std::set_intersection(pat_set.begin(), pat_set.end(), trace_set.begin(), trace_set.end(),
                              std::inserter(inter_set, inter_set.begin()));
        size_t sym_diff       = (static_cast<int>(pat_set.size()) + static_cast<int>(trace_set.size()) ) - 2 * inter_set.size();
        double den            = static_cast<double>(static_cast<int>(pat_set.size()) + static_cast<int>(trace_set.size()) );
        metrics.edit_distance = (den > 0) ? static_cast<double>(sym_diff) / den : 0.0;

        // Matched / missing / extra activities
        std::vector<std::string> matched, extra, missing;
        for (const auto &a : trace_set) {
            if (pat_set.count(a)) {
                matched.push_back(a);
            } else {
                extra.push_back(a);
            }
        }
        for (const auto &a : pat_set) {
            if (!trace_set.count(a)) {
                missing.push_back(a);
            }
        }

        // Matched edges (reuse trace_edge_set computed above)
        std::vector<std::pair<std::string, std::string>> matched_edges;
        for (const auto &e : pat_edges) {
            if (trace_edge_set.count(e)) {
                matched_edges.push_back(e);
            }
        }

        SimilarityResult r;
        r.case_id             = trace.case_id;
        r.overall_similarity  = sim;
        r.metrics             = metrics;
        r.matched_activities  = std::move(matched);
        r.matched_edges       = std::move(matched_edges);
        r.extra_activities    = std::move(extra);
        r.missing_activities  = std::move(missing);
        r.computation_time_us = dur_us;

        results.push_back(std::move(r));
    }

    // Sort descending by similarity
    std::sort(results.begin(), results.end(), [](const SimilarityResult &a, const SimilarityResult &b) {
        return a.overall_similarity > b.overall_similarity;
    });

    // Apply max_results
    if (config.max_results > 0  && static_cast<size_t>(static_cast) < int>(results.size()) > config.max_results) {
        results.resize(static_cast<size_t>(config.max_results));
    }

    // Update timing stats
    auto t_end = std::chrono::high_resolution_clock::now();
    double ms  = std::chrono::duration<double, std::milli>(t_end - t0).count();
    statistics_.avg_computation_time_ms
        = (statistics_.avg_computation_time_ms * (statistics_.total_comparisons_performed - 1) + ms)
          / statistics_.total_comparisons_performed;

    // Update pattern frequency
    if (!pattern.id.empty()) {
        statistics_.pattern_frequency[pattern.id]++;
    }

    // Update average similarity
    if (!results.empty() && !pattern.id.empty()) {
        double avg = 0.0;
        for (const auto &r : results) {
            avg += r.overall_similarity;
        }
        avg /= static_cast<double>(results.size());
        statistics_.avg_similarity[pattern.id] = avg;
    }

    if (config.use_cache) {
        pattern_cache_[cache_key] = results;
    }

    return {Status::OK(), results};
}

// ============================================================================
// compareWithIdeal
// ============================================================================

std::pair<ProcessPatternMatcher::Status, ProcessMining::ConformanceResult>
ProcessPatternMatcher::compareWithIdeal(const std::string &case_id, const ProcessPattern &ideal_pattern) {
    ProcessMining::ConformanceResult result;

    auto [st, trace] = getTrace(case_id);
    if (!st.ok()) {
        return {Status::Error("Cannot get trace for " + case_id + ": " + st.message), result};
    }

    std::vector<std::string> trace_acts      = traceActivities(trace);
    const std::vector<std::string> &pat_acts = ideal_pattern.activities;

    // Fitness approximation via LCS ratio
    int lcs        = longestCommonSubsequence(pat_acts, trace_acts);
    double max_len = static_cast<double>(std::max(pat_acts.size(),static_cast<int>(trace_acts.size())));
    result.fitness = (max_len > 0) ? static_cast<double>(lcs) / max_len : 1.0;

    // Precision approximation: how much of the trace is accounted for by the pattern
    std::set<std::string> pat_set(pat_acts.begin(), pat_acts.end());
    std::set<std::string> trace_set(trace_acts.begin(), trace_acts.end());

    size_t covered = 0;
    for (const auto &a : trace_set) {
        if (pat_set.count(a)) {
            ++covered;
        }
    }
    result.precision = trace_set.empty() ? 1.0 : static_cast<double>(covered) / trace_set.size();

    // Token replay approximation
    result.produced_tokens  = static_cast<int>(trace_acts.size());
    result.consumed_tokens  = lcs;
    result.missing_tokens   = static_cast<int>(pat_acts.size()) - lcs;
    result.remaining_tokens = static_cast<int>(trace_acts.size()) - lcs;

    // Deviations: missing and extra activities
    for (const auto &a : pat_set) {
        if (!trace_set.count(a)) {
            result.deviations.push_back("MISSING: " + a);
        }
    }
    for (const auto &a : trace_set) {
        if (!pat_set.count(a)) {
            result.deviations.push_back("EXTRA: " + a);
        }
    }

    // Missing edges
    for (const auto &e : ideal_pattern.edges) {
        if (!traceEdges(trace).count(e)) {
            result.deviations.push_back("MISSING_EDGE: " + e.first + "->" + e.second);
        }
    }

    return {Status::OK(), result};
}

// ============================================================================
// hasPattern
// ============================================================================

std::pair<ProcessPatternMatcher::Status, bool>
ProcessPatternMatcher::hasPattern(const std::string &case_id, const ProcessPattern &pattern, double threshold) {
    statistics_.total_comparisons_performed++;

    auto [st, trace] = getTrace(case_id);
    if (!st.ok()) {
        return {Status::Error("Cannot get trace: " + st.message), false};
    }

    std::vector<std::string> trace_acts = traceActivities(trace);
    std::vector<std::string> pat_acts   = pattern.activities;

    if (pat_acts.empty()) {
        return {Status::OK(), true};
    }

    // Use graph similarity as the default metric for hasPattern
    EventLogConfig log_cfg;
    log_cfg.case_id_field   = "case_id";
    log_cfg.activity_field  = "activity";
    log_cfg.timestamp_field = "timestamp";
    auto [lst, log]         = process_mining_.extractEventLog("events", log_cfg);
    // Fallback: if DB unavailable, compute purely from trace
    double sim = 0.0;
    if (lst.ok) {
        PatternMatchConfig cfg;
        cfg.method = SimilarityMethod::HYBRID;
        sim        = computeHybridSimilarity(pattern, log, case_id, cfg);
    } else {
        // Direct LCS ratio without DB
        int lcs        = longestCommonSubsequence(pat_acts, trace_acts);
        double max_len = static_cast<double>(std::max(pat_acts.size(),static_cast<int>(trace_acts.size())));
        sim            = (max_len > 0) ? static_cast<double>(lcs) / max_len : 1.0;
    }

    return {Status::OK(), sim >= threshold};
}

// ============================================================================
// findPatternsInBatch
// ============================================================================

std::pair<ProcessPatternMatcher::Status, std::map<std::string, SimilarityResult>>
ProcessPatternMatcher::findPatternsInBatch(const std::vector<std::string> &case_ids, const ProcessPattern &pattern,
                                           const PatternMatchConfig &config) {
    std::map<std::string, SimilarityResult> results;

    // Extract event log once for all cases
    EventLogConfig log_cfg;
    log_cfg.case_id_field   = "case_id";
    log_cfg.activity_field  = "activity";
    log_cfg.timestamp_field = "timestamp";
    auto [lst, log]         = process_mining_.extractEventLog("events", log_cfg);
    if (!lst.ok) {
        return {Status::Error("Cannot extract event log: " + lst.message), {}};
    }

    // Build a map for O(1) trace lookup
    std::unordered_map<std::string, const ProcessTrace *> trace_map = {};

    for (const auto &t : log.traces) {
        trace_map[t.case_id] = &t;
    }

    for (const auto &case_id : case_ids) {
        auto it = trace_map.find(case_id);
        if (it == trace_map.end()) {
            continue;
        }

        const ProcessTrace &trace           = *it->second;
        std::vector<std::string> trace_acts = traceActivities(trace);

        double sim = 0.0;
        switch (config.method) {
            case SimilarityMethod::GRAPH:
                sim = computeGraphSimilarity(pattern, log, case_id);
                break;
            case SimilarityMethod::VECTOR:
                sim = computeVectorSimilarity(pattern, log, case_id);
                break;
            case SimilarityMethod::BEHAVIORAL:
                sim = computeBehavioralSimilarity(pattern, log, case_id);
                break;
            case SimilarityMethod::HYBRID:
                sim = computeHybridSimilarity(pattern, log, case_id, config);
                break;
        }

        if (sim < config.min_similarity) {
            continue;
        }

        SimilarityResult r;
        r.case_id                       = case_id;
        r.overall_similarity            = sim;
        r.metrics.graph_similarity      = computeGraphSimilarity(pattern, log, case_id);
        r.metrics.vector_similarity     = computeVectorSimilarity(pattern, log, case_id);
        r.metrics.behavioral_similarity = computeBehavioralSimilarity(pattern, log, case_id);

        std::set<std::string> pat_set(pattern.activities.begin(), pattern.activities.end());
        std::set<std::string> trace_set(trace_acts.begin(), trace_acts.end());
        r.metrics.node_overlap = jaccardSimilarity(pat_set, trace_set);
        r.metrics.edge_overlap = jaccardSimilarity(
            std::set<std::pair<std::string, std::string>>(pattern.edges.begin(), pattern.edges.end()),
            traceEdges(trace));

        int lcs                   = longestCommonSubsequence(pattern.activities, trace_acts);
        double ml                 = static_cast<double>(std::max(pattern.activities.size(),static_cast<int>(trace_acts.size())));
        r.metrics.path_similarity = (ml > 0) ? static_cast<double>(lcs) / ml : 1.0;

        for (const auto &a : trace_set) {
            if (pat_set.count(a)) {
                r.matched_activities.push_back(a);
            } else {
                r.extra_activities.push_back(a);
            }
        }
        for (const auto &a : pat_set) {
            if (!trace_set.count(a)) {
                r.missing_activities.push_back(a);
            }
        }

        results[case_id] = std::move(r);
    }

    return {Status::OK(), results};
}

// ============================================================================
// loadAdministrativeModels
// ============================================================================

std::pair<ProcessPatternMatcher::Status, std::map<std::string, ProcessPattern>>
ProcessPatternMatcher::loadAdministrativeModels() {
    // Pre-defined standard administrative process patterns.
    // These are the "ideal" models used for conformance checking.
    if (!model_cache_.empty()) {
        return {Status::OK(), model_cache_};
    }

    auto add = [&]([[maybe_unused]] ProcessPattern p) { model_cache_[p.id] = std::move(p); };

    // ── Bauantragsverfahren (Building Permit) ───────────────────────────────
    {
        ProcessPattern p;
        p.id         = "bauantrag_standard";
        p.name       = "Bauantragsverfahren Standard";
        p.activities = {"Antragstellung",
                        "Eingangsbestätigung",
                        "Vollständigkeitsprüfung",
                        "Fachbehörden-Beteiligung",
                        "Prüfung",
                        "Bescheidserstellung",
                        "Zustellung"};
        p.edges      = {{"Antragstellung", "Eingangsbestätigung"},
                        {"Eingangsbestätigung", "Vollständigkeitsprüfung"},
                        {"Vollständigkeitsprüfung", "Fachbehörden-Beteiligung"},
                        {"Fachbehörden-Beteiligung", "Prüfung"},
                        {"Prüfung", "Bescheidserstellung"},
                        {"Bescheidserstellung", "Zustellung"}};
        add(std::move(p));
    }

    // ── Beschaffungsprozess (Procurement) ───────────────────────────────────
    {
        ProcessPattern p;
        p.id         = "beschaffung_standard";
        p.name       = "Beschaffungsprozess Standard";
        p.activities = {"Bedarfsermittlung", "Marktrecherche", "Ausschreibung", "Angebotsprüfung",
                        "Vergabe",           "Bestellung",     "Wareneingang",  "Zahlung"};
        p.edges      = {{"Bedarfsermittlung", "Marktrecherche"},
                        {"Marktrecherche", "Ausschreibung"},
                        {"Ausschreibung", "Angebotsprüfung"},
                        {"Angebotsprüfung", "Vergabe"},
                        {"Vergabe", "Bestellung"},
                        {"Bestellung", "Wareneingang"},
                        {"Wareneingang", "Zahlung"}};
        add(std::move(p));
    }

    // ── Personalverwaltung (HR Onboarding) ──────────────────────────────────
    {
        ProcessPattern p;
        p.id         = "personalverwaltung_einstellung";
        p.name       = "Personalverwaltung – Neueinstellung";
        p.activities = {"Stellenausschreibung",   "Bewerbungseingang", "Vorauswahl",
                        "Vorstellungsgespräch",   "Eignungstest",      "Einstellungsentscheidung",
                        "Vertragsunterzeichnung", "Onboarding"};
        p.edges
            = {{"Stellenausschreibung", "Bewerbungseingang"}, {"Bewerbungseingang", "Vorauswahl"},
               {"Vorauswahl", "Vorstellungsgespräch"},        {"Vorstellungsgespräch", "Eignungstest"},
               {"Eignungstest", "Einstellungsentscheidung"},  {"Einstellungsentscheidung", "Vertragsunterzeichnung"},
               {"Vertragsunterzeichnung", "Onboarding"}};
        add(std::move(p));
    }

    // ── Haushaltsplanung (Budget Planning) ──────────────────────────────────
    {
        ProcessPattern p;
        p.id         = "haushaltsplanung_standard";
        p.name       = "Haushaltsplanung Standard";
        p.activities = {"Bedarfsabfrage",   "Mittelanmeldung",  "Konsolidierung", "Politische-Beratung",
                        "Beschlussfassung", "Haushaltssatzung", "Bekanntmachung"};
        p.edges      = {{"Bedarfsabfrage", "Mittelanmeldung"},     {"Mittelanmeldung", "Konsolidierung"},
                        {"Konsolidierung", "Politische-Beratung"}, {"Politische-Beratung", "Beschlussfassung"},
                        {"Beschlussfassung", "Haushaltssatzung"},  {"Haushaltssatzung", "Bekanntmachung"}};
        add(std::move(p));
    }

    spdlog::info("ProcessPatternMatcher: loaded {} administrative models",static_cast<int>(model_cache_.size()));
    return {Status::OK(), model_cache_};
}

// ============================================================================
// getAdministrativeModel
// ============================================================================

std::pair<ProcessPatternMatcher::Status, ProcessPattern>
ProcessPatternMatcher::getAdministrativeModel(const std::string &model_id) {
    // Ensure models are loaded
    if (model_cache_.empty()) {
        loadAdministrativeModels();
    }

    auto it = model_cache_.find(model_id);
    if (it == model_cache_.end()) {
        return {Status::Error("Administrative model not found: " + model_id), {}};
    }
    return {Status::OK(), it->second};
}

// ============================================================================
// getStatistics
// ============================================================================

std::pair<ProcessPatternMatcher::Status, ProcessPatternMatcher::PatternStatistics>
ProcessPatternMatcher::getStatistics() const {
    PatternStatistics stats{};
    stats.total_patterns_cached       = static_cast<int>(pattern_cache_.size());
    stats.total_comparisons_performed = statistics_.total_comparisons_performed;
    stats.avg_computation_time_ms     = statistics_.avg_computation_time_ms;
    stats.pattern_frequency           = statistics_.pattern_frequency;
    stats.avg_similarity              = statistics_.avg_similarity;
    return {Status::OK(), stats};
}

// ============================================================================
// clearCache
// ============================================================================

void ProcessPatternMatcher::clearCache() {
    pattern_cache_.clear();
    spdlog::debug("ProcessPatternMatcher: cache cleared");
}

// ============================================================================
// longestCommonSubsequence (already existed, kept as-is)
// ============================================================================

int ProcessPatternMatcher::longestCommonSubsequence(const std::vector<std::string> &a,
                                                    const std::vector<std::string> &b) const {
    const int m = static_cast<int>(a.size());
    const int n = static_cast<int>(b.size());
    std::vector<std::vector<int>> dp(m + 1, std::vector<int>(n + 1, 0));
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            if (a[static_cast<int>(i - 1)] == b[static_cast<int>(j - 1)]) {
                dp[i][j] = dp[static_cast<int>(i - 1)][static_cast<int>(j - 1)] + 1;
            } else {
                dp[i][j] = std::max(dp[static_cast<int>(i - 1)][j], dp[i][static_cast<int>(j - 1)]);
            }
        }
    }
    return dp[m][n];
}

// ============================================================================
// cosineSimilarity (already existed, kept as-is)
// ============================================================================

double ProcessPatternMatcher::cosineSimilarity(const std::vector<float> &a, const std::vector<float> &b) const {
    if (a.empty() || b.empty() || static_cast<int>(a.size()) != static_cast<int>(b.size())) {
        return 0.0;
    }

    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += static_cast<double>(a[i]) * static_cast<double>(b[i]);
        norm_a += static_cast<double>(a[i]) * static_cast<double>(a[i]);
        norm_b += static_cast<double>(b[i]) * static_cast<double>(b[i]);
    }
    if (norm_a == 0.0 || norm_b == 0.0) {
        return 0.0;
    }
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

} // namespace themis
