---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-11-18"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# AI Module Performance Expectations

## Scope and evidence

This page covers `AIPluginGenerator` validation and local orchestration only.
CAI callback runtime and provider inference require separate measurements.
Vector storage, plugin lifecycle, RAG, and inference-serving claims belong to
their owning modules.

The registered benchmark is
[`benchmarks/ai/bench_ai_plugin_generator.cpp`](../../benchmarks/ai/bench_ai_plugin_generator.cpp):

| Case | Measured path | Excluded |
|---|---|---|
| `BM_AIPluginGeneratorValidatePrompt` | local prompt validation | transport/provider |
| `BM_AIPluginGeneratorGeneratePlugin` | local validation, canned callback, parse/map path | provider queueing/inference, output quality, cost |
| `BM_AIPluginGeneratorErrorPathMalformedJson` | local malformed-JSON rejection | provider behavior |

These are callback-driven microbenchmarks, not production throughput or
provider-latency evidence. The checked-in benchmark does not establish a CAI
gate or live-provider baseline.

## Proposed release gates

| Gate | Threshold / condition | Evidence |
|---|---|---|
| Local orchestration regression | p99 no more than 10% slower than same-build, same-hardware baseline | versioned benchmark results and environment metadata |
| Benchmark completeness | 100% of the three registered cases emit results | release benchmark manifest |
| Retry bound | at most 3 attempts; 100 ms then 200 ms delay; no transport-route failover | focused transport tests |
| Invalid-state safety | invalid prompt/config/gate state must fail before endpoint I/O; no invalid or fallback result passes acceptance | focused AI tests |
| Provider-backed performance | no numeric target is ratified yet; collect p50/p95/p99 and error rate on a versioned acceptance set | provider/model/prompt/config provenance and repeated runs |

The p99 regression and benchmark-completeness thresholds are proposed gates
from [`ROADMAP.md`](./ROADMAP.md); they are not current benchmark results.
Provider-backed quality, latency, throughput, and cost targets require at
least three repeated runs of a versioned task set and maintainer ratification
before release enforcement.

## Timeout and tail-latency limits

The built-in curl transport applies `timeout_ms` to each attempt. An injected
`endpoint_invoke_fn` receives the timeout but must enforce it itself.
`setLlmHttpPostFn()` has no timeout parameter and may block indefinitely. The
three-attempt retry count is bounded, but a wall-clock bound is not guaranteed
for callbacks that ignore or cannot receive a timeout. Do not claim a bounded
end-to-end latency baseline for those callback routes.
