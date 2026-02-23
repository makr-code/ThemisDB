# Zero-Trust Policy Enforcer

## Overview

`ZeroTrustPolicyEnforcer` implements the Phase 4 zero-trust network policy enforcement
roadmap item for ThemisDB.  It provides **per-request identity and network verification**
following the core zero-trust principle of *never trust, always verify*.

The enforcer is designed to sit **in front of** the existing `AccessControlManager` /
RBAC / ABAC stack.  Every inbound request must pass the zero-trust gate before any
RBAC or ABAC evaluation is attempted.

## Key Features

| Feature | Description |
|---------|-------------|
| Per-request identity verification | Token/credential is checked on every call — no implicit session trust |
| IPv4/CIDR network policies | Per-identity allow-lists and deny-lists; deny-by-default posture |
| Trust score | Composite `[0.0, 1.0]` score combining identity, network, device, and freshness |
| Pluggable token verifier | Injected `TokenVerifier` lambda keeps the class independent of JWT/`AuthMiddleware` |
| Thread-safe | Policy mutations and verification are protected by `std::mutex`; metrics are lock-free atomics |
| Runtime policy management | Add and remove `NetworkPolicy` objects at runtime without restart |

## API Reference

### Core types

```cpp
namespace themis::security {

// Per-request context – populate and pass to verify() for every inbound call
struct ZeroTrustContext {
    std::string request_id;               // unique request ID
    std::string user_id;                  // claimed identity
    std::string client_ip;                // source IPv4 address
    std::string token;                    // bearer token / API key / cert fingerprint
    std::string resource;                 // resource being accessed
    std::string action;                   // action being performed
    std::optional<std::string> device_id; // optional device identifier
    std::chrono::system_clock::time_point timestamp;
    std::unordered_map<std::string, std::string> attributes;
};

// Network policy: CIDR-based access control for a specific identity
struct NetworkPolicy {
    std::string policy_id;
    std::string identity;                  // user_id this policy applies to
    std::vector<std::string> allowed_cidrs;
    std::vector<std::string> denied_cidrs; // takes precedence over allowed_cidrs
    bool default_deny = true;              // zero-trust: deny unless explicitly allowed
    std::optional<std::chrono::seconds> max_token_age;
};

// Verification result returned by verify()
struct VerificationResult {
    bool verified;           // overall result
    bool identity_verified;  // token check passed
    bool network_policy_passed; // IP/CIDR check passed
    double trust_score;      // composite score [0.0, 1.0]
    std::string reason;      // human-readable explanation
    std::string request_id;  // echoed from context
    std::string policy_id;   // policy responsible for decision (if any)
};

} // namespace themis::security
```

### ZeroTrustPolicyEnforcer

```cpp
// Token verifier callback signature
using TokenVerifier = std::function<bool(const std::string& token,
                                         const std::string& user_id)>;

class ZeroTrustPolicyEnforcer {
public:
    explicit ZeroTrustPolicyEnforcer(TokenVerifier token_verifier = nullptr);

    // Policy management
    void addNetworkPolicy(const NetworkPolicy& policy);
    bool removeNetworkPolicy(const std::string& policy_id);
    std::vector<NetworkPolicy> getNetworkPolicies() const;

    // Per-request verification (call this for every request)
    VerificationResult verify(const ZeroTrustContext& context);

    // Individual checks (available for staged enforcement)
    bool verifyToken(const std::string& token, const std::string& user_id) const;
    bool isIpAllowed(const std::string& client_ip, const std::string& identity) const;
    double computeTrustScore(const ZeroTrustContext& context,
                             bool identity_verified, bool network_ok) const;

    // Prometheus-compatible metrics
    const Metrics& getMetrics() const;
};
```

## Trust Score Breakdown

| Signal | Weight | Description |
|--------|--------|-------------|
| Identity verified | +0.4 | Token/credential validated by `TokenVerifier` |
| Network policy passed | +0.4 | Source IP matches allowed CIDR |
| Device ID present | +0.1 | Non-empty `device_id` in context |
| Request freshness | +0.1 | Timestamp is within 60 seconds of now |

## Quick Start

```cpp
#include "security/zero_trust_policy_enforcer.h"
#include "security/access_control_manager.h"

using namespace themis::security;

// 1. Create the enforcer with a token verifier
ZeroTrustPolicyEnforcer zt([](const std::string& token, const std::string& user_id) {
    // Delegate to your existing JWT/session validator
    return auth_middleware->validateToken(token).authorized;
});

// 2. Register network policies for each identity / user group
//    denied_cidrs take precedence over allowed_cidrs:
//    here 10.0.0.0/24 is blocked even though 10.0.0.0/8 is in allowed_cidrs
zt.addNetworkPolicy({
    .policy_id    = "backend-service-net",
    .identity     = "svc-backend",
    .allowed_cidrs = {"10.0.0.0/8", "172.16.0.0/12"},
    .denied_cidrs  = {"10.0.0.0/24"},  // block a sub-range (deny takes precedence)
    .default_deny  = true
});

// 3. Verify every inbound request before RBAC/ABAC
ZeroTrustContext ctx;
ctx.request_id = generate_uuid();
ctx.user_id    = extracted_user_id;
ctx.client_ip  = peer_address;
ctx.token      = bearer_token;
ctx.resource   = "data";
ctx.action     = "read";

auto zt_result = zt.verify(ctx);
if (!zt_result.verified) {
    return http_response(403, zt_result.reason);
}

// 4. Now proceed with RBAC/ABAC via AccessControlManager
auto ac_decision = acm->authorize(security_context, ctx.resource, ctx.action);
```

## Integration with AccessControlManager

The zero-trust enforcer is orthogonal to `AccessControlManager`: it provides the
network-layer identity gate while `AccessControlManager` provides the application-layer
RBAC / ABAC gate.  Both should be used together:

```
Request
  → ZeroTrustPolicyEnforcer::verify()   ← network identity gate (this module)
      → AccessControlManager::checkAccess()  ← application RBAC/ABAC gate
          → Database operation
```

## Network Policy Evaluation Order

1. **Denied CIDRs** are evaluated first — if the source IP matches any denied CIDR, access is denied immediately.
2. **Allowed CIDRs** are evaluated next — if the source IP matches any allowed CIDR, access is permitted.
3. If no CIDR matches and `default_deny = true` (recommended), access is denied.
4. If no policy is registered **for this identity** but other policies exist, access is denied (zero-trust default).
5. If **no policies at all** are registered, the system allows all (unconfigured/development mode).

## Metrics

| Metric | Type | Description |
|--------|------|-------------|
| `requests_total` | counter | Total calls to `verify()` |
| `identity_failures` | counter | Requests blocked due to failed token verification |
| `network_policy_denials` | counter | Requests blocked by network policy |
| `requests_allowed` | counter | Requests that passed all checks |
| `requests_denied` | counter | Requests that failed any check |

All counters are `std::atomic<uint64_t>` and can be polled for Prometheus integration.

## Known Limitations

- IPv4 CIDR notation only (`w.x.y.z/prefix`). IPv6 support is planned for a follow-up.
- One network policy per identity (first match wins). Multiple policies per identity will be supported in a follow-up.
- `TokenVerifier` is synchronous; async verification will require a wrapper.

## See Also

- [Access Control Framework](./access_control_framework.md)
- [Security Module Roadmap](../../src/security/ROADMAP.md)
- Header: `include/security/zero_trust_policy_enforcer.h`
- Implementation: `src/security/zero_trust_policy_enforcer.cpp`
- Tests: `tests/test_zero_trust_policy_enforcer.cpp`
