/**
 * @file pii_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "exporters/pii_detector.h"
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

namespace themis::exporters {

PIIDetector::PIIDetector() : PIIDetector(Config{}) {}

PIIDetector::PIIDetector(const Config& config) : config_(config) {
    initPatterns();
}

void PIIDetector::initPatterns() {
    // Email pattern (simplified but practical)
    email_pattern_ = std::regex(
        R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})",
        std::regex::icase
    );
    
    // Phone patterns (US format, international format)
    phone_pattern_ = std::regex(
        R"((\+?1[-.\s]?)?\(?\d{3}\)?[-.\s]?\d{3}[-.\s]?\d{4})"
    );
    
    // SSN pattern (XXX-XX-XXXX)
    ssn_pattern_ = std::regex(
        R"(\b\d{3}-\d{2}-\d{4}\b)"
    );
    
    // Credit card pattern (basic, 13-19 digits with optional spaces/dashes)
    credit_card_pattern_ = std::regex(
        R"(\b(?:\d{4}[-\s]?){3}\d{4,7}\b)"
    );
    
    // IP address pattern (IPv4)
    ip_pattern_ = std::regex(
        R"(\b(?:\d{1,3}\.){3}\d{1,3}\b)"
    );
}

std::vector<PIIDetector::PIIMatch> PIIDetector::detectPII(const std::string& text) const {
    std::vector<PIIMatch> matches;
    
    if (config_.detect_email) {
        std::sregex_iterator it(text.begin(), text.end(), email_pattern_);
        std::sregex_iterator end = {};
        for (; it != end; ++it) {
            PIIMatch match;
            match.type = PIIType::EMAIL;
            match.value = it->str();
            match.start_pos = it->position();
            match.end_pos = it->position() + it->length();
            matches.push_back(match);
        }
    }
    
    if (config_.detect_phone) {
        std::sregex_iterator it(text.begin(), text.end(), phone_pattern_);
        std::sregex_iterator end = {};
        for (; it != end; ++it) {
            PIIMatch match;
            match.type = PIIType::PHONE;
            match.value = it->str();
            match.start_pos = it->position();
            match.end_pos = it->position() + it->length();
            matches.push_back(match);
        }
    }
    
    if (config_.detect_ssn) {
        std::sregex_iterator it(text.begin(), text.end(), ssn_pattern_);
        std::sregex_iterator end = {};
        for (; it != end; ++it) {
            PIIMatch match;
            match.type = PIIType::SSN;
            match.value = it->str();
            match.start_pos = it->position();
            match.end_pos = it->position() + it->length();
            matches.push_back(match);
        }
    }
    
    if (config_.detect_credit_card) {
        std::sregex_iterator it(text.begin(), text.end(), credit_card_pattern_);
        std::sregex_iterator end = {};
        for (; it != end; ++it) {
            PIIMatch match;
            match.type = PIIType::CREDIT_CARD;
            match.value = it->str();
            match.start_pos = it->position();
            match.end_pos = it->position() + it->length();
            matches.push_back(match);
        }
    }
    
    if (config_.detect_ip_address) {
        std::sregex_iterator it(text.begin(), text.end(), ip_pattern_);
        std::sregex_iterator end = {};
        for (; it != end; ++it) {
            PIIMatch match;
            match.type = PIIType::IP_ADDRESS;
            match.value = it->str();
            match.start_pos = it->position();
            match.end_pos = it->position() + it->length();
            matches.push_back(match);
        }
    }
    
    return matches;
}

std::string PIIDetector::redactPII(const std::string& text) const {
    return redactPII(text, config_.default_strategy);
}

std::string PIIDetector::redactPII(const std::string& text, RedactionStrategy strategy) const {
    auto matches = detectPII(text);
    
    if (matches.empty()) {
        return text;
    }
    
    // Sort matches by position (reverse order for replacement)
    std::sort(matches.begin(), matches.end(), 
        [](const PIIMatch& a, const PIIMatch& b) {
            return a.start_pos > b.start_pos;
        });
    
    std::string result = text;
    
    for (const auto& match : matches) {
        auto redaction_strategy = getStrategy(match.type);
        if (strategy != config_.default_strategy) {
            redaction_strategy = strategy;  // Override with specified strategy
        }
        
        std::string redacted = applyRedaction(match.value, redaction_strategy);
        result.replace(match.start_pos, match.end_pos - match.start_pos, redacted);
    }
    
    return result;
}

bool PIIDetector::containsPII(const std::string& text) const {
    return !detectPII(text).empty();
}

PIIDetector::RedactionStrategy PIIDetector::getStrategy(PIIType type) const {
    auto it = config_.strategy_per_type.find(type);
    if (it != config_.strategy_per_type.end()) {
        return it->second;
    }
    return config_.default_strategy;
}

std::string PIIDetector::applyRedaction(const std::string& value, RedactionStrategy strategy) const {
    switch (strategy) {
        case RedactionStrategy::MASK:
            return maskString(value);
        case RedactionStrategy::HASH:
            return hashString(value);
        case RedactionStrategy::REMOVE:
            return "[REDACTED]";
        case RedactionStrategy::PARTIAL:
            return partialRedact(value);
        default:
            return maskString(value);
    }
}

std::string PIIDetector::maskString(const std::string& value) const {
    return std::string(value.length(), '*');
}

std::string PIIDetector::hashString(const std::string& value) const {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(value.data()), value.size(), hash);
    
    std::ostringstream oss = {};
    oss << "SHA256:";
    // Use first 8 bytes for readability and space efficiency
    // Note: This reduces uniqueness but is acceptable for PII redaction
    // where the goal is anonymization, not cryptographic security
    for (int i = 0; i < 8; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return oss.str();
}

std::string PIIDetector::partialRedact(const std::string& value) const {
    if (value.length() <= config_.partial_keep_prefix + config_.partial_keep_suffix) {
        return maskString(value);
    }
    
    std::string result = {};
    result += value.substr(0, config_.partial_keep_prefix);
    result += std::string(value.length() - config_.partial_keep_prefix - config_.partial_keep_suffix, '*');
    result += value.substr(value.length() - config_.partial_keep_suffix);
    return result;
}

} // namespace themis::exporters
