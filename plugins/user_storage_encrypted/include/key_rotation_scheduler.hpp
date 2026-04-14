/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            key_rotation_scheduler.hpp                         ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:58:44                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     167                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 256e7651d1  2026-03-24  Changes before error encountered        ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security_level.hpp"
#include "encryption_backend_interface.hpp"
#include <functional>
#include <memory>
#include <string>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief Minimal key-value persistence interface for rotation state.
 *
 * Allows any backend (RocksDB, in-memory, file-based) to persist rotation
 * state without coupling the scheduler to a specific storage implementation.
 * Use makeRocksDBRotationStore() to create a RocksDB-backed instance.
 */
class IRotationStore {
public:
    virtual ~IRotationStore() = default;

    /**
     * @brief Read a value by key.
     * @param key  Storage key
     * @param out  Value (set only when true is returned)
     * @return     true if the key existed
     */
    virtual bool get(const std::string& key, std::string& out) const = 0;

    /**
     * @brief Write a key-value pair.
     * @return true on success
     */
    virtual bool put(const std::string& key, const std::string& value) = 0;
};

/**
 * @brief Key rotation scheduler for automatic key rotation
 * 
 * Features:
 * - Schedule-based rotation (e.g., every 90 days)
 * - Zero-downtime rotation process
 * - Notification on rotation completion
 * - Thread-safe operation
 * - Optional persistence via IRotationStore (Feature 3)
 * 
 * Rotation Process:
 * 1. Create new container with new key
 * 2. Copy data from old to new container
 * 3. Atomically switch containers (rename)
 * 4. Keep old container as backup
 * 5. Notify administrators
 */
class KeyRotationScheduler {
public:
    /**
     * @brief Callback for key rotation events
     * 
     * Parameters: level, success, error_message
     */
    using RotationCallback = std::function<void(SecurityLevel, bool, const std::string&)>;
    
    KeyRotationScheduler();
    ~KeyRotationScheduler();
    
    /**
     * @brief Initialize scheduler, optionally loading persisted rotation state.
     *
     * When a non-null @p store is supplied the scheduler loads any previously
     * persisted last_check_ms values for each SecurityLevel so that rotation
     * intervals survive process restarts.  After each successful callback
     * invocation the updated state is written back to the store.
     *
     * RocksDB integration:
     * @code
     *   auto store = makeRocksDBRotationStore(&rocksdb_wrapper);
     *   scheduler.initialize(3600, std::move(store));
     * @endcode
     *
     * @param check_interval_seconds  How often to check for rotation needs
     * @param store                   Optional persistence backend (may be nullptr)
     */
    Result<void> initialize(
        int check_interval_seconds = 3600,
        std::shared_ptr<IRotationStore> store = nullptr
    );
    
    /**
     * @brief Shutdown scheduler
     */
    void shutdown();
    
    /**
     * @brief Schedule rotation for a security level
     * 
     * @param level Security level to rotate
     * @param interval_days Rotation interval in days
     * @param auto_rotate Enable automatic rotation
     * @param callback Callback function for rotation events
     */
    Result<void> scheduleRotation(
        SecurityLevel level,
        int interval_days,
        bool auto_rotate,
        RotationCallback callback
    );
    
    /**
     * @brief Cancel scheduled rotation for a level
     */
    void cancelRotation(SecurityLevel level);
    
    /**
     * @brief Check if rotation is due for a level
     * 
     * @param level Security level
     * @param last_rotation_ms Last rotation timestamp
     * @return true if rotation is due
     */
    bool isRotationDue(SecurityLevel level, int64_t last_rotation_ms);
    
    /**
     * @brief Get next rotation time for a level
     * 
     * @return Timestamp in milliseconds, 0 if not scheduled
     */
    int64_t getNextRotationTime(SecurityLevel level);
    
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
    
    void schedulerLoop();
    int64_t getCurrentTimeMs() const;

    void persistRotationState(SecurityLevel level);
    void loadRotationState(SecurityLevel level);
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
