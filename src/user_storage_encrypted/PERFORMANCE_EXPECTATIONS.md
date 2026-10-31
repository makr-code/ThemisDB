---
Author: ThemisDB Contributors
Created: 2026-08-08
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Performance Expectations - user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This page maps existing benchmark sources to the behavior they actually
measure. The repository sources inspected do not establish benchmark results,
hardware-specific baselines, or automated enforcement of the numeric labels
in benchmark comments.

## Benchmark mapping

| Source | Cases | Evidence class / limit |
|---|---|---|
| `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp` | Backend initialization, availability, mount-state, fast-fail paths, optional API mount cycle, dispatch | API-level probe/fast-fail measurements; cycle may invoke gocryptfs; not registered by the module CMake file |
| `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates.cpp` | Enum cast, switch dispatch, descriptor construction, batch casts | Microbenchmarks of contract/data handling; not encrypted mount or I/O |
| `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp` | Named mount/unmount/KDF/rotation/concurrency/write cases | Sleep-based placeholders and simulations; not production runtime measurement |
| `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp` | Stub write/read/rotation/string enc-dec | In-process stub only; not production encryption or storage |

`benchmarks/user_storage_encrypted/CMakeLists.txt` registers release-gates,
lifecycle-gates, and dedicated-gates targets when benchmarks are enabled. It
explicitly registers only the lifecycle-gates target as a CTest benchmark
smoke entry with `user_storage_encrypted` and `release_critical` labels.

## Current evidence status

- The benchmark C++ sources contain candidate thresholds/labels, but do not
  assert them as pass/fail gates.
- No benchmark result or baseline capture is recorded here.
- Representative hardware, gocryptfs/FUSE version, compiler/build mode, and
  repeated-run data are needed before publishing p95/p99 or throughput claims.
- Simulated and stub-backed cases must not be used to claim user-visible
  encrypted-storage performance.

See [benchmark directory notes](../../benchmarks/user_storage_encrypted/README.md)
for build and registration details.
