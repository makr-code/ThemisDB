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

# Chaos module source/build review

## Scope and limits

This is a static review of current module source and CMake wiring. No build,
focused test, benchmark, security audit, or deployment validation was run as
part of this review. Therefore, no test pass, performance result, or production
readiness conclusion is asserted.

## Verified evidence

- Runtime implementation: `src/chaos/chaos_framework.cpp`.
- Public declarations and recommended contract: `include/chaos/chaos_framework.h`
  and `include/chaos/chaos_contract.h`.
- `tests/chaos/CMakeLists.txt` globs `test_*.cpp`; eight matching test source
  files are present.
- `bench_chaos_release_gates` and `bench_chaos_microbenchmarks` have conditional
  CMake target definitions and benchmark CTest registrations.
- The `bench_chaos_stress.cpp` source exists under `benchmarks/chaos/`, but its
  CMake condition checks `benchmarks/bench_chaos_stress.cpp` at the wrong path.
  The condition is false in this checkout, so that target is not added.
- `THEMIS_CHAOS_ADMIN` defaults to `OFF`; CMake conditionally adds the handler
  source. However, the compile definition is scoped to `themis_server` while
  the guarded route/handler sources belong to module targets; additionally,
  `http_server.cpp` calls a no-argument `ChaosScheduler` constructor that the
  public header does not declare. The enabled route is therefore a **TODO**, not
  a verified build capability.

## Open verification items

1. **TODO:** Correct or confirm the stress benchmark CMake path before relying on
   that benchmark as a built target.
2. **TODO:** Execute the focused registered tests and record results.
3. **TODO:** Build and run benchmarks, then establish whether emitted threshold
   counters are compared/enforced by CI.
4. **TODO:** Fix and build the optional admin integration with
   `THEMIS_CHAOS_ADMIN=ON`; verify the compile-definition propagation,
   `ChaosScheduler` construction, route dispatch, and deployment controls before
   claiming that the endpoint is usable.

Source references: [`../../include/chaos/chaos_contract.h`](../../include/chaos/chaos_contract.h),
[`../../tests/chaos/CMakeLists.txt`](../../tests/chaos/CMakeLists.txt),
[`../../benchmarks/CMakeLists.txt`](../../benchmarks/CMakeLists.txt), and
[`ARCHITECTURE.md`](ARCHITECTURE.md).
