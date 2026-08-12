/**
 * @file pii_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// PII (Personally Identifiable Information) detection and redaction
class PIIDetector {
public:
    /// PII category types
    enum class PIIType {
        EMAIL,
        PHONE,
        SSN,
        CREDIT_CARD,
        IP_ADDRESS,
        CUSTOM
    };
    
    /// Redaction strategy
    enum class RedactionStrategy {
        MASK,       // Replace with ***
        HASH,       // Replace with SHA-256 hash
        REMOVE,     // Remove entirely
        PARTIAL     // Keep first/last characters
    };
    
    /// PII detection result
    struct PIIMatch {
        PIIType type;
        std::string value;
        size_t start_pos;
        size_t end_pos;
    };
    
    /// Configuration for PII detection
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
    
    explicit PIIDetector();
    explicit PIIDetector(const Config& config);
    
    /// Detect PII in text
    std::vector<PIIMatch> detectPII(const std::string& text) const;
    
    /// Redact PII in text
    std::string redactPII(const std::string& text) const;
    
    /// Redact with specific strategy
    std::string redactPII(const std::string& text, RedactionStrategy strategy) const;
    
    /// Check if text contains PII
    bool containsPII(const std::string& text) const;
    
    /// Get redaction strategy for type
    RedactionStrategy getStrategy(PIIType type) const;
    
private:
    Config config_;
    
    // Pattern matchers
    std::regex email_pattern_;
    std::regex phone_pattern_;
    std::regex ssn_pattern_;
    std::regex credit_card_pattern_;
    std::regex ip_pattern_;
    
    void initPatterns();
    std::string applyRedaction(const std::string& value, RedactionStrategy strategy) const;
    std::string maskString(const std::string& value) const;
    std::string hashString(const std::string& value) const;
    std::string partialRedact(const std::string& value) const;
};

/// PII detection metrics
struct PIIMetrics {
    size_t total_checks = 0;
    size_t pii_detected = 0;
    size_t pii_redacted = 0;
    std::map<PIIDetector::PIIType, size_t> detections_by_type;
    
    void recordDetection(PIIDetector::PIIType type) {
        total_checks++;
        pii_detected++;
        detections_by_type[type]++;
    }
    
    void recordRedaction() {
        pii_redacted++;
    }
};

} // namespace themis::exporters
