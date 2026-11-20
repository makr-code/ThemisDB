---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-11-19"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot; OpenAI assistant"
Human_Review: false
Human_Author: "unknown"
---

> Original creation date was not recorded; this provenance block was first recorded on 2026-10-31.
> Exact model backend not verifiable from final document text alone.

Source level: 1
SOT domain: module-behavior
Canonical references: `tests/query/CMakeLists.txt`, `tests/CMakeLists.txt`, `src/query/ROADMAP.md`, `include/query/`
Related issue: #7371
Related milestone: `DOC-WEEKLY-2026-W47`

# Query module tests

`tests/query/` contains 61 C++ test source files and six Markdown documents. The
test sources cover AQL parsing and validation, query execution and optimization,
federation, caching, full-text and geospatial behavior, continuous queries,
resource limits, and related regression cases. This is an inventory of files,
not a claim that every test has been built or run.

## Build and registration

`tests/query/CMakeLists.txt` defines module-focused targets conditionally when
their source files exist. The repository also has query-related targets in
`tests/CMakeLists.txt`; the presence of a `.cpp` file in this directory alone
does not establish that it is registered or passing. Check the applicable CMake
registration before selecting a target, and consult the generated build/CTest
configuration for the active build.

For benchmark registration, see `benchmarks/query/CMakeLists.txt`. There are
currently no Markdown files in `benchmarks/query/`.

## Focused concurrency regressions

The following source-conditional registrations are in
[`CMakeLists.txt`](CMakeLists.txt):

| Test source | Focused target / registration NAME | Source-level coverage |
|---|---|---|
| [`test_continuous_query_lock_order_deadlock.cpp`](test_continuous_query_lock_order_deadlock.cpp) | `test_continuous_query_lock_order_deadlock_focused` / `ContinuousQueryLockOrderDeadlockTests` (180 s) | Concurrent injection/evaluation, registry and result reads, register/drop churn, pending injections, and repeated shutdown. |
| [`test_query_deadlock_concurrent_timeout.cpp`](test_query_deadlock_concurrent_timeout.cpp) | `test_query_deadlock_concurrent_timeout_focused` / `QueryDeadlockConcurrentTimeoutTests` (120 s) | Quick/slow/timeout-pressure scans, concurrent scans on one executor, empty/sequential paths, hash join, aggregation, and concurrent request-ID cancellation. |

The seconds above are CTest process timeouts, not query execution guarantees.
The slow-scan elapsed-time expectation is backend-dependent (12 s with TBB
headers available, 50 s otherwise). Shutdown stress does not deterministically
prove that the 5 s detach branch was reached. Some checks use ordinary thread
joins or `future.get()` after a timed readiness check, so a hung operation can
still require the outer CTest timeout.

For the implementation evidence and distinction between lock-order protection
and cooperative cancellation limits, see
[concurrency and lock ownership](../../src/query/ARCHITECTURE.md#concurrency-and-lock-ownership),
backed by `src/query/continuous_query_engine.cpp`, `src/query/parallel_executor.cpp`,
`src/query/query_engine.cpp`, and `src/query/query_canceller.cpp`.
This is a source/registration inventory, not evidence that these tests were
built or passed.

## Source of truth

Behavior and public contracts are defined by the query implementation and
headers, not by test names or this inventory. Use
[`../../src/query/ROADMAP.md`](../../src/query/ROADMAP.md) for module status and
[`../../include/query/`](../../include/query/) for public declarations.
