/**
 * @file snapshot_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=7, M=10, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Temporal Snapshot Manager Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/snapshot_manager.h"
#include <algorithm>
#include <chrono>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>

namespace themisdb {
namespace temporal {

// ============================================================================
// Public methods
// ============================================================================

TemporalSnapshotManager::TemporalSnapshotManager(ClockFn clock)
    : clock_(std::move(clock)) {}

SnapshotHandle TemporalSnapshotManager::createSnapshot(
    const std::map<std::string, const SystemVersionedTable*>& tables) {

    Timestamp creation_ts = clock_();

    SnapshotData data;
    data.handle.snapshot_id    = generateSnapshotId();
    data.handle.creation_time  = creation_ts;

    for (const auto& [name, table_ptr] : tables) {
        if (!table_ptr) {
            continue;
        }
        data.handle.included_tables.push_back(name);
        // Capture the state as-of the creation timestamp
        data.tables[name] = table_ptr->scan(creation_ts);
    }

    std::lock_guard<std::mutex> lock(mutex_);
    data.handle.version_number = next_version_++;
    SnapshotHandle result_handle = data.handle; // copy before move
    snapshots_[data.handle.snapshot_id] = std::move(data);
    ++total_created_;

    return result_handle;
}

std::vector<VersionedDocument> TemporalSnapshotManager::querySnapshot(
    const SnapshotHandle& handle,
    const std::string& table_name,
    const std::vector<std::pair<std::string, nlohmann::json>>& filters) const {

    std::lock_guard<std::mutex> lock(mutex_);

    auto snap_it = snapshots_.find(handle.snapshot_id);
    if (snap_it == snapshots_.end()) {
        return {};
    }

    auto tbl_it = snap_it->second.tables.find(table_name);
    if (tbl_it == snap_it->second.tables.end()) {
        return {};
    }

    if (filters.empty()) {
        return tbl_it->second;
    }

    std::vector<VersionedDocument> result;
    for (const auto& row : tbl_it->second) {
        bool match = true;
        for (const auto& [field, value] : filters) {
            auto it = row.data.find(field);
            if (it == row.data.end() || *it != value) {
                match = false;
                break;
            }
        }
        if (match) {
            result.push_back(row);
        }
    }
    return result;
}

bool TemporalSnapshotManager::releaseSnapshot(const SnapshotHandle& handle) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = snapshots_.find(handle.snapshot_id);
    if (it == snapshots_.end()) {
        return false;
    }
    snapshots_.erase(it);
    ++total_released_;
    return true;
}

bool TemporalSnapshotManager::isAlive(const SnapshotHandle& handle) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshots_.count(handle.snapshot_id) > 0;
}

size_t TemporalSnapshotManager::snapshotCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return snapshots_.size();
}

SnapshotMetadata TemporalSnapshotManager::getSnapshotMetadata(
    const SnapshotHandle& handle) const {

    std::lock_guard<std::mutex> lock(mutex_);

    auto it = snapshots_.find(handle.snapshot_id);
    if (it == snapshots_.end()) {
        return SnapshotMetadata{};
    }

    const SnapshotData& data = it->second;
    SnapshotMetadata meta;
    meta.handle       = data.handle;
    meta.total_tables = data.tables.size();
    meta.is_valid     = true;

    for (const auto& [name, rows] : data.tables) {
        meta.total_rows += rows.size();
    }

    return meta;
}

size_t TemporalSnapshotManager::garbageCollectByAge(Timestamp max_age_ms) {
    if (max_age_ms <= 0) {
        return 0;
    }

    const Timestamp cutoff = clock_() - max_age_ms;
    std::vector<std::string> to_remove;

    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [id, data] : snapshots_) {
        if (data.handle.creation_time < cutoff) {
            to_remove.push_back(id);
        }
    }

    for (const auto& id : to_remove) {
        snapshots_.erase(id);
        ++total_released_;
        ++total_gc_collected_;
    }

    return to_remove.size();
}

size_t TemporalSnapshotManager::garbageCollectByCount(size_t max_snapshots) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (snapshots_.size() <= max_snapshots) {
        return 0;
    }

    // Collect snapshot IDs ordered by version_number (oldest first)
    std::vector<std::pair<uint64_t, std::string>> ordered;
    ordered.reserve(snapshots_.size());
    for (const auto& [id, data] : snapshots_) {
        ordered.emplace_back(data.handle.version_number, id);
    }
    std::sort(ordered.begin(), ordered.end());

    const size_t to_remove_count = snapshots_.size() - max_snapshots;
    for (size_t i = 0; i < to_remove_count; ++i) {
        snapshots_.erase(ordered[i].second);
        ++total_released_;
        ++total_gc_collected_;
    }

    return to_remove_count;
}

nlohmann::json TemporalSnapshotManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {{"active_snapshots", snapshots_.size()},
            {"total_created", total_created_},
            {"total_released", total_released_},
            {"total_gc_collected", total_gc_collected_},
            {"next_version", next_version_}};
}

// ============================================================================
// Private helpers
// ============================================================================

std::string TemporalSnapshotManager::generateSnapshotId() {
    auto ts = std::chrono::duration_cast<std::chrono::milliseconds>(
                  std::chrono::system_clock::now().time_since_epoch())
                  .count();

    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::mutex gen_mutex;

    std::lock_guard<std::mutex> lock(gen_mutex);
    std::uniform_int_distribution<uint32_t> dist;

    std::ostringstream oss;
    oss << "snap_" << ts << "_" << dist(gen);
    return oss.str();
}

// ============================================================================
// SnapshotDiff helpers
// ============================================================================

nlohmann::json TemporalSnapshotManager::SnapshotDiff::toJson() const {
    auto table_map_to_json = [](const std::map<std::string,
                                                std::vector<std::string>>& m) {
        nlohmann::json j = nlohmann::json::object();
        for (const auto& [table, keys] : m) {
            j[table] = keys;
        }
        return j;
    };
    return {{"tables_examined", tables_examined},
            {"empty",           empty()},
            {"modified",        table_map_to_json(modified)},
            {"added",           table_map_to_json(added)},
            {"removed",         table_map_to_json(removed)}};
}

// ============================================================================
// TemporalSnapshotManager::diff
// ============================================================================

TemporalSnapshotManager::SnapshotDiff
TemporalSnapshotManager::diff(const SnapshotHandle& base,
                               const SnapshotHandle& other) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it_base  = snapshots_.find(base.snapshot_id);
    auto it_other = snapshots_.find(other.snapshot_id);

    if (it_base == snapshots_.end()) {
        throw std::invalid_argument(
            "diff: base snapshot '" + base.snapshot_id + "' is not valid / was released");
    }
    if (it_other == snapshots_.end()) {
        throw std::invalid_argument(
            "diff: other snapshot '" + other.snapshot_id + "' is not valid / was released");
    }

    const auto& base_tables  = it_base->second.tables;
    const auto& other_tables = it_other->second.tables;

    // Collect all table names that appear in either snapshot.
    std::set<std::string> all_tables;
    for (const auto& [t, _] : base_tables)  all_tables.insert(t);
    for (const auto& [t, _] : other_tables) all_tables.insert(t);

    SnapshotDiff result;

    for (const auto& table : all_tables) {
        auto b_it = base_tables.find(table);
        auto o_it = other_tables.find(table);

        const bool in_base  = (b_it != base_tables.end());
        const bool in_other = (o_it != other_tables.end());

        if (!in_base && in_other) {
            // Entire table added
            ++result.tables_examined;
            for (const auto& row : o_it->second) {
                result.added[table].push_back(row.key);
            }
            continue;
        }
        if (in_base && !in_other) {
            // Entire table removed
            ++result.tables_examined;
            for (const auto& row : b_it->second) {
                result.removed[table].push_back(row.key);
            }
            continue;
        }

        // Table present in both snapshots — compare per-key.
        ++result.tables_examined;

        // Build key → latest-version maps for each snapshot.
        auto build_key_map = [](const std::vector<VersionedDocument>& rows)
            -> std::unordered_map<std::string, const VersionedDocument*>
        {
            std::unordered_map<std::string, const VersionedDocument*> m;
            for (const auto& row : rows) {
                auto it = m.find(row.key);
                if (it == m.end() || row.sys_time.start > it->second->sys_time.start) {
                    m[row.key] = &row;
                }
            }
            return m;
        };

        auto base_map  = build_key_map(b_it->second);
        auto other_map = build_key_map(o_it->second);

        // Added / modified
        for (const auto& [key, o_row] : other_map) {
            auto b_it2 = base_map.find(key);
            if (b_it2 == base_map.end()) {
                result.added[table].push_back(key);
            } else if (o_row->data != b_it2->second->data) {
                result.modified[table].push_back(key);
            }
        }

        // Removed
        for (const auto& [key, _] : base_map) {
            if (other_map.find(key) == other_map.end()) {
                result.removed[table].push_back(key);
            }
        }
    }

    // Sort key lists for deterministic output.
    auto sort_keys = [](std::map<std::string, std::vector<std::string>>& m) {
        for (auto& [_, keys] : m) {
            std::sort(keys.begin(), keys.end());
        }
    };
    sort_keys(result.modified);
    sort_keys(result.added);
    sort_keys(result.removed);

    return result;
}

} // namespace temporal
} // namespace themisdb
