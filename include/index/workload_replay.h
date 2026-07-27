/**
 * @file workload_replay.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.18
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: workload_replay.h | Version: 0.0.18 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 168
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #2517 [index] Automated index adv... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <mutex>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "metadata/index_recommender.h"

namespace themis {

using json = nlohmann::json;

// Use declarations for metadata types (previously in themis, now in themis::metadata)
using metadata::IndexRecommendation;
using metadata::ColumnAccess;
using metadata::IndexRecommender;

/**
 * @brief A single recorded query event in a captured workload.
 *
 * Stores one column access observation (table, column, access type,
 * selectivity) so it can be serialised and replayed later.
 */
struct WorkloadEvent {
    std::string  table_name;
    std::string  column_name;
    IndexRecommender::AccessType access_type = IndexRecommender::AccessType::FILTER;
    double       selectivity = 1.0;   ///< 0 = very selective, 1 = not selective

    json toJSON() const;
    static WorkloadEvent fromJSON(const json& j);
};

/**
 * @brief Records and serialises a sequence of workload events.
 *
 * Captures column access observations and the total query count so that
 * the same workload can be replayed through the index advisor later.
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Usage:
 * @code
 *   WorkloadCapture capture;
 *   // During normal query execution:
 *   capture.recordQuery();
 *   capture.recordEvent({"users", "email", IndexRecommender::AccessType::FILTER, 0.01});
 *   // Persist:
 *   json j = capture.toJSON();
 *   // Load later:
 *   auto loaded = WorkloadCapture::fromJSON(j);
 * @endcode
 */
class WorkloadCapture {
public:
    WorkloadCapture() = default;
    ~WorkloadCapture() = default;

    // Disable copy to avoid accidental duplication of large workloads
    WorkloadCapture(const WorkloadCapture&) = delete;
    WorkloadCapture& operator=(const WorkloadCapture&) = delete;

    // Move is allowed
    WorkloadCapture(WorkloadCapture&& other) noexcept;
    WorkloadCapture& operator=(WorkloadCapture&& other) noexcept;

    /// Record one column access event.
    void recordEvent(const WorkloadEvent& event);

    /// Convenience overload that constructs the event in-place.
    void recordEvent(std::string_view table_name,
                     std::string_view column_name,
                     IndexRecommender::AccessType access_type,
                     double selectivity = 1.0);

    /// Increment the total query counter (called once per logical query).
    void recordQuery();

    /// Number of recorded events.
    size_t eventCount() const;

    /// Total number of recorded queries (used for benefit normalisation).
    uint64_t totalQueries() const;

    /// Return a snapshot of all recorded events.
    std::vector<WorkloadEvent> events() const;

    /// Clear all recorded data.
    void clear();

    /// Serialise to JSON.
    json toJSON() const;

    /// Deserialise from JSON.
    static WorkloadCapture fromJSON(const json& j);

private:
    mutable std::mutex       mutex_;
    std::vector<WorkloadEvent> events_;
    uint64_t                 total_queries_ = 0;
};

/**
 * @brief Replays a captured workload through the index advisor.
 *
 * Feeds the events from a @ref WorkloadCapture into a fresh
 * @ref IndexRecommender and returns the resulting index recommendations.
 *
 * The replayer is stateless – each call to `replay()` / `replayAll()`
 * creates an isolated IndexRecommender so that previous replays do not
 * affect subsequent ones.
 *
 * Usage:
 * @code
 *   WorkloadReplayer replayer;
 *   auto recs = replayer.replay(capture, "users");
 *   for (const auto& r : recs) {
 *       spdlog::info("Suggestion: {} {} on {}.{}",
 *           (r.action == IndexRecommendation::Action::ADD) ? "ADD" : "DROP",
 *           r.index_type, r.table_name, r.column_name);
 *   }
 * @endcode
 */
class WorkloadReplayer {
public:
    WorkloadReplayer() = default;
    ~WorkloadReplayer() = default;

    /**
     * @brief Replay the workload and return recommendations for one table.
     *
     * @param capture          Recorded workload to replay.
     * @param table_name       Table to generate recommendations for.
     * @param existing_indexes Columns that already have an index (to detect DROP candidates).
     * @return Recommendations sorted by benefit score (highest first).
     */
    std::vector<IndexRecommendation> replay(
        const WorkloadCapture& capture,
        std::string_view table_name,
        const std::vector<std::string>& existing_indexes = {}) const;

    /**
     * @brief Replay the workload and return recommendations for all tables.
     *
     * @param capture          Recorded workload to replay.
     * @param existing_indexes Per-table map of already-indexed columns.
     * @return Map of table_name → sorted recommendation vector.
     */
    std::map<std::string, std::vector<IndexRecommendation>> replayAll(
        const WorkloadCapture& capture,
        const std::map<std::string, std::vector<std::string>>& existing_indexes = {}) const;

private:
    /// Feed all events from @p capture into @p rec.
    static void feed(const WorkloadCapture& capture, IndexRecommender& rec);
};

} // namespace themis
