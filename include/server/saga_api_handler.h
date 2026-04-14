/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            saga_api_handler.h                                 ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:28:54                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     84                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
