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
> Canonical source references: [graph module README](README.md), [architecture](ARCHITECTURE.md), [public graph headers](../../include/graph/)
> Related issue: #7122
>
> Exact model backend not verifiable from final document text alone.

# Graph module contributor guide

## Start here

The graph module is the C++ graph-planning and execution layer. Begin with [README.md](README.md) for scope, [ARCHITECTURE.md](ARCHITECTURE.md) for integration planes, and [ROADMAP.md](ROADMAP.md) for active work. Use [API.md](API.md) to find interfaces and [IMPLEMENTATION.md](IMPLEMENTATION.md) for a code-to-test workflow.

## Architecture map

1. **Planning and execution:** optimizer plans, graph rewrites/explanations, bounded local traversal.
2. **Execution variants:** parallel multi-source traversal, shard-executor coordination, and GPU-assisted CSR traversal with fallback status.
3. **Semantic layer:** ontology concepts/axioms, facts/rules, inference chains, and constrained paths.
4. **Tensor utilities:** tensor-train fingerprint similarity and deduplication hooks; this is separate from knowledge-graph link prediction.
5. **Maintenance helpers:** temporal watermarks and scheduled edge refresh integrate with graph-index, changefeed, ANN, and analytics interfaces.

The graph module consumes graph-index/storage interfaces and is used by query and server layers. It does not own the canonical storage engine, HTTP/wire API, API authentication/authorization, deployment lifecycle, or client visualization. See [ARCHITECTURE.md](ARCHITECTURE.md), [include/index/graph_index.h](../../include/index/graph_index.h), and [include/server/graph_api_handler.h](../../include/server/graph_api_handler.h).

## Where to make changes

- Public behavior or types: `include/graph/*.h`; verify exact consumers and update the focused tests.
- Algorithm/runtime behavior: matching source under `src/graph/`.
- Graph record/index behavior: coordinate with `include/index/graph_index.h` and its owning module; do not implement a parallel persistence format in this module.
- Query integration: coordinate with query-layer graph functions and check the rewriter/optimizer tests.
- Operational symptom or recovery guidance: update [RUNBOOK.md](RUNBOOK.md) or [OPERATIONS.md](OPERATIONS.md) only when source behavior changes.
- Security boundary: graph-level constraints are not authorization. Changes that affect request authorization belong with the server/API access-control owner.

## Tests and regressions

Focused suites are located in [tests/graph](../../tests/graph/). Pick the narrowest relevant test first:

- planning and exact traversal: `test_graph_query_optimizer.cpp`, `test_graph_exact_traversal.cpp`;
- constraints and reasoning: `test_path_constraints_semantic.cpp`, `test_ontology_manager.cpp`, `test_knowledge_graph_reasoner.cpp`;
- parallel/distributed/acceleration: `test_graph_parallel_traversal.cpp`, `test_graph_distributed.cpp`, `test_gpu_traversal.cpp`;
- tensor and learned completion: `test_tensor_fingerprint_graph.cpp`, `test_rotate_completion.cpp`, `test_rotate_completion_acceptance.cpp`.

The target registration and repository test commands are documented in [tests/graph/README.md](../../tests/graph/README.md). Never report a test as passing unless it was run and its output reviewed. Benchmark case names in [PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md) are not benchmark results.

## Concurrency and failure review

Before changing shared state, follow the synchronization in the concrete implementation rather than assuming all graph classes share one thread-safety policy. Preserve the graph error taxonomy and distinguish invalid preconditions, retryable fallback, and reasoning conflict. Include tests for timeout, empty/malformed IDs, conflicts between required and forbidden values, missing shards/devices, and result truncation where relevant.

## Documentation map

The nine developer pages are intentionally separated by purpose: research/evaluation ([RESEARCH](RESEARCH.md)), data/algorithm representation ([MODELING](MODELING.md)), learned embeddings ([EMBEDDING](EMBEDDING.md)), contributor workflow ([IMPLEMENTATION](IMPLEMENTATION.md)), C++ interfaces ([API](API.md)), this onboarding page, and operator guidance ([RUNBOOK](RUNBOOK.md), [OPERATIONS](OPERATIONS.md), [TRUST](TRUST.md)). Canonical module docs remain [README](README.md), [ARCHITECTURE](ARCHITECTURE.md), [ROADMAP](ROADMAP.md), [SECURITY](SECURITY.md), and [FUTURE_ENHANCEMENTS](FUTURE_ENHANCEMENTS.md).

## Graph developer pages

[RESEARCH](RESEARCH.md) · [MODELING](MODELING.md) · [EMBEDDING](EMBEDDING.md) · [IMPLEMENTATION](IMPLEMENTATION.md) · [API](API.md) · [MODULE_GUIDE](MODULE_GUIDE.md) · [RUNBOOK](RUNBOOK.md) · [OPERATIONS](OPERATIONS.md) · [TRUST](TRUST.md)
