/**
 * @file update_history_logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "updates/update_history_logger.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...)  SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...)  SPDLOG_WARN(__VA_ARGS__)

#include <fstream>
#include <filesystem>
#include <mutex>
#include <algorithm>

namespace themis {
namespace updates {

namespace fs = std::filesystem;

// ============================================================================
// UpdateHistoryEntry JSON serialisation
// ============================================================================

json UpdateHistoryEntry::toJson() const {
    return {
        {"who",           who},
        {"timestamp_ms",  timestamp_ms},
        {"from_version",  from_version},
        {"to_version",    to_version},
        {"event_type",    event_type},
        {"success",       success},
        {"error_message", error_message}
    };
}

UpdateHistoryEntry UpdateHistoryEntry::fromJson(const json& j) {
    UpdateHistoryEntry e;
    e.who           = j.value("who",           "");
    e.timestamp_ms  = j.value("timestamp_ms",  int64_t{0});
    e.from_version  = j.value("from_version",  "");
    e.to_version    = j.value("to_version",    "");
    e.event_type    = j.value("event_type",    "");
    e.success       = j.value("success",       false);
    e.error_message = j.value("error_message", "");
    return e;
}

// ============================================================================
// UpdateHistoryLogger
// ============================================================================

UpdateHistoryLogger::UpdateHistoryLogger(const std::string& log_file_path)
    : log_file_path_(log_file_path) {
    // Ensure the parent directory exists
    auto parent = fs::path(log_file_path_).parent_path();
    if (!parent.empty()) {
        fs::create_directories(parent);
    }
    LOG_INFO("UpdateHistoryLogger initialised: {}", log_file_path_);
}

void UpdateHistoryLogger::record(const UpdateHistoryEntry& entry) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto entries = loadEntries();
    entries.push_back(entry);
    saveEntries(entries);
    LOG_INFO("Update history recorded: {} {} -> {} (success={})",
             entry.who, entry.from_version, entry.to_version, entry.success);
}

std::vector<UpdateHistoryEntry> UpdateHistoryLogger::getHistory([[maybe_unused]] size_t limit) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto entries = loadEntries();
    // Newest first
    std::reverse(entries.begin(), entries.end());
    if (limit > 0 && entries.size() > limit) {
        entries.resize(limit);
    }
    return entries;
}

void UpdateHistoryLogger::clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    saveEntries({});
    LOG_INFO("Update history cleared: {}", log_file_path_);
}

const std::string& UpdateHistoryLogger::logFilePath() const {
    return log_file_path_;
}

// ============================================================================
// Private helpers
// ============================================================================

std::vector<UpdateHistoryEntry> UpdateHistoryLogger::loadEntries() const {
    std::vector<UpdateHistoryEntry> entries;
    if (!fs::exists(log_file_path_)) {
        return entries;
    }
    try {
        std::ifstream file(log_file_path_);
        if (!file.is_open()) {
            LOG_WARN("Cannot open update history file: {}", log_file_path_);
            return entries;
        }
        json arr;
        file >> arr;
        if (!arr.is_array()) {
            LOG_WARN("Update history file is not a JSON array: {}", log_file_path_);
            return entries;
        }
        for (const auto& item : arr) {
            entries.push_back(UpdateHistoryEntry::fromJson(item));
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to load update history: {}", e.what());
    }
    return entries;
}

void UpdateHistoryLogger::saveEntries(const std::vector<UpdateHistoryEntry>& entries) const {
    try {
        json arr = json::array();
        for (const auto& e : entries) {
            arr.push_back(e.toJson());
        }
        std::ofstream file(log_file_path_);
        if (!file.is_open()) {
            LOG_ERROR("Cannot write update history file: {}", log_file_path_);
            return;
        }
        file << arr.dump(2);
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to save update history: {}", e.what());
    }
}

} // namespace updates
} // namespace themis
