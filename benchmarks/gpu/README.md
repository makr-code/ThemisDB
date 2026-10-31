# GPU Benchmarks

This directory contains GPU-related benchmark sources. It is not a mirror of
`src/gpu/`; benchmark files here include module-control, backend, vector-index,
training, transfer, and subsystem measurements.

The root `benchmarks/CMakeLists.txt` registers most GPU benchmark targets and
conditions some on backend availability. `benchmarks/gpu/CMakeLists.txt`
registers `bench_gpu_dedicated_gates` when benchmarks are enabled. Consult
those CMake files for the current target list; the presence of a source file
does not guarantee its target is enabled or has been executed.

For the source-to-case map and evidence expectations, see
[`../../src/gpu/PERFORMANCE_EXPECTATIONS.md`](../../src/gpu/PERFORMANCE_EXPECTATIONS.md).
