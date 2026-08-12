/**
 * @file schema_audit_log.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
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
#include <string_view>
#include <vector>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {

// Forward declaration
class RocksDBWrapper;

using json = nlohmann::json;

// ============================================================================
// SchemaAuditEntry – one record in the audit log
// ============================================================================

/// A single entry in the schema audit log.
///
/// Each schema mutation (table create / update / delete, rollback, import)
/// produces one entry stored durably under the key prefix `audit:schema:`.
///
/// JSON layout:
/// {
///   "id":          "<table>:<timestamp_ns>",
///   "table_name":  "users",
///   "operation":   "create" | "update" | "delete" | "rollback" | "import",
///   "author":      "admin",
///   "description": "added phone column",
///   "timestamp":   "2026-02-20T18:00:00Z",
///   "version":     2,
///   "metadata":    { ... }          // optional extra fields
/// }
struct SchemaAuditEntry {
    std::string id;               ///< Storage key suffix: "<table>:<ns since epoch>"
    std::string table_name;
    std::string operation;        ///< "create","update","delete","rollback","import"
    std::string author;           ///< Who triggered the change (empty = system)
    std::string description;
    std::chrono::system_clock::time_point timestamp;
    uint64_t version = 0;         ///< Associated schema version (0 if N/A)
    json metadata;                ///< Free-form extra context

    json toJSON() const;
    static SchemaAuditEntry fromJSON(const json& j);
};

// ============================================================================
// SchemaAuditLog
// ============================================================================

/// SchemaAuditLog – durable append-only audit trail for schema changes.
///
/// Each entry is stored in RocksDB under:
///     `audit:schema:<table_name>:<timestamp_ns>`
///
/// This layout allows efficient prefix-scan retrieval by table name.
///
/// All writes use `RocksDBWrapper::put()`.  Reads use prefix-scan iteration.
///
/// Thread-safety: thread-safe for concurrent reads and sequential writes
/// (individual puts are atomic at the RocksDB level).
///
/// Usage:
///   SchemaAuditLog audit_log(db);
///   audit_log.record("users", "update", "alice", "added phone column", 2);
///   auto history = audit_log.getHistory("users");
class SchemaAuditLog {
public:
    /// RocksDB key prefix used for all audit entries
    static constexpr std::string_view kKeyPrefix = "audit:schema:";

    /// Construct with a storage reference.
    /// @param db  RocksDB instance (non-owning reference)
    explicit SchemaAuditLog(RocksDBWrapper& db);
    ~SchemaAuditLog() = default;

    SchemaAuditLog(const SchemaAuditLog&) = delete;
    SchemaAuditLog& operator=(const SchemaAuditLog&) = delete;

    // ========================================================================
    // Write API
    // ========================================================================

    /// Record a schema change event.
    /// @param table_name    Name of the affected table
    /// @param operation     Operation type: "create", "update", "delete", "rollback", "import"
    /// @param author        Who performed the change (may be empty for system operations)
    /// @param description   Human-readable description of the change
    /// @param version       Associated schema version number (0 if N/A)
    /// @param extra_meta    Optional additional JSON metadata
    /// @return true on success; logs a warning and returns false on storage error
    bool record(
        std::string_view table_name,
        std::string_view operation,
        std::string_view author      = "",
        std::string_view description = "",
        uint64_t         version     = 0,
        const json&      extra_meta  = json::object()
    );

    // ========================================================================
    // Read API
    // ========================================================================

    /// Return all audit entries for a given table, in ascending timestamp order.
    /// An empty vector is returned if no entries exist or on storage error.
    std::vector<SchemaAuditEntry> getHistory(std::string_view table_name) const;

    /// Return audit entries across ALL tables, in ascending timestamp order.
    std::vector<SchemaAuditEntry> getFullHistory() const;

    /// Return the most recent N entries for a table (newest-first).
    std::vector<SchemaAuditEntry> getRecentHistory(
        std::string_view table_name,
        size_t           limit = 50
    ) const;

    // ========================================================================
    // JSON export
    // ========================================================================

    /// Serialize the audit history for a table as a JSON array.
    json historyToJSON(std::string_view table_name) const;

    /// Serialize the full audit history as a JSON array.
    json fullHistoryToJSON() const;

private:
    RocksDBWrapper& db_;

    /// Build the full RocksDB key for an entry.
    static std::string buildKey(std::string_view table_name, uint64_t timestamp_ns);

    /// Prefix used for scanning all entries of a specific table.
    static std::string tablePrefix(std::string_view table_name);
};

} // namespace themis
