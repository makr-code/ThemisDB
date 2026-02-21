/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ranger_adapter.h                                   ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:35:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     67                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b9a7c6cd9  2025-11-02  fix(build): resolve Windows/Linux compilation errors - Ch... ║
    • 468f98ae0  2025-11-02  feat(ranger): add timeouts + retry with env config; docs:... ║
    • 20c4e9c84  2025-11-02  feat: Complete feature set - Auth, Governance, Compliance... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

#include "server/policy_engine.h" // provides themis::PolicyEngine and nested Policy

namespace themis { namespace server {

struct RangerClientConfig {
    std::string base_url;           // e.g. https://ranger.example.com
    std::string policies_path;      // e.g. /service/public/v2/api/policy
    std::string service_name;       // e.g. themisdb-prod
    std::string bearer_token;       // Authorization: Bearer <token>
    bool tls_verify = true;         // verify peer
    std::optional<std::string> ca_cert_path;       // optional custom CA
    std::optional<std::string> client_cert_path;   // optional mTLS
    std::optional<std::string> client_key_path;    // optional mTLS
    // Timeouts (milliseconds)
    long connect_timeout_ms = 5000; // default 5s connect timeout
    long request_timeout_ms = 15000; // default 15s total timeout
    // Retry policy
    int max_retries = 2;            // number of retries on transient errors (in addition to first try)
    long retry_backoff_ms = 500;    // initial backoff between retries, exponential
};

class RangerClient {
public:
    explicit RangerClient(RangerClientConfig cfg);

    // Fetch policies for configured service from Ranger REST API
    // Returns parsed JSON array/object on success.
    std::optional<nlohmann::json> fetchPolicies(std::string* err = nullptr) const;

    // Convert Ranger policies JSON to internal PolicyEngine::Policy vector
    static std::vector<themis::PolicyEngine::Policy> convertFromRanger(const nlohmann::json& rangerJson);

    // Convert internal policies to a minimal Ranger-like JSON
    static nlohmann::json convertToRanger(const std::vector<themis::PolicyEngine::Policy>& policies,
                                          const std::string& service_name);

private:
    RangerClientConfig cfg_;
};

}} // namespace themis::server
