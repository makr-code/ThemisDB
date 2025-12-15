// Copyright (c) 2025 ThemisDB Contributors
// Licensed under the MIT License

#pragma once

#include <string>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace enterprise {

/**
 * @brief Result of license validation from server
 */
struct ValidationResult {
    bool is_valid;
    std::string message;
    std::chrono::system_clock::time_point validated_at;
    std::chrono::system_clock::time_point next_validation;
    std::string server_version;
};

/**
 * @brief Client for online license validation
 * 
 * Communicates with ThemisDB license validation server to:
 * - Verify license authenticity
 * - Check license status (active, suspended, expired)
 * - Validate usage limits (node count, etc.)
 * - Report telemetry (optional, configurable)
 */
class LicenseValidationClient {
public:
    /**
     * @brief Configuration for validation client
     */
    struct Config {
        std::string server_url = "https://license.themisdb.io/api/v1";
        std::string ca_cert_path;  // For SSL verification
        int timeout_seconds = 10;
        bool enable_telemetry = false;
        int retry_count = 3;
    };
    
    explicit LicenseValidationClient(const Config& config);
    ~LicenseValidationClient();
    
    /**
     * @brief Validate license with server
     * @param license_key License key to validate
     * @param edition Edition (enterprise or hyperscaler)
     * @param node_count Current number of nodes
     * @return Validation result, or nullopt on communication error
     */
    std::optional<ValidationResult> validateLicense(
        const std::string& license_key,
        const std::string& edition,
        int node_count
    );
    
    /**
     * @brief Check if license needs revalidation
     * @param last_validation Time of last successful validation
     * @return true if revalidation is needed
     */
    bool needsRevalidation(const std::chrono::system_clock::time_point& last_validation) const;
    
    /**
     * @brief Report deployment telemetry (if enabled)
     * @param license_key License key
     * @param telemetry_data JSON telemetry data
     * @return true if reported successfully
     */
    bool reportTelemetry(
        const std::string& license_key,
        const nlohmann::json& telemetry_data
    );

private:
    Config config_;
    
    // HTTP request helper
    std::optional<std::string> httpPost(
        const std::string& endpoint,
        const nlohmann::json& payload
    );
    
    // Validation interval (24 hours default)
    std::chrono::hours validation_interval_{24};
};

} // namespace enterprise
} // namespace themis
