// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file workspace_state_manager.h
 * @brief Persistent workspace state management with corruption detection.
 *
 * Provides:
 *  - JSON-based state persistence (state.json)
 *  - Atomic write-replace semantics (write to temp, rename on success)
 *  - SHA-256 checksum validation on load
 *  - Fallback recovery via append-only transaction log (state.log)
 *
 * ## State Schema
 *
 * @code
 * {
 *   "version": "1.0.0",
 *   "created_at": "2026-08-01T12:00:00Z",
 *   "last_updated": "2026-08-01T12:00:00Z",
 *   "checksum": "sha256:abc123...",
 *   "workspace_root": "/path/to/workspace",
 *   "links": {
 *     "page_a": ["page_b", "page_c"],
 *     "page_b": ["page_a"]
 *   },
 *   "tasks": {
 *     "task_id": {
 *       "type": "contradiction_review",
 *       "pages": ["page_a", "page_b"],
 *       "status": "open",
 *       "created_at": "2026-08-01T11:50:00Z"
 *     }
 *   }
 * }
 * @endcode
 *
 * @version 1.0.0 (Phase 3 hardening)
 */

#pragma once

#include <chrono>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

// Forward-declare RocksDBWrapper to keep the public header free of heavy
// RocksDB includes. Users that enable the RocksDB backend must link against
// the storage library but do not need to include rocksdb_wrapper.h themselves.
namespace themis {
class RocksDBWrapper;
} // namespace themis

namespace themis {
namespace llm_wiki {

// ============================================================================
// Status type for workspace operations
// ============================================================================

/**
 * @brief Result type for workspace state operations.
 */
struct WorkspaceStatus {
    enum class Code {
        Ok,                     ///< Operation succeeded
        Error,                  ///< Generic failure
        CorruptState,          ///< State file is corrupted (checksum mismatch)
        InvalidJson,           ///< JSON parse error
        FileNotFound,          ///< State file doesn't exist
        WriteFailure,          ///< Failed to write state
        ChecksumMismatch,      ///< SHA-256 checksum validation failed
    };

    Code        code = Code::Ok;
    std::string message;

    [[nodiscard]] bool ok() const noexcept { return code == Code::Ok; }

    [[nodiscard]] static WorkspaceStatus Ok() {
        return {Code::Ok, {}};
    }
    
    [[nodiscard]] static WorkspaceStatus Error(std::string msg) {
        return {Code::Error, std::move(msg)};
    }
    
    [[nodiscard]] static WorkspaceStatus CorruptState(std::string msg) {
        return {Code::CorruptState, std::move(msg)};
    }
    
    [[nodiscard]] static WorkspaceStatus ChecksumMismatch(std::string expected,
                                                          std::string actual) {
        return {Code::ChecksumMismatch,
                "Checksum mismatch: expected=" + expected + ", actual=" + actual};
    }
};

// ============================================================================
// Workspace state structure
// ============================================================================

/**
 * @brief In-memory representation of persistent workspace state.
 */
struct WorkspaceState {
    /// Schema version (for compatibility management)
    std::string version = "1.0.0";
    
    /// Timestamp when workspace was created
    std::string created_at;
    
    /// Last update timestamp
    std::string last_updated;
    
    /// SHA-256 checksum of the JSON (computed at save time)
    std::string checksum;
    
    /// Root directory of the workspace
    std::string workspace_root;
    
    /// Link graph: page -> [referenced pages]
    std::unordered_map<std::string, std::vector<std::string>> links;
    
    /// Tasks: task_id -> task details
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> tasks;
};

// ============================================================================
// Workspace state manager
// ============================================================================

/**
 * @brief Manages persistent workspace state with durability and corruption recovery.
 *
 * Responsibilities:
 *  - Load state.json with checksum validation
 *  - Save state atomically (write temp file, rename on success)
 *  - Detect and recover from corruption
 *  - Maintain append-only transaction log for recovery
 *
 * ## Optional RocksDB backend (Phase A)
 *
 * When a `RocksDBWrapper` instance is supplied via `useRocksDB()`, the
 * state JSON is persisted as a single RocksDB key–value pair under the key
 * `"workspace_state:<workspace_root>"`.  The WAL provides durability and
 * atomic replace semantics, making the temp-rename and manual checksum
 * unnecessary.  Recovery is handled transparently by RocksDB itself.  The
 * JSON fallback (state.json + state.log) remains active when no RocksDB
 * instance is attached.
 *
 * Thread-safety: Methods are NOT thread-safe by default. Caller must
 * serialize access via external mutex if needed.
 */
class WorkspaceStateManager {
public:
    explicit WorkspaceStateManager(const std::filesystem::path& workspace_root)
        : workspace_root_(workspace_root),
          state_file_(workspace_root / "wiki" / "state.json"),
          log_file_(workspace_root / "wiki" / "state.log") {}

    ~WorkspaceStateManager() = default;

    // Non-copyable
    WorkspaceStateManager(const WorkspaceStateManager&) = delete;
    WorkspaceStateManager& operator=(const WorkspaceStateManager&) = delete;

    /**
     * @brief Attach an optional RocksDB backend for durable state persistence.
     *
     * When set, `save()` stores the serialized state JSON as a RocksDB
     * key–value entry and `load()` reads it back, bypassing the JSON file +
     * checksum mechanism.  The RocksDB WAL provides durability and
     * crash-recovery without the temp-rename pattern.
     *
     * The key used in RocksDB is `"workspace_state:<workspace_root>"` so that
     * multiple workspaces can share a single RocksDB instance without
     * collisions.
     *
     * @param db  Non-owning pointer to an open `RocksDBWrapper` instance.
     *            Must remain valid for the lifetime of this manager.
     */
    void useRocksDB(themis::RocksDBWrapper* db) noexcept;

    /**
     * @brief Returns true when a RocksDB backend has been attached.
     * @return True if `useRocksDB()` was called with a non-null pointer.
     */
    [[nodiscard]] bool hasRocksDB() const noexcept;

    /**
     * @brief Load workspace state.
     *
     * When a RocksDB backend is attached, reads the state from RocksDB.
     * Otherwise:
     * 1. Read state.json
     * 2. Validate SHA-256 checksum embedded in the file
     * 3. On checksum failure, attempt recovery from state.log
     * 4. Return error if both fail
     *
     * @return OK if state loaded successfully, or error code otherwise.
     */
    [[nodiscard]] WorkspaceStatus load(WorkspaceState& out_state) noexcept;

    /**
     * @brief Save workspace state.
     *
     * When a RocksDB backend is attached, stores the JSON-serialized state as
     * a RocksDB `put` under the workspace-scoped key.  Otherwise performs an
     * atomic write:
     * 1. Serialize state to JSON
     * 2. Compute SHA-256 checksum
     * 3. Write to temporary file
     * 4. Rename temp file to state.json (atomic on POSIX)
     * 5. Append to state.log for durability
     *
     * @param state  State to persist.
     * @return       OK on success, error code otherwise.
     */
    [[nodiscard]] WorkspaceStatus save(const WorkspaceState& state) noexcept;

    /**
     * @brief Validate the checksum of a state file.
     *
     * Reads the SHA-256 field from the JSON and compares it to the
     * computed hash of the file contents (excluding the checksum field itself).
     *
     * @param file_path  Path to state.json.
     * @return           OK if checksum matches, ChecksumMismatch otherwise.
     */
    [[nodiscard]] static WorkspaceStatus validateChecksum(
        const std::filesystem::path& file_path) noexcept;

    /**
     * @brief Attempt recovery from the append-only transaction log.
     *
     * If state.json is corrupted, try to reconstruct the state from
     * state.log (which has one JSON object per line).
     *
     * @param out_state  Reconstructed state (best-effort).
     * @return           OK if recovery succeeds, error otherwise.
     */
    [[nodiscard]] WorkspaceStatus recoverFromLog(WorkspaceState& out_state) noexcept;

private:
    std::filesystem::path    workspace_root_;
    std::filesystem::path    state_file_;
    std::filesystem::path    log_file_;

    /// Non-owning pointer to an optional RocksDB backend (Phase A).
    /// Null by default → JSON file path is used.
    themis::RocksDBWrapper*  rocksdb_{nullptr};

    /// RocksDB key scoped to this workspace instance.
    /// Computed lazily on first use by rocksdbKey().
    mutable std::string      rocksdb_key_;

    /// Return the RocksDB lookup key for this workspace instance.
    [[nodiscard]] const std::string& rocksdbKey() const noexcept;

    /// Load state JSON from RocksDB (requires rocksdb_ != nullptr).
    [[nodiscard]] WorkspaceStatus loadFromRocksDB(WorkspaceState& out_state) noexcept;

    /// Save state JSON to RocksDB (requires rocksdb_ != nullptr).
    [[nodiscard]] WorkspaceStatus saveToRocksDB(const WorkspaceState& state) noexcept;
};

} // namespace llm_wiki
} // namespace themis
