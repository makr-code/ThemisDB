/**
 * @file pii_pseudonymizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/pii_detector.h"
#include "utils/audit_logger.h"
#include "security/encryption.h"
#include "storage/rocksdb_wrapper.h"
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace utils {

/**
 * @brief PII Pseudonymization Manager with DSGVO Art. 17 compliance
 * 
 * DSGVO Art. 17 Compliant PII Management:
 * - Replace detected PII with UUIDs in entities
 * - Store encrypted PII→UUID mapping in separate RocksDB Column Family
 * - revealPII(): Decrypt and return original value (with audit log)
 * - erasePII(): Delete mapping → original value irrecoverable
 * 
 * Workflow:
 * 1. Entity import: Detect PII → Replace with UUID → Store mapping
 * 2. Query: Entity contains UUIDs, original values hidden
 * 3. Reveal: Authorized user → decrypt PII mapping → log access
 * 4. Erasure: Delete mapping → UUID remains, but original lost forever
 */
class PIIPseudonymizer {
public:
    /**
     * @brief Initialize with storage and encryption
     * @param db RocksDB wrapper (must have "pii_mapping" column family)
     * @param enc Field encryption for PII values
     * @param detector PII detector for automatic detection
     * @param audit_logger Audit logger for PII access/erasure events (optional)
     */
    PIIPseudonymizer(std::shared_ptr<themis::RocksDBWrapper> db,
                     std::shared_ptr<FieldEncryption> enc,
                     std::shared_ptr<PIIDetector> detector,
                     std::shared_ptr<AuditLogger> audit_logger = nullptr);
    
    /**
     * @brief Pseudonymize detected PII in JSON object.
     *
     * Replaces PII values with UUIDs. Requires a valid pseudonymization key;
     * if the key is unavailable, the operation fails closed rather than
     * producing plaintext in the output.
     *
     * @param data Input JSON with potential PII
     * @return JSON with PII replaced by UUIDs, and list of UUID mappings created
     * @throws std::runtime_error if pseudonymization key is unavailable (fail-closed)
     *
     * @error_contract
     * | Condition | ErrorCode | Severity | Logging | Recovery |
     * |-----------|-----------|----------|---------|----------|
     * | Pseudonymization key unavailable (RAND_bytes failure) | CRYPTO_KEY_DERIVATION_FAILED (9050) | Critical | key_id | Throw (fail-closed – no plaintext produced) |
     * | Database unavailable (cannot persist mapping) | CRYPTO_KEY_NOT_FOUND (9053) | Critical | pii_uuid | Throw (fail-closed) |
     *
     * @degradation fail-closed – key unavailable → reject; never returns unredacted data on error
     * @see ErrorCode 9050-9059 for crypto error taxonomy
     * @see ErrorCode 9040-9049 for privacy detection taxonomy
     */
    std::pair<nlohmann::json, std::vector<std::string>> pseudonymize(
        const nlohmann::json& data
    );
    
    /**
     * @brief Reveal original PII value for authorized user
     * @param pii_uuid UUID from pseudonymized entity
     * @param user_id User requesting reveal (for audit log)
     * @return Original PII value or empty if not found/unauthorized
     */
    std::optional<std::string> revealPII(const std::string& pii_uuid, 
                                        const std::string& user_id);
    
    /**
     * @brief Erase PII mapping (DSGVO Art. 17 - Right to be Forgotten)
     * @param pii_uuid UUID to erase
     * @return true if mapping existed and was deleted
     */
    bool erasePII(const std::string& pii_uuid);

    /**
     * @brief Register a cache invalidation callback for GDPR Art. 17 propagation.
     *
     * When set, this callback is invoked automatically after every successful
     * erasePII() call so that any cache tier (e.g. AdaptiveQueryCache) holding
     * data tagged with pii_uuid is purged without requiring caller coordination.
     *
     * @param fn  Callable that receives the pii_uuid to be purged from cache.
     *            Must be non-throwing; exceptions are logged and suppressed.
     */
    void registerCacheInvalidator(std::function<void(const std::string&)> fn);

    /**
     * @brief Soft-Delete eines PII-Mappings (ausblenden, aber nicht löschen)
     * Markiert das Mapping als inactive und setzt deleted_at.
     * @param pii_uuid UUID des Mappings
     * @param user_id Benutzer, der die Aktion ausführt (für Audit)
     * @return true, wenn Mapping existierte und aktualisiert wurde
     */
    bool softDeletePII(const std::string& pii_uuid, const std::string& user_id);
    
    /**
     * @brief Find all PII UUIDs for a specific entity
     * @param entity_pk Primary key of entity
     * @return List of PII UUIDs associated with this entity
     */
    std::vector<std::string> findPIIForEntity(const std::string& entity_pk);
    
    /**
     * @brief Erase all PII for entity (DSGVO Art. 17 complete erasure)
     * @param entity_pk Primary key of entity
     * @return Number of PII mappings erased
     */
    size_t eraseAllPIIForEntity(const std::string& entity_pk);

private:
    std::string generateUUID() const;
    std::string dbKey(const std::string& pii_uuid) const;
    std::string entityIndexKey(const std::string& entity_pk) const;
    
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::shared_ptr<FieldEncryption> enc_;
    std::shared_ptr<PIIDetector> detector_;
    std::shared_ptr<AuditLogger> audit_logger_;
    std::function<void(const std::string&)> cache_invalidator_;

    // Plain mutex: eraseAllPIIForEntity calls findPIIForEntity and erasePII
    // sequentially (each acquires and releases the lock independently, no
    // re-entrant locking occurs on any code path).
    std::mutex mu_;
    std::string key_id_ = "pii_mapping_key";
};

} // namespace utils
} // namespace themis
