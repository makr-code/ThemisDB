---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "themisdb-doc-orchestrator"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "architecture-governance"
Canonical_References:
  - "README.md"
  - "API.md"
  - "../../include/security/key_provider.h"
  - "../../include/security/encryption.h"
  - "../../include/security/rbac.h"
  - "../../include/security/aql_injection_detector.h"
  - "../../include/security/row_level_security.h"
  - "../../include/query/aql_parser.h"
  - "../../include/security/query_masking_policy.h"
  - "../../include/security/security_evidence_collector.h"
  - "field_encryption.cpp"
  - "rbac.cpp"
  - "aql_injection_detector.cpp"
Related_Issue: "#7167 (closes #7091), #7379"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Security Module Architecture

## Scope, boundaries, and ownership

`src/security/` owns security-specific primitives and policy surfaces: key-provider integration, encryption, authorization and row-level controls, query/payload checks, masking/redaction, and security evidence. It supplies these controls to callers; the server owns transport admission, while storage and query own their respective data and execution paths.

## Components and public interfaces

| Component | Public interface | Source-backed responsibility |
|---|---|---|
| Key and encryption | [`KeyProvider`](../../include/security/key_provider.h), [`encryption.h`](../../include/security/encryption.h) | Key metadata/provider access and field encryption/decryption operations. |
| Authorization and policy | [`RBAC`](../../include/security/rbac.h), [`AccessControlManager`](../../include/security/access_control_manager.h), [`ZeroTrustPolicyEnforcer`](../../include/security/zero_trust_policy_enforcer.h), [`RLSManager`](../../include/security/row_level_security.h) | Role/permission decisions and request/row policy checks. |
| Query and data protection | [`AQLInjectionDetector`](../../include/security/aql_injection_detector.h), [`QueryMaskingPolicy`](../../include/security/query_masking_policy.h), [`PIIRedactionPolicy`](../../include/security/pii_redaction_policy.h) | Parameterized-query validation, result masking, and redaction policies. |
| Evidence and specialized controls | [`SecurityEvidenceCollector`](../../include/security/security_evidence_collector.h), [`MalwareScanner`](../../include/security/malware_scanner.h), [`AiOperationGuard`](../../include/security/ai_operation_guard.h) | Evidence export, scanning, and AI-operation guard decisions. |

This table names representative contracts; TLS, PKI, HSM, timestamping, and other security helpers have their own narrower headers.

## Data and decision flow

Security components are invoked at their owning boundaries rather than forming one mandatory global pipeline:

1. A request-side caller supplies identity/context to an access-control or policy interface.
2. Query callers may validate AQL parameters and apply result masking/redaction.
3. Data callers may invoke field encryption with a configured key provider.
4. Audit/evidence callers collect security records and export structured evidence.

For example, `AQLInjectionDetector` accepts query text and parameters; `FieldEncryption` uses the security key-provider/encryption APIs; `RBAC` implements permission matching. See [`field_encryption.cpp`](field_encryption.cpp), [`rbac.cpp`](rbac.cpp), and [`aql_injection_detector.cpp`](aql_injection_detector.cpp).

## Failure domains and edge cases

- Missing, revoked, or unavailable keys can block key-dependent operations; encryption errors must not be interpreted as plaintext success.
- Invalid policy or identity context can yield denial/rejection; callers remain responsible for honoring the returned decision.
- Invalid query parameters or detector failures are separate from parser/executor failures in the query module.
- Security evidence export depends on its configured audit/evidence inputs and output path; it is not itself proof that every caller emitted an event.

## Explicit non-goals

- Owning the HTTP/gRPC listener, transport routing, or the complete authentication session lifecycle.
- Owning database persistence, query execution, or application business rules.
- Claiming all security controls are enabled in every deployment or that module-level presence establishes production sign-off.

## Verified cross-module references

| Boundary | Evidence and link |
|---|---|
| Query | [`aql_injection_detector.h`](../../include/security/aql_injection_detector.h) directly includes `query/aql_parser.h` and consumes parser types; `aql_runner.cpp` includes security policy interfaces. See [query architecture](../query/ARCHITECTURE.md). |
| Storage | Storage has its own signature and encrypted-blob components; no direct `FieldEncryption` hand-off from storage was confirmed in the inspected source. See [storage architecture](../storage/ARCHITECTURE.md). |
| API/server | `AccessControlManager` includes server authentication/policy interfaces; `api/ws_handler.cpp` uses server `AuthMiddleware`. See [API architecture](../api/ARCHITECTURE.md). |
| Audit utilities | Security evidence code uses the utility audit logger; this page does not claim an observability-module dependency. |

## References

- [Module README](README.md) · [API reference](API.md) · [module roadmap](ROADMAP.md) · [module gaps](MODULE_GAPS.md)
- [`security_api_contract.h`](../../include/security/security_api_contract.h) · [`key_provider.h`](../../include/security/key_provider.h) · [`security_evidence_collector.h`](../../include/security/security_evidence_collector.h)
