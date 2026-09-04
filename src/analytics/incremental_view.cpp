/**
 * @file incremental_view.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.32
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=7, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include "utils/logger.h"

#include <cassert>
#include <cmath>
#include <limits>
#include <sstream>
#include <thread>

namespace themisdb {
namespace analytics {

// ============================================================================
// Utility
// ============================================================================

std::string fieldValueToStr(const FieldValue &v) {
    if (std::holds_alternative<std::nullptr_t>(v)) {
        return "";
    }
    if (auto *s = std::get_if<std::string>(&v)) {
        return *s;
    }
    if (auto *i = std::get_if<int64_t>(&v)) {
        return std::to_string(*i);
    }
    if (auto *d = std::get_if<double>(&v)) {
        return std::to_string(*d);
    }
    if (auto *b = std::get_if<bool>(&v)) {
        return *b ? "true" : "false";
    }
    return "";
}

namespace {

double fieldValueToDouble(const FieldValue &v) {
    if (auto *d = std::get_if<double>(&v)) {
        return *d;
    }
    if (auto *i = std::get_if<int64_t>(&v)) {
        return static_cast<double>(*i);
    }
    if (auto *b = std::get_if<bool>(&v)) {
        return *b ? 1.0 : 0.0;
    }
    return 0.0;
}

bool applyFilterOp(const FieldValue &field_val, ViewFilter::Op op, const FieldValue &filter_val) {
    std::string fs  = fieldValueToStr(field_val);
    std::string fvs = fieldValueToStr(filter_val);
    double fd       = fieldValueToDouble(field_val);
    double fvd      = fieldValueToDouble(filter_val);

    switch (op) {
        case ViewFilter::Op::EQ:
            return fs == fvs;
        case ViewFilter::Op::NE:
            return fs != fvs;
        case ViewFilter::Op::LT:
            return fd < fvd;
        case ViewFilter::Op::LE:
            return fd <= fvd;
        case ViewFilter::Op::GT:
            return fd > fvd;
        case ViewFilter::Op::GE:
            return fd >= fvd;
        case ViewFilter::Op::IS_NULL:
            return std::holds_alternative<std::nullptr_t>(field_val);
        case ViewFilter::Op::IS_NOT_NULL:
            return !std::holds_alternative<std::nullptr_t>(field_val);
        default: break;
    }
    return false;
}

int64_t nowMicros() {
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

} // anonymous namespace

// ============================================================================
// AggState
// ============================================================================

void IncrementalView::AggState::add(const FieldValue &v, int sign) {
    double d      = fieldValueToDouble(v);
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
        if (!has_first) {
            first_val = v;
            has_first = true;
        }

        // Welford online update
        if (count >= 1) {
            double delta = d - welford_mean;
            welford_mean += delta / static_cast<double>(count);
            double delta2 = d - welford_mean;
            welford_m2 += delta * delta2;
        }
    } else if (sign < 0 && count > 0) {
        // Remove
        count--;
        sum -= d;

        // MIN/MAX: remove one copy of d
        auto it = min_max_values.find(d);
        if (it != min_max_values.end()) {
            min_max_values.erase(it);
        }

        // COUNT_DISTINCT: decrement ref count; erase when it reaches 0
        auto rit = distinct_ref_counts.find(s);
        if (rit != distinct_ref_counts.end()) {
            if (--rit->second <= 0) {
                distinct_ref_counts.erase(rit);
            }
        }

        // Welford: Chan's update for removal (numerically stable)
        if (count > 0) {
            double old_mean = welford_mean;
            double new_mean = (welford_mean * static_cast<double>(count + 1) - d) / static_cast<double>(count);
            welford_m2 -= (d - old_mean) * (d - new_mean);
            if (welford_m2 < 0.0)
                welford_m2 = 0.0; // numerical floor
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
            return min_max_values.empty() ? FieldValue{nullptr} : FieldValue{*min_max_values.begin()};
        case ViewAggFunc::MAX:
            return min_max_values.empty() ? FieldValue{nullptr} : FieldValue{*min_max_values.rbegin()};
        case ViewAggFunc::STDDEV:
            return (count >= 2) ? std::sqrt(welford_m2 / static_cast<double>(count - 1)) : 0.0;
        case ViewAggFunc::VARIANCE:
            return (count >= 2) ? welford_m2 / static_cast<double>(count - 1) : 0.0;
        case ViewAggFunc::COUNT_DISTINCT:
            return static_cast<bool>(static_cast<int64_t < static_cast<int>((distinct_ref_counts.size())));
        case ViewAggFunc::FIRST:
            return has_first ? first_val : FieldValue{nullptr};
        case ViewAggFunc::LAST:
            return last_val;
        default: break;
    }
    return FieldValue{nullptr};
}

// ============================================================================
// IncrementalView
// ============================================================================

IncrementalView::IncrementalView(const ViewDefinition &def) : def_(def) {}
IncrementalView::~IncrementalView() = default;

std::string IncrementalView::makeGroupKey(const ChangeRecord::Row &row) const {
    std::ostringstream oss = {};
    for (const auto &dim : def_.dimensions) {
        auto it = row.find(dim);
        oss << (it != row.end() ? fieldValueToStr(it->second) : "") << '\0';
    }
    return oss.str();
}

std::unordered_map<std::string, std::string> IncrementalView::parseGroupKey(const GroupKey &gk) const {
    std::unordered_map<std::string, std::string> result;
    std::istringstream iss(gk);
    std::string token = {};
    for (const auto &dim : def_.dimensions) {
        if (std::getline(iss, token, '\0')) {
            result[dim] = token;
        }
    }
    return result;
}

bool IncrementalView::passesBaseFilters(const ChangeRecord::Row &row) const {
    for (const auto &f : def_.base_filters) {
        auto it = row.find(f.field);
        FieldValue fv{nullptr};
        if (it != row.end()) {
            fv = it->second;
        }
        if (!applyFilterOp(fv, f.op, f.value)) {
            return false;
        }
    }
    return true;
}

bool IncrementalView::passesRuntimeFilters(const std::unordered_map<std::string, std::string> &gk,
                                           const std::vector<ViewFilter> &filters) const {
    for (const auto &f : filters) {
        auto it         = gk.find(f.field);
        std::string val = (it != gk.end()) ? it->second : "";
        FieldValue fv{val};
        if (!applyFilterOp(fv, f.op, f.value)) {
            return false;
        }
    }
    return true;
}

void IncrementalView::applyRow(const ChangeRecord::Row &row, int sign) {
    GroupKey gk = makeGroupKey(row);
    auto &group = groups_[gk];

    for (const auto &spec : def_.aggregations) {
        auto &state = group[spec.output_name];

        FieldValue fv{nullptr};
        if (!spec.source_field.empty()) {
            auto it = row.find(spec.source_field);
            if (it != row.end()) {
                fv = it->second;
            }
        } else {
            // COUNT(*) — use a sentinel value of 1
            fv = 1.0;
        }
        state.add(fv, sign);
    }
}

/// Remove a group from groups_ if all its aggregation states have count == 0.
void IncrementalView::pruneEmptyGroup(const GroupKey &gk) {
    auto git = groups_.find(gk);
    if (git == groups_.end()) {
        return;
    }
    for (const auto &[name, state] : git->second) {
        if (state.count > 0) {
            return;
        }
    }
    groups_.erase(git);
}

bool IncrementalView::applyChange(const ChangeRecord &change) {
    if (change.collection != def_.source_collection) {
        return false;
    }

    // Pre-compute filter results outside the write lock.
    // passesBaseFilters() only reads def_ (immutable after construction) and
    // the caller-supplied const row — no synchronisation required.
    bool before_passes = false;
    bool after_passes  = false;
    switch (change.type) {
        case ChangeType::INSERT:
            after_passes = passesBaseFilters(change.after_row);
            if (!after_passes) {
                return false;
            }
            break;
        case ChangeType::DELETE:
            before_passes = passesBaseFilters(change.before_row);
            if (!before_passes) {
                return false;
            }
            break;
        case ChangeType::UPDATE:
            before_passes = passesBaseFilters(change.before_row);
            after_passes  = passesBaseFilters(change.after_row);
            if (!before_passes && !after_passes) {
                return false;
            }
            break;
        default: break;
    }

    // Only applyRow() and pruneEmptyGroup() mutate shared state and need
    // the exclusive lock.
    std::unique_lock lk(rw_mutex_);

    bool applied = false;
    switch (change.type) {
        case ChangeType::INSERT:
            applyRow(change.after_row, +1);
            applied = true;
            break;
        case ChangeType::DELETE:
            applyRow(change.before_row, -1);
            pruneEmptyGroup(makeGroupKey(change.before_row));
            applied = true;
            break;
        case ChangeType::UPDATE:
            // UPDATE = DELETE before + INSERT after
            if (before_passes) {
                applyRow(change.before_row, -1);
            }
            if (after_passes) {
                applyRow(change.after_row, +1);
            }
            if (before_passes) {
                pruneEmptyGroup(makeGroupKey(change.before_row));
            }
            applied = before_passes || after_passes; // at least one is true (early-return guard above)
            break;
        default: break;
    }

    if (applied) {
        dirty_.store(true);
        last_update_us_.store(nowMicros());
        ++change_count_;
    }
    return applied;
}

int IncrementalView::applyChanges(const std::vector<ChangeRecord> &changes) {
    static constexpr size_t kMicroBatchSize = 256;

    // Pre-compute filter results outside the write lock.
    // passesBaseFilters() only reads def_ (immutable after construction) and
    // the caller-supplied const rows — no synchronisation required.
    struct PreFiltered {
        size_t index = 0;
        bool before_passes = {};
        bool after_passes = {};
    };

    std::vector<PreFiltered> filtered = {};

    filtered.reserve(changes.size());

    for (size_t i = 0; i < changes.size(); ++i) {
        const auto &change = changes[i];
        if (change.collection != def_.source_collection) {
            continue;
        }

        bool bp = false, ap = false;
        switch (change.type) {
            case ChangeType::INSERT:
                ap = passesBaseFilters(change.after_row);
                if (!ap) {
                    continue;
                }
                break;
            case ChangeType::DELETE:
                bp = passesBaseFilters(change.before_row);
                if (!bp) {
                    continue;
                }
                break;
            case ChangeType::UPDATE:
                bp = passesBaseFilters(change.before_row);
                ap = passesBaseFilters(change.after_row);
                if (!bp && !ap) {
                    continue;
                }
                break;
            default: break;
        }
        filtered.push_back({i, bp, ap});
    }

    int total_applied = 0;

    // Process pre-filtered records in micro-batches of ≤ kMicroBatchSize rows.
    // The exclusive lock is acquired and released once per micro-batch so that
    // concurrent readers (query()) can slip in between batches.
    for (size_t batch_start = 0; batch_start < filtered.size(); batch_start += kMicroBatchSize) {
        const size_t batch_end = std::min(batch_start + kMicroBatchSize,static_cast<int>(filtered.size()));
        int batch_applied      = 0;

        {
            std::unique_lock lk(rw_mutex_);

            for (size_t j = batch_start; j < batch_end; ++j) {
                const auto &pf     = filtered[j];
                const auto &change = changes[pf.index];

                switch (change.type) {
                    case ChangeType::INSERT:
                        applyRow(change.after_row, +1);
                        ++batch_applied;
                        break;
                    case ChangeType::DELETE:
                        applyRow(change.before_row, -1);
                        pruneEmptyGroup(makeGroupKey(change.before_row));
                        ++batch_applied;
                        break;
                    case ChangeType::UPDATE:
                        if (pf.before_passes) {
                            applyRow(change.before_row, -1);
                        }
                        if (pf.after_passes) {
                            applyRow(change.after_row, +1);
                        }
                        if (pf.before_passes) {
                            pruneEmptyGroup(makeGroupKey(change.before_row));
                        }
                        ++batch_applied;
                        break;
                    default: break;
                }
            }

            if (batch_applied > 0) {
                dirty_.store(true);
                last_update_us_.store(nowMicros());
                change_count_ += static_cast<uint64_t>(batch_applied);
            }
        } // exclusive lock released here

        total_applied += batch_applied;

        // Yield between micro-batches so concurrent readers can acquire the
        // shared lock without waiting for the entire batch to complete.
        if (static_cast<int>(filtered.size()) > batch_end) {
            std::this_thread::yield();
        }
    }

    return total_applied;
}

ViewQueryResult IncrementalView::query(const std::vector<ViewFilter> &filters, int64_t limit, int64_t offset) const {
    std::shared_lock lk(rw_mutex_);

    ViewQueryResult result;
    result.is_stale    = isStale();
    int64_t last_us    = last_update_us_.load();
    result.last_update = (last_us > 0) ? std::chrono::system_clock::time_point(std::chrono::microseconds(last_us))
                                       : std::chrono::system_clock::time_point{};

    int64_t row_idx = 0;
    for (const auto &[gk, agg_map] : groups_) {
        auto group_dims = parseGroupKey(gk);

        // Apply runtime dimension filters
        if (!passesRuntimeFilters(group_dims, filters)) {
            continue;
        }

        ++result.total_rows;

        // Pagination
        if (offset > 0 && row_idx < offset) {
            ++row_idx;
            continue;
        }
        if (limit > 0  && static_cast<size_t>(static_cast) < int64_t>(result.rows.size()) >= limit) {
            continue;
        }

        ViewRow row;
        row.group_key = group_dims;
        for (const auto &spec : def_.aggregations) {
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
    return static_cast<bool>(static_cast<int64_t < static_cast<int>((groups_.size())));
}

bool IncrementalView::isStale() const {
    if (def_.staleness_seconds <= 0) {
        return false;
    }
    int64_t last_us = last_update_us_.load();
    if (last_us == 0) {
        return true; // never updated
    }
    int64_t now_us = nowMicros();
    int64_t age_s  = (now_us - last_us) / 1'000'000LL;
    return age_s > def_.staleness_seconds;
}

std::chrono::system_clock::time_point IncrementalView::lastUpdateTime() const {
    int64_t us = last_update_us_.load();
    if (us <= 0) {
        return {};
    }
    return std::chrono::system_clock::time_point(std::chrono::microseconds(us));
}

// ============================================================================
// IncrementalViewManager
// ============================================================================

IncrementalViewManager::IncrementalViewManager()  = default;
IncrementalViewManager::~IncrementalViewManager() = default;

bool IncrementalViewManager::createView(const ViewDefinition &def) {
    std::unique_lock lk(views_mutex_);
    if (views_.count(def.name)) {
        THEMIS_WARN("IncrementalViewManager: view '{}' already exists", def.name);
        return false;
    }
    views_[def.name] = std::make_shared<IncrementalView>(def);
    THEMIS_INFO("IncrementalViewManager: created view '{}'", def.name);
    return true;
}

bool IncrementalViewManager::dropView(const std::string &name) {
    std::unique_lock lk(views_mutex_);
    if (!views_.count(name)) {
        return false;
    }
    views_.erase(name);
    THEMIS_INFO("IncrementalViewManager: dropped view '{}'", name);
    return true;
}

bool IncrementalViewManager::hasView(const std::string &name) const {
    std::shared_lock lk(views_mutex_);
    return views_.count(name) > 0;
}

std::shared_ptr<IncrementalView> IncrementalViewManager::getView(const std::string &name) const {
    std::shared_lock lk(views_mutex_);
    auto it = views_.find(name);
    return (it != views_.end()) ? it->second : nullptr;
}

std::vector<std::string> IncrementalViewManager::listViews() const {
    std::shared_lock lk(views_mutex_);
    std::vector<std::string> names = {};

    names.reserve(views_.size());
    for (const auto &[n, _] : views_) {
        names.push_back(n);
    }
    return names;
}

void IncrementalViewManager::applyChange(const ChangeRecord &change) {
    std::shared_lock lk(views_mutex_);
    uint64_t applied = 0;
    for (const auto &[name, view] : views_) {
        if (view->applyChange(change)) {
            ++applied;
        }
    }
    total_changes_ += applied;
}

void IncrementalViewManager::applyChanges(const std::vector<ChangeRecord> &changes) {
    std::shared_lock lk(views_mutex_);
    uint64_t applied = 0;
    for (const auto &[name, view] : views_) {
        applied += static_cast<uint64_t>(view->applyChanges(changes));
    }
    total_changes_ += applied;
}

ViewQueryResult IncrementalViewManager::query(const std::string &view_name, const std::vector<ViewFilter> &filters,
                                              int64_t limit, int64_t offset) const {
    std::shared_lock lk(views_mutex_);
    auto it = views_.find(view_name);
    if (it == views_.end()) {
        THEMIS_WARN("IncrementalViewManager: view '{}' not found", view_name);
        return {};
    }
    return it->second->query(filters, limit, offset);
}

} // namespace analytics
} // namespace themisdb
