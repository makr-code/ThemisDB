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

# Security Embedding Guide

AI provenance note: Exact model backend not verifiable from final document text alone.


This document explains how the security module is embedded into the runtime path of ThemisDB. The goal is not a generic security overview but a concise map of where enforcement actually sits in the codebase and how those controls interact.

## Execution embedding points

The module is designed to be called from the request, query, and storage planes rather than operating as an isolated sidecar.

### Server and access path

- `include/security/access_control.h` and `include/security/rbac.h` form the access control and session model used in server-side request validation.
- `zero_trust_policy_enforcer.cpp` adds identity, network, and risk-score checks to a request before the request is allowed to proceed.
- `ai_operation_guard.h` provides a gate for AI-initiated mutating operations before tools run.

### Query path

- `aql_injection_detector.cpp` checks AQL templates, AST content, and mutation patterns before they are accepted for execution.
- `query_masking_policy.cpp` and `pii_redaction_policy.cpp` are part of the same trust boundary when results or prompts are filtered before they reach downstream consumers.

### Storage and encryption path

- `field_encryption.cpp` and `encryption.h` protect fields as data is written or read.
- `key_provider.h` defines the key retrieval and rotation contract that providers implement.
- `vault_key_provider.cpp`, `pki_key_provider.cpp`, and `hsm_provider_pkcs11.cpp` are the concrete providers that integrate external key material.

## How the embedding works in practice

The embedding pattern is consistent:

1. validate request/identity or query
2. evaluate policy or suspicion signal
3. enforce fail-closed behavior on errors
4. apply encryption or masking where needed
5. emit evidence or an audit event

This pattern appears repeatedly throughout the security module. It is a design choice to keep security decisions close to the call sites rather than relying on an external, disconnected policy layer.

## Embedded assumptions

The module assumes that every trust boundary must be explicit:

- no implicit allow for invalid identity
- no permissive fallback for key errors
- no silent bypass of policy or risk checks
- no mutation without classification and approval when the action is destructive

These assumptions are encoded by the contract and by the implementation style in the security sources.

## Performance and reliability implications

Because these checks are embedded into critical execution paths, they must be cheap and deterministic. The repository reflects this by documenting hard limits and benchmark gates in `ROADMAP.md` and `benchmarks/security`.

The cost is manageable because the module uses focused validation logic rather than broad diagnostic scanning on every request; however, the enforcement rules still prefer blocking unsafe operations over degraded throughput.

## Safety posture

The embedding model is intentionally conservative. It is easier to stop a request early and reject it than to allow a partially trusted path to proceed into data or system mutation. This is exactly why the module uses deny-by-default, fail-closed evaluation and explicit evidence collection.
