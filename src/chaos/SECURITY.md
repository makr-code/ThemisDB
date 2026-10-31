---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: OpenAI
Human_Review: false
Human_Author: ThemisDB Contributors
---

Exact model backend not verifiable from final document text alone.

# Security - Chaos Module

Report vulnerabilities via project-level [SECURITY.md](../../SECURITY.md).

## Source-visible boundary

`FaultInjector` and `ChaosScheduler` keep state in-process. They do not perform
real network or disk manipulation, cross-process communication, or persistent
audit logging. This is a simulator boundary, not an authorization control.

The runtime rejects an empty target ID and probabilities below 0 or above 1
during `injectFault()`. It does not enforce the contract's recommended queue,
duration, tick, identifier-size, or callback-count limits. Callback registration
is unsynchronized, callbacks are not exception-isolated, and recovery callbacks
run under the injector mutex.

## Optional server integration

Server source contains a guarded route that checks admin access, and CMake
defaults `THEMIS_CHAOS_ADMIN` to `OFF`. However, CMake defines that macro only
on `themis_server` while `http_server.cpp` is compiled into `themis_core`;
`http_server.cpp` also attempts to default-construct `ChaosScheduler`, whose
public constructor requires an injector. **TODO:** fix and build this optional
integration before treating its access check as an effective endpoint control.
CMake warns against shipping the option in production. Deployment configuration
and production suitability remain **TODO/assumption**.

## Required caller controls (not enforced by the chaos runtime)

- Restrict scenarios to explicitly approved, isolated test/staging targets.
- Bound schedule sizes, durations, tick intervals, and callback work in the
  harness.
- Keep callbacks non-throwing and do not re-enter the injector from recovery
  callbacks.
- Persist any audit record required by the test or deployment outside this
  process-local module.

Canonical evidence: [`chaos_framework.cpp`](chaos_framework.cpp),
[`../../include/chaos/chaos_framework.h`](../../include/chaos/chaos_framework.h),
[`../../include/chaos/chaos_contract.h`](../../include/chaos/chaos_contract.h),
and [`ARCHITECTURE.md`](ARCHITECTURE.md).
