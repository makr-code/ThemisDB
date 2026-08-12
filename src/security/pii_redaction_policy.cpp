/**
 * @file pii_redaction_policy.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/pii_redaction_policy.h"
#include "utils/pii_detection_engine.h"
#include <algorithm>
#include <cstdlib>
#include <spdlog/spdlog.h>

namespace themis {
namespace security {

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------

PIIRedactionPolicy& PIIRedactionPolicy::get() {
    static PIIRedactionPolicy instance;
    return instance;
}

PIIRedactionPolicy::PIIRedactionPolicy()
    : strict_mode_(false) {
    // Honour environment variable set before process start.
    const char* env = std::getenv("THEMIS_PII_STRICT");
    if (env != nullptr && std::string(env) == "1") {
        strict_mode_ = true;
    }

    // Lazily construct the detector with the default config path.
    // Falls back to embedded regex defaults when the YAML is absent.
    detector_ = std::make_shared<themis::utils::PIIDetector>(
        "config/pii_patterns.yaml");

    spdlog::info("PIIRedactionPolicy: Initialised (strict_mode={})", strict_mode_);
}

// ---------------------------------------------------------------------------
// Policy management
// ---------------------------------------------------------------------------

bool PIIRedactionPolicy::reload(const std::string& config_path) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (detector_) {
        bool ok = detector_->reload(config_path);
        if (ok) {
            spdlog::info("PIIRedactionPolicy: Reloaded from '{}'",
                         config_path.empty() ? "<default>" : config_path);
        } else {
            spdlog::warn("PIIRedactionPolicy: Reload failed – retaining previous config");
        }
        return ok;
    }
    return false;
}

bool PIIRedactionPolicy::isStrictMode() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return strict_mode_;
}

void PIIRedactionPolicy::setStrictMode(bool strict) {
    std::lock_guard<std::mutex> lock(mutex_);
    strict_mode_ = strict;
}

// ---------------------------------------------------------------------------
// Internal helper
// ---------------------------------------------------------------------------

std::string PIIRedactionPolicy::applyRedaction(const std::string& text) const {
    // detector_ is initialised in the constructor and never null afterwards.
    auto findings = detector_->detectInText(text);

    if (findings.empty()) {
        return text;
    }

    // Sort findings by start offset (detectInText already returns them sorted,
    // but be defensive here).
    std::sort(findings.begin(), findings.end(),
              [](const themis::utils::PIIFinding& a,
                 const themis::utils::PIIFinding& b) {
                  return a.start_offset < b.start_offset;
              });

    std::string result;
    result.reserve(text.size());

    size_t pos = 0;
    for (const auto& f : findings) {
        if (f.start_offset < pos) {
            // Overlapping finding – skip (deduplication already happened in
            // detectInText but guard here as well).
            continue;
        }
        // Copy the non-PII segment verbatim.
        result.append(text, pos, f.start_offset - pos);

        // Produce the masked replacement.
        std::string mode = strict_mode_ ? "strict"
                                        : detector_->getRedactionRecommendation(f.type);
        result.append(themis::utils::PIITypeUtils::maskValue(f.type, f.value, mode));

        pos = f.end_offset;
    }

    // Append any trailing non-PII content.
    if (pos < text.size()) {
        result.append(text, pos, text.size() - pos);
    }

    return result;
}

// ---------------------------------------------------------------------------
// Public redaction methods
// ---------------------------------------------------------------------------

std::string PIIRedactionPolicy::redactForLog(const std::string& message) const {
    std::lock_guard<std::mutex> lock(mutex_);
    return applyRedaction(message);
}

std::map<std::string, std::string> PIIRedactionPolicy::redactAttributes(
    const std::map<std::string, std::string>& attributes) const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::map<std::string, std::string> result;
    for (const auto& [key, value] : attributes) {
        // If the attribute key itself is a recognised PII field name, replace
        // the entire value with the appropriate mask rather than a substring
        // replacement – this covers cases where the value is exactly a PII
        // token (e.g. key="email", value="alice@example.com").
        auto field_type = detector_->classifyFieldName(key);
        if (field_type != themis::utils::PIIType::UNKNOWN) {
            std::string mode = strict_mode_ ? "strict"
                                            : detector_->getRedactionRecommendation(field_type);
            result[key] = themis::utils::PIITypeUtils::maskValue(field_type, value, mode);
        } else {
            result[key] = applyRedaction(value);
        }
    }
    return result;
}

std::map<std::string, std::string> PIIRedactionPolicy::redactLabels(
    const std::map<std::string, std::string>& labels) const {
    // Delegate to redactAttributes – identical semantics.
    return redactAttributes(labels);
}

std::string PIIRedactionPolicy::redactAttributeValue(
    const std::string& key, const std::string& value) const {
    std::lock_guard<std::mutex> lock(mutex_);
    // Mirror the logic in redactAttributes() for a single key/value pair,
    // without the overhead of building a temporary map.
    auto field_type = detector_->classifyFieldName(key);
    if (field_type != themis::utils::PIIType::UNKNOWN) {
        std::string mode = strict_mode_ ? "strict"
                                        : detector_->getRedactionRecommendation(field_type);
        return themis::utils::PIITypeUtils::maskValue(field_type, value, mode);
    }
    return applyRedaction(value);
}

} // namespace security
} // namespace themis
