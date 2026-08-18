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
#include "utils/error_contracts.h"
#include "utils/pki_client.h"
#include "utils/error_contracts.h"
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
     * @brief Log a single SAGA step (buffered).
     *
     * Buffers the step for batch writing. When the batch size threshold is reached,
     * `signAndFlushBatch()` is called automatically.
     *
     * @param step SAGA step to record
     * @throws std::runtime_error if log file unavailable or encryption fails (fail-closed)
     *
     * @error_contract
     * | Condition | ErrorCode | Severity | Logging | Recovery |
     * |-----------|-----------|----------|---------|----------|
     * | Batch encryption fails (LEK unavailable) | AUDIT_ENCRYPTION_FAILED (9015) | Critical | batch_id, error | Throw (fail-closed – no plaintext persisted) |
     * | Log file write fails (backend unavailable) | AUDIT_PERSISTENCE_FAILED (9012) | Critical | path, error | Throw (fail-closed) |
     *
     * @degradation fail-closed – encryption failure blocks persistence; no plaintext fallback
     * @see ErrorCode 9010-9019 for audit error taxonomy
     */
    void logStep(const SAGAStep& step);
    
    /**
     * @brief Force flush current batch (useful for shutdown).
     *
     * @throws std::runtime_error if flush fails (propagated from signAndFlushBatch)
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

    /**
     * @brief Log error context for diagnostics
     * @param ctx Error context with details about the error
     */
    void logErrorContext(const ErrorContext& ctx);

private:
    void signAndFlushBatch();
    std::vector<uint8_t> sha256(const std::vector<uint8_t>& data);
    void appendJsonLine(const std::string& path, const nlohmann::json& j);
    std::string generateBatchId() const;
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
