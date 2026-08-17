/**
 * @file saga_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "security/encryption.h"
#include "utils/pki_client.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace utils {

/**
 * @brief Configuration for SAGA logging with batch signing
 */
struct SAGALoggerConfig {
    bool enabled = true;
    bool encrypt_then_sign = true;
    size_t batch_size = 1000;                          // Sign every N entries
    std::chrono::minutes batch_interval{5};            // Or every X minutes
    std::string log_path = "data/logs/saga.jsonl";
    std::string signature_path = "data/logs/saga_signatures.jsonl";
    std::string key_id = "saga_lek";                   // Log Encryption Key ID
};

/**
 * @brief Single SAGA step entry
 */
struct SAGAStep {
    std::string saga_id;
    std::string step_name;
    std::string action;        // "forward" | "compensate"
    std::string entity_id;
    nlohmann::json payload;    // Step-specific data
    std::string status;        // "success" | "failed" | "pending"
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Signed batch metadata
 */
struct SignedBatch {
    std::string batch_id;
    size_t entry_count = 0;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::string lek_id;
    uint32_t key_version = 0;
    std::vector<uint8_t> iv;
    std::vector<uint8_t> tag;
    std::vector<uint8_t> ciphertext_hash;  // SHA-256 over encrypted batch
    SignatureResult signature;
    
    nlohmann::json toJson() const;
    static SignedBatch fromJson(const nlohmann::json& j);
};

/**
 * @brief SAGA Logger with PKI-signed batch encryption for tamper-proof audit trail
 * 
 * Workflow:
 * 1. Collect SAGA steps in memory buffer
 * 2. When batch_size or batch_interval reached:
 *    a. Serialize batch to canonical JSON
 *    b. Encrypt with current LEK (AES-256-GCM)
 *    c. Compute SHA-256 hash over ciphertext
 *    d. Sign hash with PKI
 *    e. Persist ciphertext + signature metadata
 * 3. Verification: Load ciphertext → verify signature → decrypt → validate
 */
class SAGALogger {
public:
    SAGALogger(std::shared_ptr<FieldEncryption> enc,
               std::shared_ptr<VCCPKIClient> pki,
               SAGALoggerConfig cfg);
    
    /**
     * @brief Log a single SAGA step (buffered)
     * 
     * Adds a SAGA step to the in-memory buffer. When buffer reaches batch_size
     * or batch_interval, a signed batch is persisted to disk.
     * Phase 2.10: Comprehensive error contract documentation.
     * 
     * @param step SAGA step record containing saga_id, step_name, action, entity_id, payload, status
     * 
     * @return void (failures logged via ErrorContext)
     * 
     * @error_contract
     * **Phase 2.3 Error Codes (7300-7309, 7363):**
     * - ERR_AUDIT_BUFFER_OVERFLOW (7300): Step payload exceeds max size
     *   - Recovery: Step truncated or dropped; logged as warning
     *   - Severity: WARNING
     *   - User Action: Reduce SAGA step payload size
     * 
     * - ERR_SAGA_EVENT_LOSS (7363): Buffer full; step cannot be added
     *   - Recovery: Auto-triggers batch flush; step retried after flush
     *   - Severity: WARNING
     *   - User Action: Check disk I/O performance; increase batch_size
     * 
     * - ERR_AUDIT_SERIALIZATION_FAILED (7302): Step JSON serialization fails
     *   - Recovery: Step logged as fallback text format
     *   - Severity: ERROR
     *   - User Action: Verify payload JSON structure
     * 
     * **Buffering Semantics:**
     * - Steps are buffered in memory; not durable until batch flush
     * - Batch flush triggered by: buffer full, batch_size reached, or explicit flush()
     * - Flush encrypts, signs, and persists batch atomically
     * - If flush fails: buffer remains; retry on next flush opportunity
     * 
     * @bounded_resources
     * - Buffer size: approximately batch_size * avg_step_size bytes
     * - Watermark: triggers flush when buffer >= (batch_size * 0.9)
     * - Resource check: rejects steps that would exceed buffer capacity
     * 
     * @thread_safety Thread-safe via internal mutex (buffer_mu_)
     * @performance O(1) amortized; O(n) on batch flush where n = batch_size
     * 
     * @see SAGAStep for step structure
     * @see flush() for batch persistence details
     * @see ErrorCode for complete error taxonomy
     */
    void logStep(const SAGAStep& step);
    
    /**
     * @brief Force flush current batch (useful for shutdown)
     * 
     * Immediately persists current buffer as a signed batch to disk,
     * regardless of batch_size or batch_interval. Useful during shutdown
     * to ensure all pending steps are durable.
     * Phase 2.10: Comprehensive error contract documentation.
     * 
     * @return void (see @error_contract below)
     * 
     * @error_contract
     * **Phase 2.3 Error Codes (7300-7309, 7363):**
     * - ERR_AUDIT_LOG_WRITE_FAILED (7301): Cannot write batch to disk
     *   - Recovery: Batch retained in buffer; retry on next flush
     *   - Severity: ERROR
     *   - User Action: Check disk space and file permissions
     * 
     * - ERR_AUDIT_SERIALIZATION_FAILED (7302): Batch serialization fails
     *   - Recovery: Batch logged as text fallback; contents may be incomplete
     *   - Severity: ERROR
     *   - User Action: Check for corruption in batch data
     * 
     * - ERR_AUDIT_SERVICE_DEGRADED (7308): PKI signing service unavailable
     *   - Recovery: Batch written unsigned (metadata preserved); logged as warning
     *   - Severity: WARNING
     *   - User Action: Restore PKI service availability
     * 
     * - ERR_SAGA_EVENT_LOSS (7363): Batch lost during persistence (non-recoverable)
     *   - Recovery: Buffer cleared to prevent duplicate submission
     *   - Severity: CRITICAL
     *   - User Action: Investigate disk I/O failure; restore from backup
     * 
     * **Flush Semantics:**
     * - Encrypt-then-sign flow: serialize → encrypt → hash → sign → persist
     * - Atomic: flush is all-or-nothing (no partial batches)
     * - If flush fails: buffer remains for retry; caller should retry
     * - Empty buffer: no-op; returns immediately
     * 
     * @thread_safety Thread-safe via internal mutex (buffer_mu_)
     * @performance O(batch_size) for encryption and signing operations
     * 
     * @note Critical for shutdown: ensures no step loss
     * @see logStep() for step buffering
     * @see ErrorCode for complete error taxonomy
     */
    void flush();
    
    /**
     * @brief Verify a signed batch by batch_id
     * @return true if signature valid and decryption successful
     */
    bool verifyBatch(const std::string& batch_id);
    
    /**
     * @brief Load and decrypt a batch's entries
     * @return Decrypted SAGA steps or empty if verification fails
     */
    std::vector<SAGAStep> loadBatch(const std::string& batch_id);
    
    /**
     * @brief Get all batch IDs in chronological order
     */
    std::vector<std::string> listBatches() const;

private:
    void signAndFlushBatch();
    std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);
    void appendJsonLine(const std::string& path, const nlohmann::json& j);
    std::string generateBatchId() const;
    
    /**
     * @brief Log an error context to stderr (Phase 2.3 hardening)
     * 
     * Helper method for logging errors to stderr to avoid recursion
     * when normal logging fails. Used during buffer overflow and flush failures.
     */
    void logErrorContext(const ErrorContext& ctx);
    
    std::shared_ptr<FieldEncryption> enc_;
    std::shared_ptr<VCCPKIClient> pki_;
    SAGALoggerConfig cfg_;
    
    std::mutex mu_;
    std::vector<SAGAStep> buffer_;
    std::chrono::system_clock::time_point batch_start_time_;
};

// ---------------------------------------------------------------------------
// SAGALogCompactor
// ---------------------------------------------------------------------------

/**
 * @brief Compacts the SAGA WAL, archiving completed transactions.
 *
 * compact(before_txn_id) rewrites the WAL retaining only steps for
 * in-flight or failed transactions; completed transactions are archived.
 */
class SAGALogCompactor {
public:
    explicit SAGALogCompactor(const SAGALoggerConfig& cfg);

    /**
     * @brief Compact WAL, removing steps for completed transactions up to before_txn_id.
     * @param before_txn_id All completed sagas with id < before_txn_id are archived.
     * @return Number of steps archived.
     */
    size_t compact(const std::string& before_txn_id);

    /**
     * @brief Get the path where archived steps are written.
     */
    std::string archivePath() const;

private:
    SAGALoggerConfig cfg_;
    std::string archive_path_;
};

// ---------------------------------------------------------------------------
// SAGALogReplayer
// ---------------------------------------------------------------------------

/**
 * @brief Replays incomplete SAGA transactions for disaster recovery.
 */
class SAGALogReplayer {
public:
    explicit SAGALogReplayer(const SAGALoggerConfig& cfg);

    using RecoveryHandler = std::function<void(const SAGAStep&)>;

    /**
     * @brief Scan WAL for transactions in COMPENSATING state and invoke handler.
     * @param handler Called once per unconfirmed compensation step.
     * @return Number of steps replayed.
     */
    size_t replay_incomplete(RecoveryHandler handler);

private:
    SAGALoggerConfig cfg_;
};

} // namespace utils
} // namespace themis
