/**
 * @file opa_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "server/policy_engine.h"
#include <string>
#include <optional>

namespace themis {

/**
 * @brief Adapter that evaluates authorization decisions via an OPA sidecar.
 *
 * Implements PolicyEngine::IPolicyEvaluator by posting a JSON input document
 * to `POST /v1/data/{policy_path}` on a local OPA server.  If OPA is
 * unreachable or returns a non-2xx response within the configured timeout,
 * evaluate() returns std::nullopt so the caller can fall back to native
 * PolicyEngine evaluation.
 *
 * OPA input format:
 * @code{.json}
 * {
 *   "input": {
 *     "user":       "alice",
 *     "action":     "read",
 *     "resource":   "/data/x",
 *     "client_ip":  "10.0.0.1",       // optional
 *     "user_agent": "ThemisClient/1.0" // optional
 *   }
 * }
 * @endcode
 *
 * OPA response:
 * @code{.json}
 * { "result": true }
 * @endcode
 *
 * Usage:
 * @code{.cpp}
 * OpaAdapter::Config cfg;
 * cfg.endpoint_url = "http://localhost:8181";
 * cfg.policy_path  = "themis/authz/allow";
 * cfg.timeout_ms   = 50;
 *
 * auto adapter = std::make_shared<OpaAdapter>(cfg);
 * engine.setOpaEvaluator(adapter.get());
 * @endcode
 */
class OpaAdapter : public PolicyEngine::IPolicyEvaluator {
public:
    struct Config {
        /// Base URL of the OPA server (e.g. "http://localhost:8181").
        std::string endpoint_url = "http://localhost:8181";
        /// Rego decision path below /v1/data/ (e.g. "themis/authz/allow").
        std::string policy_path  = "themis/authz/allow";
        /// Total request timeout in milliseconds (default: 50 ms).
        long timeout_ms = 50;
    };

    explicit OpaAdapter(const Config& config);
    ~OpaAdapter() override;

    OpaAdapter(const OpaAdapter&)            = delete;
    OpaAdapter& operator=(const OpaAdapter&) = delete;
    OpaAdapter(OpaAdapter&&)                 = delete;
    OpaAdapter& operator=(OpaAdapter&&)      = delete;

    /**
     * @brief Query OPA for an authorization decision.
     *
     * Sends a synchronous HTTP POST to `{endpoint_url}/v1/data/{policy_path}`.
     * Returns a PolicyEngine::Decision on success.
     * Returns std::nullopt when OPA is unreachable, times out, or returns an
     * unexpected response so the caller can fall back to native evaluation.
     */
    std::optional<PolicyEngine::Decision> evaluate(
        const std::string& user_id,
        const std::string& action,
        const std::string& resource_path,
        const std::optional<std::string>& client_ip,
        const std::optional<std::string>& user_agent) const override;

    const Config& getConfig() const { return config_; }

private:
    Config config_;

    /// Build the full OPA query URL from config.
    std::string buildUrl() const;

    /// Build the JSON request body.
    static std::string buildRequestBody(
        const std::string& user_id,
        const std::string& action,
        const std::string& resource_path,
        const std::optional<std::string>& client_ip,
        const std::optional<std::string>& user_agent);

    /// Parse OPA response and extract the boolean result field.
    static std::optional<bool> parseOpaResponse(const std::string& body);
};

} // namespace themis
