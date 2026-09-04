/**
 * @file column_lineage.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=8, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/column_lineage.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <queue>
#include <sstream>
#include <unordered_set>
#include <spdlog/spdlog.h>

namespace themis {
namespace metadata {

// ─── ColumnRef ──────────────────────────────────────────────────────────────

std::string ColumnRef::toString() const {
    return table_name + "." + column_name;
}

nlohmann::json ColumnRef::toJSON() const {
    return {{"table", table_name}, {"column", column_name}};
}

ColumnRef ColumnRef::fromJSON(const nlohmann::json& j) {
    ColumnRef ref;
    ref.table_name  = j.at("table").get<std::string>();
    ref.column_name = j.at("column").get<std::string>();
    return ref;
}

// ─── ColumnRefHash ──────────────────────────────────────────────────────────

std::size_t ColumnRefHash::operator()(const ColumnRef& ref) const noexcept {
    std::size_t h1 = std::hash<std::string>{}(ref.table_name);
    std::size_t h2 = std::hash<std::string>{}(ref.column_name);
    // Combine hashes with a standard mixing constant
    return h1 ^ (h2 * 0x9e3779b9 + (h1 << 6) + (h1 >> 2));
}

// ─── TransformationType helpers ──────────────────────────────────────────────

std::string transformationTypeToString(TransformationType t) {
    switch (t) {
        case TransformationType::DIRECT_COPY:   return "DIRECT_COPY";
        case TransformationType::RENAME:        return "RENAME";
        case TransformationType::CAST:          return "CAST";
        case TransformationType::COMPUTED:      return "COMPUTED";
        case TransformationType::AGGREGATION:   return "AGGREGATION";
        case TransformationType::ANONYMIZATION: return "ANONYMIZATION";
        case TransformationType::ENRICHMENT:    return "ENRICHMENT";
        case TransformationType::CUSTOM:        return "CUSTOM";
    }
    return "UNKNOWN";
}

TransformationType transformationTypeFromString(const std::string& s) {
    std::string upper = s;
    std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
    if (upper == "DIRECT_COPY") {
      return TransformationType::DIRECT_COPY;
    }
    if (upper == "RENAME") {
      return TransformationType::RENAME;
    }
    if (upper == "CAST") {
      return TransformationType::CAST;
    }
    if (upper == "COMPUTED") {
      return TransformationType::COMPUTED;
    }
    if (upper == "AGGREGATION") {
      return TransformationType::AGGREGATION;
    }
    if (upper == "ANONYMIZATION") {
      return TransformationType::ANONYMIZATION;
    }
    if (upper == "ENRICHMENT") {
      return TransformationType::ENRICHMENT;
    }
    return TransformationType::CUSTOM;
}

// ─── ColumnLineageEntry ──────────────────────────────────────────────────────

nlohmann::json ColumnLineageEntry::toJSON() const {
    nlohmann::json j;
    j["entry_id"]   = entry_id;
    j["target"]     = target_column.toJSON();
    j["transformation"] = transformationTypeToString(transformation);
    j["timestamp_ms"]   = timestamp_ms;

    nlohmann::json sources = nlohmann::json::array();
    for (const auto& src : source_columns) {
        sources.push_back(src.toJSON());
    }
    j["source_columns"] = std::move(sources);

    if (!transformation_expression.empty()) {
        j["transformation_expression"] = transformation_expression;
    }
    if (!performed_by.empty()) {
        j["performed_by"] = performed_by;
    }
    if (!metadata.is_null() && !metadata.empty()) {
        j["metadata"] = metadata;
    }
    return j;
}

// ─── ColumnLineageRecord ─────────────────────────────────────────────────────

nlohmann::json ColumnLineageRecord::toJSON() const {
    nlohmann::json j;
    j["column"]      = column.toJSON();
    j["entry_count"] = entries.size();
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& e : entries) {
        arr.push_back(e.toJSON());
    }
    j["entries"] = std::move(arr);
    return j;
}

// ─── ColumnLineageTracker ────────────────────────────────────────────────────

std::string ColumnLineageTracker::assignEntryId() {
    uint64_t seq = next_entry_seq_.fetch_add(1, std::memory_order_relaxed);
    std::ostringstream oss;
    oss << "col-lineage-" << seq;
    return oss.str();
}

void ColumnLineageTracker::recordDerivation(ColumnLineageEntry entry) {
    // Auto-assign timestamp
    if (entry.timestamp_ms == 0) {
        entry.timestamp_ms = static_cast<int64_t>(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
    }
    // Auto-assign entry_id
    if (entry.entry_id.empty()) {
        entry.entry_id = assignEntryId();
    }

    spdlog::info(
        "ColumnLineageTracker: recording derivation entry_id='{}' target='{}.{}' "
        "transformation='{}' sources={}",
        entry.entry_id,
        entry.target_column.table_name,
        entry.target_column.column_name,
        transformationTypeToString(entry.transformation),
        entry.source_columns.size());

    std::lock_guard<std::mutex> lock(mutex_);

    // Primary index: target → entries
    entries_by_target_[entry.target_column].push_back(entry);

    // Secondary index: source → targets
    for (const auto& src : entry.source_columns) {
        targets_by_source_[src].push_back(entry.target_column);
    }

    // Insertion-order list for full export
    all_entries_.push_back(std::move(entry));
}

ColumnLineageRecord ColumnLineageTracker::getColumnLineage(const ColumnRef& col) const {
    std::lock_guard<std::mutex> lock(mutex_);
    ColumnLineageRecord record;
    record.column = col;
    auto it = entries_by_target_.find(col);
    if (it != entries_by_target_.end()) {
        record.entries = it->second;
    }
    return record;
}

std::vector<ColumnRef> ColumnLineageTracker::getUpstreamColumns(const ColumnRef& col) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ColumnRef> result;
    std::unordered_set<ColumnRef, ColumnRefHash> visited;

    // BFS upward (source direction)
    std::queue<ColumnRef> frontier;
    frontier.push(col);
    visited.insert(col);

    while (!frontier.empty()) {
        ColumnRef current = frontier.front();
        frontier.pop();

        auto it = entries_by_target_.find(current);
        if (it == entries_by_target_.end()) {
          continue;
        }

        for (const auto& entry : it->second) {
            for (const auto& src : entry.source_columns) {
                if (visited.find(src) == visited.end()) {
                    visited.insert(src);
                    result.push_back(src);
                    frontier.push(src);
                }
            }
        }
    }

    return result;
}

std::vector<ColumnRef> ColumnLineageTracker::getDownstreamColumns(const ColumnRef& col) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ColumnRef> result;
    std::unordered_set<ColumnRef, ColumnRefHash> visited;

    // BFS downward (derived direction)
    std::queue<ColumnRef> frontier;
    frontier.push(col);
    visited.insert(col);

    while (!frontier.empty()) {
        ColumnRef current = frontier.front();
        frontier.pop();

        auto it = targets_by_source_.find(current);
        if (it == targets_by_source_.end()) {
          continue;
        }

        for (const auto& target : it->second) {
            if (visited.find(target) == visited.end()) {
                visited.insert(target);
                result.push_back(target);
                frontier.push(target);
            }
        }
    }

    return result;
}

nlohmann::json ColumnLineageTracker::getColumnProvenance(const ColumnRef& col) const {
    // getUpstreamColumns and getDownstreamColumns each acquire the lock;
    // call them before building the JSON so we do not hold the lock twice.
    ColumnLineageRecord record = getColumnLineage(col);
    std::vector<ColumnRef> upstream   = getUpstreamColumns(col);
    std::vector<ColumnRef> downstream = getDownstreamColumns(col);

    nlohmann::json j;
    j["column"] = col.toJSON();

    nlohmann::json entries_arr = nlohmann::json::array();
    for (const auto& e : record.entries) {
        entries_arr.push_back(e.toJSON());
    }
    j["entries"] = std::move(entries_arr);

    nlohmann::json up_arr = nlohmann::json::array();
    for (const auto& ref : upstream) {
        up_arr.push_back(ref.toJSON());
    }
    j["upstream_columns"] = std::move(up_arr);

    nlohmann::json down_arr = nlohmann::json::array();
    for (const auto& ref : downstream) {
        down_arr.push_back(ref.toJSON());
    }
    j["downstream_columns"] = std::move(down_arr);

    return j;
}

nlohmann::json ColumnLineageTracker::exportTableLineage(const std::string& table_name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json result = nlohmann::json::array();
    for (const auto& [col_ref, entries] : entries_by_target_) {
        if (col_ref.table_name != table_name) {
          continue;
        }
        ColumnLineageRecord record;
        record.column   = col_ref;
        record.entries  = entries;
        result.push_back(record.toJSON());
    }
    return result;
}

nlohmann::json ColumnLineageTracker::exportAllLineage() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json entries_arr = nlohmann::json::array();
    for (const auto& e : all_entries_) {
        entries_arr.push_back(e.toJSON());
    }
    nlohmann::json j;
    j["entries"]       = std::move(entries_arr);
    j["total_entries"] = all_entries_.size();
    return j;
}

size_t ColumnLineageTracker::totalEntryCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return all_entries_.size();
}

} // namespace metadata
} // namespace themis
