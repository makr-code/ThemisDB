/**
 * @file update_history_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace updates {

using json = nlohmann::json;

/**
 * @brief A single entry in the update history log.
 *
 * Records who performed an update, when it happened, and what changed
 * (from_version → to_version), together with success/failure state.
 */
struct UpdateHistoryEntry {
    /// Actor that triggered the update (username, service account, or "system").
    std::string who;
    /// UTC timestamp in milliseconds since epoch.
    int64_t timestamp_ms{0};
    /// Version before the operation.
    std::string from_version;
    /// Version after the operation (same as from_version for failed attempts).
    std::string to_version;
    /// "update" or "rollback".
    std::string event_type;
    /// true if the operation completed successfully.
    bool success{false};
    /// Non-empty when success == false.
    std::string error_message;

    json toJson() const;
    static UpdateHistoryEntry fromJson(const json& j);
};

/**
 * @brief Persistent, append-only log of update operations.
 *
 * Entries are written as a JSON array to a single log file.  The file is
 * read on construction and appended on every @ref record call.  The class
 * is intentionally lightweight and does not depend on RocksDB or any heavy
 * storage layer.
 *
 * ### Thread Safety
 * All public methods are protected by an internal mutex and are safe to call
 * from multiple threads.
 *
 * ### Usage
 * @code
 * UpdateHistoryLogger logger("/var/lib/themisdb/update_history.json");
 *
 * UpdateHistoryEntry e;
 * e.who          = "admin";
 * e.from_version = "1.4.0";
 * e.to_version   = "1.5.0";
 * e.event_type   = "update";
 * e.success      = true;
 * e.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
 *     std::chrono::system_clock::now().time_since_epoch()).count();
 * logger.record(e);
 *
 * auto history = logger.getHistory(10);  // last 10 entries
 * @endcode
 */
class UpdateHistoryLogger {
public:
    /**
     * @brief Construct the logger.
     * @param log_file_path Path to the JSON history file.
     *        The file is created if it does not exist.
     */
    explicit UpdateHistoryLogger(const std::string& log_file_path);

    /**
     * @brief Append an entry to the history log.
     * @param entry The update event to persist.
     */
    void record(const UpdateHistoryEntry& entry);

    /**
     * @brief Retrieve the most recent history entries.
     * @param limit Maximum number of entries to return (0 = all, newest first).
     * @return Vector of entries ordered newest-first.
     */
    std::vector<UpdateHistoryEntry> getHistory(size_t limit = 0) const;

    /**
     * @brief Remove all entries from the history log.
     */
    void clear();

    /**
     * @brief Return the path of the backing log file.
     */
    const std::string& logFilePath() const;

private:
    std::string log_file_path_;
    mutable std::mutex mutex_;

    /// Load entries from the JSON file; returns empty vector on any error.
    std::vector<UpdateHistoryEntry> loadEntries() const;

    /// Persist @p entries to the JSON file (overwrites).
    void saveEntries(const std::vector<UpdateHistoryEntry>& entries) const;
};

} // namespace updates
} // namespace themis
