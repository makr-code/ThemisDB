---
Author: ThemisDB Contributors
Created: 2026-10-19
Last Updated: 2026-10-19
Status: active
AI_Used: true
AI_Models: MAI-Code-1.1-Flash
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Security Module Guide

AI provenance note: Exact model backend not verifiable from final document text alone.


This guide is the entry point for engineers working on the security module. It connects the module's public interfaces, implementation files, and validation sources without duplicating the full architecture or roadmap.

## Module purpose

The security module in `src/security` provides the runtime controls that protect identity, data, and system integrity in ThemisDB.

The main sources of truth are:

- `include/security/security_api_contract.h` — enforced contract and boundaries
- `include/security/*.h` — public API declarations
- `src/security/*.cpp` — concrete enforcement logic
- `tests/security` — validation and regression coverage
- `benchmarks/security` — latency and throughput gates

## Recommended reading order

1. `README.md` — high-level context and module boundaries
2. `ARCHITECTURE.md` — control flow and trust boundaries
3. `API.md` — public interfaces and semantics
4. `IMPLEMENTATION.md` — how the code matches the contract
5. `ROADMAP.md` and `MODULE_GAPS.md` — active status and remaining work
6. `RUNBOOK.md` and `OPERATIONS.md` — diagnosis and incident handling

## Module ownership map

- Public API: `include/security/`
- Internal logic: `src/security/`
- Validation: `tests/security/`
- Performance gates: `benchmarks/security/`

## Development safeguards

When changing security code, keep the following invariants in mind:

- do not introduce a silent fail-open behavior
- do not weaken TLS or key lifecycle requirements
- do not weaken audit/evidence logging requirements
- do not allow unsupported HSM fallback behavior in production by default
- do not ignore policy or identity validation errors

## Common subsystem entry points

### Encryption and key material

- `field_encryption.cpp`
- `encryption.h`
- `key_provider.h`
- `vault_key_provider.cpp`
- `hsm_provider_pkcs11.cpp`
- `pki_key_provider.cpp`

### Access control and trust

- `access_control.h`
- `rbac.cpp`
- `row_level_security.cpp`
- `zero_trust_policy_enforcer.cpp`
- `access_control_manager.cpp`

### Detection and policy validation

- `aql_injection_detector.cpp`
- `behavioral_anomaly_detector.cpp`
- `malware_scanner.cpp`
- `prompt_injection_pattern_registry.cpp`
- `query_masking_policy.cpp`

### Audit and evidence

- `security_evidence_collector.h`
- `security_evidence_collector.cpp`
- `audit_log_store.cpp`

## Change checklist

Before merging a security-related change, confirm:

- code remains fail-closed under invalid inputs
- policy or auth decisions are explicit, not inferred
- key or vault failure is surfaced and not masked
- audit/evidence output stays intact
- tests cover the changed access or policy path

## Known status boundary

The module has strong production-style evidence and active hardening work, but the current roadmap explicitly records that final sign-off remains pending until residual gap closure is completed. This status should be reflected honestly in any internal documentation or change notes.
