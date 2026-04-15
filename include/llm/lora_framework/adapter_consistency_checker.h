/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_consistency_checker.h                      ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:03:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     163                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
