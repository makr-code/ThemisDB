---
Author: "ThemisDB Team"
Created: "2026-05-13"
Last Updated: "2026-10-31"
Status: deprecated
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Advanced Index Features (Archived)

This feature guide has been retired because its API examples, configuration
fields, readiness labels, and performance claims no longer consistently match
the current declarations. The old examples are intentionally not retained as
working usage instructions.

Use the source-checked [Index API reference](API.md) for current public
contracts and [module architecture](ARCHITECTURE.md) for implementation
boundaries. Feature-specific declarations are authoritative:

- [`gpu_vector_index.h`](../../include/index/gpu_vector_index.h) — single-GPU
  configuration, initialization, batch search, and the explicit
  `allowCPUFallback` policy.
- [`multi_gpu_vector_index.h`](../../include/index/multi_gpu_vector_index.h) —
  multi-device API and partition/merge surface. Its header describes the
  current implementation as CPU-only scaffolding; hardware execution and
  collectives are listed there as planned.
- [`gpu_memory_oversubscription.h`](../../include/index/gpu_memory_oversubscription.h) —
  oversubscription manager interface. Do not infer that every configuration
  field in older examples is part of `GPUVectorIndex::Config`.
- [`vector_index.h`](../../include/index/vector_index.h) — storage-backed
  vector index and optional ANN integration.

The relevant tests are registered conditionally through
[`../../tests/index/CMakeLists.txt`](../../tests/index/CMakeLists.txt); file
presence alone does not establish that a test target is enabled. GPU hardware
parity and performance are not established by this document. See the
[module roadmap](ROADMAP.md) for tracked validation gaps.
