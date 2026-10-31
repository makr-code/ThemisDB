---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Analytics benchmark validation checklist

Use this checklist for a benchmark change. It records work to perform; it is
not a claim that the current benchmark suite has passed.

## Source and build

- [ ] Confirm the benchmark target is registered in the current CMake files.
- [ ] Confirm each case name and workload against its C++ source.
- [ ] Identify whether the case calls a production analytics API or a local
  simulation/stub; label the result accordingly.
- [ ] Configure with `THEMIS_BUILD_BENCHMARKS=ON` and build only the target(s)
  under test.
- [ ] Use `--benchmark_list_tests` to confirm the cases available in the built
  executable.

## Execution evidence

- [ ] Record the commit, compiler, build type, platform, CPU model, and exact
  command.
- [ ] Save machine-readable output (for example, Google Benchmark JSON).
- [ ] Compare against a baseline only when hardware, build settings, and
  workload are sufficiently comparable.
- [ ] Report measured values and artifacts; do not describe an unrun threshold
  as passed.

## Correctness and interpretation

- [ ] Run the focused analytics unit tests for behavior changed by the
  benchmarked code path.
- [ ] Separate benchmark smoke-test success from performance acceptance.
- [ ] State limitations such as synthetic workloads, stubs, optional backend
  availability, and unvalidated hardware profiles.

See [`INDEX.md`](./INDEX.md) for registered targets and
[`BENCH_CI_INTEGRATION_GUIDE.md`](./BENCH_CI_INTEGRATION_GUIDE.md) for build and
execution examples.
