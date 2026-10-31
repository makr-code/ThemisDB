---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Execution Module Developer Guide

## Purpose and reader

Use this page as the entry point for developers onboarding to or changing
execution. The module provides bounded in-process query admission and bounded
task dispatch; it is not a distributed executor or a request-level integration
framework.

## Start here

1. Read [README.md](README.md) for scope and existing build/test commands.
2. Read [ARCHITECTURE.md](ARCHITECTURE.md) for boundaries and data flow.
3. Use [API.md](API.md) for signatures and caller-visible failure behavior.
4. Use [IMPLEMENTATION.md](IMPLEMENTATION.md) before changing synchronization,
   counters, or shutdown paths.
5. Use [RUNBOOK.md](RUNBOOK.md) and [OPERATIONS.md](OPERATIONS.md) for
   diagnosis and production lifecycle responsibilities.
6. Use [TRUST.md](TRUST.md) with the canonical [SECURITY.md](SECURITY.md) for
   trust assumptions and residual risks.

Research and evidence-oriented references:

- [RESEARCH.md](RESEARCH.md) — evaluation questions, sources, and limits
- [MODELING.md](MODELING.md) — admission and dispatch model
- [EMBEDDING.md](EMBEDDING.md) — composing primitives into a host
- [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) — benchmark case
  interpretation and baselines
- [ROADMAP.md](ROADMAP.md) and [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md)
  — planned work, not current runtime guarantees

## Source ownership

| Area | Canonical path |
|---|---|
| Public declarations | `include/execution/query_scheduler.h`, `include/execution/thread_pool_manager.h` |
| Internal implementation | `src/execution/query_scheduler.cpp`, `src/execution/thread_pool_manager.cpp` |
| Focused scheduler and pool tests | `tests/integration/test_load_balancing.cpp`, `tests/integration/test_resource_pooling.cpp` |
| Capacity stress simulation (local models) | `tests/execution/test_execution_highcardinality_stress.cpp` |
| Dedicated benchmark harness (local models) | `benchmarks/execution/bench_execution_dedicated_gates.cpp` |
| Conditional server lifecycle wiring | `include/server/http_server.h`, `src/server/http_server.cpp` |

Public API changes belong in `include/execution/` and must update
`API.md`, relevant Doxygen comments, and focused tests. Internal changes belong
in `src/execution/`; update this guide or the relevant contract page when
behavior changes. Do not promote reserved extension structures to supported
runtime behavior without implementation and regression coverage.

## Build and validate

The module README documents the CMake configuration and CTest filters. Run
focused scheduler tests for admission/order changes and the integration tests
for pool worker/lifecycle changes. The stress and benchmark artifacts use local
models and must not substitute for production-runtime capacity or performance
evidence. Record the configuration and result; the execution primitives do not
expose a standalone CLI or network endpoint.

## Current implementation boundary

The scheduler orders by absolute deadline and reports completion metrics only
when explicitly called. The pool starts a fixed worker set and dispatches from
a central queue. It currently does not implement live per-worker work stealing,
elastic growth, expiry/cancellation, or a built-in metrics exporter.

Exact model backend not verifiable from final document text alone.
