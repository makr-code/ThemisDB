/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            stubs.cpp                                          ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:58:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 RELEASE-CANDIDATE                            ║
    • Quality Score:   75.0/100                                       ║
    • Total Lines:     111                                            ║
    • Open Issues:     TODOs: 0, Stubs: 5                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ⚠️  Needs Work                                              ║
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

