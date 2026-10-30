---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Chaos Runtime Implementation

## Scope and source ownership

The production implementation for this module is
[chaos_framework.cpp](chaos_framework.cpp); public declarations and the frozen
v1.x contract live in
[chaos_framework.h](../../include/chaos/chaos_framework.h) and
[chaos_contract.h](../../include/chaos/chaos_contract.h). This guide is for
contributors changing registry, callback, or scheduling behavior.

## Runtime invariants

- `FaultInjector` identifies an active entry by target ID and `FaultType`.
  Re-injection of that pair replaces its descriptor, injection timestamp, and
  expiry.
- A positive duration expires against `steady_clock`; zero and negative
  durations are effectively permanent until explicit recovery.
- Validation in `injectFault()` rejects an empty target and a probability
  outside `[0, 1]`. It does not enforce the recommended descriptor limits in
  the contract; NaN also passes the current range comparisons.
- The scheduler owns one worker thread, uses the selected tick strategy to
  find due entries, and delegates activation to `FaultInjector`.
- `stop()` joins the worker but retains unfired entries. `clearPending()` is a
  separate operation.
- Event callbacks are synchronous and not exception-isolated. Injection calls
  callbacks after registry mutation; recovery calls callbacks under the
  registry mutex, before erasing the corresponding entry.

These are current implementation facts, not all normative guardrails in the
contract. Update this page and
[API.md](API.md) when implementation behavior changes; update the contract and
major-version migration notes for a breaking v1.x contract change.

## Edge cases and implementation risks

- Callers must validate queue size, tick interval, durations, and callback
  registration count because the runtime does not enforce the documented
  recommended maxima/minima.
- Callback registration must finish before concurrent inject/recover use. The
  callback vector is not protected against concurrent registration and
  dispatch.
- Recovery callbacks must not call back into the same injector because they run
  while its mutex is held. A thrown recovery callback prevents the current
  entry from being erased.
- A callback exception from a scheduler-fired injection escapes the worker
  thread; keep callbacks non-throwing or catch failures inside the callback.
- Stopping the scheduler does not discard queued entries. The caller must
  inspect `pendingCount()` and call `clearPending()` when appropriate.
- The fault registry and pending queue are process-local; restarting the
  process is not recovery for side effects a test consumer may have performed.

## Focused validation

The module CMake file registers tests matching `tests/chaos/test_*.cpp`.
Relevant cases include
[framework](../../tests/chaos/test_chaos_framework.cpp),
[stress](../../tests/chaos/test_chaos_stress.cpp),
[concurrency](../../tests/chaos/test_chaos_concurrency_hardening.cpp),
[callback determinism](../../tests/chaos/test_chaos_callback_determinism.cpp),
and [scheduler timing](../../tests/chaos/test_chaos_scheduler_timing.cpp).
After configuring a project build, run the focused registered tests with
`ctest --test-dir <build-dir> --output-on-failure -R 'test_chaos_(framework|stress|concurrency_hardening|callback_determinism|scheduler_timing)_chaos_FocusedTests'`.

See [MODELING.md](MODELING.md) for the state model and
[MODULE_GUIDE.md](MODULE_GUIDE.md) for consumer usage.
