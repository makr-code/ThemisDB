/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            incremental_view.cpp                               ║
  Version:         0.0.17                                             ║
  Last Modified:   2026-02-23 03:57:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     522                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 02a0d7f03  2026-02-21  feat(analytics): implement Phase 2 streaming & incrementa... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * Incremental Materialized Views - Implementation
 *
 * Delta-maintenance algorithm for GROUP BY + aggregation views:
 *
 *   INSERT(row)  → for each dimension group:  agg.add(row, +1)
 *   DELETE(row)  → for each dimension group:  agg.add(row, -1)
 *   UPDATE(r,r') → DELETE(before) + INSERT(after)  (rewrite rule)
 *
 * MIN/MAX use std::multiset so removal of the current extremum gives the
 * correct next value in O(log n).
 *
 * STDDEV/VARIANCE use Welford's online algorithm adapted for removals via
 * Chan's parallel formula when count > 0.
 *
 * COUNT_DISTINCT uses a std::set<string> — removals decrement a reference
 * counter per distinct value.
 */

#include "analytics/incremental_view.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <spdlog/spdlog.h>
#include <sstream>

namespace themisdb {
namespace analytics {

// ============================================================================
// Utility
// ============================================================================

std::string fieldValueToStr(const FieldValue& v) {
    if (std::holds_alternative<std::nullptr_t>(v)) return "";
    if (auto* s = std::get_if<std::string>(&v)) return *s;
    if (auto* i = std::get_if<int64_t>(&v))     return std::to_string(*i);
    if (auto* d = std::get_if<double>(&v))       return std::to_string(*d);
    if (auto* b = std::get_if<bool>(&v))         return *b ? "true" : "false";
    return "";
}

namespace {

double fieldValueToDouble(const FieldValue& v) {
    if (auto* d = std::get_if<double>(&v))   return *d;
    if (auto* i = std::get_if<int64_t>(&v))  return static_cast<double>(*i);
    if (auto* b = std::get_if<bool>(&v))     return *b ? 1.0 : 0.0;
    return 0.0;
}

bool applyFilterOp(const FieldValue& field_val,
                   ViewFilter::Op    op,
                   const FieldValue& filter_val) {
    std::string fs  = fieldValueToStr(field_val);
    std::string fvs = fieldValueToStr(filter_val);
    double fd  = fieldValueToDouble(field_val);
    double fvd = fieldValueToDouble(filter_val);

    switch (op) {
        case ViewFilter::Op::EQ:          return fs == fvs;
        case ViewFilter::Op::NE:          return fs != fvs;
        case ViewFilter::Op::LT:          return fd < fvd;
        case ViewFilter::Op::LE:          return fd <= fvd;
        case ViewFilter::Op::GT:          return fd > fvd;
        case ViewFilter::Op::GE:          return fd >= fvd;
        case ViewFilter::Op::IS_NULL:     return std::holds_alternative<std::nullptr_t>(field_val);
        case ViewFilter::Op::IS_NOT_NULL: return !std::holds_alternative<std::nullptr_t>(field_val);
    }
    return false;
}

int64_t nowMicros() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

} // anonymous namespace

// ============================================================================
// AggState
// ============================================================================

void IncrementalView::AggState::add(const FieldValue& v, int sign) {
    double d = fieldValueToDouble(v);
    std::string s = fieldValueToStr(v);

    if (sign > 0) {
        // Insert
        count++;
        sum += d;

        // MIN/MAX sorted structure
        min_max_values.insert(d);

        // COUNT_DISTINCT: reference-counted so duplicate field values
        // don't disappear prematurely on a single DELETE.
        distinct_ref_counts[s]++;

        // FIRST/LAST
        last_val = v;
        if (!has_first) { first_val = v; has_first = true; }

        // Welford online update
        if (count >= 1) {
            double delta  = d - welford_mean;
            welford_mean += delta / static_cast<double>(count);
            double delta2 = d - welford_mean;
            welford_m2   += delta * delta2;
        }
    } else if (sign < 0 && count > 0) {
        // Remove
        count--;
        sum -= d;

        // MIN/MAX: remove one copy of d
        auto it = min_max_values.find(d);
        if (it != min_max_values.end()) min_max_values.erase(it);

        // COUNT_DISTINCT: decrement ref count; erase when it reaches 0
        auto rit = distinct_ref_counts.find(s);
        if (rit != distinct_ref_counts.end()) {
            if (--rit->second <= 0) distinct_ref_counts.erase(rit);
        }

        // Welford: Chan's update for removal (numerically stable)
        if (count > 0) {
            double old_mean = welford_mean;
            double new_mean = (welford_mean * static_cast<double>(count + 1) - d)
                              / static_cast<double>(count);
            welford_m2 -= (d - old_mean) * (d - new_mean);
            if (welford_m2 < 0.0) welford_m2 = 0.0; // numerical floor
            welford_mean = new_mean;
        } else {
            welford_mean = 0.0;
            welford_m2   = 0.0;
        }
    }
}

FieldValue IncrementalView::AggState::result(ViewAggFunc func) const {
    switch (func) {
        case ViewAggFunc::COUNT:
            return static_cast<int64_t>(count);
        case ViewAggFunc::SUM:
            return sum;
        case ViewAggFunc::AVG:
            return (count > 0) ? sum / static_cast<double>(count) : 0.0;
        case ViewAggFunc::MIN:
            return min_max_values.empty()
                ? FieldValue{nullptr}
                : FieldValue{*min_max_values.begin()};
        case ViewAggFunc::MAX:
            return min_max_values.empty()
                ? FieldValue{nullptr}
                : FieldValue{*min_max_values.rbegin()};
        case ViewAggFunc::STDDEV:
            return (count >= 2)
                ? std::sqrt(welford_m2 / static_cast<double>(count - 1))
                : 0.0;
        case ViewAggFunc::VARIANCE:
            return (count >= 2)
                ? welford_m2 / static_cast<double>(count - 1)
                : 0.0;
        case ViewAggFunc::COUNT_DISTINCT:
            return static_cast<int64_t>(distinct_ref_counts.size());
        case ViewAggFunc::FIRST:
            return has_first ? first_val : FieldValue{nullptr};
        case ViewAggFunc::LAST:
            return last_val;
    }
    return FieldValue{nullptr};
}

// ============================================================================
// IncrementalView
// ============================================================================

IncrementalView::IncrementalView(const ViewDefinition& def) : def_(def) {}
IncrementalView::~IncrementalView() = default;

std::string IncrementalView::makeGroupKey(const ChangeRecord::Row& row) const {
    std::ostringstream oss;
    for (const auto& dim : def_.dimensions) {
        auto it = row.find(dim);
        oss << (it != row.end() ? fieldValueToStr(it->second) : "") << '\0';
    }
    return oss.str();
}

std::unordered_map<std::string, std::string>
IncrementalView::parseGroupKey(const GroupKey& gk) const {
    std::unordered_map<std::string, std::string> result;
    std::istringstream iss(gk);
    std::string token;
    for (const auto& dim : def_.dimensions) {
        if (std::getline(iss, token, '\0')) {
            result[dim] = token;
        }
    }
    return result;
}

bool IncrementalView::passesBaseFilters(const ChangeRecord::Row& row) const {
    for (const auto& f : def_.base_filters) {
        auto it = row.find(f.field);
        FieldValue fv{nullptr};
        if (it != row.end()) fv = it->second;
        if (!applyFilterOp(fv, f.op, f.value)) return false;
    }
    return true;
}

bool IncrementalView::passesRuntimeFilters(
    const std::unordered_map<std::string, std::string>& gk,
    const std::vector<ViewFilter>& filters) const
{
    for (const auto& f : filters) {
        auto it = gk.find(f.field);
        std::string val = (it != gk.end()) ? it->second : "";
        FieldValue fv{val};
        if (!applyFilterOp(fv, f.op, f.value)) return false;
    }
    return true;
}

void IncrementalView::applyRow(const ChangeRecord::Row& row, int sign) {
    GroupKey gk = makeGroupKey(row);
    auto& group = groups_[gk];

    for (const auto& spec : def_.aggregations) {
        auto& state = group[spec.output_name];

        FieldValue fv{nullptr};
        if (!spec.source_field.empty()) {
            auto it = row.find(spec.source_field);
            if (it != row.end()) fv = it->second;
        } else {
            // COUNT(*) — use a sentinel value of 1
            fv = 1.0;
        }
        state.add(fv, sign);
    }
}

bool IncrementalView::applyChange(const ChangeRecord& change) {
    if (change.collection != def_.source_collection) return false;

    std::unique_lock lk(rw_mutex_);

    bool applied = false;
    switch (change.type) {
        case ChangeType::INSERT: {
            if (!passesBaseFilters(change.after_row)) break;
            applyRow(change.after_row, +1);
            applied = true;
            break;
        }
        case ChangeType::DELETE: {
            if (!passesBaseFilters(change.before_row)) break;
            applyRow(change.before_row, -1);
            // Remove empty groups
            GroupKey gk = makeGroupKey(change.before_row);
            auto git = groups_.find(gk);
            if (git != groups_.end()) {
                bool all_empty = true;
                for (const auto& [n, s] : git->second) {
                    if (s.count > 0) { all_empty = false; break; }
                }
                if (all_empty) groups_.erase(git);
            }
            applied = true;
            break;
        }
        case ChangeType::UPDATE: {
            // UPDATE = DELETE before + INSERT after
            bool before_passes = passesBaseFilters(change.before_row);
            bool after_passes  = passesBaseFilters(change.after_row);

            if (before_passes) applyRow(change.before_row, -1);
            if (after_passes)  applyRow(change.after_row,  +1);

            // Clean up empty groups
            if (before_passes) {
                GroupKey gk = makeGroupKey(change.before_row);
                auto git = groups_.find(gk);
                if (git != groups_.end()) {
                    bool all_empty = true;
                    for (const auto& [n, s] : git->second) {
                        if (s.count > 0) { all_empty = false; break; }
                    }
                    if (all_empty) groups_.erase(git);
                }
            }
            applied = before_passes || after_passes;
            break;
        }
    }

    if (applied) {
        dirty_.store(true);
        last_update_us_.store(nowMicros());
        ++change_count_;
    }
    return applied;
}

int IncrementalView::applyChanges(const std::vector<ChangeRecord>& changes) {
    int applied = 0;
    std::unique_lock lk(rw_mutex_);

    for (const auto& change : changes) {
        if (change.collection != def_.source_collection) continue;

        bool ok = false;
        switch (change.type) {
            case ChangeType::INSERT:
                if (passesBaseFilters(change.after_row)) {
                    applyRow(change.after_row, +1);
                    ok = true;
                }
                break;
            case ChangeType::DELETE:
                if (passesBaseFilters(change.before_row)) {
                    applyRow(change.before_row, -1);
                    ok = true;
                }
                break;
            case ChangeType::UPDATE: {
                bool bp = passesBaseFilters(change.before_row);
                bool ap = passesBaseFilters(change.after_row);
                if (bp) applyRow(change.before_row, -1);
                if (ap) applyRow(change.after_row,  +1);
                ok = bp || ap;
                break;
            }
        }
        if (ok) ++applied;
    }

    if (applied > 0) {
        dirty_.store(true);
        last_update_us_.store(nowMicros());
        change_count_ += static_cast<uint64_t>(applied);
    }
    return applied;
}

ViewQueryResult IncrementalView::query(
    const std::vector<ViewFilter>& filters,
    int64_t limit,
    int64_t offset) const
{
    std::shared_lock lk(rw_mutex_);

    ViewQueryResult result;
    result.is_stale    = isStale();
    int64_t last_us    = last_update_us_.load();
    result.last_update = (last_us > 0)
        ? std::chrono::system_clock::time_point(std::chrono::microseconds(last_us))
        : std::chrono::system_clock::time_point{};

    int64_t row_idx = 0;
    for (const auto& [gk, agg_map] : groups_) {
        auto group_dims = parseGroupKey(gk);

        // Apply runtime dimension filters
        if (!passesRuntimeFilters(group_dims, filters)) continue;

        ++result.total_rows;

        // Pagination
        if (offset > 0 && row_idx < offset) { ++row_idx; continue; }
        if (limit > 0 && static_cast<int64_t>(result.rows.size()) >= limit) continue;

        ViewRow row;
        row.group_key = group_dims;
        for (const auto& spec : def_.aggregations) {
            auto it = agg_map.find(spec.output_name);
            if (it != agg_map.end()) {
                row.values[spec.output_name] = it->second.result(spec.func);
            }
        }
        result.rows.push_back(std::move(row));
        ++row_idx;
    }

    return result;
}

void IncrementalView::clear() {
    std::unique_lock lk(rw_mutex_);
    groups_.clear();
    dirty_.store(false);
    last_update_us_.store(0);
}

int64_t IncrementalView::groupCount() const {
    std::shared_lock lk(rw_mutex_);
    return static_cast<int64_t>(groups_.size());
}

bool IncrementalView::isStale() const {
    if (def_.staleness_seconds <= 0) return false;
    int64_t last_us = last_update_us_.load();
    if (last_us == 0) return true; // never updated
    int64_t now_us  = nowMicros();
    int64_t age_s   = (now_us - last_us) / 1'000'000LL;
    return age_s > def_.staleness_seconds;
}

std::chrono::system_clock::time_point IncrementalView::lastUpdateTime() const {
    int64_t us = last_update_us_.load();
    if (us <= 0) return {};
    return std::chrono::system_clock::time_point(std::chrono::microseconds(us));
}

// ============================================================================
// IncrementalViewManager
// ============================================================================

IncrementalViewManager::IncrementalViewManager() = default;
IncrementalViewManager::~IncrementalViewManager() = default;

bool IncrementalViewManager::createView(const ViewDefinition& def) {
    std::unique_lock lk(views_mutex_);
    if (views_.count(def.name)) {
        spdlog::warn("IncrementalViewManager: view '{}' already exists", def.name);
        return false;
    }
    views_[def.name] = std::make_shared<IncrementalView>(def);
    spdlog::info("IncrementalViewManager: created view '{}'", def.name);
    return true;
}

bool IncrementalViewManager::dropView(const std::string& name) {
    std::unique_lock lk(views_mutex_);
    if (!views_.count(name)) return false;
    views_.erase(name);
    spdlog::info("IncrementalViewManager: dropped view '{}'", name);
    return true;
}

bool IncrementalViewManager::hasView(const std::string& name) const {
    std::shared_lock lk(views_mutex_);
    return views_.count(name) > 0;
}

std::shared_ptr<IncrementalView> IncrementalViewManager::getView(
    const std::string& name) const
{
    std::shared_lock lk(views_mutex_);
    auto it = views_.find(name);
    return (it != views_.end()) ? it->second : nullptr;
}

std::vector<std::string> IncrementalViewManager::listViews() const {
    std::shared_lock lk(views_mutex_);
    std::vector<std::string> names;
    names.reserve(views_.size());
    for (const auto& [n, _] : views_) names.push_back(n);
    return names;
}

void IncrementalViewManager::applyChange(const ChangeRecord& change) {
    std::shared_lock lk(views_mutex_);
    uint64_t applied = 0;
    for (const auto& [name, view] : views_) {
        if (view->applyChange(change)) ++applied;
    }
    total_changes_ += applied;
}

void IncrementalViewManager::applyChanges(const std::vector<ChangeRecord>& changes) {
    std::shared_lock lk(views_mutex_);
    uint64_t applied = 0;
    for (const auto& [name, view] : views_) {
        applied += static_cast<uint64_t>(view->applyChanges(changes));
    }
    total_changes_ += applied;
}

ViewQueryResult IncrementalViewManager::query(
    const std::string& view_name,
    const std::vector<ViewFilter>& filters,
    int64_t limit,
    int64_t offset) const
{
    std::shared_lock lk(views_mutex_);
    auto it = views_.find(view_name);
    if (it == views_.end()) {
        spdlog::warn("IncrementalViewManager: view '{}' not found", view_name);
        return {};
    }
    return it->second->query(filters, limit, offset);
}

} // namespace analytics
} // namespace themisdb
