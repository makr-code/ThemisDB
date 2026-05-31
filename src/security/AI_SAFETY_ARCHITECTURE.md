# AI Safety Architecture - Security Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · SECURITY.md -->

## Purpose

This document describes the AI-safety-relevant control surfaces that are implemented directly in `src/security/`. It focuses on source-verifiable controls for classifying risky AI-driven operations, detecting prompt or query abuse patterns, masking sensitive output, and enforcing request trust policies.

## Scope

In scope:
- AI-oriented destructive-operation classification in `ai_operation_guard.cpp`
- rule-based intent classification for malicious or destructive query patterns
- canonical prompt-injection pattern registry shared across consumers
- PII redaction and query-result masking policies
- zero-trust request verification and behavioral anomaly scoring

Out of scope:
- authentication flow ownership in `src/auth/`
- full server-side approval orchestration and transport UX
- storage rollback/checkpoint execution semantics
- module-wide cryptography and key-provider architecture already covered by `ARCHITECTURE.md`

## Threat Model

| Threat | Example | Primary security-module control |
|---|---|---|
| destructive AI-issued operation | `delete_entity`, `drop_index`, unfiltered AQL `REMOVE` | `AiOperationGuard` classification and environment blocking |
| malicious query intent | injection, exfiltration, privilege escalation, schema mutation | `IntentClassifier` rule-based scoring |
| prompt injection payload reuse across subsystems | override or jailbreak patterns inside retrieved/context text | `PromptInjectionPatternRegistry` canonical shared pattern set |
| sensitive output leakage | PII in logs, labels, query results, or attributes | `PIIRedactionPolicy`, `QueryMaskingPolicy` |
| untrusted request context | weak token verification, disallowed IP origin, rising session risk | `ZeroTrustPolicyEnforcer` |
| suspicious session behavior | burst rate, off-hours access, privileged action on new resource | `BehavioralAnomalyDetector` |

## Control Architecture

### 1. Destructive Operation Guard

- Files:
  - `src/security/ai_operation_guard.cpp`
  - `include/security/ai_operation_guard.h`
- Verified behavior:
  - classifies tool and AQL requests into `READ_ONLY`, `WRITE_SAFE`, `DESTRUCTIVE`, or `CRITICAL`
  - escalates access against system collections to `CRITICAL`
  - can hard-block requests through environment checks before approval flow
  - builds preview and approval-response payloads, including configured `auto_snapshot` and `snapshot_dir` metadata
- Important limit:
  - this component documents and emits approval-related metadata, but actual approval transport and rollback execution are owned outside this file.

### 2. Intent Classification

- Files:
  - `src/security/intent_classifier.cpp`
  - `include/security/intent_classifier.h`
- Verified behavior:
  - scores query text for SQL injection, data exfiltration, privilege escalation, data destruction, and schema mutation
  - uses a rule-based classifier by default
  - supports injected inference backends for a future LoRA-backed path
  - can emit alerts only when confidence clears a caller-supplied threshold
- Important limit:
  - the current default classifier is explicitly documented in source as a rule-based placeholder rather than the final LoRA-backed implementation.

### 3. Shared Prompt Injection Patterns

- Files:
  - `src/security/prompt_injection_pattern_registry.cpp`
  - `include/security/prompt_injection_pattern_registry.h`
- Verified behavior:
  - owns the canonical singleton registry returned by `defaultRegistry()`
  - centralizes shared override, jailbreak, system-prompt-leak, special-token, and persona-takeover patterns
  - exposes a shared keyword list for downstream prompt-injection detectors
- Integration boundary:
  - this registry is a shared source of patterns for consumers in other modules, but does not itself execute the downstream detection workflow.

### 4. Redaction and Masking

- Files:
  - `src/security/pii_redaction_policy.cpp`
  - `src/security/query_masking_policy.cpp`
  - `include/security/pii_redaction_policy.h`
  - `include/security/query_masking_policy.h`
- Verified behavior:
  - `PIIRedactionPolicy` redacts free-text logs, labels, and attribute values using `utils::PIIDetector`
  - `QueryMaskingPolicy` masks result JSON for non-privileged callers and supports explicit field declarations plus automatic PII detection
  - both controls fail through explicit initialization paths rather than silently disabling detection logic

### 5. Request Trust and Session Risk

- Files:
  - `src/security/zero_trust_policy_enforcer.cpp`
  - `src/security/behavioral_anomaly_detector.cpp`
  - `include/security/zero_trust_policy_enforcer.h`
  - `include/security/behavioral_anomaly_detector.h`
- Verified behavior:
  - `ZeroTrustPolicyEnforcer` performs token verification, network-policy evaluation, risk-score revocation, and trust-score computation
  - token verification and empty-policy handling are fail-closed by default unless explicitly relaxed for tests or rollout modes
  - `BehavioralAnomalyDetector` scores burst rate, off-hours access, privileged action escalation, and unusual resource access within a session window

## Operating Limits

- These controls primarily govern AI-relevant safety surfaces inside the security module; they do not replace auth, RBAC/RLS, or cryptographic provider controls.
- Some AI-safety outcomes depend on caller-supplied configuration, thresholds, and upstream orchestration.
- Server-owned approval UX, snapshot execution, and transport-specific flow control must be documented in the owning module, not here.
- Performance and hardening follow-ups remain tracked in `ROADMAP.md` and `FUTURE_ENHANCEMENTS.md`.

## Boundaries

| Topic | Owning document |
|---|---|
| module-wide security architecture | `src/security/ARCHITECTURE.md` |
| global threat model and controls | `src/security/SECURITY.md` |
| audit findings and remediation status | `src/security/AUDIT.md` |
| authentication and auth-flow details | `src/auth/ARCHITECTURE.md` |
| governance and compliance policy layer | `src/governance/ARCHITECTURE.md` |

## Sourcecode Verification (Module: security/ai-safety-architecture)

- Verified files:
  - `src/security/ai_operation_guard.cpp`
  - `src/security/intent_classifier.cpp`
  - `src/security/prompt_injection_pattern_registry.cpp`
  - `src/security/pii_redaction_policy.cpp`
  - `src/security/query_masking_policy.cpp`
  - `src/security/zero_trust_policy_enforcer.cpp`
  - `src/security/behavioral_anomaly_detector.cpp`
- Verified behavior surfaces:
  - AI-operation classification and environment blocking
  - rule-based malicious intent scoring and alert thresholding
  - shared prompt-injection pattern registry ownership
  - PII redaction and query-result masking
  - zero-trust verification and session-anomaly scoring
- Result:
  - source-verifiable AI-safety controls are documented conservatively
  - prior over-assertive claims about approval transport, rollback execution, and broader orchestration have been removed from this module-local architecture reference
