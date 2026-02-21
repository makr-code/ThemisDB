/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            lek_manager.h                                      ║
  Version:         0.0.18                                             ║
  Last Modified:   2026-02-21 18:43:59                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     164                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/encryption.h"
#include "utils/pki_client.h"
#include "storage/rocksdb_wrapper.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace themis {
namespace utils {

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
    
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<VCCPKIClient> pki_;
    std::shared_ptr<KeyProvider> key_provider_;
    
    std::mutex mu_;
    std::unordered_map<std::string, std::string> lek_cache_; // date -> key_id
    std::string kek_key_id_ = "lek_kek";

    // Phase 5: Revocation list
    mutable std::mutex revocation_mu_;
    std::unordered_set<std::string> revoked_keys_;
};

} // namespace utils
} // namespace themis
