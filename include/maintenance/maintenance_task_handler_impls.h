/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            maintenance_task_handler_impls.h                   ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 11:25:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     258                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • af1b624522  2026-03-12  fix: address review feedback - null safety, HTTP route fo... ║
    • 717093f9bc  2026-03-12  feat: implement IMaintenanceTaskHandler registry for main... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file maintenance_task_handler_impls.h
 * @brief Concrete IMaintenanceTaskHandler implementations for the built-in
 *        storage and sharding modules.
 *
 * These classes wire module-specific maintenance logic into the
 * DatabaseMaintenanceOrchestrator via its registerTaskHandler() API.
 *
 * ### Typical wiring at startup
 *
 * @code
 * // Storage module registers compaction handler:
 * orchestrator.registerTaskHandler(
 *     MaintenanceTaskType::STORAGE_COMPACTION,
 *     std::make_shared<StorageCompactionHandler>(compaction_manager));
 *
 * // Sharding module registers replica validation handler:
 * orchestrator.registerTaskHandler(
 *     MaintenanceTaskType::REPLICA_VALIDATION,
 *     std::make_shared<ReplicaValidationHandler>(consistency_check_fn));
 *
 * // Storage engine registers MVCC cleanup handler:
 * orchestrator.registerTaskHandler(
 *     MaintenanceTaskType::MVCC_CLEANUP,
 *     std::make_shared<MvccCleanupHandler>(mvcc_store, watermark_ms));
 * @endcode
 */

#pragma once

#include "maintenance/i_maintenance_task_handler.h"
#include "storage/compaction_manager.h"
#include "storage/mvcc_store.h"
#include "storage/hlc.h"

#include <functional>
#include <memory>
#include <string>
#include <chrono>

namespace themis {
namespace maintenance {

// ---------------------------------------------------------------------------
// StorageCompactionHandler
// ---------------------------------------------------------------------------

/**
 * @brief Handles STORAGE_COMPACTION by triggering a full RocksDB compaction.
 *
 * Registered by the storage module at startup:
 * @code
 *   orchestrator.registerTaskHandler(
 *       MaintenanceTaskType::STORAGE_COMPACTION,
 *       std::make_shared<StorageCompactionHandler>(compaction_manager));
 * @endcode
 */
class StorageCompactionHandler : public IMaintenanceTaskHandler {
public:
    explicit StorageCompactionHandler(
        std::shared_ptr<CompactionManager> compaction_manager)
        : compaction_manager_(std::move(compaction_manager)) {}

    Result<std::string> execute(const std::string& /*job_id*/,
                                MaintenanceTaskType /*task_type*/) override {
        if (!compaction_manager_) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                        "StorageCompactionHandler: CompactionManager is null"));
        }
        auto result = compaction_manager_->compactAll();
        if (!result) {
            return tl::unexpected(result.error());
        }
        return std::string("Storage compaction completed successfully");
    }

    std::string handlerName() const override {
        return "StorageCompactionHandler";
    }

private:
    std::shared_ptr<CompactionManager> compaction_manager_;
};

// ---------------------------------------------------------------------------
// ReplicaValidationHandler
// ---------------------------------------------------------------------------

/**
 * @brief Handles REPLICA_VALIDATION by invoking the sharding consistency checker.
 *
 * Accepts a callable that performs the actual consistency check, allowing the
 * sharding module to wire in its own checker without introducing a compile-time
 * dependency on sharding types from the maintenance module.
 *
 * Registered by the sharding module at startup:
 * @code
 *   auto checker = [replica_mgr]() -> Result<std::string> {
 *       auto issues = replica_mgr->runConsistencyCheck();
 *       return "Replica validation: " + std::to_string(issues) + " issue(s) found";
 *   };
 *   orchestrator.registerTaskHandler(
 *       MaintenanceTaskType::REPLICA_VALIDATION,
 *       std::make_shared<ReplicaValidationHandler>(checker));
 * @endcode
 */
class ReplicaValidationHandler : public IMaintenanceTaskHandler {
public:
    using CheckFn = std::function<Result<std::string>()>;

    explicit ReplicaValidationHandler(CheckFn check_fn)
        : check_fn_(std::move(check_fn)) {}

    Result<std::string> execute(const std::string& /*job_id*/,
                                MaintenanceTaskType /*task_type*/) override {
        if (!check_fn_) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                        "ReplicaValidationHandler: no check function configured"));
        }
        return check_fn_();
    }

    std::string handlerName() const override {
        return "ReplicaValidationHandler";
    }

private:
    CheckFn check_fn_;
};

// ---------------------------------------------------------------------------
// MvccCleanupHandler
// ---------------------------------------------------------------------------

/**
 * @brief Handles MVCC_CLEANUP by GC-ing all MVCC tombstones older than a
 *        configurable watermark.
 *
 * Registered by the StorageEngine at startup:
 * @code
 *   // Retain versions from the last 24 hours.
 *   orchestrator.registerTaskHandler(
 *       MaintenanceTaskType::MVCC_CLEANUP,
 *       std::make_shared<MvccCleanupHandler>(mvcc_store, 86400000));
 * @endcode
 *
 * @param watermark_ms  Versions older than now() - watermark_ms are eligible
 *                      for garbage collection.  Defaults to 24 hours.
 */
class MvccCleanupHandler : public IMaintenanceTaskHandler {
public:
    static constexpr int64_t kDefaultWatermarkMs = 24LL * 60 * 60 * 1000; // 24 h

    explicit MvccCleanupHandler(
        std::shared_ptr<MVCCStore> mvcc_store,
        int64_t watermark_ms = kDefaultWatermarkMs)
        : mvcc_store_(std::move(mvcc_store))
        , watermark_ms_(watermark_ms) {}

    Result<std::string> execute(const std::string& /*job_id*/,
                                MaintenanceTaskType /*task_type*/) override {
        if (!mvcc_store_) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
                                        "MvccCleanupHandler: MVCCStore is null"));
        }
        if (watermark_ms_ < 0) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                        "MvccCleanupHandler: watermark_ms must be non-negative"));
        }
        // Compute the GC cutoff timestamp: everything older than now - watermark_ms_.
        auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        auto cutoff_ms = static_cast<uint64_t>(
            now_ms > watermark_ms_ ? now_ms - watermark_ms_ : 0);

        HLCTimestamp cutoff = HLCTimestamp::from(cutoff_ms, 0);
        uint64_t versions_removed = mvcc_store_->gcAllBefore(cutoff);
        return "MVCC cleanup: removed " + std::to_string(versions_removed) +
               " stale version(s)";
    }

    std::string handlerName() const override {
        return "MvccCleanupHandler";
    }

private:
    std::shared_ptr<MVCCStore> mvcc_store_;
    int64_t watermark_ms_;
};

// ---------------------------------------------------------------------------
// FunctionMaintenanceTaskHandler  (convenience wrapper)
// ---------------------------------------------------------------------------

/**
 * @brief Wraps a std::function as an IMaintenanceTaskHandler.
 *
 * Useful for ad-hoc or test handler registrations:
 * @code
 *   orchestrator.registerTaskHandler(
 *       MaintenanceTaskType::QUOTA_CHECK,
 *       std::make_shared<FunctionMaintenanceTaskHandler>(
 *           "QuotaCheckHandler",
 *           [](const std::string& job_id, MaintenanceTaskType) {
 *               return Result<std::string>{"Quota check passed"};
 *           }));
 * @endcode
 */
class FunctionMaintenanceTaskHandler : public IMaintenanceTaskHandler {
public:
    using ExecuteFn = std::function<Result<std::string>(
        const std::string&, MaintenanceTaskType)>;

    FunctionMaintenanceTaskHandler(std::string name, ExecuteFn fn)
        : name_(std::move(name)), fn_(std::move(fn)) {}

    Result<std::string> execute(const std::string& job_id,
                                MaintenanceTaskType task_type) override {
        if (!fn_) {
            return tl::unexpected(Error(errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
                                        "FunctionMaintenanceTaskHandler '" + name_ +
                                        "' called with empty execute function"));
        }
        return fn_(job_id, task_type);
    }

    std::string handlerName() const override { return name_; }

private:
    std::string name_;
    ExecuteFn   fn_;
};

} // namespace maintenance
} // namespace themis
