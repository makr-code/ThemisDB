/**
 * @file adapter_consistency_checker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "lora_config.h"
#include <string>
#include <optional>
#include <vector>
#include <cstdint>

namespace themis {
namespace llm {
namespace lora {

struct ConsistencyCheckResult {
    /**
     * @brief Consistency Check Result.
     * @return Return value.
     */
    virtual ~ConsistencyCheckResult() = default;
    bool is_valid = false;
    std::string checksum;          // SHA-256 hex
    std::string signature;         // Digital signature (if enabled)
    bool signature_valid = false;
    std::string error_message;
    
    // Version information
    std::string version;           // Version string (e.g., "1.0.0", "v2")
    uint64_t timestamp = 0;        // Unix timestamp (nanoseconds)
};

class AdapterConsistencyChecker {
public:
    struct Config {
        bool enable_checksums = true;
        bool enable_signatures = true;
        bool strict_mode = false;      // Fail on any inconsistency
        std::string signature_algorithm = "ed25519";
    };
    
    /**
     * @brief Adapter Consistency Checker.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AdapterConsistencyChecker(const Config& config);
    /**
     * @brief Adapter Consistency Checker.
     * @return Return value.
     */
    explicit AdapterConsistencyChecker();
    ~AdapterConsistencyChecker();
    
    // Disable copy
    AdapterConsistencyChecker(const AdapterConsistencyChecker&) = delete;
    AdapterConsistencyChecker& operator=(const AdapterConsistencyChecker&) = delete;
    
    /**
     * @brief Calculate Checksum.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::string calculateChecksum(const std::vector<uint8_t>& data) const;
    
    /**
     * @brief Verify Checksum.
     * @param[in] data Input parameter.
     * @param[in] expected_checksum Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyChecksum(
        const std::vector<uint8_t>& data,
        const std::string& expected_checksum
    ) const;
    
    std::string generateSignature(
        const std::vector<uint8_t>& data,
        const std::string& private_key = ""
    ) const;
    
    bool verifySignature(
        const std::vector<uint8_t>& data,
        const std::string& signature,
        const std::string& public_key = ""
    ) const;
    
    /**
     * @brief Check Adapter.
     * @param[in] adapter_id Identifier of the adapter.
     * @param[in] data Input parameter.
     * @param[in] metadata Input parameter.
     * @return Return value.
     */
    ConsistencyCheckResult checkAdapter(
        const std::string& adapter_id,
        const std::vector<uint8_t>& data,
        const AdapterMetadata& metadata
    ) const;
    
    /**
     * @brief Compare Versions.
     * @param[in] local_result Input parameter.
     * @param[in] remote_result Input parameter.
     * @return Return value.
     */
    int compareVersions(
        const ConsistencyCheckResult& local_result,
        const ConsistencyCheckResult& remote_result
    ) const;
    
    /**
     * @brief Resolve Conflict.
     * @param[in] local_result Input parameter.
     * @param[in] remote_result Input parameter.
     * @return Return value.
     */
    ConsistencyCheckResult resolveConflict(
        const ConsistencyCheckResult& local_result,
        const ConsistencyCheckResult& remote_result
    ) const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace lora
} // namespace llm
} // namespace themis
