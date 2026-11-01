---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_Sources:
  - "src/onnx_clip/onnx_clip_plugin.cpp"
  - "benchmarks/onnx_clip/bench_onnx_clip_cpu.cpp"
  - "benchmarks/onnx_clip/bench_onnx_clip_vit_backend.cpp"
  - "benchmarks/onnx_clip/README.md"
Related_Issue: "#7365"
---

Exact model backend not verifiable from final document text alone.

# ONNX CLIP performance evidence

## Current status

No source-backed performance target or measured result is established for
`ONNXClipPlugin`. Its current implementation generates deterministic
hash/PRNG-based reference vectors; it does not execute a CLIP model. The two
benchmark programs under `benchmarks/onnx_clip/` use mock workloads rather than
calling the plugin, so their targets and checked-in baseline values cannot be
used as plugin latency, throughput, memory, or release-gate evidence.

The former OC/ONXG/NG tables and benchmark mappings in this file are retired:
their cited cases and release thresholds are not validated for the current
implementation. No replacement numeric values are asserted here.

## Evidence required before publishing targets

For model-backed performance claims, first provide benchmark code that exercises
the production plugin and a real, identified model artifact. Record the runtime
and provider, hardware/build configuration, input and batch sizes, measurement
method, repeated results, and memory methodology. Keep proposed targets clearly
separate from observed results and from release-blocking policy.

See [`../../benchmarks/onnx_clip/README.md`](../../benchmarks/onnx_clip/README.md)
for the boundary of the current benchmark sources and
[`ROADMAP.md`](./ROADMAP.md) for the module's pending model-backed evidence.
