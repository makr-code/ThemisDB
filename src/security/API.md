---
Author: ThemisDB Contributors
Created: 2026-10-19
Last Updated: 2026-10-19
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Security Module API Reference

AI provenance note: Exact model backend not verifiable from final document text alone.


This page summarizes the public security API surface used by the security module and the code paths that enforce the contract in practice.

## Public contracts

### 1. Key management

Source: `include/security/key_provider.h`

Relevant concepts:

- `KeyStatus` — `ACTIVE`, `ROTATING`, `DEPRECATED`, `DELETED`
- `KeyMetadata` — logical key identifier, version, algorithm, creation time, expiry time, status
- `KeyProvider` — key retrieval, rotation, metadata lookup, and deletion
- `KeyCache` — in-memory caching for key material
- `KeyNotFoundException` and `KeyOperationException` — explicit failure types

Key invariants:

- key lookups must be version-aware
- rotation must maintain an explicit status model
- missing or revoked keys must not silently pass as valid

### 2. Access control and policy enforcement

Sources:

- `include/security/access_control.h`
- `include/security/rbac.h`
- `include/security/zero_trust_policy_enforcer.h`

Relevant concepts:

- `AccessControl` — principal authentication, session management, authorization decisions
- `ZeroTrustPolicyEnforcer` — identity + network policy + trust-score verification
- `RBACConfig` and related role structures — role and permission mapping

Core contract:

- deny-by-default for missing or conflicting policy state
- explicit `ALLOW` or `DENY` outcomes only
- invalid identity and network policy rejections are treated as denial, not recovery

### 3. Query and injection defense

Source:

- `include/security/aql_injection_detector.h`
- `include/security/query_masking_policy.h`
- `include/security/pii_redaction_policy.h`

The design enforces that suspicious templates, dangerous AST nodes, and SQL-like payloads are rejected near the query boundary before execution.

### 4. AI operation governance

Source:

- `include/security/ai_operation_guard.h`

The `AiOperationGuard` API classifies destructive operations by class and environment. It builds blocked and approval-required responses so AI-initiated tool calls remain under explicit policy control.

### 5. Evidence and audit

Source:

- `include/security/security_evidence_collector.h`

The evidence model exposes structured audit bundles that contain:

- audit log export records
- security metrics snapshots
- key-rotation records
- access-control reports
- network-control evidence
- change-management evidence

## Runtime semantics

The module's public behavior is intentionally conservative:

- invalid certificates and weak transport settings are rejected
- rotation overlap windows are bounded and explicit
- policy evaluation uses a hard timeout and fail-closed semantics
- data encryption is bound to key metadata to prevent dangling or mismatched decrypts

## API usage pattern

A typical call path is:

1. authenticate or verify a principal
2. check a role or trust policy
3. inspect or sanitize a query or payload
4. encrypt or redact data when required
5. record the decision in a security evidence bundle

## Important compatibility note

The contract in `security_api_contract.h` is the canonical baseline for the v1.x major line. This means compatibility is assumed to be additive and fail-closed rather than permissive. Any production behavior change that broadens trust or weakens denial should be treated as a release change requiring explicit lifecycle documentation.
