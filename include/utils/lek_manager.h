/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lek_manager.h                                      ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:14:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     234                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 2427cfd480  2026-02-28  Implement LEK rotation automation: startAutoRotation/stop... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/encryption.h"
#include "utils/pki_client.h"
#include "storage/rocksdb_wrapper.h"
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis {
namespace utils {

// Forward declaration to avoid heavy header pull-in
class AuditLogger;

/**
 * @brief Log Encryption Key Manager with daily rotation
 * 
 * Key Hierarchy:
 * 1. KEK (Key Encryption Key) - Derived from PKI certificate via HKDF
 * 2. LEK (Log Encryption Key) - Random 256-bit AES key, rotated daily
 * 3. LEK stored encrypted with KEK in RocksDB: lek:<date> = AES-GCM(KEK, LEK)
 * 
 * Usage:
 * - Encrypt logs with current LEK
 * - Decrypt historical logs by loading LEK for specific date
 * - Automatic rotation at midnight (configurable)
 */
class LEKManager {
public:
    /**
     * @brief Initialize LEK Manager
     * @param db RocksDB wrapper for persistent storage
     * @param pki PKI client for KEK derivation
     * @param key_provider For encrypting/decrypting LEKs with KEK
     */
    LEKManager(std::shared_ptr<themis::RocksDBWrapper> db,
               std::shared_ptr<VCCPKIClient> pki,
               std::shared_ptr<KeyProvider> key_provider);

    /**
     * @brief Destructor – stops the auto-rotation thread if running.
     */
    ~LEKManager();

    /**
     * @brief Get current LEK (creates if not exists)
     * @return LEK key_id for use with FieldEncryption
     */
    std::string getCurrentLEK();
    
    /**
     * @brief Get LEK for specific date (for decrypting old logs)
     * @param date_str Format: "YYYY-MM-DD"
     * @return LEK key_id or empty if not found
     */
    std::string getLEKForDate(const std::string& date_str);
    
    /**
     * @brief Force rotation (creates new LEK for today)
     */
    void rotate();
    
    /**
     * @brief Get current date string (YYYY-MM-DD)
     */
    static std::string getCurrentDateString();

    // -----------------------------------------------------------------------
    // Automated Key Rotation
    // -----------------------------------------------------------------------

    /**
     * @brief Attach an AuditLogger so rotation events are recorded.
     *
     * Call before startAutoRotation() if audit-trail integration is required.
     * Thread-safe; may be called at any time.
     *
     * @param logger  AuditLogger instance (may be nullptr to disable auditing).
     */
    void setAuditLogger(std::shared_ptr<AuditLogger> logger);

    /**
     * @brief Start the background auto-rotation worker.
     *
     * The worker wakes up every @p check_interval and calls getCurrentLEK()
     * to ensure today's key exists.  When the calendar date changes past
     * midnight the new LEK is created automatically without operator
     * intervention.  Additionally, any cached key whose date exceeds
     * @p max_age_days is revoked and a KEY_ROTATED audit event is emitted.
     *
     * Calling startAutoRotation() while a worker is already running is a
     * no-op; call stopAutoRotation() first if you need to change parameters.
     *
     * @param check_interval  How often the worker polls (default: 1 hour).
     * @param max_age_days    Maximum key age before forced revocation;
     *                        must be >= 1 (default: 30 days).
     * @throws std::invalid_argument if max_age_days < 1.
     */
    void startAutoRotation(
        std::chrono::seconds check_interval = std::chrono::seconds(3600),
        int max_age_days = 30);

    /**
     * @brief Stop the background auto-rotation worker and join the thread.
     *
     * Blocks until the worker thread has exited.  Safe to call even if no
     * worker is running.
     */
    void stopAutoRotation();

    /**
     * @brief Returns true when the auto-rotation worker is active.
     */
    bool isAutoRotationRunning() const noexcept;

    // -----------------------------------------------------------------------
    // Phase 5: Key Lifecycle Management
    // -----------------------------------------------------------------------

    /**
     * @brief Revoke a LEK key for the given date, preventing its future use.
     *
     * Revoked keys are added to an in-memory revocation list and also
     * persisted to the RocksDB store under the prefix "lek_revoked:<date>".
     *
     * @param date_str  "YYYY-MM-DD" date of the key to revoke.
     * @return true if the key was present and successfully revoked.
     */
    bool revokeKey(const std::string& date_str);

    /**
     * @brief Check whether a LEK for the given date has been revoked.
     * @param date_str  "YYYY-MM-DD" date of the key to check.
     * @return true if the key is on the revocation list.
     */
    bool isRevoked(const std::string& date_str) const;

    /**
     * @brief Return all currently revoked date strings.
     */
    std::vector<std::string> getRevokedKeys() const;

    /**
     * @brief Check whether a key has exceeded its maximum age.
     *
     * Keys older than @p max_age_days are considered expired and
     * should be rotated (they cannot be revoked retrospectively, but
     * callers can treat them as unavailable for new encryption operations).
     *
     * @param date_str     "YYYY-MM-DD" date of the key to check.
     * @param max_age_days Maximum age in calendar days.
     * @return true if the key's date is more than @p max_age_days days ago.
     */
    static bool isExpired(const std::string& date_str, int max_age_days = 30);

    /**
     * @brief Migrate a key from an old date label to a new one.
     *
     * Copies the encrypted LEK blob from @p old_date to @p new_date in
     * the RocksDB store and invalidates the old entry.  Useful when
     * re-labelling keys during compliance key-rotation operations.
     *
     * @param old_date  Source date label.
     * @param new_date  Destination date label.
     * @return true if the migration succeeded.
     */
    bool migrateKey(const std::string& old_date, const std::string& new_date);

private:
    void ensureLEKExists(const std::string& date_str);
    std::vector<uint8_t> deriveKEK();
    std::string lekKeyId(const std::string& date_str) const;
    std::string dbKey(const std::string& date_str) const;

    // Background auto-rotation worker implementation
    void autoRotationLoop(std::chrono::seconds check_interval, int max_age_days);
    
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<VCCPKIClient> pki_;
    std::shared_ptr<KeyProvider> key_provider_;
    
    std::mutex mu_;
    std::unordered_map<std::string, std::string> lek_cache_; // date -> key_id
    std::string kek_key_id_ = "lek_kek";

    // Phase 5: Revocation list
    mutable std::mutex revocation_mu_;
    std::unordered_set<std::string> revoked_keys_;

    // Auto-rotation background thread state
    std::atomic<bool>       rotation_running_{false};
    std::thread             rotation_thread_;
    std::mutex              rotation_cv_mu_;
    std::condition_variable rotation_cv_;
    bool                    rotation_stop_{false};

    // Optional audit logger for rotation events
    std::mutex                   audit_mu_;
    std::shared_ptr<AuditLogger> audit_logger_;
};

} // namespace utils
} // namespace themis
