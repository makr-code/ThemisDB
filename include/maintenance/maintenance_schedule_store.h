/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            maintenance_schedule_store.h                       ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-04-13 04:16:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     130                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 5d2cef871d  2026-03-12  fix: address PR review comments for schedule persistence ║
    • 9f068f7075  2026-03-12  feat: implement Schedule Persistence (RocksDB) for mainte... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file maintenance_schedule_store.h
 * @brief MaintenanceScheduleStore — durable RocksDB-backed persistence for
 *        MaintenanceScheduleEntry objects.
 *
 * Wraps the existing IStorageEngine API and maps each schedule to a single
 * key-value pair:
 *
 *   Key:   "maint_sched::<id>"   (UTF-8)
 *   Value: JSON serialisation of MaintenanceScheduleEntry  (UTF-8)
 *
 * The store is write-through: every mutation (save / remove) is committed
 * synchronously before the caller's mutex is released.
 *
 * Corrupt or unparseable JSON values encountered during loadAll() are
 * skipped with a WARN-level log entry; all remaining valid entries are
 * loaded normally.
 */

#pragma once

#include "maintenance/maintenance_schedule.h"
#include "themis/base/interfaces/storage_interface.h"

#include <map>
#include <string>

namespace themis {
namespace maintenance {

/**
 * @brief Durable store for MaintenanceScheduleEntry objects backed by
 *        an IStorageEngine (typically RocksDB via StorageEngine).
 *
 * ### Thread safety
 * The store itself is NOT internally synchronised.  Callers must hold the
 * appropriate external lock (schedules_mutex_ in
 * DatabaseMaintenanceOrchestrator) when coordinating save/remove with shared
 * in-memory schedule state.
 *
 * loadAll() may be called without holding the orchestrator mutex provided
 * that entries are first collected into a caller-owned temporary container
 * and only merged into shared state while that external lock is held.
 */
class MaintenanceScheduleStore {
public:
    /// RocksDB key prefix used for all schedule entries.
    static constexpr std::string_view kKeyPrefix = "maint_sched::";

    /**
     * @brief Construct a store backed by @p engine.
     *
     * @p engine must outlive this object.  The pointer must be non-null.
     */
    explicit MaintenanceScheduleStore(IStorageEngine* engine);

    ~MaintenanceScheduleStore() = default;

    // Non-copyable, non-movable.
    MaintenanceScheduleStore(const MaintenanceScheduleStore&) = delete;
    MaintenanceScheduleStore& operator=(const MaintenanceScheduleStore&) = delete;

    /**
     * @brief Persist (or overwrite) a single schedule entry.
     *
     * Serialises @p entry to JSON and calls IStorageEngine::put() with
     * key "maint_sched::<entry.id>".
     *
     * @return Result<void> – ok on success, storage error on failure.
     */
    Result<void> save(const MaintenanceScheduleEntry& entry);

    /**
     * @brief Delete the persisted entry for @p id.
     *
     * Calls IStorageEngine::del() for key "maint_sched::<id>".
     * Returns ok even when the key does not exist (idempotent).
     *
     * @return Result<void> – ok on success, storage error on failure.
     */
    Result<void> remove(const std::string& id);

    /**
     * @brief Load all previously persisted schedules into @p schedules.
     *
     * Scans the "maint_sched::" prefix using IStorageEngine::scanPrefix().
     * For each entry:
     *   • Valid JSON → deserialised and inserted into @p schedules.
     *   • Corrupt / unparseable JSON → WARN log, entry skipped.
     *
     * Existing contents of @p schedules are not cleared; loaded entries
     * are merged (overwrite on id collision).
     *
     * @param schedules  Output map keyed by schedule id.
     * @return Result<void> – ok even if some entries were skipped; a hard
     *         storage error is returned only when the scan itself fails.
     */
    Result<void> loadAll(std::map<std::string, MaintenanceScheduleEntry>& schedules);

private:
    static std::string makeKey(const std::string& id);

    IStorageEngine* engine_;
};

} // namespace maintenance
} // namespace themis
