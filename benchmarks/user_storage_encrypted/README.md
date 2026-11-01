---
Author: ThemisDB Contributors
Created: 2026-06-01
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Benchmarks for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This page distinguishes benchmark source coverage from real storage
measurements and from CMake/CTest registration. The source comments contain
candidate thresholds, but the benchmark programs do not fail when a threshold
is exceeded. No measured baseline is asserted here.

## Benchmark sources and what they measure

| Source | Actual workload |
|---|---|
| `bench_user_storage_encrypted_release_gates.cpp` | Error-enum casts, switch dispatch, descriptor construction, and batch enum casts. It does not mount containers or measure encrypted I/O. |
| `bench_user_storage_encrypted_lifecycle_gates.cpp` | Placeholder sleeps and in-process simulation for mount/unmount, KDF, rotation, concurrent mounts, and writes. It does not call the production backend, perform encrypted file I/O, or contact Vault. Its labels/comments are not measured performance guarantees. |
| `bench_encrypted_storage_dedicated_gates.cpp` | In-process stub map, string-prefix “encrypt/decrypt,” and stub rotation. It is not representative encrypted storage performance. |
| `bench_user_storage_mount_latency.cpp` | Calls backend initialization, availability, mount-state, fast-fail, and mount/unmount API paths. The cycle may invoke gocryptfs when available; the other cases include probes, missing-path behavior, or API overhead. This source is not registered in the directory CMakeLists. |

## Build and registration

`benchmarks/user_storage_encrypted/CMakeLists.txt` is guarded by
`THEMIS_BUILD_BENCHMARKS` and defines targets for:

- `bench_user_storage_encrypted_release_gates`
- `bench_user_storage_encrypted_lifecycle_gates`
- `bench_encrypted_storage_dedicated_gates` (when its source exists)

Only the lifecycle target is explicitly registered through
`themis_register_benchmark_ctest()`, with module label
`user_storage_encrypted`, timeout 300 seconds, and extra label
`release_critical`. The release-gates and dedicated-gates targets are built as
benchmarks but are not explicitly registered by this directory's CMake file.
`bench_user_storage_mount_latency.cpp` is not included in the listed CMake
targets.

Example:

```bash
cmake --preset linux-release -D THEMIS_BUILD_BENCHMARKS=ON
cmake --build --preset linux-release --target bench_user_storage_encrypted_release_gates
cmake --build --preset linux-release --target bench_user_storage_encrypted_lifecycle_gates
ctest --preset linux-release -L user_storage_encrypted --output-on-failure
```

Executable locations depend on the configured build output directory. Run a
benchmark executable directly to collect its measurements; a CTest smoke
registration is not itself a baseline or a pass/fail performance threshold.

## Performance evidence status

The numeric limits in the C++ comments and benchmark labels are proposed
targets only. The repository sources inspected here do not establish
hardware-specific p50/p95/p99 baselines, a regression comparison workflow, or
automated threshold enforcement for all cases. Benchmark runs and representative
hardware capture are required before making latency or throughput claims.

## Source references

- `benchmarks/user_storage_encrypted/CMakeLists.txt`
- `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_release_gates.cpp`
- `benchmarks/user_storage_encrypted/bench_user_storage_encrypted_lifecycle_gates.cpp`
- `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp`
- `benchmarks/user_storage_encrypted/bench_user_storage_mount_latency.cpp`
