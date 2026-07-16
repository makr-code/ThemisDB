/**
 * @file saga_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: saga_api_handler.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <memory>
#include <vector>
#include <nlohmann/json.hpp>
#include "utils/saga_logger.h"
#include "security/encryption.h"
#include "utils/pki_client.h"

namespace themis {
namespace server {

struct SAGABatchInfo {
    std::string batch_id;
    int64_t start_time_ms;
    int64_t end_time_ms;
    size_t entry_count;
    std::string lek_id;
    uint32_t key_version;
    bool signature_valid;
    std::string signature_id;
    std::string cert_serial;
    std::string algorithm;
    
    nlohmann::json toJson() const;
};

struct SAGABatchDetail {
    SAGABatchInfo info;
    std::vector<themis::utils::SAGAStep> steps;
    std::string ciphertext_hash_b64;
    std::string signature_b64;
    
    nlohmann::json toJson() const;
};

class SAGAApiHandler {
public:
    SAGAApiHandler(std::shared_ptr<themis::utils::SAGALogger> saga_logger);

    // List all SAGA batches with summary info
    nlohmann::json listBatches();
    
    // Get detailed info for a specific batch (including verification)
    nlohmann::json getBatchDetail(const std::string& batch_id);
    
    // Verify a batch's signature and integrity
    nlohmann::json verifyBatch(const std::string& batch_id);
    
    // Force flush current buffer to create new batch (admin operation)
    nlohmann::json flushCurrentBatch();

private:
    std::shared_ptr<themis::utils::SAGALogger> saga_logger_;
    
    // Parse batch metadata from signature file
    SAGABatchInfo parseBatchInfo(const std::string& batch_id);
};

} // namespace server
} // namespace themis
