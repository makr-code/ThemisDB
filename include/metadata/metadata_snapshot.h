/**
 * @file metadata_snapshot.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <chrono>
#include <map>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "schema_manager.h"

namespace themis {
namespace metadata {

using json = nlohmann::json;

// ── MetadataSnapshotException ─────────────────────────────────────────────────

/**
 * @brief Exception thrown when a snapshot operation fails.
 *
 * Carries the affected snapshot ID and a human-readable reason so callers can
 * distinguish "not found" from "corrupt data" or "store full".
 */
class MetadataSnapshotException : public std::runtime_error {
public:
    MetadataSnapshotException(std::string_view snapshot_id,
                              std::string_view reason)
        : std::runtime_error(
              "MetadataSnapshotException: snapshot_id='" +
              std::string(snapshot_id) +
              "' reason='" + std::string(reason) + "'")
        , snapshot_id_(snapshot_id)
        , reason_(reason) {}

    /// The snapshot ID involved in the failed operation.
    const std::string& snapshotId() const noexcept { return snapshot_id_; }

    /// Human-readable description of the failure.
    const std::string& reason()     const noexcept { return reason_; }

private:
    std::string snapshot_id_;
    std::string reason_;
};

// ── MetadataSnapshot ──────────────────────────────────────────────────────────

/**
 * @brief Immutable point-in-time capture of all table schemas.
 *
 * @note  snapshot_id must be non-empty when passed to a store's save()
 *        method.  Callers are responsible for generating meaningful IDs
 *        (e.g. UUID v4, semantic version, ISO-8601 timestamp string).
 */
struct MetadataSnapshot {
    /// Unique identifier for this snapshot (e.g. "v1.2.0" or a UUID).
    std::string snapshot_id;

    /// Wall-clock time at which this snapshot was taken.
    std::chrono::system_clock::time_point created_at;

    /// Complete list of table schemas captured in this snapshot.
    std::vector<SchemaManager::TableSchema> tables;

    /// Identity of the person or process that created this snapshot.
    std::string author;

    /// Human-readable description of what changed or why this snapshot exists.
    std::string description;

    // ── Accessors ─────────────────────────────────────────────────────────────

    /**
     * @brief Find a table schema by name.
     *
     * @param name  Table name to look up (case-sensitive).
     * @return      Pointer to the matching TableSchema, or nullptr if not found.
     */
    const SchemaManager::TableSchema* findTable(std::string_view name) const {
        for (const auto& t : tables) {
            if (t.name == name) {
              return &t;
            }
        }
        return nullptr;
    }

    /** @return Number of tables captured in this snapshot. */
    size_t tableCount() const noexcept { return tables.size(); }

    /**
     * @brief Serialise the snapshot to JSON.
     *
     * Schema:
     * @code
     * {
     *   "snapshot_id": "<id>",
     *   "created_at":  "<ISO-8601>",
     *   "author":      "<string>",
     *   "description": "<string>",
     *   "table_count": <n>,
     *   "tables":      [ <TableSchema JSON>, ... ]
     * }
     * @endcode
     */
    json toJSON() const {
        // Convert time_point to ISO-8601 string via time_t.
        std::time_t tt = std::chrono::system_clock::to_time_t(created_at);
        char time_buf[32] = {};
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%dT%H:%M:%SZ",
                      std::gmtime(&tt));

        json j;
        j["snapshot_id"]  = snapshot_id;
        j["created_at"]   = std::string(time_buf);
        j["author"]       = author;
        j["description"]  = description;
        j["table_count"]  = tables.size();
        j["tables"]       = json::array();
        for (const auto& t : tables) {
            j["tables"].push_back(t.toJSON());
        }
        return j;
    }
};

// ── IMetadataSnapshotStore ────────────────────────────────────────────────────

/**
 * @brief Abstract persistence interface for MetadataSnapshot objects.
 *
 * Implementations MUST be thread-safe.
 */
class IMetadataSnapshotStore {
public:
    virtual ~IMetadataSnapshotStore() = default;

    /**
     * @brief Persist @p snapshot and return its ID.
     *
     * @param snapshot  The snapshot to store.  snapshot.snapshot_id must be
     *                  non-empty.
     * @return          The snapshot ID (same as snapshot.snapshot_id).
     * @throws MetadataSnapshotException if snapshot_id is empty or on I/O
     *         error.
     */
    [[nodiscard]] virtual std::string save(const MetadataSnapshot& snapshot) = 0;

    /**
     * @brief Load the snapshot identified by @p id.
     *
     * @param id  Snapshot ID.
     * @return    The loaded snapshot, or std::nullopt if @p id is not found.
     */
    [[nodiscard]] virtual std::optional<MetadataSnapshot> load(std::string_view id) = 0;

    /**
     * @brief Return a sorted list of all stored snapshot IDs.
     */
    [[nodiscard]] virtual std::vector<std::string> listSnapshotIds() = 0;

    /**
     * @brief Remove the snapshot identified by @p id.
     *
     * @return true if the snapshot existed and was removed; false if @p id
     *         was not found.
     */
    [[nodiscard]] virtual bool remove(std::string_view id) = 0;

    /**
     * @brief Return the number of snapshots currently held in the store.
     */
    [[nodiscard]] virtual size_t size() = 0;
};

// ── InMemoryMetadataSnapshotStore ─────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of IMetadataSnapshotStore.
 *
 * All snapshots are held in a std::map keyed by snapshot_id.  Data does not
 * survive process exit; this implementation is intended for unit tests,
 * integration tests, and embedded scenarios that do not require durability.
 *
 * Thread-safety guarantee: all public methods acquire the internal mutex
 * before accessing the snapshot map.
 *
 * Example:
 * @code
 *   InMemoryMetadataSnapshotStore store;
 *
 *   MetadataSnapshot snap;
 *   snap.snapshot_id = "v1.0.0";
 *   snap.created_at  = std::chrono::system_clock::now();
 *   snap.author      = "ci-pipeline";
 *   snap.tables      = schema_manager.getAllTables();
 *   store.save(snap);
 *
 *   auto loaded = store.load("v1.0.0");
 *   assert(loaded.has_value());
 * @endcode
 */
class InMemoryMetadataSnapshotStore : public IMetadataSnapshotStore {
public:
    // ── IMetadataSnapshotStore ────────────────────────────────────────────────

    /**
     * @brief Store @p snapshot.
     *
     * If a snapshot with the same ID already exists it is overwritten.
     *
     * @throws MetadataSnapshotException if snapshot.snapshot_id is empty.
     */
    std::string save(const MetadataSnapshot& snapshot) override {
        if (snapshot.snapshot_id.empty()) {
            throw MetadataSnapshotException(
                "", "snapshot_id must not be empty");
        }
        std::unique_lock<std::mutex> lk(mutex_);
        snapshots_[snapshot.snapshot_id] = snapshot;
        return snapshot.snapshot_id;
    }

    /**
     * @brief Load snapshot by ID.
     *
     * @return The snapshot, or std::nullopt if not found.
     */
    std::optional<MetadataSnapshot> load(std::string_view id) override {
        std::unique_lock<std::mutex> lk(mutex_);
        auto it = snapshots_.find(std::string(id));
        if (it == snapshots_.end()) {
          return std::nullopt;
        }
        return it->second;
    }

    /**
     * @brief Return all snapshot IDs in ascending lexicographic order.
     */
    std::vector<std::string> listSnapshotIds() override {
        std::unique_lock<std::mutex> lk(mutex_);
        std::vector<std::string> ids = {};

        ids.reserve(snapshots_.size());
        for (const auto& kv : snapshots_) {
            ids.push_back(kv.first);
        }
        // std::map iterates in sorted key order; the result is already sorted.
        return ids;
    }

    /**
     * @brief Remove the snapshot with the given ID.
     *
     * @return true if the snapshot was found and removed; false otherwise.
     */
    bool remove(std::string_view id) override {
        std::unique_lock<std::mutex> lk(mutex_);
        return snapshots_.erase(std::string(id)) > 0;
    }

    /** @return Number of snapshots currently held. */
    size_t size() override {
        std::unique_lock<std::mutex> lk(mutex_);
        return snapshots_.size();
    }

    // ── Extra helpers ─────────────────────────────────────────────────────────

    /**
     * @brief Remove all snapshots from the store.
     *
     * Useful for resetting state between unit-test cases.
     */
    void clear() {
        std::unique_lock<std::mutex> lk(mutex_);
        snapshots_.clear();
    }

private:
    mutable std::mutex mutex_;
    std::map<std::string, MetadataSnapshot> snapshots_;
};

} // namespace metadata
} // namespace themis

