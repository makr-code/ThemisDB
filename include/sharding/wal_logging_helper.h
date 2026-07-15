// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// WAL Logging Helper — shared utility for 2PC coordinator and participant.
//
// Both TwoPhaseCommitCoordinator and TwoPhaseCommitParticipant perform the
// same WAL-write pattern:
//   1. Build a WALEntry with the current wall-clock timestamp.
//   2. Append it to a WALManager.
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
 * @brief Utility namespace for shared WAL-logging operations used by 2PC
 *        coordinator and participant.
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
 * @brief Append a WAL entry and optionally flush, swallowing exceptions.
 *
 * If @p wal is nullptr the function is a no-op.  Any exception thrown by
 * WALManager::append() or WALManager::flush() is caught and logged as an
 * error using @p component_label and @p component_id so callers see a
 * consistent diagnostic message without crashing.
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
    if (!wal) return;

    try {
        WALEntry entry = buildEntry(type, txn_id, data);
        wal->append(entry);
        if (sync) {
            wal->flush();
        }
    } catch (const std::exception& e) {
        THEMIS_ERROR("2PC {} [{}] WAL write failed for txn {}: {}",
                     component_label, component_id, txn_id, e.what());
    }
}

} // namespace WALLoggingHelper
} // namespace themis::sharding
