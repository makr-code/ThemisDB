---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources: "benchmarks/llama_cpp/bench_llama_cpp_inference.cpp; benchmarks/CMakeLists.txt; src/llama_cpp/llama_cpp_plugin.cpp"
Related_Issue: "#7356"
Related_Milestone: "DOC-WEEKLY-2026-44"
---

> Exact model backend not verifiable from final document text alone.

# benchmarks/llama_cpp

Benchmarks for the `src/llama_cpp` plugin. `benchmarks/CMakeLists.txt` contains
an intended `bench_llama_cpp_inference` target block, but its existence guard
checks for `bench_llama_cpp_inference.cpp` directly under the benchmarks
directory. The checked-in source is instead under `benchmarks/llama_cpp/`, and
the root-level file is absent. Consequently, the guard is false for this tree
and CMake may skip target creation. Build registration is not verified; do not
assume a root-level `./bench_llama_cpp_inference` binary exists.

## LLCPG benchmark cases

The LLCPG cases use an empty model path; they do not load a GGUF model. Their
results are plugin/fallback measurements, not real-model latency or embedding
throughput. The benchmark target itself does not define
`THEMIS_LLAMA_CPP_STUB_MODE`: generation is synthetic only when that definition
is supplied by the build; otherwise the no-wrapper path returns an error.

| Case | Workload | Source-reported threshold | Measurement limitation |
|---|---|---|---|
| LLCPG-1 | `generate()` with an empty model path; 20 real-time iterations | 5 ms stub maximum and 1100 ms production P95 are emitted as counters | Does not run or gate a production-model P95 |
| LLCPG-2 | `embed()` on 100 identical documents; 10 real-time iterations | 8500 tok/s is emitted as a counter | Empty-path fallback returns zero vectors; not embedding throughput |
| LLCPG-3 | LoRA registry `loadLoRA()`/`unloadLoRA()`; 100 real-time iterations | 75 ms maximum is emitted as a counter | Measures registry metadata operations, not adapter loading or computed P99 |
| LLCPG-4 | `generate()` with an empty model path; minimum one second | 8% regression maximum is emitted as a counter | Stub/no-model baseline; the source does not enforce the threshold |

These threshold counters describe values recorded by the benchmark source; they
are not assertions or an automatic release-gate result.

### Running only LLCPG cases

Only if the configured build actually creates the target, run its executable
from the build's reported output location with Google Benchmark's
`--benchmark_filter=LLCPG` and desired output/repetition options. This page
does not prescribe a guaranteed executable path.

### Comparing runs

Save baseline and current results with Google Benchmark's JSON output, then use
the comparison utility from the Google Benchmark installation/vendor source.
This repository does not provide `tools/compare.py`.

The LLCPG-4 source records an 8% threshold counter, but does not itself compare
against a baseline or fail the benchmark when that value is exceeded.

## Real-model benchmarks

The other fixture-based inference cases require a model path through
`THEMIS_BENCH_LLAMA_MODEL_PATH` (environment or compile-time definition), a
readable artifact, and a successful warmup. They skip on missing/invalid model
input or failed warmup. Confirm the build enables the real LLM backend and
record the artifact and hardware before interpreting those measurements as
real-model evidence.
