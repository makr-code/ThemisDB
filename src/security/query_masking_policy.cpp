/**
 * @file query_masking_policy.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/query_masking_policy.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace themis {
namespace security {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

QueryMaskingPolicy::QueryMaskingPolicy(const std::string& config_path)
    : QueryMaskingPolicy(Config{}, config_path)
{}

QueryMaskingPolicy::QueryMaskingPolicy(Config config, const std::string& config_path)
    : config_(std::move(config))
{
    try {
        detector_ = std::make_shared<utils::PIIDetector>(config_path);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            "Failed to initialize QueryMaskingPolicy: could not load PII patterns from '" +
            config_path + "': " + e.what());
    }
    spdlog::info("QueryMaskingPolicy: Initialised (enabled={}, auto_detect={}, mask_by_field_name={})",
                 config_.enabled, config_.auto_detect_pii, config_.mask_by_field_name);
}

std::shared_ptr<QueryMaskingPolicy> QueryMaskingPolicy::create(
    const std::string& config_path)
{
    return std::make_shared<QueryMaskingPolicy>(config_path);
}

std::shared_ptr<QueryMaskingPolicy> QueryMaskingPolicy::create(
    Config config,
    const std::string& config_path)
{
    return std::make_shared<QueryMaskingPolicy>(std::move(config), config_path);
}

// ---------------------------------------------------------------------------
// Explicit field declarations
// ---------------------------------------------------------------------------

void QueryMaskingPolicy::declareField(
    const std::string& field_name,
    const std::string& mask_mode,
    utils::PIIType pii_type)
{
    std::lock_guard<std::mutex> lock(mutex_);
    declared_fields_[field_name] = FieldMaskConfig{mask_mode, pii_type};
    spdlog::debug("QueryMaskingPolicy: declared field '{}' mask_mode='{}'", field_name, mask_mode);
}

void QueryMaskingPolicy::undeclareField(const std::string& field_name)
{
    std::lock_guard<std::mutex> lock(mutex_);
    declared_fields_.erase(field_name);
}

// ---------------------------------------------------------------------------
// Policy management
// ---------------------------------------------------------------------------

bool QueryMaskingPolicy::isEnabled() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return config_.enabled;
}

void QueryMaskingPolicy::setEnabled([[maybe_unused]] bool enabled)
{
    std::lock_guard<std::mutex> lock(mutex_);
    config_.enabled = enabled;
}

bool QueryMaskingPolicy::isPrivileged(const std::vector<std::string>& user_roles) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& role : user_roles) {
        if (config_.privileged_roles.count(role) > 0) {
            return true;
        }
    }
    return false;
}

const QueryMaskingPolicy::Config& QueryMaskingPolicy::config() const
{
    return config_;
}

// ---------------------------------------------------------------------------
// Core masking – public entry points
// ---------------------------------------------------------------------------

nlohmann::json QueryMaskingPolicy::maskResult(
    const nlohmann::json& result,
    const std::vector<std::string>& user_roles) const
{
    DeclaredFieldsSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!config_.enabled) {
            return result;
        }
        for (const auto& role : user_roles) {
            if (config_.privileged_roles.count(role) > 0) {
                return result;  // privileged roles see unmasked data
            }
        }
        // Take a consistent snapshot of declared_fields_ under the lock so
        // that maskNode/maskStringValue can read it without holding the mutex.
        snapshot = declared_fields_;
    }

    return maskNode(result, "", snapshot);
}

nlohmann::json QueryMaskingPolicy::maskResultSet(
    const nlohmann::json& results,
    const std::vector<std::string>& user_roles) const
{
    DeclaredFieldsSnapshot snapshot;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!config_.enabled) {
            return results;
        }
        for (const auto& role : user_roles) {
            if (config_.privileged_roles.count(role) > 0) {
                return results;
            }
        }
        snapshot = declared_fields_;
    }

    if (!results.is_array()) {
        return maskNode(results, "", snapshot);
    }

    nlohmann::json masked = nlohmann::json::array();
    for (const auto& item : results) {
        masked.push_back(maskNode(item, "", snapshot));
    }
    return masked;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

nlohmann::json QueryMaskingPolicy::maskNode(
    const nlohmann::json& node,
    const std::string& key,
    const DeclaredFieldsSnapshot& snapshot) const
{
    if (node.is_object()) {
        nlohmann::json out = nlohmann::json::object();
        for (auto it = node.begin(); it != node.end(); ++it) {
            out[it.key()] = maskNode(it.value(), it.key(), snapshot);
        }
        return out;
    }

    if (node.is_array()) {
        nlohmann::json out = nlohmann::json::array();
        for (const auto& elem : node) {
            out.push_back(maskNode(elem, key, snapshot));
        }
        return out;
    }

    if (node.is_string()) {
        std::string val = node.get<std::string>();
        return maskStringValue(val, key, snapshot);
    }

    // Non-string scalars (numbers, booleans, null) pass through unchanged.
    return node;
}

std::string QueryMaskingPolicy::maskStringValue(
    const std::string& value,
    const std::string& key,
    const DeclaredFieldsSnapshot& snapshot) const
{
    // 1. Explicit declared field takes highest priority.
    {
        auto it = snapshot.find(key);
        if (it != snapshot.end()) {
            const auto& cfg = it->second;
            utils::PIIType type = cfg.pii_type;
            // If no explicit type given, try to infer from field name.
            if (type == utils::PIIType::UNKNOWN) {
                type = detector_->classifyFieldName(key);
            }
            if (type == utils::PIIType::UNKNOWN) {
                type = utils::PIIType::PERSON_NAME;  // generic fallback
            }
            return utils::PIITypeUtils::maskValue(type, value, cfg.mask_mode);
        }
    }

    // 2. Field-name hint masking.
    if (config_.mask_by_field_name) {
        auto field_type = detector_->classifyFieldName(key);
        if (field_type != utils::PIIType::UNKNOWN) {
            std::string mode = detector_->getRedactionRecommendation(field_type);
            return utils::PIITypeUtils::maskValue(field_type, value, mode);
        }
    }

    // 3. Auto-detect PII in the value string.
    if (config_.auto_detect_pii && !value.empty()) {
        auto findings = detector_->detectInText(value);
        if (!findings.empty()) {
            // Sort by start offset (detector already returns them sorted, but be defensive).
            std::sort(findings.begin(), findings.end(),
                      [](const utils::PIIFinding& a, const utils::PIIFinding& b) {
                          return a.start_offset < b.start_offset;
                      });

            std::string result = {};
            result.reserve(value.size());
            size_t pos = 0;
            for (const auto& f : findings) {
                if (f.start_offset < pos) {
                    continue;  // overlapping finding already covered
                }
                // Verbatim prefix up to this finding.
                result.append(value, pos, f.start_offset - pos);
                // Masked replacement.
                std::string mode = detector_->getRedactionRecommendation(f.type);
                result.append(utils::PIITypeUtils::maskValue(f.type, f.value, mode));
                pos = f.end_offset;
            }
            // Remaining suffix.
            if (pos < value.size()) {
                result.append(value, pos, value.size() - pos);
            }
            return result;
        }
    }

    return value;
}

} // namespace security
} // namespace themis
