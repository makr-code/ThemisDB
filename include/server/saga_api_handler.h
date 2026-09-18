/**
 * @file saga_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};

struct SAGABatchDetail {
    SAGABatchInfo info;
    std::vector<themis::utils::SAGAStep> steps;
    std::string ciphertext_hash_b64;
    std::string signature_b64;
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
};


class SAGAApiHandler {
public:
    SAGAApiHandler(std::shared_ptr<themis::utils::SAGALogger> saga_logger);

    /**
     * @brief List Batches.
     * @return Return value.
     */
    nlohmann::json listBatches();
    
    /**
     * @brief Get Batch Detail.
     * @param[in] batch_id Identifier of the batch.
     * @return Return value.
     */
    nlohmann::json getBatchDetail(const std::string& batch_id);
    
    /**
     * @brief Verify Batch.
     * @param[in] batch_id Identifier of the batch.
     * @return Return value.
     */
    nlohmann::json verifyBatch(const std::string& batch_id);
    
    /**
     * @brief Flush Current Batch.
     * @return Return value.
     */
    nlohmann::json flushCurrentBatch();

private:
    std::shared_ptr<themis::utils::SAGALogger> saga_logger_;
    
    /**
     * @brief Parse Batch Info.
     * @param[in] batch_id Identifier of the batch.
     * @return Return value.
     */
    SAGABatchInfo parseBatchInfo(const std::string& batch_id);
};

} // namespace server
} // namespace themis
