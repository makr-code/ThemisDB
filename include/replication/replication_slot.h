/**
 * @file replication_slot.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Replication Slot Management API
 *
 * Implements the Phase 3 ROADMAP item (Issue #2249):
 *   "Replication slot management API (pause/resume individual slots)"
 *
 * A replication slot tracks the progress of a single downstream consumer
 * (follower or logical subscriber) in the WAL stream.  Pausing a slot
 * causes the WAL manager to retain segments beyond the slot's current LSN
 * so that the consumer can resume without data loss.  Resuming re-enables
 * shipping.
 *
 * Slot state is persisted to a JSON file beside the WAL directory so that
 * it survives process restarts.  The state file path is:
 *   <wal_directory>/slots/<slot_name>.json
 *
 * Design constraints:
 *   - pause() / resume() are atomic with respect to the WAL manager.
 *   - A paused slot does NOT block WAL writes on the leader.
 *   - Slot state (confirmed_lsn, status) is persisted synchronously on
 *     every advance() call.
 *   - No two slots may share the same name within a ReplicationSlotManager.
 *
 * Target: v1.x (Phase 3 – Short-term)
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "replication/replication_manager.h"

#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themisdb {
namespace replication {

/**
 * ReplicationSlot
 *
 * Represents a single named replication slot, tracking one downstream
 * consumer's confirmed WAL position.
 *
 * Thread-safety: All public methods are safe to call concurrently.
 */
class ReplicationSlot {
public:
    enum class SlotStatus {
        ACTIVE,     ///< Slot is active; WAL is being shipped normally
        PAUSED,     ///< Slot is paused; WAL segments are retained but not shipped
        DROPPED     ///< Slot has been dropped; WAL retention requirement is released
    };

    struct SlotState {
        std::string  name;
        SlotStatus   status            = SlotStatus::ACTIVE;
        uint64_t     confirmed_lsn     = 0;   ///< Last WAL LSN acked by the consumer
        uint64_t     restart_lsn       = 0;   ///< Oldest LSN the consumer may need on restart
        std::string  plugin_name;              ///< Output plugin (e.g., "physical", "json")
        std::string  downstream_node_id;       ///< Node ID of the downstream consumer
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point last_activity;
    };

    // -----------------------------------------------------------------------
    // Construction
    // -----------------------------------------------------------------------

    /**
     * Create a new slot.
     *
     * @param name               Unique slot identifier within this node.
     * @param plugin_name        Output plugin (e.g., "physical", "json").
     * @param downstream_node_id Identifier of the replica or consumer.
     * @param wal_manager        WAL manager used to query sequence ranges.
     * @param state_file_path    Path where slot state is persisted.
     */
    ReplicationSlot(
        const std::string&           name,
        const std::string&           plugin_name,
        const std::string&           downstream_node_id,
        std::shared_ptr<WALManager>  wal_manager,
        const std::string&           state_file_path
    );

    ~ReplicationSlot() = default;

    // Non-copyable, movable
    ReplicationSlot(const ReplicationSlot&)            = delete;
    ReplicationSlot& operator=(const ReplicationSlot&) = delete;
    ReplicationSlot(ReplicationSlot&&)                 noexcept = default;
    ReplicationSlot& operator=(ReplicationSlot&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Control API
    // -----------------------------------------------------------------------

    /**
     * Pause this slot.
     *
     * WAL segments beyond confirmed_lsn will be retained until the slot
     * is resumed or dropped.  Returns false if the slot is already paused
     * or has been dropped.
     */
    bool pause();

    /**
     * Resume a paused slot.
     * Returns false if the slot is ACTIVE or DROPPED.
     */
    bool resume();

    /**
     * Drop this slot permanently.
     * WAL retention for this slot's LSN range is released.
     * After calling drop() all subsequent control calls return false.
     */
    bool drop();

    // -----------------------------------------------------------------------
    // Progress tracking
    // -----------------------------------------------------------------------

    /**
     * Advance the confirmed LSN for this slot (called by the consumer on ack).
     * Persists the new LSN to the state file.
     * Returns false when the slot is paused or dropped.
     */
    bool advance(uint64_t confirmed_lsn);

    // -----------------------------------------------------------------------
    // Introspection
    // -----------------------------------------------------------------------

    const std::string&  name()   const;
    SlotStatus          status() const;
    SlotState           state()  const;

    /** Lag in number of WAL sequences between the leader and this slot. */
    uint64_t lag() const;

private:
    mutable std::mutex          state_mutex_;
    SlotState                   state_;
    std::shared_ptr<WALManager> wal_manager_;
    std::string                 state_file_path_;

    void persistState() const;
    void loadState();
};

// ============================================================================
// ReplicationSlotManager
// ============================================================================

/**
 * ReplicationSlotManager
 *
 * Central registry for all replication slots on a node.
 *
 * Thread-safety: All public methods are safe to call concurrently.
 */
class ReplicationSlotManager {
public:
    struct ManagerConfig {
        std::string wal_directory;  ///< Base WAL directory; slot states stored under <wal_directory>/slots/
    };

    explicit ReplicationSlotManager(
        const ManagerConfig&         config,
        std::shared_ptr<WALManager>  wal_manager
    );

    // -----------------------------------------------------------------------
    // Slot lifecycle
    // -----------------------------------------------------------------------

    /**
     * Create and register a new slot.
     * Returns nullptr if a slot with this name already exists.
     */
    std::shared_ptr<ReplicationSlot> createSlot(
        const std::string& name,
        const std::string& plugin_name       = "physical",
        const std::string& downstream_node_id = ""
    );

    /**
     * Look up an existing slot by name.
     * Returns nullptr if not found.
     */
    std::shared_ptr<ReplicationSlot> getSlot(const std::string& name) const;

    /**
     * Drop and remove a slot by name.
     * Returns false if the slot does not exist.
     */
    bool dropSlot(const std::string& name);

    // -----------------------------------------------------------------------
    // Introspection
    // -----------------------------------------------------------------------

    /** List states of all registered slots (snapshot). */
    std::vector<ReplicationSlot::SlotState> listSlots() const;

    /** Number of currently registered (non-dropped) slots. */
    size_t slotCount() const;

    /**
     * Minimum confirmed LSN across all active slots.
     * WAL segments before this LSN may be safely purged.
     * Returns 0 if there are no active slots.
     */
    uint64_t minConfirmedLsn() const;

    /**
     * Reload slot states from disk (call on startup to restore persisted slots).
     */
    void loadPersistedSlots();

private:
    ManagerConfig                                        config_;
    std::shared_ptr<WALManager>                          wal_manager_;

    mutable std::mutex                                   slots_mutex_;
    std::map<std::string, std::shared_ptr<ReplicationSlot>> slots_;
};

} // namespace replication
} // namespace themisdb

