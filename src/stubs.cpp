/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stubs.cpp                                          ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:10:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   85.0/100                                       ║
    • Total Lines:     128                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Stub implementations for linking purposes
// These stubs allow themis_tests to link successfully
// They are minimal implementations that prevent linker errors

#include <memory>
#include <map>
#include <string>
#include <vector>
#include "security/pii_redaction_policy.h"
#include "utils/audit_logger.h"

namespace themis::llm::lora {

struct Feedback {
    // Stub
};

struct TrainingTriggerPlugin {
    virtual ~TrainingTriggerPlugin() = default;
};

struct CacheAwareWeightingPlugin {
    virtual ~CacheAwareWeightingPlugin() = default;
    virtual void process(Feedback&) {}
};

struct LoRATrainingConfig {
    static LoRATrainingConfig loadFromFile(const std::string&) {
        return {};
    }
    
    std::shared_ptr<TrainingTriggerPlugin> createTrainingTriggerPlugin(const std::string&) const {
        return std::make_shared<TrainingTriggerPlugin>();
    }
    
    std::shared_ptr<CacheAwareWeightingPlugin> createCacheWeightingPlugin(const std::string&) const {
        return std::make_shared<CacheAwareWeightingPlugin>();
    }
};

struct FeedbackStorageService {
    std::vector<Feedback> getFeedbackForAdapter(const std::string&, unsigned __int64) const {
        return {};
    }
};

} // namespace themis::llm::lora

namespace themis::security {

PIIRedactionPolicy& PIIRedactionPolicy::get() {
    static PIIRedactionPolicy instance;
    return instance;
}

PIIRedactionPolicy::PIIRedactionPolicy() : strict_mode_(false) {}

std::string PIIRedactionPolicy::redactForLog(const std::string& message) const {
    return message;
}

std::map<std::string, std::string> PIIRedactionPolicy::redactAttributes(
    const std::map<std::string, std::string>& attributes) const {
    return attributes;
}

std::string PIIRedactionPolicy::redactAttributeValue(const std::string&, const std::string& value) const {
    return value;
}

std::map<std::string, std::string> PIIRedactionPolicy::redactLabels(
    const std::map<std::string, std::string>& labels) const {
    return labels;
}

bool PIIRedactionPolicy::reload(const std::string&) {
    return true;
}

bool PIIRedactionPolicy::isStrictMode() const {
    return strict_mode_;
}

void PIIRedactionPolicy::setStrictMode(bool strict) {
    strict_mode_ = strict;
}

std::string PIIRedactionPolicy::applyRedaction(const std::string& text) const {
    return text;
}

} // namespace themis::security

namespace themis::utils {

void AuditLogger::logSecurityEvent(
    [[maybe_unused]] SecurityEventType event_type,
    [[maybe_unused]] const std::string& user_id,
    [[maybe_unused]] const std::string& resource,
    [[maybe_unused]] const nlohmann::json& details) {
}

} // namespace themis::utils

