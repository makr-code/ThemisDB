/**
 * @file retention_api_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "utils/retention_manager.h"

namespace themis { namespace server {

struct RetentionQueryFilter {
    std::string name_filter;           // Filter by policy name (substring match)
    std::string classification_filter; // Filter by classification level
    int page = 1;
    int page_size = 100;
};

class RetentionApiHandler {
public:
    explicit RetentionApiHandler(std::shared_ptr<vcc::RetentionManager> retention_manager = nullptr);

    /**
     * @brief List Policies.
     * @param[in] filter Input parameter.
     * @return Return value.
     */
    nlohmann::json listPolicies(const RetentionQueryFilter& filter);

    /**
     * @brief Create Or Update Policy.
     * @param[in] policy_json Input parameter.
     * @return Return value.
     */
    nlohmann::json createOrUpdatePolicy(const nlohmann::json& policy_json);

    /**
     * @brief Delete Policy.
     * @param[in] policy_name Name of the retention policy.
     * @return Return value.
     */
    nlohmann::json deletePolicy(const std::string& policy_name);

    nlohmann::json getHistory(size_t limit = 100);

    /**
     * @brief Return the stored statistics for a retention policy.
     * @param[in] policy_name Name of the retention policy to query.
     * @return Stored statistics, or a default-initialized record if the policy is unknown.
     */
    nlohmann::json getPolicyStats(const std::string& policy_name);

private:
    std::shared_ptr<vcc::RetentionManager> retention_manager_;

    /**
     * @brief Policy To Json.
     * @param[in] policy Input parameter.
     * @return Return value.
     */
    nlohmann::json policyToJson(const vcc::RetentionManager::RetentionPolicy& policy);

    /**
     * @brief Json To Policy.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    vcc::RetentionManager::RetentionPolicy jsonToPolicy(const nlohmann::json& j);

    /**
     * @brief Action To Json.
     * @param[in] action Input parameter.
     * @return Return value.
     */
    nlohmann::json actionToJson(const vcc::RetentionManager::RetentionAction& action);
};

}} // namespace themis::server
