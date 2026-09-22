**Author:** ThemisDB Contributors  
**Created:** 2026-09-21  
**Last Updated:** 2026-09-21  
**Status:** active

# Security — Execution Module

Report vulnerabilities via the repository root `SECURITY.md` process.

## Threat Model

| Threat | Current mitigation surface |
|---|---|
| queue-flood / resource-exhaustion attempts | bounded scheduler and thread-pool queues with timeout-based rejection |
| work submission after shutdown | explicit shutdown flags reject new scheduler entries and thread-pool tasks |
| task exception crash amplification | worker loop catches task exceptions and records them instead of terminating the pool |
| silent overload under mixed workloads | low-priority shedding count and explicit submission failure surfaces |
| ambiguous operator assumptions about execution behavior | source-aligned docs and Doxygen describing the smaller live contract |

## Security Controls

- The scheduler and thread pool are **fail closed** for post-shutdown submissions.
- Capacity waits are bounded by caller-provided timeouts rather than unbounded blocking.
- LOW-priority scheduler work is rejected once the configured shed threshold is reached instead of allowing unbounded queue growth.
- Worker task exceptions do not crash the process-local worker loop directly; failures are counted for operator inspection.

## Known Limitations

- The module does not provide tenant isolation, privilege separation, or sandboxing for submitted callables.
- There is no built-in cancellation or eviction of expired scheduler entries, so upstream code must decide how to handle late work.
- Metrics are in-memory only; exporting them securely is a responsibility of the integrating component.
- The current thread pool does not yet provide separate per-worker queues, so all pending work shares one dispatch queue mutex.

## Sourcecode Verification (Module: execution/security)

- Verified files:
  - `src/execution/query_scheduler.cpp`
  - `src/execution/thread_pool_manager.cpp`
  - `include/execution/query_scheduler.h`
  - `include/execution/thread_pool_manager.h`
- Verified controls:
  - bounded-capacity queue admission and rejection paths
  - shutdown-flag checks on scheduler and thread-pool submission
  - worker exception containment and failure counting
