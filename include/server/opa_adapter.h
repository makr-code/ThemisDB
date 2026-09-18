/**
 * @file opa_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/policy_engine.h"
#include <string>
#include <optional>

namespace themis {

class OpaAdapter : public PolicyEngine::IPolicyEvaluator {
public:
    struct Config {
        std::string endpoint_url = "http://localhost:8181";
        std::string policy_path  = "themis/authz/allow";
        long timeout_ms = 50;
    };

    /**
     * @brief Opa Adapter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit OpaAdapter(const Config& config);
    ~OpaAdapter() override;

    OpaAdapter(const OpaAdapter&)            = delete;
    OpaAdapter& operator=(const OpaAdapter&) = delete;
    OpaAdapter(OpaAdapter&&)                 = delete;
    OpaAdapter& operator=(OpaAdapter&&)      = delete;

    std::optional<PolicyEngine::Decision> evaluate(
        const std::string& user_id,
        const std::string& action,
        const std::string& resource_path,
        const std::optional<std::string>& client_ip,
        const std::optional<std::string>& user_agent) const override;

    const Config& getConfig() const { return config_; }

private:
    Config config_;

    /**
     * @brief Build Url.
     * @return Return value.
     */
    std::string buildUrl() const;

    /**
     * @brief Build Request Body.
     * @param[in] user_id Identifier of the user.
     * @param[in] action Input parameter.
     * @param[in] resource_path Path to the resource.
     * @param[in] client_ip Input parameter.
     * @param[in] user_agent Input parameter.
     * @return Return value.
     */
    static std::string buildRequestBody(
        const std::string& user_id,
        const std::string& action,
        const std::string& resource_path,
        const std::optional<std::string>& client_ip,
        const std::optional<std::string>& user_agent);

    /**
     * @brief Parse Opa Response.
     * @param[in] body Input parameter.
     * @return Return value.
     */
    static std::optional<bool> parseOpaResponse(const std::string& body);
};

} // namespace themis
