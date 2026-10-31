---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: ThemisDB documentation orchestration specialist
Human_Review: false
Human_Author: unknown
Source level: level1
SOT domain: module-behavior
Canonical source references:
  - ../../include/graph/
  - ../../src/graph/
  - ../../tests/graph/
Related milestone: DOC-WEEKLY-2026-44
AI_Model_Note: Exact model backend not verifiable from final document text alone.
---

# Graph feature map

The current graph source tree contains these implementation areas:

- query planning/execution, path constraints, rewrite and explain (`graph_query_optimizer`,
  `path_constraints`, `graph_query_rewriter`, `explain_plan`);
- parallel, GPU-oriented, and shard-executor traversal (`parallel_traversal`, `gpu_traversal`,
  `distributed_graph`);
- ontology and rule/fact reasoning (`ontology_manager`, `knowledge_graph_reasoner`);
- RotatE completion and separate tensor-fingerprint/deduplication utilities;
- scheduled edge refresh and graph watermark utilities.

This list identifies code surfaces only. It does not assert that every optional backend is enabled,
that every API is production-ready, or that performance acceptance criteria have been met. Use
the public headers, focused tests, and current CMake registration for specific claims.

See [API.md](API.md), [ARCHITECTURE.md](ARCHITECTURE.md), and
[IMPLEMENTATION.md](IMPLEMENTATION.md).
