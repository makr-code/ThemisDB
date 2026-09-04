/**
 * @file bitemporal_join.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "temporal/bitemporal_join.h"

#include <algorithm>
#include <unordered_map>

namespace themisdb {
namespace temporal {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

BiTemporalJoin::BiTemporalJoin(std::vector<BiTemporalRow> left,
                                std::vector<BiTemporalRow> right)
    : BiTemporalJoin(std::move(left), std::move(right), Config{})
{
}

BiTemporalJoin::BiTemporalJoin(std::vector<BiTemporalRow> left,
                                std::vector<BiTemporalRow> right,
                                Config                     config)
    : left_(std::move(left)), right_(std::move(right)), config_(std::move(config))
{
    // Default key extractors
    if (!config_.left_key_fn)
        config_.left_key_fn  = [](const BiTemporalRow& r){ return r.key; };
    if (!config_.right_key_fn)
        config_.right_key_fn = [](const BiTemporalRow& r){ return r.key; };
}

// ─────────────────────────────────────────────────────────────────────────────
// Static predicate helpers
// ─────────────────────────────────────────────────────────────────────────────

bool BiTemporalJoin::overlaps(const TimeRange& a, const TimeRange& b) noexcept {
    return a.start < b.end && b.start < a.end;
}

bool BiTemporalJoin::containedIn(const TimeRange& inner,
                                  const TimeRange& outer) noexcept {
    return inner.start >= outer.start && inner.end <= outer.end;
}

TimeRange BiTemporalJoin::intersection(const TimeRange& a,
                                        const TimeRange& b) noexcept {
    Timestamp s = std::max(a.start, b.start);
    Timestamp e = std::min(a.end,   b.end);
    if (s >= e) return TimeRange{0, 0};  // Empty / invalid
    return TimeRange{s, e};
}

// ─────────────────────────────────────────────────────────────────────────────
// rowMatches — temporal predicate per join mode
// ─────────────────────────────────────────────────────────────────────────────

bool BiTemporalJoin::rowMatches(const BiTemporalRow& l,
                                 const BiTemporalRow& r) const noexcept {
    // Keys must match first
    if (config_.left_key_fn(l) != config_.right_key_fn(r)) {
      return false;
    }

    Timestamp t = config_.as_of == kMaxTimestamp ? now() : config_.as_of;

    switch (config_.mode) {
        case JoinMode::NON_SEQUENCED:
            // Ignore temporal axes — plain equi-join
            return true;

        case JoinMode::SEQUENCED:
            // Valid-time periods must overlap
            return overlaps(l.valid_time, r.valid_time);

        case JoinMode::CURRENT:
            // Both rows must be current at time `as_of`
            return l.valid_time.contains(t) && r.valid_time.contains(t);

        case JoinMode::CONTAINED_IN:
            // Left valid-time ⊆ right valid-time
            return containedIn(l.valid_time, r.valid_time);

        case JoinMode::OVERLAPPING:
            // Strict non-empty overlap
            return overlaps(l.valid_time, r.valid_time);

        case JoinMode::SNAPSHOT:
            // Both rows must be visible at the given system-time snapshot
            if (!l.sys_time.contains(t) || !r.sys_time.contains(t)) {
              return false;
            }
            if (config_.apply_sys_time_predicate)
                return overlaps(l.sys_time, r.sys_time) &&
                       overlaps(l.valid_time, r.valid_time);
            return overlaps(l.valid_time, r.valid_time);
    }
    return false;
}

// ─────────────────────────────────────────────────────────────────────────────
// makeResult
// ─────────────────────────────────────────────────────────────────────────────

BiTemporalJoinResult
BiTemporalJoin::makeResult(const BiTemporalRow& l,
                            const BiTemporalRow& r) const noexcept {
    BiTemporalJoinResult res;
    res.key             = config_.left_key_fn(l);
    res.left_payload    = l.payload;
    res.right_payload   = r.payload;
    res.sys_time_overlap   = intersection(l.sys_time,   r.sys_time);
    res.valid_time_overlap = intersection(l.valid_time, r.valid_time);
    return res;
}

// ─────────────────────────────────────────────────────────────────────────────
// forEach — streaming execution
// ─────────────────────────────────────────────────────────────────────────────

void BiTemporalJoin::forEach(std::function<bool(BiTemporalJoinResult)> cb) const {
    if (!cb) {
      return;
    }

    // Build a hash index on the right side, keyed by right_key_fn
    std::unordered_map<std::string, std::vector<const BiTemporalRow*>> right_idx;
    right_idx.reserve(right_.size());
    for (const auto& row : right_) {
        right_idx[config_.right_key_fn(row)].push_back(&row);
    }

    for (const auto& lrow : left_) {
        const std::string lkey = config_.left_key_fn(lrow);
        auto it = right_idx.find(lkey);
        if (it == right_idx.end()) {
          continue;
        }

        for (const BiTemporalRow* rrow : it->second) {
            if (rowMatches(lrow, *rrow)) {
                if (!cb(makeResult(lrow, *rrow))) {
                  return;
                }
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// execute — materialise all results
// ─────────────────────────────────────────────────────────────────────────────

std::vector<BiTemporalJoinResult> BiTemporalJoin::execute() const {
    std::vector<BiTemporalJoinResult> results;
    forEach([&results](BiTemporalJoinResult r) -> bool {
        results.push_back(std::move(r));
        return true;
    });

    // Sort by key, then by valid_time_overlap.start
    std::sort(results.begin(), results.end(),
              [](const BiTemporalJoinResult& a, const BiTemporalJoinResult& b){
                  if (a.key != b.key) {
                    return a.key < b.key;
                  }
                  return a.valid_time_overlap.start < b.valid_time_overlap.start;
              });
    return results;
}

} // namespace temporal
} // namespace themisdb
