/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            saga_api_handler.h                                 ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     88                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
