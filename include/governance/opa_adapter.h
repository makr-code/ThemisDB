/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            opa_adapter.h                                      ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 04:10:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     143                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 99dc8e3f41  2026-02-27  feat(governance): integrate OPA as alternative policy eva... ║
    • 0766c4a216  2026-02-24  fix(auth/opa): code audit - remove redundant static, thre... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "governance/policy_engine.h"
#include <optional>
#include <string>
#include <unordered_map>

namespace themis {
namespace governance {

/**
 * @brief Adapter that evaluates governance policy decisions via an OPA sidecar.
 *
 * Implements PolicyEngine::IPolicyEvaluator by posting a JSON input document
 * to `POST /v1/data/{policy_path}` on a local OPA server.  If OPA is
 * unreachable or returns a non-2xx response within the configured timeout,
 * evaluate() returns std::nullopt so the caller falls back to native
 * PolicyEngine evaluation.
 *
 * OPA input format:
 * @code{.json}
 * {
 *   "input": {
 *     "headers": {
 *       "X-Classification": "vs-nfd",
 *       "X-User-Id":        "alice"
 *     },
 *     "route": "/vector/search"
 *   }
 * }
 * @endcode
 *
 * OPA response (structured):
 * @code{.json}
 * {
 *   "result": {
 *     "allow":                    true,
 *     "classification":           "offen",
 *     "mode":                     "enforce",
 *     "encrypt_logs":             false,
 *     "redaction":                "standard",
 *     "ann_allowed":              true,
 *     "require_content_encryption": false,
 *     "export_allowed":           true,
 *     "cache_allowed":            true,
 *     "retention_days":           365
 *   }
 * }
 * @endcode
 *
 * OPA response (simple boolean):
 * - `{"result": false}` — deny with strict defaults (no export, encrypt everything).
 * - `{"result": true}`  — OPA allows but provides no governance details;
 *                         returns std::nullopt so native evaluation fills in the
 *                         classification details.
 *
 * Usage:
 * @code{.cpp}
 * governance::OpaAdapter::Config cfg;
 * cfg.endpoint_url = "http://localhost:8181";
 * cfg.policy_path  = "themis/governance/allow";
 * cfg.timeout_ms   = 50;
 *
 * auto adapter = std::make_shared<governance::OpaAdapter>(cfg);
 * governance_engine.setOpaEvaluator(adapter.get());
 * @endcode
 */
class OpaAdapter : public PolicyEngine::IPolicyEvaluator {
public:
    struct Config {
        /// Base URL of the OPA server (e.g. "http://localhost:8181").
        std::string endpoint_url = "http://localhost:8181";
        /// Rego decision path below /v1/data/ (e.g. "themis/governance/allow").
        std::string policy_path  = "themis/governance/allow";
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
     * @brief Query OPA for a governance policy decision.
     *
     * Sends a synchronous HTTP POST to `{endpoint_url}/v1/data/{policy_path}`.
     * Returns a PolicyDecision on success.
     * Returns std::nullopt when OPA is unreachable, times out, or returns an
     * unexpected response (including `{"result": true}` without governance
     * details) so the caller can fall back to native evaluation.
     */
    std::optional<PolicyDecision> evaluate(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route) const override;

    const Config& getConfig() const { return config_; }

private:
    Config config_;

    /// Build the full OPA query URL from config.
    std::string buildUrl() const;

    /// Build the JSON request body.
    static std::string buildRequestBody(
        const std::unordered_map<std::string, std::string>& headers,
        const std::string& route);

    /// Parse OPA response and extract a PolicyDecision.
    static std::optional<PolicyDecision> parseOpaResponse(const std::string& body);
};

} // namespace governance
} // namespace themis
