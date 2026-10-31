---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: deprecated
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Index Header Guide (Archived)

This legacy guide has been retired. Its former API examples, configuration
names, performance figures, and readiness claims were not consistently
supported by current declarations and are intentionally not reproduced here.

Use the source-checked [Index API reference](../../src/index/API.md) for current
public API summaries. The declarations under this directory remain the
authoritative contracts; in particular, see
[`vector_index.h`](vector_index.h),
[`gpu_vector_index.h`](gpu_vector_index.h),
[`multi_gpu_vector_index.h`](multi_gpu_vector_index.h), and
[`index_manager.h`](index_manager.h).

For module architecture and implementation boundaries, see
[`../../src/index/ARCHITECTURE.md`](../../src/index/ARCHITECTURE.md). Test
registration and limitations are documented in
[`../../tests/index/README.md`](../../tests/index/README.md).
