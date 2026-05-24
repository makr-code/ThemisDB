/*
 * ThemisDB | File: lora_security_validator.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <regex>
#include <unordered_set>
#include <chrono>
#include <memory>
#include <nlohmann/json.hpp>
#include "llm/lora_certificate_store.h"

using json = nlohmann::json;

namespace themis {
namespace llm {

// Forward declaration to avoid a circular include with llm_model_audit_logger.h
class LLMModelAuditLogger;

/**
 * @file lora_security_validator.h
 * @brief Security validation for LoRa adapters
 * 
 * Provides security checks for LoRa adapters including:
 * - Signature verification
 * - Integrity checks
 * - Anomaly detection in weights
 * - Metadata validation
 */

/**
 * @brief LoRa signature verification result
 */
struct LoRASignatureResult {
    bool is_valid = false;
    std::string signer_identity;
    std::string signature_algorithm;
    std::string error_message;
    std::chrono::system_clock::time_point verified_at;
};

/**
 * @brief LoRa integrity check result
 */
struct LoRAIntegrityResult {
    bool is_intact = true;
    std::string checksum_algorithm = "SHA-256";
    std::string calculated_checksum;
    std::string expected_checksum;
    std::vector<std::string> anomalies;  // Detected weight anomalies
};

/**
 * @brief Security configuration for LoRa validation
 */
struct LoRASecurityConfig {
    // Signature verification
    bool require_signature = true;
    std::vector<std::string> trusted_signers;  // X.509 cert fingerprints
    bool enforce_cert_validity = true;

    // Certificate store paths
    // Local store: PEM files named <fingerprint>.pem
    std::string cert_store_path = "config/security/lora_certs/";
    // System store fallback (empty disables system-store lookup)
    std::string system_cert_store_path = "/etc/ssl/certs";
    
    // Integrity checks
    bool verify_checksum = true;
    bool detect_weight_anomalies = true;
    float anomaly_threshold = 0.95f;  // 95th percentile
    
    // Metadata validation
    bool validate_metadata = true;
    std::unordered_set<std::string> allowed_base_models;
    size_t max_adapter_size_mb = 1024;  // 1 GB
    size_t min_rank = 4;
    size_t max_rank = 128;
};

/**
 * @brief Security validator for LoRa adapters
 */
class LoRASecurityValidator {
public:
    explicit LoRASecurityValidator(const LoRASecurityConfig& config);
    /// @brief Virtual destructor to allow safe polymorphic use in tests and
    ///        custom validator implementations injected via Config::security_validator.
    virtual ~LoRASecurityValidator() = default;
    
    /**
     * @brief Verify signature of a LoRa adapter file
     * 
     * @param lora_path Path to LoRa adapter file
     * @param signature_path Path to detached signature file (.sig)
     * @return Signature verification result
     */
    LoRASignatureResult verifySignature(
        const std::string& lora_path,
        const std::string& signature_path
    );
    
    /**
     * @brief Verify embedded signature in LoRa metadata
     * 
     * @param lora_path Path to LoRa adapter file
     * @return Signature verification result
     */
    LoRASignatureResult verifyEmbeddedSignature(const std::string& lora_path);
    
    /**
     * @brief Check integrity of LoRa adapter
     * 
     * Performs:
     * - Checksum verification (if expected checksum provided)
     * - Weight anomaly detection
     * - Structural validation
     * 
     * @param lora_path Path to LoRa adapter file
     * @param expected_checksum Optional expected checksum
     * @return Integrity check result
     */
    LoRAIntegrityResult checkIntegrity(
        const std::string& lora_path,
        const std::optional<std::string>& expected_checksum = std::nullopt
    );
    
    /**
     * @brief Validate LoRa metadata
     * 
     * Checks:
     * - Base model compatibility
     * - Rank plausibility
     * - Size constraints
     * - Format version
     * 
     * Virtual to allow test doubles and custom validators to be injected via
     * MultiLoRAManager::Config::security_validator without subclassing the
     * full implementation.
     *
     * @param lora_path Path to LoRa adapter file
     * @return true if metadata is valid
     */
    virtual bool validateMetadata(const std::string& lora_path);
    
    /**
     * @brief Detect anomalies in LoRa weights
     * 
     * Statistical analysis of weight distributions:
     * - Outlier detection
     * - Distribution shifts
     * - Unusual patterns
     * 
     * @param weights Weight tensor data
     * @return List of detected anomalies
     */
    std::vector<std::string> detectWeightAnomalies(
        const std::vector<float>& weights
    );
    
    /**
     * @brief Calculate checksum of LoRa adapter
     * 
     * @param lora_path Path to LoRa adapter file
     * @param algorithm Checksum algorithm (SHA-256, SHA-512, etc.)
     * @return Hex-encoded checksum
     */
    std::string calculateChecksum(
        const std::string& lora_path,
        const std::string& algorithm = "SHA-256"
    );
    
    /**
     * @brief Add trusted signer certificate
     * 
     * @param cert_fingerprint SHA-256 fingerprint of X.509 cert
     */
    void addTrustedSigner(const std::string& cert_fingerprint);
    
    /**
     * @brief Remove trusted signer
     * 
     * @param cert_fingerprint SHA-256 fingerprint to remove
     */
    void removeTrustedSigner(const std::string& cert_fingerprint);
    
    /**
     * @brief Check if signer is trusted
     * 
     * @param cert_fingerprint SHA-256 fingerprint to check
     * @return true if signer is trusted
     */
    bool isTrustedSigner(const std::string& cert_fingerprint) const;
    
    /**
     * @brief Get security configuration
     * 
     * @return Current security configuration
     */
    LoRASecurityConfig getConfig() const { return config_; }
    
    /**
     * @brief Update security configuration
     * 
     * @param config New configuration
     */
    void setConfig(const LoRASecurityConfig& config);

    /**
     * @brief Attach an audit logger so that signature verification
     *        failures and successes are persisted via LLMModelAuditLogger.
     *
     * Optional: when not set, security events are only written to spdlog.
     * The logger is owned externally; the validator keeps a shared_ptr.
     *
     * @param logger Shared audit-logger instance (may be nullptr to detach)
     */
    void setAuditLogger(const std::shared_ptr<LLMModelAuditLogger>& logger);

    /**
     * @brief Replace the certificate store used for fingerprint lookups.
     *
     * By default the validator constructs its own LoRACertificateStore from
     * the paths in LoRASecurityConfig.  Call this to inject a custom or
     * pre-populated store (e.g., in tests).
     *
     * @param store Shared ownership of the certificate store.
     */
    void setCertificateStore(std::shared_ptr<LoRACertificateStore> store);

    /**
     * @brief Return the active certificate store.
     */
    std::shared_ptr<LoRACertificateStore> getCertificateStore() const;

private:
    LoRASecurityConfig config_;
    std::shared_ptr<LLMModelAuditLogger> audit_logger_;
    std::shared_ptr<LoRACertificateStore> cert_store_;
    
    // Helper methods
    bool loadLoRAFile(const std::string& path, std::vector<uint8_t>& data);
    bool parseLoRAMetadata(const std::vector<uint8_t>& data, json& metadata);
    std::vector<float> loadWeightsFromLoRAFile(const std::string& path);
    bool verifyX509Signature(
        const std::vector<uint8_t>& data,
        const std::vector<uint8_t>& signature,
        const std::string& cert_fingerprint
    );
    
    // Statistical helpers for anomaly detection
    float calculateMean(const std::vector<float>& values);
    float calculateStdDev(const std::vector<float>& values, float mean);
    std::vector<size_t> findOutliers(const std::vector<float>& values, float threshold);
    bool detectDistributionShift(const std::vector<float>& weights);
};

/**
 * @brief Prompt injection detector
 * 
 * Detects suspicious patterns in prompts that may indicate injection attacks.
 */
class PromptInjectionDetector {
public:
    struct Config {
        bool enabled = true;
        float risk_threshold = 0.7f;  // 0.0 = no risk, 1.0 = high risk
        bool block_high_risk = false;  // Block or just warn
        bool log_detections = true;
    };
    
    explicit PromptInjectionDetector(const Config& config);
    ~PromptInjectionDetector() = default;
    
    /**
     * @brief Check if prompt is suspicious
     * 
     * @param prompt User input or system prompt
     * @return true if prompt appears malicious
     */
    bool isSuspicious(const std::string& prompt);
    
    /**
     * @brief Calculate risk score for prompt
     * 
     * @param prompt User input or system prompt
     * @return Risk score between 0.0 (safe) and 1.0 (high risk)
     */
    float getRiskScore(const std::string& prompt);
    
    /**
     * @brief Get detailed analysis of prompt
     * 
     * @param prompt User input
     * @return JSON with detected patterns and scores
     */
    json analyzePrompt(const std::string& prompt);
    
    /**
     * @brief Sanitize prompt by removing suspicious patterns
     * 
     * @param prompt Original prompt
     * @return Sanitized prompt
     */
    std::string sanitizePrompt(const std::string& prompt);
    
private:
    Config config_;
    
    // Pattern matching
    std::vector<std::regex> injection_patterns_;
    std::vector<std::string> dangerous_keywords_;
    
    // Helper methods
    void initializePatterns();
    float calculatePatternScore(const std::string& prompt);
    float calculateKeywordScore(const std::string& prompt);
    float calculateSyntaxScore(const std::string& prompt);
    bool containsSystemPromptBypass(const std::string& prompt);
    bool containsJailbreakAttempt(const std::string& prompt);
};

/**
 * @brief Embedding anomaly detector
 * 
 * Detects anomalous embeddings in vector database that may indicate poisoning.
 */
class EmbeddingAnomalyDetector {
public:
    struct Config {
        bool enabled = true;
        float outlier_threshold = 3.0f;  // Standard deviations from mean
        size_t min_samples = 100;  // Minimum samples for statistics
        bool use_isolation_forest = false;  // Advanced ML-based detection
    };
    
    explicit EmbeddingAnomalyDetector(const Config& config);
    ~EmbeddingAnomalyDetector() = default;
    
    /**
     * @brief Check if embedding is anomalous
     * 
     * @param embedding Vector embedding to check
     * @return Anomaly score (0.0 = normal, 1.0 = highly anomalous)
     */
    float getAnomalyScore(const std::vector<float>& embedding);
    
    /**
     * @brief Update baseline statistics with new embedding
     * 
     * @param embedding Normal embedding to include in baseline
     */
    void updateBaseline(const std::vector<float>& embedding);
    
    /**
     * @brief Reset baseline statistics
     */
    void resetBaseline();
    
    /**
     * @brief Get baseline statistics
     * 
     * @return JSON with mean, stddev, samples count
     */
    json getBaselineStats() const;
    
private:
    Config config_;
    
    // Baseline statistics
    std::vector<float> mean_embedding_;
    std::vector<float> stddev_embedding_;
    size_t sample_count_ = 0;
    
    // Helper methods
    float calculateCosineSimilarity(
        const std::vector<float>& a,
        const std::vector<float>& b
    );
    float calculateEuclideanDistance(
        const std::vector<float>& a,
        const std::vector<float>& b
    );
    bool isOutlier(const std::vector<float>& embedding);
};

} // namespace llm
} // namespace themis
