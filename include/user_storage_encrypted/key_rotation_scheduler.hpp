/**
 * @file key_rotation_scheduler.hpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security_level.hpp"
#include "encryption_backend_interface.hpp"
#include "irotation_store.hpp"
#include <functional>
#include <memory>
#include <string>

namespace themis {
namespace plugins {
namespace user_storage {

/**
 * @brief Key rotation scheduler for automatic key rotation
 * 
 * Features:
 * - Schedule-based rotation (e.g., every 90 days)
 * - Zero-downtime rotation process
 * - Notification on rotation completion
 * - Thread-safe operation
 * - Optional persistent state via IRotationStore (last_check_ms survives restart)
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
     * @brief Attach a persistence store for last_check_ms.
     *
     * Must be called before initialize().  Defaults to NullRotationStore.
     */
    void setRotationStore(std::shared_ptr<IRotationStore> store);

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
    * @param check_interval_seconds  How often to check for rotation needs.
    * @param store                   Optional persistence backend (may be nullptr).
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

    /**
     * @brief Manually trigger rotation check for a level (for testing).
     */
    void triggerRotation(SecurityLevel level);
    
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
