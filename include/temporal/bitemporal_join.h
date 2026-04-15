/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bitemporal_join.h                                  ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 05:38:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     216                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 39ac8c3efe  2026-03-20  Split default-arg constructors into overloads ║
    • 15e6e31437  2026-03-09  feat: implement all features from problem statement ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

/**
 * @file bitemporal_join.h
 * @brief Bi-temporal join operators for SQL:2011 combined temporal predicates.
 *
 * A bi-temporal join correlates two versioned tables on BOTH the system-time
 * axis (when the data was stored) and the valid-time axis (when the fact held
 * in the modelled reality).  This file implements the three canonical join
 * modes from SQL:2011 §T005:
 *
 * | Mode                      | Predicate                                   |
 * |---------------------------|---------------------------------------------|
 * | SEQUENCED                 | Join only rows whose valid-time periods overlap |
 * | NON-SEQUENCED             | Ignore temporal axes; plain equi-join        |
 * | CURRENT                   | Join only rows current at a given point-in-time |
 *
 * Plus ThemisDB extensions:
 * - CONTAINED_IN join  (left valid-time ⊆ right valid-time)
 * - OVERLAPPING join   (left valid-time ∩ right valid-time ≠ ∅)
 * - SNAPSHOT join      (both tables seen at the same system-time snapshot)
 *
 * ## Usage
 * ```cpp
 * BiTemporalJoin join(left_rows, right_rows, config);
 * auto results = join.execute();
 * ```
 *
 * @note Thread Safety: BiTemporalJoin instances are not thread-safe.
 *   Create separate instances per thread or query.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/temporal_types.h"

#include <functional>
#include <string>
#include <utility>
#include <vector>

namespace themisdb {
namespace temporal {

// ─────────────────────────────────────────────────────────────────────────────
// BiTemporalRow – one versioned row from either input table
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single versioned row with both temporal axes.
 */
struct BiTemporalRow {
    std::string key;          ///< Logical row key (join key)
    Document    payload;      ///< Row payload
    TimeRange   sys_time;     ///< System-time period (when stored)
    TimeRange   valid_time;   ///< Application valid-time period
};

// ─────────────────────────────────────────────────────────────────────────────
// BiTemporalJoinResult – output row from a join
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Output row of a bi-temporal join.
 *
 * The result carries the intersection of the two time ranges plus the merged
 * payloads from both input rows.
 */
struct BiTemporalJoinResult {
    std::string key;               ///< Common join key
    Document    left_payload;      ///< Payload from the left table
    Document    right_payload;     ///< Payload from the right table

    TimeRange   sys_time_overlap;  ///< Intersection of system-time periods
    TimeRange   valid_time_overlap;///< Intersection of valid-time periods

    bool operator==(const BiTemporalJoinResult& o) const noexcept {
        return key == o.key;  // Simplified equality (key + period uniqueness)
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// BiTemporalJoin
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief SQL:2011 bi-temporal join operator.
 *
 * Supports SEQUENCED, NON-SEQUENCED, CURRENT, CONTAINED_IN, OVERLAPPING,
 * and SNAPSHOT join semantics.
 */
class BiTemporalJoin {
public:
    /**
     * @brief Join mode controlling which temporal predicates are applied.
     */
    enum class JoinMode {
        /// SQL:2011 SEQUENCED: result row's valid-time = intersection of both valid-times
        SEQUENCED,
        /// SQL:2011 NON-SEQUENCED: ignore temporal axes; plain equi-join on key
        NON_SEQUENCED,
        /// Join rows valid at a single point in time (snapshot)
        CURRENT,
        /// Left valid-time ⊆ right valid-time (containment)
        CONTAINED_IN,
        /// Left valid-time ∩ right valid-time ≠ ∅ (strict overlap)
        OVERLAPPING,
        /// Both tables viewed at the same system-time snapshot
        SNAPSHOT,
    };

    /**
     * @brief Configuration for the join operator.
     */
    struct Config {
        JoinMode mode = JoinMode::SEQUENCED;

        /// For CURRENT and SNAPSHOT modes: the reference point-in-time.
        Timestamp as_of = kMaxTimestamp;  ///< kMaxTimestamp = "now"

        /// For SNAPSHOT mode: constrain system-time as well as valid-time.
        bool apply_sys_time_predicate = false;

        /// Key extractor for the left table (default: use BiTemporalRow::key).
        std::function<std::string(const BiTemporalRow&)> left_key_fn;
        /// Key extractor for the right table (default: use BiTemporalRow::key).
        std::function<std::string(const BiTemporalRow&)> right_key_fn;
    };

    /**
     * @brief Construct the join operator with input row sets.
     *
     * @param left   Left-hand versioned rows.
     * @param right  Right-hand versioned rows.
     * @param config Join configuration (mode, point-in-time, etc.).
     */
    BiTemporalJoin(std::vector<BiTemporalRow> left,
                   std::vector<BiTemporalRow> right);

    explicit BiTemporalJoin(std::vector<BiTemporalRow> left,
                            std::vector<BiTemporalRow> right,
                            Config                     config);

    /**
     * @brief Execute the join and return all matching result rows.
     *
     * @return Vector of bi-temporal join results, sorted by key then by
     *         valid_time_overlap.start.
     */
    std::vector<BiTemporalJoinResult> execute() const;

    /**
     * @brief Stream results row by row via a callback (avoids materialisation).
     *
     * @param cb  Called once for each result row.  Return false to stop early.
     */
    void forEach(std::function<bool(BiTemporalJoinResult)> cb) const;

    const Config& config() const noexcept { return config_; }

    // ── Static predicate helpers (exposed for testing) ────────────────────────

    /**
     * @brief True iff two TimeRanges overlap (non-empty intersection).
     */
    static bool overlaps(const TimeRange& a, const TimeRange& b) noexcept;

    /**
     * @brief True iff @p inner is fully contained within @p outer.
     */
    static bool containedIn(const TimeRange& inner,
                             const TimeRange& outer) noexcept;

    /**
     * @brief Compute the intersection of two TimeRanges.
     *
     * @return Intersection, or an empty/invalid range when they do not overlap.
     */
    static TimeRange intersection(const TimeRange& a, const TimeRange& b) noexcept;

private:
    std::vector<BiTemporalRow> left_;
    std::vector<BiTemporalRow> right_;
    Config                     config_;

    bool rowMatches(const BiTemporalRow& l, const BiTemporalRow& r) const noexcept;
    BiTemporalJoinResult makeResult(const BiTemporalRow& l,
                                    const BiTemporalRow& r) const noexcept;
};

} // namespace temporal
} // namespace themisdb
