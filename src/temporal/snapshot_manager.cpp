/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            snapshot_manager.cpp                               ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   96.0/100                                       ║
    • Total Lines:     154                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * ThemisDB Temporal Snapshot Manager Implementation
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#include "temporal/snapshot_manager.h"
#include <chrono>
#include <random>
#include <sstream>

namespace themisdb {
namespace temporal {

// ============================================================================
// Public methods
// ============================================================================

SnapshotHandle TemporalSnapshotManager::createSnapshot(
    const std::map<std::string, const SystemVersionedTable*>& tables) {

    Timestamp creation_ts = now();

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

nlohmann::json TemporalSnapshotManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return {{"active_snapshots", snapshots_.size()},
            {"total_created", total_created_},
            {"total_released", total_released_}};
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

} // namespace temporal
} // namespace themisdb
