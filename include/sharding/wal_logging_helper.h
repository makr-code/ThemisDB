/**
 * @file wal_logging_helper.h
 * @brief Write-ahead log helpers for the sharding subsystem.
 *
 * Utility functions and RAII guards for writing and fsync-ing WAL
 * entries used by the shard coordinator during distributed commits.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// WAL Logging Helper — shared utility for sharding 2PC coordinators/participants.
//
// The sharding-side transaction coordinators/participants perform the same
// WAL-write pattern:
//   1. Build a WALEntry with the current wall-clock timestamp.
//   2. Append it to a WALManager and optionally return the assigned LSN.
//   3. Optionally flush (sync_wal_writes).
//   4. Swallow exceptions and emit an error log.
//
// This header centralises that pattern so neither class needs to duplicate it.

#pragma once

#include "sharding/wal_manager.h"
#include "utils/logger.h"
#include <chrono>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/**
 * @brief Utility namespace for shared WAL-logging operations used by sharding
 *        2PC coordinators and participants.
 *
 * All functions are stateless and operate on an externally-owned WALManager
 * pointer (may be nullptr, in which case calls are no-ops).
 */
namespace WALLoggingHelper {

/**
 * @brief Build a WALEntry populated with the current wall-clock timestamp.
 *
 * @param type    Logical operation type to persist.
 * @param txn_id  Transaction identifier to embed in the entry.
 * @param data    Additional JSON payload (operation details, phase tags, …).
 * @return        A fully initialised WALEntry ready to be appended.
 */
[[nodiscard]] inline WALEntry buildEntry(
    WALEntryType          type,
    const std::string&    txn_id,
    const nlohmann::json& data)
{
    WALEntry entry;
    entry.type           = type;
    entry.transaction_id = txn_id;
    entry.timestamp      = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    entry.data = data;
    return entry;
}

/**
 * @brief Append a WAL entry, optionally flush, and return the assigned LSN.
 *
 * If @p wal is nullptr the function is a no-op.  Any exception thrown by
 * WALManager::append() or WALManager::flush() is caught and logged as an
 * error using @p component_label and @p component_id so callers see a
 * consistent diagnostic message without crashing. The returned LSN can be used
 * by callers that need to emit additional success diagnostics; std::nullopt
 * indicates that nothing was persisted because the WAL was disabled or a write
 * failed.
 *
 * @param wal             Target WAL manager (may be nullptr).
 * @param type            Logical operation type.
 * @param txn_id          Transaction identifier.
 * @param data            Additional JSON payload.
 * @param sync            When true, call wal->flush() after append.
 * @param component_label Human-readable role label, e.g. "coordinator" or
 *                        "participant". Used in the error log message.
 * @param component_id    Instance identifier, e.g. coordinator_id_ or
 *                        shard_id_. Used in the error log message.
 * @return                Assigned LSN on success; std::nullopt on noop/failure.
 */
[[nodiscard]] inline std::optional<LSN> appendEntryWithResult(
    WALManager*           wal,
    WALEntryType          type,
    const std::string&    txn_id,
    const nlohmann::json& data,
    bool                  sync,
    std::string_view      component_label,
    std::string_view      component_id)
{
    if (!wal) {
      return std::nullopt;
    }

    try {
        WALEntry entry = buildEntry(type, txn_id, data);
        const LSN lsn = wal->append(entry);
        if (sync) {
            wal->flush();
        }
        return lsn;
    } catch (const std::exception& e) {
        THEMIS_ERROR("2PC {} [{}] WAL write failed for txn {}: {}",
                     component_label, component_id, txn_id, e.what());
    }

    return std::nullopt;
}

/**
 * @brief Append a WAL entry and optionally flush, swallowing exceptions.
 *
 * This convenience wrapper is intended for callers that only care about
 * best-effort durability and not about the assigned LSN.
 *
 * @param wal             Target WAL manager (may be nullptr).
 * @param type            Logical operation type.
 * @param txn_id          Transaction identifier.
 * @param data            Additional JSON payload.
 * @param sync            When true, call wal->flush() after append.
 * @param component_label Human-readable role label, e.g. "coordinator" or
 *                        "participant". Used in the error log message.
 * @param component_id    Instance identifier, e.g. coordinator_id_ or
 *                        shard_id_. Used in the error log message.
 */
inline void appendEntry(
    WALManager*           wal,
    WALEntryType          type,
    const std::string&    txn_id,
    const nlohmann::json& data,
    bool                  sync,
    std::string_view      component_label,
    std::string_view      component_id)
{
    (void)appendEntryWithResult(
        wal, type, txn_id, data, sync, component_label, component_id
    );
}

} // namespace WALLoggingHelper
} // namespace themis::sharding
