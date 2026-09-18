/**
 * @file pii_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <regex>
#include <vector>
#include <map>
#include <functional>

namespace themis::exporters {

class PIIDetector {
public:
    enum class PIIType {
        EMAIL,
        PHONE,
        SSN,
        CREDIT_CARD,
        IP_ADDRESS,
        CUSTOM
    };
    
    enum class RedactionStrategy {
        MASK,       // Replace with ***
        HASH,       // Replace with SHA-256 hash
        REMOVE,     // Remove entirely
        PARTIAL     // Keep first/last characters
    };
    
    struct PIIMatch {
        PIIType type;
        std::string value;
        size_t start_pos;
        size_t end_pos;
    };
    
    struct Config {
        bool detect_email = true;
        bool detect_phone = true;
        bool detect_ssn = true;
        bool detect_credit_card = true;
        bool detect_ip_address = false;
        
        RedactionStrategy default_strategy = RedactionStrategy::MASK;
        std::map<PIIType, RedactionStrategy> strategy_per_type;
        
        // Partial redaction config
        size_t partial_keep_prefix = 2;
        size_t partial_keep_suffix = 2;
    };
    
    /**
     * @brief PIIDetector.
     * @return Return value.
     */
    explicit PIIDetector();
    /**
     * @brief PIIDetector.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit PIIDetector(const Config& config);
    
    /**
     * @brief Detect PII.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<PIIMatch> detectPII(const std::string& text) const;
    
    /**
     * @brief Redact PII.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::string redactPII(const std::string& text) const;
    
    /**
     * @brief Redact PII.
     * @param[in] text Input parameter.
     * @param[in] strategy Input parameter.
     * @return Return value.
     */
    std::string redactPII(const std::string& text, RedactionStrategy strategy) const;
    
    /**
     * @brief Contains PII.
     * @param[in] text Input parameter.
     * @return True when the operation succeeds.
     */
    bool containsPII(const std::string& text) const;
    
    /**
     * @brief Get Strategy.
     * @param[in] type Input parameter.
     * @return Return value.
     */
    RedactionStrategy getStrategy(PIIType type) const;
    
private:
    Config config_;
    
    // Pattern matchers
    std::regex email_pattern_;
    std::regex phone_pattern_;
    std::regex ssn_pattern_;
    std::regex credit_card_pattern_;
    std::regex ip_pattern_;
    
    /**
     * @brief Init Patterns.
     */
    void initPatterns();
    /**
     * @brief Apply Redaction.
     * @param[in] value Input parameter.
     * @param[in] strategy Input parameter.
     * @return Return value.
     */
    std::string applyRedaction(const std::string& value, RedactionStrategy strategy) const;
    /**
     * @brief Mask String.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    std::string maskString(const std::string& value) const;
    /**
     * @brief Hash String.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    std::string hashString(const std::string& value) const;
    /**
     * @brief Partial Redact.
     * @param[in] value Input parameter.
     * @return Return value.
     */
    std::string partialRedact(const std::string& value) const;
};

struct PIIMetrics {
    size_t total_checks = 0;
    size_t pii_detected = 0;
    size_t pii_redacted = 0;
    std::map<PIIDetector::PIIType, size_t> detections_by_type;
    
    /**
     * @brief Record Detection.
     * @param[in] type Input parameter.
     * @details Implements recordDetection without additional internal calls.
     */
    void recordDetection(PIIDetector::PIIType type) {
        total_checks++;
        pii_detected++;
        detections_by_type[type]++;
    }
    
    /**
     * @brief Record Redaction.
     * @details Implements recordRedaction without additional internal calls.
     */
    void recordRedaction() {
        pii_redacted++;
    }
};

} // namespace themis::exporters
