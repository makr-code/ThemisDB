// Copyright (c) 2025 ThemisDB Contributors
// Licensed under the MIT License

#include "enterprise/license_validation_client.h"
#include <spdlog/spdlog.h>

#ifdef CURL_FOUND
#include <curl/curl.h>
#endif

using json = nlohmann::json;

namespace themis {
namespace enterprise {

// Helper for CURL response
static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

LicenseValidationClient::LicenseValidationClient(const Config& config)
    : config_(config) {
#ifdef CURL_FOUND
    curl_global_init(CURL_GLOBAL_DEFAULT);
#endif
}

LicenseValidationClient::~LicenseValidationClient() {
#ifdef CURL_FOUND
    curl_global_cleanup();
#endif
}

std::optional<ValidationResult> LicenseValidationClient::validateLicense(
    const std::string& license_key,
    const std::string& edition,
    int node_count) {
    
    spdlog::info("Validating license with server: {}", config_.server_url);
    
    // Prepare validation request
    json payload;
    payload["license_key"] = license_key;
    payload["edition"] = edition;
    payload["node_count"] = node_count;
    payload["version"] = THEMIS_VERSION_STRING;
    
    // Send validation request
    auto response = httpPost("/validate", payload);
    if (!response) {
        spdlog::error("Failed to communicate with license validation server");
        return std::nullopt;
    }
    
    try {
        auto response_json = json::parse(*response);
        
        ValidationResult result;
        result.is_valid = response_json.value("valid", false);
        result.message = response_json.value("message", "");
        result.server_version = response_json.value("server_version", "unknown");
        
        // Parse timestamps
        if (response_json.contains("validated_at")) {
            auto timestamp = response_json["validated_at"].get<int64_t>();
            result.validated_at = std::chrono::system_clock::from_time_t(timestamp);
        } else {
            result.validated_at = std::chrono::system_clock::now();
        }
        
        if (response_json.contains("next_validation")) {
            auto timestamp = response_json["next_validation"].get<int64_t>();
            result.next_validation = std::chrono::system_clock::from_time_t(timestamp);
        } else {
            result.next_validation = result.validated_at + validation_interval_;
        }
        
        if (result.is_valid) {
            spdlog::info("License validation successful: {}", result.message);
        } else {
            spdlog::error("License validation failed: {}", result.message);
        }
        
        return result;
        
    } catch (const std::exception& e) {
        spdlog::error("Failed to parse validation response: {}", e.what());
        return std::nullopt;
    }
}

bool LicenseValidationClient::needsRevalidation(
    const std::chrono::system_clock::time_point& last_validation) const {
    
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::hours>(now - last_validation);
    
    return elapsed >= validation_interval_;
}

bool LicenseValidationClient::reportTelemetry(
    const std::string& license_key,
    const nlohmann::json& telemetry_data) {
    
    if (!config_.enable_telemetry) {
        return true; // Success if telemetry disabled
    }
    
    spdlog::debug("Reporting telemetry to server");
    
    json payload;
    payload["license_key"] = license_key;
    payload["telemetry"] = telemetry_data;
    payload["timestamp"] = std::chrono::system_clock::to_time_t(
        std::chrono::system_clock::now()
    );
    
    auto response = httpPost("/telemetry", payload);
    return response.has_value();
}

std::optional<std::string> LicenseValidationClient::httpPost(
    const std::string& endpoint,
    const nlohmann::json& payload) {
    
#ifdef CURL_FOUND
    CURL* curl = curl_easy_init();
    if (!curl) {
        spdlog::error("Failed to initialize CURL");
        return std::nullopt;
    }
    
    std::string response_string;
    std::string url = config_.server_url + endpoint;
    std::string payload_str = payload.dump();
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload_str.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, config_.timeout_seconds);
    
    // SSL verification
    if (!config_.ca_cert_path.empty()) {
        curl_easy_setopt(curl, CURLOPT_CAINFO, config_.ca_cert_path.c_str());
    }
    
    // Retry logic
    CURLcode res = CURLE_FAILED_INIT;
    for (int attempt = 0; attempt < config_.retry_count; ++attempt) {
        res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            break;
        }
        spdlog::warn("HTTP request attempt {} failed: {}", 
            attempt + 1, curl_easy_strerror(res));
        
        if (attempt < config_.retry_count - 1) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        spdlog::error("HTTP request failed after {} attempts", config_.retry_count);
        return std::nullopt;
    }
    
    return response_string;
#else
    spdlog::error("CURL not available - cannot validate license online");
    return std::nullopt;
#endif
}

} // namespace enterprise
} // namespace themis
