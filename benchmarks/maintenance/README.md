# benchmarks/maintenance

---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
---

Exact model backend not verifiable from final document text alone.

Mirrored benchmark folder for `src/maintenance`.

`benchmarks/CMakeLists.txt` adds this directory when `THEMIS_BUILD_BENCHMARKS`
is enabled. It registers `bench_maintenance_release_gates` and
`bench_maintenance_distributed_gates`. The sources define synthetic/in-memory
proxies and thresholds; they do not establish measured production-orchestrator
performance or passing release baselines.

## Build and run

The `nightly-bench-sweep` preset enables benchmarks and uses
`build-gcc-linux-nightly`; it requires the Linux/vcpkg toolchain dependencies:

```sh
cmake --preset nightly-bench-sweep
cmake --build build-gcc-linux-nightly --target \
  bench_maintenance_release_gates bench_maintenance_distributed_gates
ctest --test-dir build-gcc-linux-nightly \
  -R '^bench_maintenance_(release|distributed)_gates$' \
  --output-on-failure
```

The benchmark tests execute source-defined cases. Capture their output and
baseline comparison separately; target registration and manifest `PASS` labels
are not run-result evidence. Although source comments and the manifest call the
limits p99, the C++ cases report repeated aggregates rather than calculating a
p99 percentile; verify any external percentile-processing step before treating
these as measured p99 gates.
