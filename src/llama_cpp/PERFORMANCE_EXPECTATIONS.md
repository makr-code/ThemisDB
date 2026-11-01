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

# llama_cpp Performance and Benchmark Evidence

## Scope

The benchmark source contains real-model fixture benchmarks and four LLCPG
cases. CMake has an intended `bench_llama_cpp_inference` target block, but its
guard checks for `bench_llama_cpp_inference.cpp` directly under the benchmarks
directory while the checked-in source is `benchmarks/llama_cpp/`. The expected
root-level file is absent, so target registration may be skipped. Verify the
configured build before assuming the target exists.

## LLCPG cases

All four LLCPG cases call `loadModel()` with an empty model path. They do not
load a GGUF model. Their source-reported limits are emitted as benchmark
counters; they are not checked as assertions and do not by themselves establish
a release pass.

| Case | Workload and setup | Recorded threshold / limitation |
|---|---|---|
| LLCPG-1 | `generate()`; 20 real-time iterations | Emits 5 ms stub maximum and 1100 ms production P95 counters. No production model or P95 gate is measured. |
| LLCPG-2 | 100 `embed()` calls per iteration; 10 real-time iterations | Emits an 8500 tok/s counter. With no wrapper or injected callback the plugin returns 384 zeroes, so this is not semantic embedding throughput. |
| LLCPG-3 | LoRA registry `loadLoRA()` plus `unloadLoRA()`; 100 real-time iterations | Emits a 75 ms maximum counter. The benchmark reports registry-operation timing; it does not calculate an adapter-load P99. |
| LLCPG-4 | `generate()`; minimum one second | Emits an 8% regression-limit counter and `baseline_mode_stub=1`; it does not compare a baseline or fail on a regression. |

The benchmark target does not itself define `THEMIS_LLAMA_CPP_STUB_MODE`.
Without a real wrapper or that compile definition, the no-model `generate()`
path returns an error rather than a synthetic response. `embed()` with an
empty-path loaded plugin and no injected callback returns the zero-vector
fallback.

## Real-model evidence

The fixture-based cases require `THEMIS_BENCH_LLAMA_MODEL_PATH` (environment
variable or compile-time definition), an existing model file, a successful
preflight where applicable, and a successful generation warmup. They skip on
missing/invalid input or failed warmup. Confirm the build enables
`THEMIS_LLM_ENABLED`; the benchmark target's CMake block does not define this
macro itself.

For meaningful real-model comparisons, record the exact artifact identity,
backend/build flags, hardware, context and generation settings, warmup
procedure, and repeated-run distributions. The checked-in benchmark source
does not establish the model-specific numeric release targets previously
listed in this document. Production acceptance therefore requires separate
real-backend evidence; stub/fallback results are only plugin-path baselines.

## Running the benchmark

If the configured build creates the target, run the executable from its actual
build output location and pass `--benchmark_filter=LLCPG` to select these cases.
No root-level executable path or target availability is guaranteed by the
current CMake guard.

Use the Google Benchmark comparison tooling for manual baseline comparison.
The LLCPG-4 counter is descriptive and is not an automated comparison gate.
