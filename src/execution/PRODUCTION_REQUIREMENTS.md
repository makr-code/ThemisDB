**Author:** ThemisDB Contributors  
**Created:** 2026-09-21  
**Last Updated:** 2026-09-21  
**Status:** active

# Production Requirements — Execution Module

## Purpose and Scope

This document defines the minimum deployment-time and operational requirements for the current execution module implementation.

It applies to:
- `themis::execution::QueryScheduler`
- `themis::resource::WorkStealingThreadPool`
- server-side integrations that instantiate these primitives under `THEMIS_EXECUTION_MODULE`

## Mandatory Production Requirements

- **MUST:** configure finite queue limits (`max_queue_depth`, `shed_threshold`) appropriate for the deployment workload; the scheduler is intentionally bounded.
- **MUST:** configure finite caller timeouts for scheduler admission and thread-pool submission paths; overload signaling is timeout-based.
- **MUST:** treat `enqueue() == 0` and `submit() == false` as fail-closed overload or shutdown signals and handle them explicitly upstream.
- **MUST:** invoke `shutdown()` during controlled teardown so blocked waiters are released and workers are joined.
- **MUST:** call `reportCompletion()` for dequeued queries if SLA-compliance metrics are used for operational decisions.
- **MUST NOT:** assume the pool grows beyond `min_threads` at runtime in the current implementation.
- **MUST NOT:** assume queued scheduler entries are auto-expired or auto-cancelled after their SLA window elapses.

## Configuration Constraints

| Surface | Current production meaning |
|---|---|
| `QueryScheduler::Config::max_queue_depth` | hard capacity gate for scheduler admission waits |
| `QueryScheduler::Config::shed_threshold` | low-priority rejection threshold |
| `QueryScheduler::Config::urgent_window_ms` | reserved compatibility field; not active in the current runtime |
| `QueryScheduler::Config::default_sla_ms` | reserved compatibility field; not active in the current runtime |
| `WorkStealingThreadPool::Config::min_threads` | number of workers started at construction and kept until shutdown |
| `WorkStealingThreadPool::Config::max_threads` | allocation/clamping bound; not an active runtime growth target today |
| `WorkStealingThreadPool::Config::max_queue_depth` | hard capacity gate for task submission waits |
| `WorkStealingThreadPool::Config::idle_timeout_ms` | worker wait interval before re-checking the queue; not a retirement timer |

## Operational Requirements

- Expose overload outcomes (`enqueue()==0`, `submit()==false`, shed counts) to the surrounding service diagnostics.
- Keep queue limits aligned with upstream retry behavior; repeated blind retries can turn bounded rejection into external overload amplification.
- Ensure task bodies are exception-safe even though the pool catches exceptions; repeated task failure still indicates an application-level production defect.
- Re-run focused scheduler/thread-pool validation after any change to server wiring, queue limits, or thread counts.

## Minimal Production Checklist

- [ ] queue capacities explicitly configured for the target deployment
- [ ] caller timeout strategy documented and bounded
- [ ] shutdown path invokes both execution primitive teardowns where integrated
- [ ] SLA metrics consumers call `reportCompletion()` consistently
- [ ] overload and task-failure counters routed into operational diagnostics
