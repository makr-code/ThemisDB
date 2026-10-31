---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# GPU Integration for Embedding Workloads

## Purpose and ownership

This page is for developers connecting an embedding or vector workload to GPU
resources. `src/gpu` is the runtime/resource layer; it does not define a
semantic embedding model, tokenizer, vector dimension contract, or the
meaning of an embedding result. Those contracts belong to the LLM and index
consumers.

The GPU module provides the adjacent mechanisms: backend discovery and
dispatch, bounded VRAM accounting, tensor buffers, streams, and fallback
status. The module overview is in [README.md](README.md); the resource and
dispatch assumptions are in [MODELING.md](MODELING.md).

## Integration boundary

Before adding an embedding path:

1. Identify the owning consumer API and its tensor shape, type, device, and
   lifetime requirements. Do not infer these from the GPU facade.
2. Use the existing GPU allocation/policy contract for device memory and
   provide a tenant identifier when the caller has one.
3. Gate backend-specific work on both build-time support and runtime
   availability/feature state.
4. Preserve the consumer's established output and error semantics when a
   backend is unavailable, an allocation is denied, or dispatch fails.
5. Test the consumer's output at the integration boundary; a successful
   allocation or dispatch status is not a vector-quality or semantic check.

Relevant integration surfaces include
[`GPUModule`](../../include/themis/gpu/gpu_module.h),
[`GPUConfig`](../../include/themis/gpu/config.h), and
[`IVRAMPolicy`](../../include/themis/gpu/ivram_policy.h). The module contains
[`tensor_buffer.cpp`](tensor_buffer.cpp) and accelerated query/training
orchestration, while the actual vector-index implementations are registered
outside this directory in `cmake/ModularBuild.cmake`.

## Evaluation and limits

Embedding evaluation must separate at least:

- numerical/output parity against the consumer's CPU implementation;
- allocation, transfer, and kernel latency for the real tensor shape;
- peak VRAM and quota behavior under concurrent tenants;
- behavior after runtime unavailability and during fallback.

Use the consumer's own correctness fixtures and the relevant GPU benchmark
mapping in [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md). Do not
report a GPU backend as an embedding-quality improvement based only on
throughput or a successful kernel launch. If a consumer has no CPU parity path,
record that as an explicit limitation rather than assuming one from the GPU
module's fallback status.
