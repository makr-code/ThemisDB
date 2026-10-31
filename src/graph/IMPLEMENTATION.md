> Author: ThemisDB Team
> Created: 2026-10-30
> Last Updated: 2026-10-30
> Status: active
> AI_Used: true
> AI_Models: unknown
> AI_Contributor: GitHub Copilot
> Human_Review: false
> Human_Author: ThemisDB Team
> Source level: level1
> SOT domain: module-behavior
> Canonical source references: [graph public headers](../../include/graph/), [graph implementation](.), [graph focused-test CMake](../../tests/graph/CMakeLists.txt)
> Related issue: #7122
>
> Exact model backend not verifiable from final document text alone.

# Graph implementation workflow

## Responsibility and boundaries

The module implements graph planning, traversal, constraints, rewrite/explain behavior, semantic reasoning, and tensor/graph utilities. It composes graph-index and storage interfaces rather than owning canonical vertex/edge persistence, raw blocks, or WAL. The server layer exposes graph behavior over HTTP/wire; authentication and authorization are not implemented by the graph query constraints. See [architecture](ARCHITECTURE.md) and [trust boundaries](TRUST.md).

## Code-to-test map

| Area | Public surface and implementation | Focused evidence |
|---|---|---|
| Planning and traversal | [graph_query_optimizer.h](../../include/graph/graph_query_optimizer.h), `graph_query_optimizer.cpp` | [optimizer](../../tests/graph/test_graph_query_optimizer.cpp), [exact traversal](../../tests/graph/test_graph_exact_traversal.cpp) |
| Path/semantic constraints | [path_constraints.h](../../include/graph/path_constraints.h), `path_constraints.cpp` | [semantic constraints](../../tests/graph/test_path_constraints_semantic.cpp), [type filtering](../../tests/graph/test_graph_type_filtering.cpp) |
| Parallel/distributed/GPU | [parallel_traversal.h](../../include/graph/parallel_traversal.h), [distributed_graph.h](../../include/graph/distributed_graph.h), [gpu_traversal.h](../../include/graph/gpu_traversal.h) and matching `.cpp` files | [parallel](../../tests/graph/test_graph_parallel_traversal.cpp), [distributed](../../tests/graph/test_graph_distributed.cpp), [GPU](../../tests/graph/test_gpu_traversal.cpp) |
| Reasoning and ontology | [knowledge_graph_reasoner.h](../../include/graph/knowledge_graph_reasoner.h), [ontology_manager.h](../../include/graph/ontology_manager.h) and matching `.cpp` files | [reasoner](../../tests/graph/test_knowledge_graph_reasoner.cpp), [ontology](../../tests/graph/test_ontology_manager.cpp) |
| RotatE completion | [rotate_completion.h](../../include/graph/rotate_completion.h), [rotate_completion.cpp](rotate_completion.cpp) | [unit](../../tests/graph/test_rotate_completion.cpp), [acceptance assertions](../../tests/graph/test_rotate_completion_acceptance.cpp) |
| Tensor fingerprint/dedup | [tensor_fingerprint_graph.h](../../include/graph/tensor_fingerprint_graph.h), `tensor_fingerprint_graph.cpp`, `tensor_deduplication_manager.cpp` | [tensor graph and persistence-hook tests](../../tests/graph/test_tensor_fingerprint_graph.cpp) |

## Invariants and edge cases to preserve

- Validate limits and query constraints before execution. Distributed traversal rejects empty start/target IDs, negative hop counts, and conflicting required/forbidden vertices and records `graph_exact_traversal_errors_total` with operation/reason labels.
- `PathConstraints` mutator methods such as `addForbiddenNode()` and property constraint methods return `void`; invalid IDs, field names, or oversized/embedded-NUL values can be ignored without a returned error. Validate inputs before adding them and inspect the resulting constraint set.
- Treat `DENIAL`, `FALLBACK`, and `REASONING_CONFLICT` differently. The stable categories and recovery hints are defined in [graph_error_taxonomy.h](../../include/graph/graph_error_taxonomy.h); unknown or invalid inputs are not a cue to silently permit an operation.
- CPU fallback is a correctness route for unavailable acceleration, not proof that the original GPU/distributed request succeeded on that backend.
- `DistributedGraphManager` only invokes registered healthy shard executors. Partial outcomes and errors must be interpreted using the operation's implementation and caller contract; do not silently relabel partial work as complete.
- Tensor fingerprint durable hooks and deduplication manager persistence call into storage services. Changes to their payload or journal behavior require the tensor snapshot/recovery tests.
- Keep the graph module free of API transport/authentication ownership and changes to canonical storage schemas; coordinate changes with the owning server, index, or storage module.

## Local test workflow

`tests/graph/CMakeLists.txt` discovers current `test_*.cpp` files and derives each focused target as `module_graph_<stem>_focused` and CTest name as `<stem>_GraphFocusedTests`, where `<stem>` includes the `test_` prefix. Use the actual files present in `tests/graph/` and this CMake registration when selecting validation targets. For example:

```bash
cmake --build --preset linux-release --target module_graph_test_graph_exact_traversal_focused
ctest --preset linux-release -R "test_graph_exact_traversal_GraphFocusedTests" --output-on-failure

# Run the graph-focused tests registered with the module label
ctest --preset linux-release -L "module:graph" --output-on-failure
```

Replace the target/filter with the test covering the changed behavior. This documents the repository's test registration; it does not assert that a local build has been configured or that the tests passed.

## Change discipline

1. Read the relevant public header, implementation, focused tests, and current module [README](README.md)/[ROADMAP](ROADMAP.md) before changing behavior.
2. Update or add a focused regression for the changed contract, especially for failure, fallback, bounds, and concurrency cases.
3. Run the focused test first, then the graph module label when appropriate. Capture actual output for claims about passing tests or performance.
4. Treat [GRAPH_API_CONTRACTS_PHASE1.md](GRAPH_API_CONTRACTS_PHASE1.md) as historical and stale where it disagrees with current declarations. Verify current contracts from headers, implementations, and tests; consult project versioning/governance and affected consumers before compatibility changes. Keep future work in [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md), historical changes in [CHANGELOG.md](CHANGELOG.md), and operational/publication summaries downstream of code.

## Graph developer pages

[RESEARCH](RESEARCH.md) · [MODELING](MODELING.md) · [EMBEDDING](EMBEDDING.md) · [IMPLEMENTATION](IMPLEMENTATION.md) · [API](API.md) · [MODULE_GUIDE](MODULE_GUIDE.md) · [RUNBOOK](RUNBOOK.md) · [OPERATIONS](OPERATIONS.md) · [TRUST](TRUST.md)
