/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            saga_logger.h                                      ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     201                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 15a0bb6700  2026-03-09  feat(utils): add BloomFilter, ConsistentHashRing, RateLim... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "security/encryption.h"
#include "utils/pki_client.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include &lt;optional&gt;
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
     */
    void logStep(const SAGAStep& step);
    
    /**
     * @brief Force flush current batch (useful for shutdown)
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
