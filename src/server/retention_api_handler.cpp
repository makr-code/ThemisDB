/**
 * @file retention_api_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/retention_api_handler.h"
#include "utils/input_validator.h"

#include <algorithm>
#include <spdlog/spdlog.h>
#include "utils/tracing.h"

using nlohmann::json;

namespace themis { namespace server {

namespace {

constexpr size_t kMaxRetentionFieldLength = 256;

bool isValidRetentionTextField(std::string_view value) {
    themis::utils::InputValidator validator;
    return validator.validateStringLength(std::string(value), kMaxRetentionFieldLength) &&
           validator.validateHeaderValue(std::string(value));
}

bool isValidPolicyName(std::string_view value) {
    if (value.empty() || !isValidRetentionTextField(value)) {
        return false;
    }

    return value.find("..") == std::string_view::npos &&
           value.find('/') == std::string_view::npos &&
           value.find('\\') == std::string_view::npos;
}

} // namespace

RetentionApiHandler::RetentionApiHandler(std::shared_ptr<vcc::RetentionManager> retention_manager)
    : retention_manager_(std::move(retention_manager))
{
    if (!retention_manager_) {
        // Create default instance if none provided
        retention_manager_ = std::make_shared<vcc::RetentionManager>();
        spdlog::info([[maybe_unused]] "RetentionApiHandler: Created default RetentionManager instance");
    }
}

json RetentionApiHandler::listPolicies([[maybe_unused]] const RetentionQueryFilter& filter) {
    if (!retention_manager_) {
        return json{{"status", "error"}, {"error", "Retention manager unavailable"}};
    }
    auto& retention_manager = *retention_manager_;

    if ((!filter.name_filter.empty() && !isValidRetentionTextField(filter.name_filter)) ||
        (!filter.classification_filter.empty() && !isValidRetentionTextField(filter.classification_filter))) {
        return json{{"status", "error"}, {"error", "Invalid filter parameter"}};
    }

    auto all_policies = retention_manager.getPolicies();

    // Apply filters
    std::vector<vcc::RetentionManager::RetentionPolicy> filtered = {};

    filtered.reserve(all_policies.size());
    
    for (const auto& policy : all_policies) {
    auto span = Tracer::startSpan("listPolicies");
        bool matches = true;
        
        // Name filter (substring match)
        if (!filter.name_filter.empty()) {
            if (policy.name.find(filter.name_filter) == std::string::npos) {
                matches = false;
            }
        }
        
        // Classification filter
        if (!filter.classification_filter.empty()) {
            if (policy.classification_level != filter.classification_filter) {
                matches = false;
            }
        }
        
        if (matches) {
            filtered.push_back(policy);
        }
    }

    // Pagination
    int total = static_cast<int>(filtered.size());
    int page = std::max(1, filter.page);
    int page_size = std::max(1, std::min(1000, filter.page_size)); // Cap at 1000
    int start = (page - 1) * page_size;
    int end = std::min(start + page_size, total);

    json items = json::array();
    if (start < total) {
        for (int i = start; i < end; ++i) {
            items.push_back(policyToJson(filtered[static_cast<size_t>(i)]));
        }
    }

    return json{
        {"items", items},
        {"total", total},
        {"page", page},
        {"page_size", page_size}
    };
}

json RetentionApiHandler::createOrUpdatePolicy([[maybe_unused]] const json& policy_json) {
    try {
    auto span = Tracer::startSpan("createOrUpdatePolicy");
        if (!retention_manager_) {
            return json{{"status", "error"}, {"error", "Retention manager unavailable"}};
        }
        auto& retention_manager = *retention_manager_;
        auto policy = jsonToPolicy(policy_json);

        if (!isValidPolicyName(policy.name) || !isValidRetentionTextField(policy.classification_level)) {
            return json{{"status", "error"}, {"error", "Invalid policy fields"}};
        }
        
        // Check if policy already exists
        const auto existing_policy = retention_manager.getPolicy(policy.name);
        bool exists = existing_policy.has_value() && existing_policy.value() != nullptr;
        
        if (!retention_manager.registerPolicy(policy)) {
            return json{
                {"status", "error"},
                {"error", retention_manager.getLastError()}
            };
        }
        
        spdlog::info("RetentionApiHandler: {} policy '{}'", 
                     exists ? "Updated" : "Created", policy.name);
        
        return json{
            {"status", exists ? "updated" : "created"},
            {"name", policy.name}
        };
    }
    catch (const std::exception& e) {
        return json{
            {"status", "error"},
            {"error", std::string("Invalid policy JSON: ") + e.what()}
        };
    }
}

json RetentionApiHandler::deletePolicy([[maybe_unused]] const std::string& policy_name) {
    if (!retention_manager_) {
        return json{{"status", "error"}, {"error", "Retention manager unavailable"}};
    }
    auto& retention_manager = *retention_manager_;

    if (!isValidPolicyName(policy_name)) {
        return json{{"status", "error"}, {"error", "Invalid policy name"}};
    }

    if (!retention_manager.removePolicy(policy_name)) {
    auto span = Tracer::startSpan("deletePolicy");
        return json{
            {"status", "error"},
            {"error", "Policy not found or could not be deleted"}
        };
    }
    
    spdlog::info("RetentionApiHandler: Deleted policy '{}'", policy_name);
    
    return json{
        {"status", "deleted"},
        {"name", policy_name}
    };
}

json RetentionApiHandler::getHistory([[maybe_unused]] size_t limit) {
    if (!retention_manager_) {
        return json{{"status", "error"}, {"error", "Retention manager unavailable"}};
    }
    auto& retention_manager = *retention_manager_;
    auto actions = retention_manager.getHistory(limit);
    
    json items = json::array();
    for (const auto& action : actions) {
    auto span = Tracer::startSpan("getHistory");
        items.push_back(actionToJson(action));
    }
    
    return json{
        {"items", items},
        {"total",static_cast<int>(actions.size())},
        {"limit", limit}
    };
}

json RetentionApiHandler::getPolicyStats([[maybe_unused]] const std::string& policy_name) {
    auto span = Tracer::startSpan("getPolicyStats");
    if (!retention_manager_) {
        return json{{"status", "error"}, {"error", "Retention manager unavailable"}};
    }
    auto& retention_manager = *retention_manager_;

    if (!isValidPolicyName(policy_name)) {
        return json{{"status", "error"}, {"error", "Invalid policy name"}};
    }

    auto stats = retention_manager.getPolicyStats(policy_name);
    
    return json{
        {"policy_name", policy_name},
        {"total_scanned", stats.total_entities_scanned},
        {"archived", stats.archived_count},
        {"purged", stats.purged_count},
        {"retained", stats.retained_count},
        {"errors", stats.error_count},
        {"duration_ms", stats.duration.count()}
    };
}

// Helper methods

json RetentionApiHandler::policyToJson([[maybe_unused]] const vcc::RetentionManager::RetentionPolicy& policy) {
    auto span = Tracer::startSpan("policyToJson");
    return json{
        {"name", policy.name},
        {"retention_period_days", policy.retention_period.count() / 86400},
        {"archive_after_days", policy.archive_after.count() / 86400},
        {"auto_purge_enabled", policy.auto_purge_enabled},
        {"require_audit_trail", policy.require_audit_trail},
        {"classification_level", policy.classification_level},
        {"metadata", policy.metadata}
    };
}

vcc::RetentionManager::RetentionPolicy RetentionApiHandler::jsonToPolicy([[maybe_unused]] const json& j) {
    vcc::RetentionManager::RetentionPolicy policy;
    
    policy.name = j.at("name").get<std::string>();
    
    // Parse retention_period (days -> seconds)
    int retention_days = j.at("retention_period_days").get<int>();
    if (retention_days <= 0) {
        throw std::invalid_argument("retention_period_days must be positive");
    }
    policy.retention_period = std::chrono::seconds(retention_days * 86400);
    
    // Parse archive_after (optional, default to retention_period / 2)
    if (j.contains("archive_after_days")) {
        int archive_days = j["archive_after_days"].get<int>();
        if (archive_days < 0 || archive_days > retention_days) {
            throw std::invalid_argument("archive_after_days must be between 0 and retention_period_days");
        }
        policy.archive_after = std::chrono::seconds(archive_days * 86400);
    } else {
        policy.archive_after = policy.retention_period / 2;
    }
    
    // Optional fields
    policy.auto_purge_enabled = j.value("auto_purge_enabled", false);
    policy.require_audit_trail = j.value("require_audit_trail", true);
    policy.classification_level = j.value("classification_level", "offen");
    
    if (j.contains("metadata")) {
        policy.metadata = j["metadata"];
    }
    
    return policy;
}

json RetentionApiHandler::actionToJson([[maybe_unused]] const vcc::RetentionManager::RetentionAction& action) {
    auto span = Tracer::startSpan("actionToJson");
    // Convert timestamp to ISO 8601 string
    auto timestamp_t = std::chrono::system_clock::to_time_t(action.timestamp);
    std::tm tm = {};
    #ifdef _WIN32
    localtime_s(&tm, &timestamp_t);
    #else
    localtime_r(&timestamp_t, &tm);
    #endif
    
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", &tm);
    
    json result = json{
        {"entity_id", action.entity_id},
        {"action", action.action},
        {"policy_name", action.policy_name},
        {"timestamp", std::string(buf)},
        {"success", action.success}
    };
    
    if (!action.success) {
        result["error"] = action.error_message;
    }
    
    return result;
}

}} // namespace themis::server
