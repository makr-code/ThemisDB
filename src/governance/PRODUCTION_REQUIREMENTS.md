---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Related: "Issue #7349"
---

Exact model backend not verifiable from final document text alone.

# Governance Deployment Checks (Non-Normative)

This file is an operator checklist, not a normative production or compliance
standard. The previous “MUST” statements, mandatory bundle requirement,
`THEMIS_PRODUCTION_MODE` / `THEMIS_ENVIRONMENT` flags, and claims that all
settings are validated at startup are not established by the current governance
interfaces and are withdrawn.

## Checks for deployments using these interfaces

- Load the intended policy YAML using `PolicyEngine::loadFromYAML()` and check
  its boolean result. The implementation recognizes `vs_classification`,
  `enforcement.resource_mapping`, `enforcement.default_mode`, and
  `data_masking.enabled` / `data_masking.rules`.
- Establish request identity and classification context in the owning
  authenticated request path. Do not treat caller-supplied headers as proof of
  identity.
- Confirm each caller enforces the returned decision and applies masking where
  the output path requires it. A policy decision alone does not enforce the
  downstream operation.
- If OPA is used, verify endpoint, policy path, timeout, response shape, and
  active mode. The built-in WASM stub is not a production evaluator.
- If lineage/audit forwarding is needed, configure and verify the corresponding
  sink separately. In-memory records are not a durable retention guarantee.
- Treat compliance reports as software output based on supplied configuration
  and evidence, not as certification.

## Unverified deployment matters

The governance module does not define a universal production mode switch,
deployment-specific retry policy, external audit store, or a single startup
validator covering all governance configuration. Key lifecycle, durable
retention, caller authentication, and regulatory acceptance need independent
deployment-level verification.

## Source references

- [`policy_engine.h`](../../include/governance/policy_engine.h) and
  [`policy_engine.cpp`](policy_engine.cpp)
- [`opa_adapter.h`](../../include/governance/opa_adapter.h) and
  [`opa_adapter.cpp`](opa_adapter.cpp)
- [`cross_tenant_policy_inheritance.h`](../../include/governance/cross_tenant_policy_inheritance.h)
- [Trust boundaries](TRUST.md) · [Module architecture](ARCHITECTURE.md)
