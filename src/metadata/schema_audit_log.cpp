/**
 * @file schema_audit_log.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/schema_audit_log.h"
#include "storage/rocksdb_wrapper.h"
#include <spdlog/spdlog.h>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <stdexcept>
#include <algorithm>

namespace themis {

// ============================================================================
// SchemaAuditEntry serialization
// ============================================================================

json SchemaAuditEntry::toJSON() const {
    json j;
    j["id"]          = id;
    j["table_name"]  = table_name;
    j["operation"]   = operation;
    j["author"]      = author;
    j["description"] = description;
    j["version"]     = version;

    auto tt = std::chrono::system_clock::to_time_t(timestamp);
    char buf[64];
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &tt);
#else
    gmtime_r(&tt, &tm_buf);
#endif
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    j["timestamp"] = buf;

    if (!metadata.is_null() && !metadata.empty()) {
        j["metadata"] = metadata;
    } else {
        j["metadata"] = json::object();
    }
    return j;
}

SchemaAuditEntry SchemaAuditEntry::fromJSON(const json& j) {
    SchemaAuditEntry e;
    e.id          = j.value("id",          std::string{});
    e.table_name  = j.value("table_name",  std::string{});
    e.operation   = j.value("operation",   std::string{});
    e.author      = j.value("author",      std::string{});
    e.description = j.value("description", std::string{});
    e.version     = j.value("version",     uint64_t{0});
    e.timestamp   = std::chrono::system_clock::now();  // Approx on reload

    if (j.contains("metadata") && j["metadata"].is_object()) {
        e.metadata = j["metadata"];
    } else {
        e.metadata = json::object();
    }
    return e;
}

// ============================================================================
// SchemaAuditLog
// ============================================================================

SchemaAuditLog::SchemaAuditLog(RocksDBWrapper& db)
    : db_(db)
{}

// ============================================================================
// Static helpers
// ============================================================================

std::string SchemaAuditLog::buildKey(std::string_view table_name, uint64_t timestamp_ns) {
    std::ostringstream oss = {};
    oss << kKeyPrefix << table_name << ":" << std::setw(20) << std::setfill('0') << timestamp_ns;
    return oss.str();
}

std::string SchemaAuditLog::tablePrefix(std::string_view table_name) {
    return std::string(kKeyPrefix) + std::string(table_name) + ":";
}

// ============================================================================
// Write
// ============================================================================

bool SchemaAuditLog::record(
    std::string_view table_name,
    std::string_view operation,
    std::string_view author,
    std::string_view description,
    uint64_t         version,
    const json&      extra_meta)
{
    try {
        auto now = std::chrono::system_clock::now();
        uint64_t ns = static_cast<uint64_t>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                now.time_since_epoch()).count());

        SchemaAuditEntry entry;
        entry.table_name  = std::string(table_name);
        entry.operation   = std::string(operation);
        entry.author      = std::string(author);
        entry.description = std::string(description);
        entry.timestamp   = now;
        entry.version     = version;
        entry.metadata    = extra_meta;
        entry.id          = std::string(table_name) + ":" + std::to_string(ns);

        std::string key   = buildKey(table_name, ns);
        std::string value = entry.toJSON().dump();

        bool ok = db_.put(key, value);
        if (!ok) {
            spdlog::warn("SchemaAuditLog: Failed to persist entry for table='{}' op='{}'",
                         table_name, operation);
            return false;
        }

        spdlog::info("SchemaAuditLog: Recorded op='{}' table='{}' author='{}' version={} desc='{}'",
                     operation, table_name, author, version, description);
        return true;

    } catch (const std::exception& e) {
        spdlog::error("SchemaAuditLog: Exception recording entry: {}", e.what());
        return false;
    }
}

// ============================================================================
// Read helpers
// ============================================================================

std::vector<SchemaAuditEntry> SchemaAuditLog::getHistory(std::string_view table_name) const {
    std::vector<SchemaAuditEntry> result;
    std::string prefix = tablePrefix(table_name);

    try {
        db_.scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
            try {
                auto j = json::parse(value);
                result.push_back(SchemaAuditEntry::fromJSON(j));
            } catch (const std::exception& ex) {
                spdlog::warn("SchemaAuditLog: Failed to parse entry key='{}': {}", key, ex.what());
            }
            return true;
        });
    } catch (const std::exception& e) {
        spdlog::warn("SchemaAuditLog: Failed to read history for '{}': {}", table_name, e.what());
    }

    // Sort by id (which embeds timestamp) ascending
    std::sort(result.begin(), result.end(), [](const SchemaAuditEntry& a, const SchemaAuditEntry& b) {
        return a.id < b.id;
    });
    return result;
}

std::vector<SchemaAuditEntry> SchemaAuditLog::getFullHistory() const {
    std::vector<SchemaAuditEntry> result;
    std::string prefix = std::string(kKeyPrefix);

    try {
        db_.scanPrefix(prefix, [&](std::string_view key, std::string_view value) {
            try {
                auto j = json::parse(value);
                result.push_back(SchemaAuditEntry::fromJSON(j));
            } catch (const std::exception& ex) {
                spdlog::warn("SchemaAuditLog: Failed to parse entry key='{}': {}", key, ex.what());
            }
            return true;
        });
    } catch (const std::exception& e) {
        spdlog::warn("SchemaAuditLog: Failed to read full audit history: {}", e.what());
    }

    std::sort(result.begin(), result.end(), [](const SchemaAuditEntry& a, const SchemaAuditEntry& b) {
        return a.id < b.id;
    });
    return result;
}

std::vector<SchemaAuditEntry> SchemaAuditLog::getRecentHistory(
    std::string_view table_name,
    size_t limit) const
{
    auto all = getHistory(table_name);
    if (all.size() > limit) {
        // Return most recent `limit` entries (last N after ascending sort)
        return std::vector<SchemaAuditEntry>(all.end() - static_cast<std::ptrdiff_t>(limit), all.end());
    }
    return all;
}

// ============================================================================
// JSON export
// ============================================================================

json SchemaAuditLog::historyToJSON(std::string_view table_name) const {
    json arr = json::array();
    for (const auto& e : getHistory(table_name)) {
        arr.push_back(e.toJSON());
    }
    return arr;
}

json SchemaAuditLog::fullHistoryToJSON() const {
    json arr = json::array();
    for (const auto& e : getFullHistory()) {
        arr.push_back(e.toJSON());
    }
    return arr;
}

} // namespace themis
