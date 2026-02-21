/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            key_rotation_scheduler.hpp                         ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:36:07                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     117                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f016f416f  2026-02-11  Add Multi-Level Encrypted User Storage Plugin with gocryp... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security_level.hpp"
#include "encryption_backend_interface.hpp"
#include <functional>
#include <memory>

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
     * @brief Initialize scheduler
     * 
     * @param check_interval_seconds How often to check for rotation needs
     */
    Result<void> initialize(int check_interval_seconds = 3600);
    
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
};

} // namespace user_storage
} // namespace plugins
} // namespace themis
