---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "CMakeLists.txt"
  - "../../src/retrieval/README.md"
  - "../../src/retrieval/CMakeLists.txt"
---

Exact model backend not verifiable from final document text alone.

# Retrieval focused tests

`CMakeLists.txt` globs `test_*.cpp` and registers each executable as a focused
CTest with the `retrieval` label via `themis_register_module_focused_test`.
The tests are not all integration tests of production retrieval behavior:

| File | What it exercises | Boundary |
|---|---|---|
| `test_dual_read_validator.cpp` | Metric calculations, regression helpers, ground-truth storage, and batch behavior | Compiles `src/retrieval/dual_read_validator.cpp` directly; its `ExecuteQuery()` is still an empty-result placeholder. |
| `test_retrieval_llm_reranking_focused.cpp` | RocksDB reopen persistence, config validation, signature pins/callback, deployment auth, quality allow/deny, and concurrency | Exercises the production `themis_retrieval` target; the authorization callback is a test boundary, not owner/RBAC implementation. |
| `test_retrieval_hybrid_parity_focused.cpp` | Test-local `MockExactRetriever`, `MockANNRetriever`, and `HybridRetrieverEngine` behavior | Self-contained mocks; it does not call a production hybrid retriever. |
| `test_retrieval_lora_trust_boundary_focused.cpp` | `AdapterConsistencyChecker` and a test-local signer allowlist | Exercises LLM LoRA checker code plus a local mock trust registry, not `LoRAManifestStore` signature policy. |
| `test_retrieval_highcardinality_stress.cpp` | Local stub routing table/shards and synthetic concurrency/failover loops | Does not exercise a production retrieval router or shard implementation. |

Build the repository's normal test configuration and run the focused tests:

```sh
cmake -S . -B build
cmake --build build
ctest --test-dir build -L retrieval --output-on-failure
```

The module's test CMake file links focused executables with `themis_retrieval`,
`themis_core`, `spdlog`, threads, and the repository's test libraries. Runtime
test registration and target availability depend on the repository's top-level
test configuration.
