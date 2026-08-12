/**
 * @file adapter_consistency_checker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Result of consistency check
 */
struct ConsistencyCheckResult {
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

/**
 * @brief Adapter Consistency Checker
 * 
 * Validates LoRA adapters for consistency across shards:
 * - Checksum validation (SHA-256)
 * - Digital signature verification
 * - Version comparison
 * - Conflict detection and resolution
 */
class AdapterConsistencyChecker {
public:
    /**
     * @brief Configuration for consistency checker
     */
    struct Config {
        bool enable_checksums = true;
        bool enable_signatures = true;
        bool strict_mode = false;      // Fail on any inconsistency
        std::string signature_algorithm = "ed25519";
    };
    
    explicit AdapterConsistencyChecker(const Config& config);
    explicit AdapterConsistencyChecker();
    ~AdapterConsistencyChecker();
    
    // Disable copy
    AdapterConsistencyChecker(const AdapterConsistencyChecker&) = delete;
    AdapterConsistencyChecker& operator=(const AdapterConsistencyChecker&) = delete;
    
    /**
     * @brief Calculate checksum for adapter data
     * @param data Binary adapter data
     * @return SHA-256 checksum (hex string)
     */
    std::string calculateChecksum(const std::vector<uint8_t>& data) const;
    
    /**
     * @brief Verify checksum of adapter data
     * @param data Binary adapter data
     * @param expected_checksum Expected checksum
     * @return true if checksum matches
     */
    bool verifyChecksum(
        const std::vector<uint8_t>& data,
        const std::string& expected_checksum
    ) const;
    
    /**
     * @brief Generate digital signature for adapter
     * @param data Binary adapter data
     * @param private_key Private key for signing (optional, uses default if empty)
     * @return Base64-encoded signature
     */
    std::string generateSignature(
        const std::vector<uint8_t>& data,
        const std::string& private_key = ""
    ) const;
    
    /**
     * @brief Verify digital signature
     * @param data Binary adapter data
     * @param signature Base64-encoded signature
     * @param public_key Public key for verification (optional, uses default if empty)
     * @return true if signature is valid
     */
    bool verifySignature(
        const std::vector<uint8_t>& data,
        const std::string& signature,
        const std::string& public_key = ""
    ) const;
    
    /**
     * @brief Perform full consistency check on adapter
     * @param adapter_id Adapter identifier
     * @param data Binary adapter data
     * @param metadata Adapter metadata
     * @return Consistency check result
     */
    ConsistencyCheckResult checkAdapter(
        const std::string& adapter_id,
        const std::vector<uint8_t>& data,
        const AdapterMetadata& metadata
    ) const;
    
    /**
     * @brief Compare two adapter versions
     * @param local_result Local adapter check result
     * @param remote_result Remote adapter check result
     * @return -1 if local is older, 0 if same, 1 if local is newer
     */
    int compareVersions(
        const ConsistencyCheckResult& local_result,
        const ConsistencyCheckResult& remote_result
    ) const;
    
    /**
     * @brief Resolve conflict between adapters
     * @param local_result Local adapter
     * @param remote_result Remote adapter
     * @return Winning adapter result (highest version/timestamp)
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
