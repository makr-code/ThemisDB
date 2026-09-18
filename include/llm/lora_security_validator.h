#pragma once

/**
 * @file lora_security_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/llm_model_audit_logger.h"
#include "llm/lora_certificate_store.h"

#include <chrono>
#include <memory>
#include <optional>
#include <regex>
#include <string>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

namespace themis {
namespace llm {

using json = nlohmann::json;

struct LoRASignatureResult {
    bool is_valid = false;
    std::string signer_identity;
    std::string signature_algorithm;
    std::string error_message;
    std::chrono::system_clock::time_point verified_at;
};

struct LoRAIntegrityResult {
    bool is_intact = true;
    std::string checksum_algorithm = "SHA-256";
    std::string calculated_checksum;
    std::string expected_checksum;
    std::vector<std::string> anomalies;  // Detected weight anomalies
};

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

class LoRASecurityValidator {
public:
    /**
     * @brief Lo RASecurity Validator.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit LoRASecurityValidator(const LoRASecurityConfig& config);
    /**
     * @brief Lo RASecurity Validator.
     * @return Return value.
     */
    virtual ~LoRASecurityValidator() = default;
    
    /**
     * @brief Verify Signature.
     * @param[in] lora_path Path to the lora.
     * @param[in] signature_path Path to the signature.
     * @return Return value.
     */
    LoRASignatureResult verifySignature(
        const std::string& lora_path,
        const std::string& signature_path
    );
    
    /**
     * @brief Verify Embedded Signature.
     * @param[in] lora_path Path to the lora.
     * @return Return value.
     */
    LoRASignatureResult verifyEmbeddedSignature(const std::string& lora_path);
    
    LoRAIntegrityResult checkIntegrity(
        const std::string& lora_path,
        const std::optional<std::string>& expected_checksum = std::nullopt
    );
    
    /**
     * @brief Validate Metadata.
     * @param[in] lora_path Path to the lora.
     * @return True when the operation succeeds.
     */
    virtual bool validateMetadata(const std::string& lora_path);
    
    /**
     * @brief Detect Weight Anomalies.
     * @param[in] weights Input parameter.
     * @return Return value.
     */
    std::vector<std::string> detectWeightAnomalies(
        const std::vector<float>& weights
    );
    
    std::string calculateChecksum(
        const std::string& lora_path,
        const std::string& algorithm = "SHA-256"
    );
    
    /**
     * @brief Add Trusted Signer.
     * @param[in] cert_fingerprint Input parameter.
     */
    void addTrustedSigner(const std::string& cert_fingerprint);
    
    /**
     * @brief Remove Trusted Signer.
     * @param[in] cert_fingerprint Input parameter.
     */
    void removeTrustedSigner(const std::string& cert_fingerprint);
    
    /**
     * @brief Is Trusted Signer.
     * @param[in] cert_fingerprint Input parameter.
     * @return True when the operation succeeds.
     */
    bool isTrustedSigner(const std::string& cert_fingerprint) const;
    
    LoRASecurityConfig getConfig() const { return config_; }
    
    /**
     * @brief Set Config.
     * @param[in] config Input parameter.
     */
    void setConfig(const LoRASecurityConfig& config);

    /**
     * @brief Set Audit Logger.
     * @param[in] logger Input parameter.
     */
    void setAuditLogger(const std::shared_ptr<LLMModelAuditLogger>& logger);

    /**
     * @brief Set Certificate Store.
     * @param[in] store Input parameter.
     */
    void setCertificateStore(std::shared_ptr<LoRACertificateStore> store);

    /**
     * @brief Get Certificate Store.
     * @return Return value.
     */
    std::shared_ptr<LoRACertificateStore> getCertificateStore() const;

private:
    LoRASecurityConfig config_;
    std::shared_ptr<LLMModelAuditLogger> audit_logger_;
    std::shared_ptr<LoRACertificateStore> cert_store_;
    
    // Helper methods
    /**
     * @brief Load Lo RAFile.
     * @param[in] path Input parameter.
     * @param[in,out] data Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool loadLoRAFile(const std::string& path, std::vector<uint8_t>& data);
    /**
     * @brief Parse Lo RAMetadata.
     * @param[in] data Input parameter.
     * @param[in,out] metadata Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool parseLoRAMetadata(const std::vector<uint8_t>& data, json& metadata);
    /**
     * @brief Load Weights From Lo RAFile.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::vector<float> loadWeightsFromLoRAFile(const std::string& path);
    
    /**
     * @brief Statistical helpers for anomaly detection
     * @param[in] values Input parameter.
     * @return Return value.
     */
    float calculateMean(const std::vector<float>& values);
    /**
     * @brief Calculate Std Dev.
     * @param[in] values Input parameter.
     * @param[in] mean Input parameter.
     * @return Return value.
     */
    float calculateStdDev(const std::vector<float>& values, float mean);
    /**
     * @brief Find Outliers.
     * @param[in] values Input parameter.
     * @param[in] threshold Input parameter.
     * @return Return value.
     */
    std::vector<size_t> findOutliers(const std::vector<float>& values, float threshold);
    /**
     * @brief Detect Distribution Shift.
     * @param[in] weights Input parameter.
     * @return True when the operation succeeds.
     */
    bool detectDistributionShift(const std::vector<float>& weights);
};

class PromptInjectionDetector {
public:
    struct Config {
        bool enabled = true;
        float risk_threshold = 0.7f;  // 0.0 = no risk, 1.0 = high risk
        bool block_high_risk = false;  // Block or just warn
        bool log_detections = true;
    };
    
    /**
     * @brief Prompt Injection Detector.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit PromptInjectionDetector(const Config& config);
    ~PromptInjectionDetector() = default;
    
    /**
     * @brief Is Suspicious.
     * @param[in] prompt Input parameter.
     * @return True when the operation succeeds.
     */
    bool isSuspicious(const std::string& prompt);
    
    /**
     * @brief Get Risk Score.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    float getRiskScore(const std::string& prompt);
    
    /**
     * @brief Analyze Prompt.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    json analyzePrompt(const std::string& prompt);
    
    /**
     * @brief Sanitize Prompt.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    std::string sanitizePrompt(const std::string& prompt);
    
private:
    Config config_;
    
    // Pattern matching
    std::vector<std::regex> injection_patterns_;
    std::vector<std::string> dangerous_keywords_;
    
    // Helper methods
    /**
     * @brief Initialize Patterns.
     */
    void initializePatterns();
    /**
     * @brief Calculate Pattern Score.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    float calculatePatternScore(const std::string& prompt);
    /**
     * @brief Calculate Keyword Score.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    float calculateKeywordScore(const std::string& prompt);
    /**
     * @brief Calculate Syntax Score.
     * @param[in] prompt Input parameter.
     * @return Return value.
     */
    float calculateSyntaxScore(const std::string& prompt);
    /**
     * @brief Contains System Prompt Bypass.
     * @param[in] prompt Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsSystemPromptBypass(const std::string& prompt);
    /**
     * @brief Contains Jailbreak Attempt.
     * @param[in] prompt Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsJailbreakAttempt(const std::string& prompt);
};

class EmbeddingAnomalyDetector {
public:
    struct Config {
        bool enabled = true;
        float outlier_threshold = 3.0f;  // Standard deviations from mean
        size_t min_samples = 100;  // Minimum samples for statistics
        bool use_isolation_forest = false;  // Advanced ML-based detection
    };
    
    /**
     * @brief Embedding Anomaly Detector.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit EmbeddingAnomalyDetector(const Config& config);
    ~EmbeddingAnomalyDetector() = default;
    
    /**
     * @brief Get Anomaly Score.
     * @param[in] embedding Input parameter.
     * @return Return value.
     */
    float getAnomalyScore(const std::vector<float>& embedding);
    
    /**
     * @brief Update Baseline.
     * @param[in] embedding Input parameter.
     */
    void updateBaseline(const std::vector<float>& embedding);
    
    /**
     * @brief Reset Baseline.
     */
    void resetBaseline();
    
    /**
     * @brief Get Baseline Stats.
     * @return Return value.
     */
    json getBaselineStats() const;
    
private:
    Config config_;
    
    // Baseline statistics
    std::vector<float> mean_embedding_;
    std::vector<float> stddev_embedding_;
    size_t sample_count_ = 0;
    
    // Helper methods
    /**
     * @brief Calculate Cosine Similarity.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    float calculateCosineSimilarity(
        const std::vector<float>& a,
        const std::vector<float>& b
    );
    /**
     * @brief Calculate Euclidean Distance.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return Return value.
     */
    float calculateEuclideanDistance(
        const std::vector<float>& a,
        const std::vector<float>& b
    );
    /**
     * @brief Is Outlier.
     * @param[in] embedding Input parameter.
     * @return True when the operation succeeds.
     */
    bool isOutlier(const std::vector<float>& embedding);
};

} // namespace llm
} // namespace themis
