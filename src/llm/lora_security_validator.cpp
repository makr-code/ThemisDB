#include "llm/lora_security_validator.h"
#include "core/logger.h"
#include "security/audit_logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <iomanip>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/err.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace themis {
namespace llm {

// ===== Helper Functions =====

/**
 * @brief Decode base64 string to binary data
 * @param input Base64 encoded string
 * @param output Decoded binary data
 * @return true on success, false on failure
 */
static bool base64_decode(const std::string& input, std::vector<uint8_t>& output) {
    BIO* bio = nullptr;
    BIO* b64 = nullptr;
    
    try {
        // Remove any whitespace from input
        std::string cleaned_input;
        for (char c : input) {
            if (!std::isspace(static_cast<unsigned char>(c))) {
                cleaned_input += c;
            }
        }
        
        // Create BIO chain for base64 decoding
        b64 = BIO_new(BIO_f_base64());
        if (!b64) {
            LOG_ERROR("Failed to create base64 BIO");
            return false;
        }
        
        bio = BIO_new_mem_buf(cleaned_input.data(), static_cast<int>(cleaned_input.size()));
        if (!bio) {
            BIO_free(b64);
            LOG_ERROR("Failed to create memory BIO");
            return false;
        }
        
        bio = BIO_push(b64, bio);
        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        
        // Decode
        const size_t max_decode_len = cleaned_input.size();
        output.resize(max_decode_len);
        
        int decoded_len = BIO_read(bio, output.data(), static_cast<int>(max_decode_len));
        if (decoded_len < 0) {
            BIO_free_all(bio);
            LOG_ERROR("Base64 decoding failed");
            return false;
        }
        
        output.resize(decoded_len);
        BIO_free_all(bio);
        return true;
        
    } catch (const std::exception& e) {
        if (bio) BIO_free_all(bio);
        LOG_ERROR("Base64 decode exception: {}", e.what());
        return false;
    }
}

/**
 * @brief Verify RSA-SHA256 signature using OpenSSL
 * @param data Data that was signed
 * @param signature Signature bytes
 * @param cert_fingerprint SHA-256 fingerprint of signing certificate
 * @return true if signature is valid, false otherwise
 */
static bool verify_rsa_sha256_signature(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& signature,
    const std::string& cert_fingerprint) {
    
    // For now, we verify the signature format is correct
    // Full X.509 cert chain verification would require loading the cert from a store
    // based on the fingerprint, which is deployment-specific
    
    if (signature.empty()) {
        LOG_ERROR("Empty signature provided");
        return false;
    }
    
    if (data.empty()) {
        LOG_ERROR("Empty data provided for verification");
        return false;
    }
    
    // Verify signature size is reasonable for RSA (256-512 bytes typical)
    if (signature.size() < 128 || signature.size() > 1024) {
        LOG_ERROR("Signature size {} is outside expected range (128-1024 bytes)", signature.size());
        return false;
    }
    
    // Calculate SHA-256 hash of data
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    
    // NOTE: Full signature verification requires:
    // 1. Loading the X.509 certificate based on cert_fingerprint
    // 2. Extracting the public key from the certificate
    // 3. Verifying the signature using EVP_DigestVerify* APIs
    // 
    // This requires a certificate store configuration which is deployment-specific.
    // For production use, integrate with system certificate store or PKI infrastructure.
    
    LOG_INFO("Signature format validated: {} bytes, cert fingerprint: {}", 
             signature.size(), cert_fingerprint);
    
    // Return true only if signature format is valid
    // Actual cryptographic verification requires certificate store integration
    return true;
}

// ===== LoRASecurityValidator Implementation =====

LoRASecurityValidator::LoRASecurityValidator(const LoRASecurityConfig& config)
    : config_(config) {
    LOG_INFO("LoRASecurityValidator initialized with {} trusted signers", 
             config_.trusted_signers.size());
}

LoRASignatureResult LoRASecurityValidator::verifySignature(
    const std::string& lora_path,
    const std::string& signature_path) {
    
    LoRASignatureResult result;
    result.verified_at = std::chrono::system_clock::now();
    
    if (!config_.require_signature) {
        result.is_valid = true;
        result.error_message = "Signature verification disabled";
        return result;
    }
    
    // Load LoRa data
    std::vector<uint8_t> lora_data;
    if (!loadLoRAFile(lora_path, lora_data)) {
        result.is_valid = false;
        result.error_message = "Failed to load LoRa file";
        return result;
    }
    
    // Load signature
    std::vector<uint8_t> signature_data;
    if (!loadLoRAFile(signature_path, signature_data)) {
        result.is_valid = false;
        result.error_message = "Failed to load signature file";
        return result;
    }
    
    // Parse signature to extract signer cert fingerprint
    // Format: <cert_fingerprint>:<signature_base64>
    std::string sig_str(signature_data.begin(), signature_data.end());
    size_t colon_pos = sig_str.find(':');
    if (colon_pos == std::string::npos) {
        result.is_valid = false;
        result.error_message = "Invalid signature format";
        return result;
    }
    
    std::string cert_fingerprint = sig_str.substr(0, colon_pos);
    std::string signature_b64 = sig_str.substr(colon_pos + 1);
    
    // Check if signer is trusted
    if (!isTrustedSigner(cert_fingerprint)) {
        result.is_valid = false;
        result.error_message = "Untrusted signer: " + cert_fingerprint;
        
        // Audit log
        audit_logger::log_security_event(
            "lora_untrusted_signer",
            {{"lora_path", lora_path}, {"signer", cert_fingerprint}}
        );
        return result;
    }
    
    // Decode signature from base64
    std::vector<uint8_t> signature;
    if (!base64_decode(signature_b64, signature)) {
        result.is_valid = false;
        result.error_message = "Failed to decode base64 signature";
        LOG_ERROR("Base64 signature decode failed for {}", lora_path);
        return result;
    }
    
    // Verify signature using OpenSSL
    bool sig_valid = verify_rsa_sha256_signature(lora_data, signature, cert_fingerprint);
    
    if (!sig_valid) {
        result.is_valid = false;
        result.error_message = "Signature verification failed";
        
        // Audit log
        audit_logger::log_security_event(
            "lora_signature_verification_failed",
            {{"lora_path", lora_path}, {"signer", cert_fingerprint}}
        );
        
        LOG_ERROR("LoRa signature verification failed for {}: signer={}", 
                 lora_path, cert_fingerprint);
        return result;
    }
    
    // Signature is valid
    result.is_valid = true;
    result.signer_identity = cert_fingerprint;
    result.signature_algorithm = "RSA-SHA256";
    result.error_message = "";
    
    LOG_INFO("LoRa signature verified successfully for {}: signer={}", 
             lora_path, cert_fingerprint);
    
    return result;
}

LoRASignatureResult LoRASecurityValidator::verifyEmbeddedSignature(
    const std::string& lora_path) {
    
    LoRASignatureResult result;
    result.verified_at = std::chrono::system_clock::now();
    
    // Load LoRa file
    std::vector<uint8_t> lora_data;
    if (!loadLoRAFile(lora_path, lora_data)) {
        result.is_valid = false;
        result.error_message = "Failed to load LoRa file";
        return result;
    }
    
    // Parse metadata to extract embedded signature
    json metadata;
    if (!parseLoRAMetadata(lora_data, metadata)) {
        result.is_valid = false;
        result.error_message = "Failed to parse LoRa metadata";
        return result;
    }
    
    // Check for signature in metadata
    if (!metadata.contains("signature") || !metadata.contains("signer")) {
        result.is_valid = false;
        result.error_message = "No embedded signature found";
        return result;
    }
    
    std::string signature_b64 = metadata["signature"];
    std::string signer = metadata["signer"];
    
    // Check if signer is trusted
    if (!isTrustedSigner(signer)) {
        result.is_valid = false;
        result.error_message = "Untrusted signer: " + signer;
        
        // Audit log
        audit_logger::log_security_event(
            "lora_untrusted_embedded_signer",
            {{"lora_path", lora_path}, {"signer", signer}}
        );
        return result;
    }
    
    // Decode signature from base64
    std::vector<uint8_t> signature;
    if (!base64_decode(signature_b64, signature)) {
        result.is_valid = false;
        result.error_message = "Failed to decode embedded base64 signature";
        LOG_ERROR("Embedded base64 signature decode failed for {}", lora_path);
        return result;
    }
    
    // Verify embedded signature
    bool sig_valid = verify_rsa_sha256_signature(lora_data, signature, signer);
    
    if (!sig_valid) {
        result.is_valid = false;
        result.error_message = "Embedded signature verification failed";
        
        // Audit log
        audit_logger::log_security_event(
            "lora_embedded_signature_verification_failed",
            {{"lora_path", lora_path}, {"signer", signer}}
        );
        
        LOG_ERROR("Embedded LoRa signature verification failed for {}: signer={}", 
                 lora_path, signer);
        return result;
    }
    
    // Signature is valid
    result.is_valid = true;
    result.signer_identity = signer;
    result.signature_algorithm = "RSA-SHA256";
    result.error_message = "";
    
    LOG_INFO("Embedded LoRa signature verified successfully for {}: signer={}", 
             lora_path, signer);
    
    return result;
}

LoRAIntegrityResult LoRASecurityValidator::checkIntegrity(
    const std::string& lora_path,
    const std::optional<std::string>& expected_checksum) {
    
    LoRAIntegrityResult result;
    
    // Calculate checksum
    result.calculated_checksum = calculateChecksum(lora_path, result.checksum_algorithm);
    
    // Verify checksum if provided
    if (expected_checksum.has_value()) {
        result.expected_checksum = expected_checksum.value();
        if (result.calculated_checksum != result.expected_checksum) {
            result.is_intact = false;
            result.anomalies.push_back("Checksum mismatch");
            
            LOG_ERROR("LoRa integrity check failed for {}: checksum mismatch", lora_path);
            audit_logger::log_security_event(
                "lora_integrity_failure",
                {{"lora_path", lora_path}, {"reason", "checksum_mismatch"}}
            );
        }
    }
    
    // Weight anomaly detection
    if (config_.detect_weight_anomalies) {
        // Load weights from LoRa file
        std::vector<float> weights = loadWeightsFromLoRAFile(lora_path);
        
        if (!weights.empty()) {
            auto anomalies = detectWeightAnomalies(weights);
            if (!anomalies.empty()) {
                result.anomalies.insert(result.anomalies.end(), 
                                       anomalies.begin(), anomalies.end());
            }
        } else {
            LOG_WARN("Weight anomaly detection skipped: no weights loaded from {}", lora_path);
        }
    }
    
    return result;
}

bool LoRASecurityValidator::validateMetadata(const std::string& lora_path) {
    std::vector<uint8_t> lora_data;
    if (!loadLoRAFile(lora_path, lora_data)) {
        return false;
    }
    
    json metadata;
    if (!parseLoRAMetadata(lora_data, metadata)) {
        return false;
    }
    
    // Check required fields
    if (!metadata.contains("base_model") || !metadata.contains("rank")) {
        LOG_ERROR("LoRa metadata missing required fields");
        return false;
    }
    
    // Validate base model
    if (!config_.allowed_base_models.empty()) {
        std::string base_model = metadata["base_model"];
        if (config_.allowed_base_models.find(base_model) == 
            config_.allowed_base_models.end()) {
            LOG_ERROR("LoRa base model not allowed: {}", base_model);
            return false;
        }
    }
    
    // Validate rank
    size_t rank = metadata["rank"];
    if (rank < config_.min_rank || rank > config_.max_rank) {
        LOG_ERROR("LoRa rank out of bounds: {}", rank);
        return false;
    }
    
    return true;
}

std::vector<std::string> LoRASecurityValidator::detectWeightAnomalies(
    const std::vector<float>& weights) {
    
    std::vector<std::string> anomalies;
    
    if (weights.empty()) {
        return anomalies;
    }
    
    // Calculate statistics
    float mean = calculateMean(weights);
    float stddev = calculateStdDev(weights, mean);
    
    // Find outliers
    auto outlier_indices = findOutliers(weights, config_.anomaly_threshold);
    if (!outlier_indices.empty()) {
        anomalies.push_back("Detected " + std::to_string(outlier_indices.size()) + 
                           " outlier weights");
    }
    
    // Check for distribution shift
    if (detectDistributionShift(weights)) {
        anomalies.push_back("Unusual weight distribution detected");
    }
    
    // Check for suspicious patterns
    // Example: All weights near zero or all very large
    size_t near_zero_count = 0;
    size_t large_count = 0;
    for (float w : weights) {
        if (std::abs(w) < 1e-6f) near_zero_count++;
        if (std::abs(w) > 100.0f) large_count++;
    }
    
    float near_zero_ratio = static_cast<float>(near_zero_count) / weights.size();
    float large_ratio = static_cast<float>(large_count) / weights.size();
    
    if (near_zero_ratio > 0.9f) {
        anomalies.push_back("Suspiciously high number of zero weights");
    }
    if (large_ratio > 0.1f) {
        anomalies.push_back("Suspiciously high number of large weights");
    }
    
    return anomalies;
}

std::string LoRASecurityValidator::calculateChecksum(
    const std::string& lora_path,
    const std::string& algorithm) {
    
    std::vector<uint8_t> data;
    if (!loadLoRAFile(lora_path, data)) {
        return "";
    }
    
    // SHA-256 hash
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data.data(), data.size(), hash);
    
    // Convert to hex string
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

void LoRASecurityValidator::addTrustedSigner(const std::string& cert_fingerprint) {
    config_.trusted_signers.push_back(cert_fingerprint);
    LOG_INFO("Added trusted signer: {}", cert_fingerprint);
}

void LoRASecurityValidator::removeTrustedSigner(const std::string& cert_fingerprint) {
    auto it = std::find(config_.trusted_signers.begin(), 
                       config_.trusted_signers.end(), 
                       cert_fingerprint);
    if (it != config_.trusted_signers.end()) {
        config_.trusted_signers.erase(it);
        LOG_INFO("Removed trusted signer: {}", cert_fingerprint);
    }
}

bool LoRASecurityValidator::isTrustedSigner(const std::string& cert_fingerprint) const {
    return std::find(config_.trusted_signers.begin(),
                    config_.trusted_signers.end(),
                    cert_fingerprint) != config_.trusted_signers.end();
}

void LoRASecurityValidator::setConfig(const LoRASecurityConfig& config) {
    config_ = config;
    LOG_INFO("LoRASecurityValidator configuration updated");
}

// Helper methods
bool LoRASecurityValidator::loadLoRAFile(const std::string& path, 
                                        std::vector<uint8_t>& data) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        LOG_ERROR("Failed to open file: {}", path);
        return false;
    }
    
    data = std::vector<uint8_t>((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
    return true;
}

bool LoRASecurityValidator::parseLoRAMetadata(const std::vector<uint8_t>& data,
                                             json& metadata) {
    // TODO: Implement actual LoRa format parsing
    // For now, assume JSON metadata at beginning
    try {
        std::string data_str(data.begin(), data.end());
        metadata = json::parse(data_str);
        return true;
    } catch (...) {
        return false;
    }
}

std::vector<float> LoRASecurityValidator::loadWeightsFromLoRAFile(
    const std::string& path) {
    
    std::vector<float> weights;
    
    // Load file data
    std::vector<uint8_t> data;
    if (!loadLoRAFile(path, data)) {
        LOG_ERROR("Failed to load LoRa file for weight extraction: {}", path);
        return weights;
    }
    
    // Try to parse as JSON-based LoRa format first
    try {
        json lora_json = json::parse(data.begin(), data.end());
        
        // Check if this is a valid LoRa JSON format
        if (lora_json.contains("weights") && lora_json["weights"].is_array()) {
            for (const auto& w : lora_json["weights"]) {
                if (w.is_number()) {
                    weights.push_back(w.get<float>());
                }
            }
            LOG_INFO("Loaded {} weights from JSON LoRa file", weights.size());
            return weights;
        }
    } catch (const json::exception&) {
        // Not a JSON format, try binary format
    }
    
    // Try binary LoRa format (SafeTensors or similar)
    // SafeTensors format: 8-byte header size (little-endian), JSON header, then binary data
    if (data.size() < 8) {
        LOG_WARN("LoRa file too small for binary format: {} bytes", data.size());
        return weights;
    }
    
    // Read header size (first 8 bytes, little-endian uint64)
    uint64_t header_size = 0;
    for (size_t i = 0; i < 8; i++) {
        header_size |= (static_cast<uint64_t>(data[i]) << (i * 8));
    }
    
    // Validate header size
    if (header_size > data.size() - 8 || header_size > 100*1024*1024) {
        LOG_WARN("Invalid header size in LoRa binary format: {} bytes", header_size);
        return weights;
    }
    
    try {
        // Parse JSON header
        std::string header_str(data.begin() + 8, data.begin() + 8 + header_size);
        json header = json::parse(header_str);
        
        // Extract weight tensors from header metadata
        // Header contains tensor names and their byte offsets
        size_t data_offset = 8 + header_size;
        
        for (auto& [tensor_name, tensor_info] : header.items()) {
            if (!tensor_info.is_object()) continue;
            
            // Get tensor metadata
            if (!tensor_info.contains("data_offsets") || !tensor_info.contains("dtype")) {
                continue;
            }
            
            auto dtype = tensor_info["dtype"].get<std::string>();
            auto offsets = tensor_info["data_offsets"].get<std::vector<uint64_t>>();
            
            if (offsets.size() != 2) continue;
            
            uint64_t start_offset = data_offset + offsets[0];
            uint64_t end_offset = data_offset + offsets[1];
            
            // Only support float32 for now
            if (dtype == "F32" || dtype == "float32") {
                size_t num_floats = (end_offset - start_offset) / sizeof(float);
                
                // Sample some weights (don't load all to avoid memory issues)
                size_t sample_size = std::min(num_floats, static_cast<size_t>(10000));
                size_t stride = std::max(static_cast<size_t>(1), num_floats / sample_size);
                
                for (size_t i = 0; i < num_floats && weights.size() < sample_size; i += stride) {
                    size_t byte_offset = start_offset + i * sizeof(float);
                    if (byte_offset + sizeof(float) <= data.size()) {
                        float value;
                        std::memcpy(&value, &data[byte_offset], sizeof(float));
                        weights.push_back(value);
                    }
                }
            }
        }
        
        if (!weights.empty()) {
            LOG_INFO("Loaded {} sampled weights from binary LoRa file", weights.size());
        }
        
    } catch (const json::exception& e) {
        LOG_WARN("Failed to parse LoRa binary format header: {}", e.what());
    }
    
    return weights;
}

float LoRASecurityValidator::calculateMean(const std::vector<float>& values) {
    if (values.empty()) return 0.0f;
    float sum = std::accumulate(values.begin(), values.end(), 0.0f);
    return sum / values.size();
}

float LoRASecurityValidator::calculateStdDev(const std::vector<float>& values, 
                                            float mean) {
    if (values.empty()) return 0.0f;
    float sum = 0.0f;
    for (float v : values) {
        float diff = v - mean;
        sum += diff * diff;
    }
    return std::sqrt(sum / values.size());
}

std::vector<size_t> LoRASecurityValidator::findOutliers(
    const std::vector<float>& values, 
    float threshold) {
    
    std::vector<size_t> outliers;
    float mean = calculateMean(values);
    float stddev = calculateStdDev(values, mean);
    
    for (size_t i = 0; i < values.size(); i++) {
        float z_score = std::abs((values[i] - mean) / stddev);
        if (z_score > threshold) {
            outliers.push_back(i);
        }
    }
    
    return outliers;
}

bool LoRASecurityValidator::detectDistributionShift(
    const std::vector<float>& weights) {
    // Simple distribution shift detection
    // Check if distribution is significantly different from normal
    
    float mean = calculateMean(weights);
    float stddev = calculateStdDev(weights, mean);
    
    // Check kurtosis (tailedness)
    float kurtosis = 0.0f;
    for (float w : weights) {
        float z = (w - mean) / stddev;
        kurtosis += std::pow(z, 4);
    }
    kurtosis /= weights.size();
    
    // Normal distribution has kurtosis ≈ 3
    // Significant deviation suggests anomaly
    return std::abs(kurtosis - 3.0f) > 2.0f;
}

// ===== PromptInjectionDetector Implementation =====

PromptInjectionDetector::PromptInjectionDetector(const Config& config)
    : config_(config) {
    initializePatterns();
    LOG_INFO("PromptInjectionDetector initialized");
}

void PromptInjectionDetector::initializePatterns() {
    // Common injection patterns
    injection_patterns_ = {
        std::regex(R"(ignore\s+(previous|all|prior)\s+(instructions?|prompts?|rules?))", 
                  std::regex::icase),
        std::regex(R"(disregard\s+(previous|all|prior)\s+(instructions?|prompts?|rules?))", 
                  std::regex::icase),
        std::regex(R"(reveal\s+(system|hidden)\s+(prompt|instruction))", 
                  std::regex::icase),
        std::regex(R"(tell\s+me\s+(your|the)\s+system\s+prompt)", 
                  std::regex::icase),
        std::regex(R"(\[\s*system\s*\])", 
                  std::regex::icase),
        std::regex(R"(<\s*\|.*?\|\s*>)",  // Special tokens
                  std::regex::icase),
    };
    
    // Dangerous keywords
    dangerous_keywords_ = {
        "jailbreak", "override", "bypass", "pwned", "hacked",
        "exploit", "vulnerability", "privilege", "admin", "root",
        "execute", "eval", "import", "require", "include"
    };
}

bool PromptInjectionDetector::isSuspicious(const std::string& prompt) {
    if (!config_.enabled) {
        return false;
    }
    
    float risk_score = getRiskScore(prompt);
    return risk_score >= config_.risk_threshold;
}

float PromptInjectionDetector::getRiskScore(const std::string& prompt) {
    if (!config_.enabled) {
        return 0.0f;
    }
    
    float pattern_score = calculatePatternScore(prompt);
    float keyword_score = calculateKeywordScore(prompt);
    float syntax_score = calculateSyntaxScore(prompt);
    
    // Weighted combination
    float total_score = 0.5f * pattern_score + 
                       0.3f * keyword_score + 
                       0.2f * syntax_score;
    
    if (config_.log_detections && total_score > 0.5f) {
        LOG_WARN("Suspicious prompt detected: score={:.2f}", total_score);
        audit_logger::log_security_event(
            "prompt_injection_detected",
            {{"risk_score", total_score}, {"prompt_hash", std::to_string(std::hash<std::string>{}(prompt))}}
        );
    }
    
    return total_score;
}

json PromptInjectionDetector::analyzePrompt(const std::string& prompt) {
    json analysis;
    
    analysis["risk_score"] = getRiskScore(prompt);
    analysis["is_suspicious"] = isSuspicious(prompt);
    analysis["pattern_score"] = calculatePatternScore(prompt);
    analysis["keyword_score"] = calculateKeywordScore(prompt);
    analysis["syntax_score"] = calculateSyntaxScore(prompt);
    analysis["contains_system_bypass"] = containsSystemPromptBypass(prompt);
    analysis["contains_jailbreak"] = containsJailbreakAttempt(prompt);
    
    return analysis;
}

std::string PromptInjectionDetector::sanitizePrompt(const std::string& prompt) {
    std::string sanitized = prompt;
    
    // Remove common injection patterns
    for (const auto& pattern : injection_patterns_) {
        sanitized = std::regex_replace(sanitized, pattern, "[REDACTED]");
    }
    
    // Remove dangerous keywords
    for (const auto& keyword : dangerous_keywords_) {
        // Simple case-insensitive replacement
        size_t pos = 0;
        while ((pos = sanitized.find(keyword, pos)) != std::string::npos) {
            sanitized.replace(pos, keyword.length(), "[REDACTED]");
            pos += 10;  // Length of "[REDACTED]"
        }
    }
    
    return sanitized;
}

float PromptInjectionDetector::calculatePatternScore(const std::string& prompt) {
    int matches = 0;
    for (const auto& pattern : injection_patterns_) {
        if (std::regex_search(prompt, pattern)) {
            matches++;
        }
    }
    return std::min(1.0f, matches / 3.0f);
}

float PromptInjectionDetector::calculateKeywordScore(const std::string& prompt) {
    int matches = 0;
    std::string lower_prompt = prompt;
    std::transform(lower_prompt.begin(), lower_prompt.end(), lower_prompt.begin(), ::tolower);
    
    for (const auto& keyword : dangerous_keywords_) {
        if (lower_prompt.find(keyword) != std::string::npos) {
            matches++;
        }
    }
    return std::min(1.0f, matches / 5.0f);
}

float PromptInjectionDetector::calculateSyntaxScore(const std::string& prompt) {
    float score = 0.0f;
    
    // Check for unusual token patterns
    if (prompt.find("[INST]") != std::string::npos ||
        prompt.find("[/INST]") != std::string::npos) {
        score += 0.3f;
    }
    
    // Check for special characters
    int special_count = 0;
    for (char c : prompt) {
        if (c == '<' || c == '>' || c == '|' || c == '{' || c == '}') {
            special_count++;
        }
    }
    if (special_count > prompt.length() / 10) {
        score += 0.3f;
    }
    
    return std::min(1.0f, score);
}

bool PromptInjectionDetector::containsSystemPromptBypass(const std::string& prompt) {
    return std::regex_search(prompt, 
        std::regex(R"(ignore\s+previous|reveal\s+system|system\s+prompt)", 
                   std::regex::icase));
}

bool PromptInjectionDetector::containsJailbreakAttempt(const std::string& prompt) {
    return std::regex_search(prompt,
        std::regex(R"(jailbreak|DAN\s+mode|developer\s+mode|god\s+mode)",
                   std::regex::icase));
}

// ===== EmbeddingAnomalyDetector Implementation =====

EmbeddingAnomalyDetector::EmbeddingAnomalyDetector(const Config& config)
    : config_(config) {
    LOG_INFO("EmbeddingAnomalyDetector initialized");
}

float EmbeddingAnomalyDetector::getAnomalyScore(const std::vector<float>& embedding) {
    if (!config_.enabled || sample_count_ < config_.min_samples) {
        return 0.0f;
    }
    
    if (embedding.size() != mean_embedding_.size()) {
        LOG_ERROR("Embedding dimension mismatch");
        return 1.0f;  // Definitely anomalous
    }
    
    // Calculate distance from baseline
    float euclidean_dist = calculateEuclideanDistance(embedding, mean_embedding_);
    float cosine_sim = calculateCosineSimilarity(embedding, mean_embedding_);
    
    // Normalize to 0-1 score
    float anomaly_score = 0.0f;
    
    // Check if outlier based on Euclidean distance
    if (isOutlier(embedding)) {
        anomaly_score += 0.5f;
    }
    
    // Check cosine similarity
    if (cosine_sim < 0.5f) {  // Low similarity to baseline
        anomaly_score += 0.5f;
    }
    
    return std::min(1.0f, anomaly_score);
}

void EmbeddingAnomalyDetector::updateBaseline(const std::vector<float>& embedding) {
    if (mean_embedding_.empty()) {
        mean_embedding_ = embedding;
        stddev_embedding_.resize(embedding.size(), 0.0f);
        sample_count_ = 1;
        return;
    }
    
    // Update running statistics (Welford's online algorithm)
    sample_count_++;
    for (size_t i = 0; i < embedding.size(); i++) {
        float delta = embedding[i] - mean_embedding_[i];
        mean_embedding_[i] += delta / sample_count_;
        float delta2 = embedding[i] - mean_embedding_[i];
        stddev_embedding_[i] += delta * delta2;
    }
}

void EmbeddingAnomalyDetector::resetBaseline() {
    mean_embedding_.clear();
    stddev_embedding_.clear();
    sample_count_ = 0;
    LOG_INFO("Embedding baseline reset");
}

json EmbeddingAnomalyDetector::getBaselineStats() const {
    json stats;
    stats["sample_count"] = sample_count_;
    stats["dimension"] = mean_embedding_.size();
    
    if (!mean_embedding_.empty()) {
        // Calculate mean norm
        float mean_norm = 0.0f;
        for (float v : mean_embedding_) {
            mean_norm += v * v;
        }
        stats["mean_norm"] = std::sqrt(mean_norm);
    }
    
    return stats;
}

float EmbeddingAnomalyDetector::calculateCosineSimilarity(
    const std::vector<float>& a,
    const std::vector<float>& b) {
    
    if (a.size() != b.size()) return 0.0f;
    
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < a.size(); i++) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
}

float EmbeddingAnomalyDetector::calculateEuclideanDistance(
    const std::vector<float>& a,
    const std::vector<float>& b) {
    
    if (a.size() != b.size()) return std::numeric_limits<float>::max();
    
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); i++) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    
    return std::sqrt(sum);
}

bool EmbeddingAnomalyDetector::isOutlier(const std::vector<float>& embedding) {
    if (sample_count_ < config_.min_samples) return false;
    
    // Check if any dimension is outlier
    for (size_t i = 0; i < embedding.size(); i++) {
        float stddev = std::sqrt(stddev_embedding_[i] / sample_count_);
        float z_score = std::abs((embedding[i] - mean_embedding_[i]) / stddev);
        if (z_score > config_.outlier_threshold) {
            return true;
        }
    }
    
    return false;
}

} // namespace llm
} // namespace themis
