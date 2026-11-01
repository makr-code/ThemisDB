> Author: ThemisDB Team
> Created: 2026-10-30
> Last Updated: 2026-10-31
> Status: active
> AI_Used: true
> AI_Models: unknown
> AI_Contributor: ThemisDB documentation orchestration specialist
> Human_Review: false
> Human_Author: ThemisDB Team
> Source level: level1
> SOT domain: module-behavior
> Canonical source references: [module graph roadmap](ROADMAP.md), [root maturity roadmap](../../ROADMAP.md), [graph headers](../../include/graph/), [graph tests](../../tests/graph/)
> Related issue: #7122
> Related milestone: DOC-WEEKLY-2026-44
>
> Exact model backend not verifiable from final document text alone.

# Graph research notes

This page records research questions, evaluation methods, assumptions, and trade-offs for graph work. It is not a release-status report and does not replace the implementation contracts in headers.

## Research scope and assumptions

- The graph module plans and executes graph operations over interfaces such as `GraphIndexManager`; it is not the owner of the persisted graph or its low-level storage format.
- Evaluation must distinguish exact graph results from model-ranked predictions. A link-prediction score is not a graph fact or an authorization decision.
- Query cost, latency, and candidate count depend on graph shape, constraints, data locality, and enabled execution paths. No performance result should be inferred from the existence of a benchmark fixture.
- GPU, distributed, and parallel paths require result-parity and failure-path evaluation against exact CPU/local behavior; unsupported-path fallback is part of the behavior under test.

## Methods and evidence

Use the following evidence sequence for a proposed graph algorithm, optimizer, or learned component:

1. Define the inputs, output semantics, constraints, and failure behavior from the relevant public header: for example [optimizer](../../include/graph/graph_query_optimizer.h), [path constraints](../../include/graph/path_constraints.h), and [RotatE completion](../../include/graph/rotate_completion.h).
2. Establish functional and edge-case correctness with focused tests. Relevant suites include [optimizer tests](../../tests/graph/test_graph_query_optimizer.cpp), [exact traversal error tests](../../tests/graph/test_graph_exact_traversal.cpp), [constraint tests](../../tests/graph/test_path_constraints_semantic.cpp), and [reasoner tests](../../tests/graph/test_knowledge_graph_reasoner.cpp).
3. For link prediction, separate training and evaluation triples, state the fixture composition and ranking metric, and compare the candidate method with the in-test baseline. The current [RotatE acceptance test](../../tests/graph/test_rotate_completion_acceptance.cpp) asserts MRR ≥ 0.10, Hits@10 ≥ 0.40, relative-to-TransE bounds, and top-20 p99 below 50 ms on a deterministic, small in-repository fixture. These are test thresholds, not captured evaluation results or external-dataset evidence.
4. For runtime performance, use the mapped cases in [performance expectations](PERFORMANCE_EXPECTATIONS.md) and benchmark sources such as [graph traversal](../../benchmarks/graph/bench_graph_traversal.cpp), [optimizer](../../benchmarks/graph/bench_graph_query_optimizer.cpp), and [tensor fingerprint graph](../../benchmarks/tensor/bench_tensor_fingerprint_graph.cpp). Record the build profile, baseline, workload, hardware, and actual run output before making quantitative claims.

The test and benchmark paths above identify validation surfaces, not successful runs. No new benchmark or model evaluation was run while creating this document.

## Trade-offs to evaluate

| Choice | Benefit to test | Cost or limitation to measure |
|---|---|---|
| BFS vs. DFS | BFS gives level-order exploration; DFS can follow deep branches with a bounded depth. | Branching factor and result caps can dominate either algorithm. |
| Dijkstra vs. unweighted traversal | Dijkstra uses edge weights for shortest-path cost. | Weight interpretation and overflow/invalid-weight behavior must be validated for the caller's data. |
| Local/parallel vs. distributed traversal | Independent starts or shards can be scheduled concurrently. | Merge behavior, timeouts, unhealthy shards, partial results, and reproducibility must be measured and tested. |
| GPU route vs. CPU route | A device route may be worth evaluating for sufficiently large inputs. | Loading/copying graph data and device availability add failure modes; CPU fallback correctness remains required. |
| RotatE vs. a simpler embedding baseline | RotatE represents relations as phase rotations in complex-valued entity space. | Learned rankings depend on fixture, initialization, training configuration, and evaluation protocol; scores are model distances, not calibrated truth probabilities. |
| LSH candidate filtering vs. exhaustive tensor comparison | Fingerprint buckets can limit similarity candidates before exact tensor comparison. | Candidate caps and approximate signatures can omit candidates; measure recall against exhaustive comparison on representative data. |

These are evaluation questions, not claims that one route is faster or more accurate.

## Open evidence gaps

- The tests expose deterministic behavior and acceptance assertions, but this page has no captured run output proving current results.
- The repository-level [ROADMAP.md](../../ROADMAP.md) distinguishes the RotatE quality targets of MRR ≥ 0.35 and Hits@10 ≥ 0.55 from the current acceptance-test floor: MRR ≥ 0.10, Hits@10 ≥ 0.40, relative-to-TransE bounds, and p99 top-20 latency below 50 ms. The acceptance assertions are thresholds, not captured evaluation results; the higher quality targets remain unverified until separately measured and recorded.
- Broad external graph datasets, workload distributions, cross-hardware parity, and sustained multi-tenant load are not established by the small in-repository fixtures cited above.
- The `IGraphEmbeddingProvider` declaration has no matching `src/graph/graph_embedding.cpp` implementation in the inspected graph source inventory. Do not infer a production backend from the interface or its enum.

## Canonical references

- [Graph roadmap](ROADMAP.md) and [future enhancements](FUTURE_ENHANCEMENTS.md) for active plans and research areas.
- [Graph architecture](ARCHITECTURE.md), [API contracts](GRAPH_API_CONTRACTS_PHASE1.md), and [performance expectations](PERFORMANCE_EXPECTATIONS.md) for module context and benchmark mapping.
- [Graph API headers](../../include/graph/) and [focused tests](../../tests/graph/) take precedence over this synthesis for current behavior.

## Graph developer pages

[RESEARCH](RESEARCH.md) · [MODELING](MODELING.md) · [EMBEDDING](EMBEDDING.md) · [IMPLEMENTATION](IMPLEMENTATION.md) · [API](API.md) · [MODULE_GUIDE](MODULE_GUIDE.md) · [RUNBOOK](RUNBOOK.md) · [OPERATIONS](OPERATIONS.md) · [TRUST](TRUST.md)
