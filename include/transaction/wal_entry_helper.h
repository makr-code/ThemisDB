// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// WALEntryHelper — shared utility for constructing and appending WAL entries
// across all 2PC coordinator implementations.
//
// All three ThemisDB 2PC coordinators (TwoPhaseCommitCoordinator,
// GlobalTransactionManager, DistributedTransactionManager) use an identical
// WAL-entry construction pattern:
//
//   1. Build a WALEntry (type, transaction_id, monotonic timestamp, JSON payload)
//   2. Call WALManager::append()
//   3. Optionally call WALManager::flush() for durability
//   4. Handle failures (rethrow or log-and-suppress, depending on call site)
//
// This helper centralises that pattern so individual coordinators contain no
// hand-rolled entry-construction or timestamp code.
//
// Exception contract (see also Issue #5376 — Callback/DI fail-fast contracts):
//   - WALEntryHelper::append()     — rethrows; caller decides how to handle.
//   - WALEntryHelper::appendOrLog() — catches, logs via THEMIS_ERROR, returns
//                                      false on failure; suitable for "complete"
//                                      entries where the decision is already
//                                      durable and failure is non-fatal.

#pragma once

#include "sharding/wal_manager.h"
#include "utils/logger.h"

#include <chrono>
#include <string>
#include <string_view>
#include <nlohmann/json.hpp>

namespace themis::transaction {

// ─────────────────────────────────────────────────────────────────────────────
// WALEntryHelper
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Shared utility for constructing and appending WAL entries.
 *
 * Centralises the identical entry-building and flushing logic that previously
 * existed in TwoPhaseCommitCoordinator::logToWAL(),
 * GlobalTransactionManager::logToWAL(), and
 * DistributedTransactionManager::logToWAL().
 *
 * All methods are static; WALEntryHelper holds no state.
 */
class WALEntryHelper {
public:
    // ── Constructing entries ─────────────────────────────────────────────

    /**
     * @brief Build a WAL entry from its components.
     *
     * Sets @c type, @c transaction_id, @c data, and @c timestamp to the
     * current wall-clock time in milliseconds since the UNIX epoch.
     *
     * @param type    Entry type (BEGIN_TX, PREPARE_TX, COMMIT_TX, ABORT_TX, …).
     * @param txn_id  Transaction identifier.
     * @param data    Arbitrary JSON payload (coordinator-specific schema).
     * @return        Fully populated WALEntry ready for appending.
     */
    [[nodiscard]] static themis::sharding::WALEntry buildEntry(
        themis::sharding::WALEntryType type,
        const std::string&             txn_id,
        const nlohmann::json&          data
    ) {
        themis::sharding::WALEntry entry;
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

    // ── Appending entries (throwing) ─────────────────────────────────────

    /**
     * @brief Append a WAL entry and optionally flush to durable storage.
     *
     * Constructs the entry via buildEntry(), calls WALManager::append(),
     * and — when @p sync_on_write is true — calls WALManager::flush().
     *
     * @par Exception contract
     * Any exception from WALManager::append() or WALManager::flush() is
     * **propagated to the caller unchanged**.  Callers that require silent
     * failures should use appendOrLog() instead.
     *
     * @param wal           WAL manager to write into.
     * @param type          WAL entry type.
     * @param txn_id        Transaction identifier.
     * @param data          JSON payload.
     * @param sync_on_write When true, flush is called after append.
     *
     * @throws std::exception (or subclass) on any WAL failure.
     */
    static void append(
        themis::sharding::WALManager&  wal,
        themis::sharding::WALEntryType type,
        const std::string&             txn_id,
        const nlohmann::json&          data,
        bool                           sync_on_write
    ) {
        auto entry = buildEntry(type, txn_id, data);
        wal.append(entry);
        if (sync_on_write) {
            wal.flush();
        }
    }

    // ── Appending entries (non-throwing) ─────────────────────────────────

    /**
     * @brief Append a WAL entry, suppressing exceptions and logging failures.
     *
     * Suitable for "completion" entries where the durable decision has already
     * been persisted and a WAL failure would not violate ACID guarantees.
     *
     * @par Fail-soft use cases
     * - Phase-2 "complete" markers written after participants have been notified.
     * - Non-critical diagnostic entries.
     *
     * @par Exception contract
     * All exceptions are caught.  A THEMIS_ERROR log line is emitted and the
     * function returns @c false.  The caller may retry, log, or continue.
     *
     * @param wal           Nullable WAL manager pointer; returns false when null.
     * @param type          WAL entry type.
     * @param txn_id        Transaction identifier.
     * @param data          JSON payload.
     * @param sync_on_write When true, flush is called after append (best-effort).
     * @param source        Human-readable component name used in the error log line
     *                      (e.g. "2PC coordinator [coord-1]").
     * @return              @c true on success; @c false if @p wal is null or an
     *                      exception was caught.
     */
    static bool appendOrLog(
        themis::sharding::WALManager*  wal,
        themis::sharding::WALEntryType type,
        const std::string&             txn_id,
        const nlohmann::json&          data,
        bool                           sync_on_write,
        std::string_view               source
    ) noexcept {
        if (!wal) {
            return false;
        }
        try {
            append(*wal, type, txn_id, data, sync_on_write);
            return true;
        } catch (const std::exception& e) {
            THEMIS_ERROR("{} WAL write failed for txn {}: {}", source, txn_id, e.what());
            return false;
        } catch (...) {
            THEMIS_ERROR("{} WAL write failed for txn {} (unknown exception)", source, txn_id);
            return false;
        }
    }
};

} // namespace themis::transaction
