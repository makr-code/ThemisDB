# Security - GPU Module

<!-- Source-reviewed: 2026-10-31; no security audit or runtime validation performed -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PRODUCTION_REQUIREMENTS.md · PERFORMANCE_EXPECTATIONS.md -->

Report vulnerabilities via project-level SECURITY.md.

## Security Scope

Security in the GPU module focuses on resource isolation, policy-gated acceleration, safe fallback behavior, and prevention of unsafe cross-device or cross-tenant execution paths.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unauthorized GPU usage | policy/feature gate checks before execution |
| tenant quota escape and noisy-neighbor pressure | edition-aware VRAM quotas and pool governance |
| unsafe degraded execution | circuit-breaker invokes a configured fallback callback when enabled; workload correctness remains caller-owned |
| cross-device data exposure risk | bounded transfer/topology handling and explicit feature gating |
| hidden operational regressions | telemetry/profiling/audit/admin observability surfaces |

## Implemented Security Controls

- default-deny style capability/policy checks gate GPU access.
- quota and pool controls constrain memory allocation surfaces.
- safe-fail tracks failures and may invoke a configured callback; it does not guarantee CPU computation, parity, or a universal fallback bound.
- feature-gated advanced surfaces (e.g. P2P/topology) prevent accidental activation.

## Security Follow-ups

- continue hardening edge paths around topology, P2P, and partitioned devices.
- tighten diagnostics for fallback and quota-related denial incidents.
- expand stress and abuse-case coverage for mixed tenant load.

## Sourcecode Verification (Module: gpu/security)

- Verified files:
  - src/gpu/policy.cpp
  - src/gpu/feature_flags.cpp
  - src/gpu/gpu_memory_manager_edition.cpp
  - src/gpu/memory_pool.cpp
  - src/gpu/safe_fail.cpp
  - src/gpu/p2p_transfer.cpp
  - src/gpu/metrics.cpp
  - src/gpu/admin_api.cpp
- Verified controls:
  - policy-gated access and bounded quota behavior
  - configured fallback-callback handling and feature-gated advanced surfaces
  - operational telemetry coverage for security-relevant incidents