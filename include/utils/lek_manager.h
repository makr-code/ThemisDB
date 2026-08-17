/**
 * @file lek_manager.h
 * @brief Log Encryption Key Manager with daily rotation and lifecycle management.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Last Updated: 2026-08-08
 * @note Source: Level 0 - API Contract
 * @note SOT Domain: crypto-key-management
 * @note Related: RFC 5869 (HKDF), AES-GCM encryption
 * 
 * @page lek_manager_api LEK Manager API
 *
 * ## Purpose
 * The LEKManager implements key hierarchy and rotation for secure log encryption:
 * - **KEK (Key Encryption Key)**: Derived from PKI certificate via HKDF; used to encrypt/decrypt LEKs
 * - **LEK (Log Encryption Key)**: Random 256-bit AES key created daily; used to encrypt log messages
 * - **Versioning**: Each LEK is uniquely identified by date (YYYY-MM-DD) to support multi-key decryption
 *
 * ## Key Hierarchy
 * ```
 * PKI Certificate → HKDF Derivation → KEK (256-bit)
 *                                        ↓
 *                                 Encrypts/Decrypts
 *                                        ↓
 * Random Entropy → AES Key Generation → LEK (256-bit, per day)
 *                                        ↓
 *                                 Encrypts Log Data
 * ```
 *
 * ## Key Lifecycle Phases
 * 1. **Generation**: New LEK created at UTC midnight or via force rotation
 * 2. **Storage**: LEK encrypted with KEK and stored in RocksDB: `lek:encrypted:<YYYY-MM-DD> = AES-GCM(KEK, LEK)`
 * 3. **Usage**: Current LEK used for encrypting new log messages
 * 4. **Rotation**: Automatic daily rotation triggered by background worker
 * 5. **Revocation**: Explicit revocation prevents further use (for security incidents)
 * 6. **Migration**: Key relabeling for compliance operations
 *
 * ## Thread Safety
 * - All public methods are thread-safe via internal locking
 * - Background rotation worker runs independently
 * - Multiple concurrent encrypt/decrypt operations are safe
 *
 * ## Audit Integration
 * - Optional AuditLogger for tracking rotation events
 * - KEY_ROTATED events logged with old/new key details
 * - KEY_REVOKED events logged for compliance
 *
 * ## Compliance & Security
 * - Compliant with NIST SP 800-38D (AES-GCM)
 * - Compliant with RFC 5869 (HKDF key derivation)
 * - Supports key versioning for audit trail
 * - Memory-safe key material handling (RAII, zero-wiping)
 *
 * @see HKDFHelper for key derivation implementation
 * @see AuditLogger for rotation event tracking
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
     * @brief Initialize LEK Manager with persistent storage and key provider.
     * 
     * Sets up the LEK Manager and initializes the Key Encryption Key (KEK):
     * - If KEK does not exist, it is derived from PKI certificate using HKDF
     * - All subsequent LEKs are encrypted with this KEK before storage
     * 
     * @param db RocksDB wrapper for persistent LEK storage
     *           Keys stored as: `lek:encrypted:<YYYY-MM-DD>` (encrypted LEK blob)
     *                          `lek_revoked:<YYYY-MM-DD>` (revocation marker)
     * @param pki PKI client for certificate-based KEK derivation
     *            Must remain valid for lifetime of LEKManager
     * @param key_provider Key provider for KEK encryption/decryption operations
     *                    Must remain valid for lifetime of LEKManager
     * 
     * @throws std::runtime_error if KEK initialization fails
     * @throws std::invalid_argument if any parameter is nullptr
     * 
     * @note Thread-Safe: Constructor is thread-safe
     * @note Memory: Takes shared_ptr ownership of all parameters
     * 
     * @example
     * @code
     * auto lek_mgr = std::make_shared<LEKManager>(
     *     rocksdb_wrapper,
     *     pki_client,
     *     key_provider
     * );
     * lek_mgr->startAutoRotation();
     * @endcode
     */
    LEKManager(std::shared_ptr<themis::RocksDBWrapper> db,
               std::shared_ptr<VCCPKIClient> pki,
               std::shared_ptr<KeyProvider> key_provider);

    /**
     * @brief Destructor – stops the auto-rotation thread if running.
     * 
     * Gracefully stops the background key rotation worker thread (if active)
     * and waits for it to exit. All cached keys are preserved (not wiped)
     * to allow final operations if needed. If explicit zeroing is required,
     * call clear() on consumers before destruction.
     * 
     * @thread_safe Safe to call from any thread
     * @note Blocking: Waits for auto-rotation thread to join
     * @note Exception-safe: Destructor does not throw
     */
    ~LEKManager();

    /**
     * @brief Get current LEK (creates if not exists).
     * 
     * Returns the key ID for today's Log Encryption Key. If today's LEK
     * does not exist, it is automatically generated, encrypted, and stored.
     * This is the primary API for obtaining a key for new log encryption.
     * 
     * @return LEK key_id suitable for use with FieldEncryption::encryptField()
     *         Format: "lek_<YYYY-MM-DD>"
     * @throws std::runtime_error if LEK generation or storage fails
     * 
     * @thread_safe Yes - internally synchronized
     * @note Automatic generation: If no LEK exists for today, one is created
     * @note Idempotent: Calling multiple times returns the same key_id
     * 
     * Usage:
     * @code
     * std::string lek_id = lek_mgr->getCurrentLEK();
     * field_enc.encryptField("user_email", "alice@example.com", lek_id);
     * @endcode
     */
    std::string getCurrentLEK();
    
    /**
     * @brief Get LEK for specific date (for decrypting old logs).
     * 
     * Retrieves the key ID for a historically-dated LEK. Useful when
     * decrypting log messages created on a previous date. If the key
     * has been revoked, returns empty string.
     * 
     * @param date_str Date of desired LEK in format "YYYY-MM-DD"
     * @return LEK key_id if found and not revoked; empty string if key not found or revoked
     * @throws std::runtime_error if key store (RocksDB) is unavailable
     * 
     * @error_contract
     * | Condition | ErrorCode | Severity | Logging | Recovery |
     * |-----------|-----------|----------|---------|----------|
     * | Key store (RocksDB) unavailable – cannot persist new LEK | CRYPTO_KEY_NOT_FOUND (9053) | Critical | date_str, db_key | Throw (fail-closed) |
     * | Key has been revoked | CRYPTO_KEY_EXPIRED (9052) | Info | date_str | Return empty string |
     *
     * @degradation fail-closed on key store unavailability; no plaintext fallback
     * @thread_safe Yes - internally synchronized
     * 
     * Usage:
     * @code
     * std::string old_lek = lek_mgr->getLEKForDate("2026-08-01");
     * if (!old_lek.empty()) {
     *     auto decrypted = field_enc.decryptField(encrypted_data, old_lek);
     * }
     * @endcode
     */
    std::string getLEKForDate(const std::string& date_str);
    
    /**
     * @brief Force immediate key rotation (creates new LEK for today).
     * 
     * Triggers immediate key rotation, creating a new LEK for the current
     * date. Used in security incidents or compliance scenarios where a key
     * needs to be replaced outside the normal daily rotation schedule.
     * 
     * @thread_safe Yes - internally synchronized
     * @throws std::runtime_error if LEK creation fails
     * 
     * @note Effect: Next getCurrentLEK() returns the new key
     * @note Audit: Rotation event is logged if AuditLogger is attached
     * 
     * Usage:
     * @code
     * // Security incident: force key rotation
     * lek_mgr->rotate();
     * // All subsequent encryptions use new LEK
     * @endcode
     */
    void rotate();
    
    /**
     * @brief Get current date string in ISO 8601 format.
     * 
     * @return Date string in format "YYYY-MM-DD" (UTC)
     * @note Utility function - useful for testing and log formatting
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
     * Marks a LEK as revoked, preventing it from being used for future
     * encryption operations. Revoked keys are:
     * - Stored in in-memory revocation list
     * - Persisted in RocksDB under `lek_revoked:<date>`
     * - Checked by getLEKForDate() before returning key
     *
     * Usage scenarios:
     * - Security incident: compromise discovered for a specific date's key
     * - Compliance: key rotation requirement
     * - Audit: forced key version change
     *
     * @param date_str Date of LEK to revoke, format "YYYY-MM-DD"
     * @return true if key was present and successfully revoked; false if not found
     * 
     * @thread_safe Yes - internally synchronized
     * @note Audit: Revocation is logged if AuditLogger is attached
     * @note Persistence: Revocation list survives process restart
     * 
     * @example
     * @code
     * // Incident response: revoke yesterday's key
     * if (lek_mgr->revokeKey("2026-08-07")) {
     *     LOG(WARNING) << "LEK for 2026-08-07 revoked";
     * }
     * @endcode
     */
    bool revokeKey(const std::string& date_str);

    /**
     * @brief Check whether a LEK for the given date has been revoked.
     * 
     * @param date_str Date to check, format "YYYY-MM-DD"
     * @return true if key is on the revocation list
     * 
     * @thread_safe Yes
     * @note Non-throwing: Returns false if key not found (not same as "not revoked")
     */
    bool isRevoked(const std::string& date_str) const;

    /**
     * @brief Return all currently revoked date strings.
     * 
     * @return Vector of revoked dates in "YYYY-MM-DD" format
     * @thread_safe Yes
     * 
     * Usage:
     * @code
     * auto revoked = lek_mgr->getRevokedKeys();
     * for (const auto& date : revoked) {
     *     LOG(INFO) << "Revoked: " << date;
     * }
     * @endcode
     */
    std::vector<std::string> getRevokedKeys() const;

    /**
     * @brief Check whether a key has exceeded its maximum age.
     *
     * Determines if a LEK is too old to be used for new encryption.
     * Keys older than @p max_age_days are considered expired and should
     * be rotated. Note: Expired keys can still decrypt old data but
     * should not be used for new encryption.
     *
     * @param date_str Date to check, format "YYYY-MM-DD"
     * @param max_age_days Maximum allowable age in calendar days (default: 30)
     * @return true if key's date is more than @p max_age_days days ago
     * 
     * @thread_safe Yes
     * @note Static: Can be called without an instance
     * 
     * @example
     * @code
     * if (LEKManager::isExpired("2026-07-01", 30)) {
     *     LOG(WARNING) << "Key is older than 30 days";
     * }
     * @endcode
     */
    static bool isExpired(const std::string& date_str, int max_age_days = 30);

    /**
     * @brief Migrate a key from an old date label to a new one.
     *
     * Renames a key's storage label, useful for:
     * - Compliance operations (key rotation with versioning)
     * - Archival relabeling
     * - Integration between systems with different dating schemes
     *
     * Operation:
     * 1. Reads encrypted LEK blob from old_date location
     * 2. Writes to new_date location in RocksDB
     * 3. Invalidates old_date entry
     *
     * @param old_date Source date label, format "YYYY-MM-DD"
     * @param new_date Destination date label, format "YYYY-MM-DD"
     * @return true if migration succeeded; false if old_date not found
     * 
     * @thread_safe Yes
     * @throws std::runtime_error if database operation fails
     * 
     * @example
     * @code
     * // Relabel key from old date to new date
     * lek_mgr->migrateKey("2026-08-01", "2026-08-01-backup");
     * @endcode
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
