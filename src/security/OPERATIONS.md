---
Author: ThemisDB Contributors
Created: 2026-10-19
Last Updated: 2026-10-30
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Security Module Operations Guide

AI provenance note: Exact model backend not verifiable from final document text alone.


This document covers operational expectations for the security module, especially how the module should be run, monitored, and checked during normal and degraded conditions.

## Operational responsibilities

The security module is a runtime control layer; it is not merely a static library of helpers. It must be operated as part of a secure production environment with attention to:

- identity verification and trust state
- key availability and rotation status
- policy validation timing and result
- audit evidence accessibility
- suspicious query or operation rejection paths

## Runtime checks and signals

Operational teams should monitor:

- request allow/deny counts from the zero-trust and RBAC systems
- key provider availability and version state
- rotation and revocation events
- audit export success and retention-window health
- anomaly or injection detection rates

The repo has existing code and evidence classes for this in `security_evidence_collector.*` and the benchmark suites in `benchmarks/security`.

### Implemented counters and evidence fields

`AccessControlManager::getMetrics()` exposes internal counters for `authentication_success`, `authentication_failure`, `authorization_success`, `authorization_failure`, and `access_denied`. `ZeroTrustPolicyEnforcer::getMetrics()` exposes request total/allowed/denied and identity/network-policy failure counters. These are component snapshots, not named Prometheus series guaranteed to be scrapeable.

Audit records are emitted for authentication and authorization outcomes when audit logging is enabled. For evidence exports, inspect `AuditLogExport::chain_intact`, event counts, retention status, and `SecurityEvidenceCollector::lastExportMetrics()` (`events_sent`, `events_confirmed`, `resend_count`, `atomicity_guaranteed`, `idempotency_verified`). These export values describe the last export only; verify them after each export and retain the resulting bundle.

Alert on sustained authentication/identity failures, unusual deny growth, missing or revoked key versions, broken audit-chain integrity, or incomplete evidence export. Compare counter deltas and investigate the corresponding principal/policy/provider event; do not lower deny thresholds or switch to allow-on-error to suppress alerts.

## Normal operating posture

A healthy security runtime should exhibit the following:

- TLS and certificate checks are enforced at the relevant ingress boundary
- policy decisions reject invalid or missing identity state
- key metadata remains versioned and current
- evidence logs are written and retained in a structured form
- suspicious queries are rejected before execution reaches unsafe paths

## Degraded-mode handling

When a dependency is unavailable, a provider is degraded, or a runtime check fails, the system must remain conservative:

- deny invalid requests instead of passing them through
- require explicit operational approval for fallback paths that weaken protection
- keep security evidence and logs intact so the degraded state is observable
- avoid silent recovery if a component is no longer trusted to enforce policy

## Monitoring strategy

Monitor for the following conditions:

- repeated identity verification failures
- a spike in policy denials or access-control failures
- unknown or expired key versions
- missing, delayed, or incomplete audit exports
- abnormal anomaly scores or query-validation rejections

The operation is considered unhealthy when the system cannot explain its trust state or when evidence becomes incomplete.

## Recovery expectations

Recovery should be narrow and evidence-backed:

- restore key-provider connectivity or rotate to an active provider
- reload valid policy state with clear verification
- confirm the trust decision path remains fail-closed
- revalidate audit export integrity before returning to normal operations

## Operational boundaries and signs of risk

The module remains at a measured readiness level because the source and roadmap explicitly state that final module sign-off is still pending until remaining gap closure is complete. Operators should treat any temporary fallback as a restricted state, not as a normal production condition.

## References

- [RUNBOOK.md](RUNBOOK.md)
- [SECURITY.md](SECURITY.md)
- `include/security/access_control_manager.h`
- `include/security/security_evidence_collector.h`
- `include/security/zero_trust_policy_enforcer.h`
