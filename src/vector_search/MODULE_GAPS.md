---
Author: "ThemisDB Team"
Created: "2026-10-21"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "unknown"
---

Exact model backend not verifiable from final document text alone.

# Module Gaps — Vector Search

## Scope

This register tracks implementation, evidence, and deployment gaps around the
shared vector search implementation. It does not claim implementation findings in
`src/vector_search/`, which contains no runtime source.

## Open gaps

### VS-GAP-01: Production-runtime performance evidence

- **Current:** `bench_vector_search_dedicated_gates.cpp` benchmarks an
  in-process flat index and exports threshold counters. Its source does not
  measure the production `VectorIndexManager` path or compute the stated
  percentiles.
- **Impact:** The documented VS-BM thresholds are not proof that production
  runtime performance meets those thresholds.
- **Action:** Add or identify runtime-backed benchmarks and record hardware,
  backend, workload, and measured percentiles.
- **Status:** Open evidence gap.

### VS-GAP-02: Runtime-backed stress and soak coverage

- **Current:** `tests/vector_search/test_vector_search_highcardinality_stress.cpp`
  and `tests/integration/test_vector_search_soak.cpp` use in-process
  flat-index fixtures.
- **Impact:** Passing these tests validates the fixtures, not production ANN,
  persistence, or RocksDB integration behavior.
- **Action:** Run focused runtime-backed tests and distinguish them from
  fixture-level coverage in release evidence.
- **Status:** Open evidence gap.

### VS-GAP-03: Distributed deployment guarantees

- **Current:** `DistributedVectorIndex` is implemented as an in-process sharded
  index. Its `RANGE` strategy currently uses hash-bucket assignment, and its
  replication factor is documented as future work.
- **Impact:** The class does not establish durable replication, remote shard
  coordination, or high availability.
- **Action:** Define and validate persistence, shard-failure recovery, and
  replication semantics before claiming multi-node production support.
- **Status:** Open capability/readiness gap.

### VS-GAP-04: Compliance evidence freshness

- **Current:** `ai_working/sourcecode_graph.json` is absent from this checkout.
  `ai_context/developer_llm_wiki/SOLL_IST_GAP_REPORT.json` lists several
  governance docs as missing even though they are present, and lists
  `src/vector_search/DOXYGEN.md` although that file is absent. Its module-doc
  inventory is stale.
- **Impact:** The generated score cannot serve as current evidence of module
  documentation status.
- **Action:** Regenerate the source graph and compliance report from the current
  checkout and review the direct-Doxygen counters.
- **Status:** Open documentation-evidence gap.

### VS-GAP-05: Field-specific multi-vector search

- **Current:** `MultiVectorSearch::searchMultiField()` checks that the field list
  is non-empty, then repeats the same query vector through the shared manager.
  `field_names` and `MultiQuery::require_all_fields` are not consumed by
  `search()`. `SearchConfig::index_name` and `ef_search` are also declared but
  not read by the implementation.
- **Impact:** The API names and configuration fields must not be interpreted as
  field-specific routing, per-request index selection, or per-request ANN tuning.
- **Action:** Implement and test field/index routing and config behavior, or
  narrow the API contract to the behavior actually supported.
- **Status:** Open implementation/documentation gap.

### VS-GAP-06: Entity dimension and finite-value validation

- **Current:** Query dimensions are checked, and `WriteBatchWrapper` and MVCC
  `addEntity` overloads check entity-vector dimensions. The direct-commit
  `addEntity` overload does not check vector length. The source does not
  establish rejection of NaN/Inf values.
- **Impact:** Callers cannot assume uniform dimension validation or finite-value
  validation across input paths.
- **Action:** Add runtime validation and tests for each insertion overload and
  finite-value edge cases, or retain explicit caller-side validation.
- **Status:** Open validation gap.

## Closed documentation cleanup

- Removed redundant `MODELING.md`, `RESEARCH.md`, `MODULE_GUIDE.md`,
  `OPERATIONS.md`, and `TRUST.md`; their active scope is covered by the primary
  architecture, roadmap, runbook, and security docs.
- Removed unsupported numeric error codes and unverified baseline measurements
  from the active module documentation.

## References

- [Roadmap](ROADMAP.md)
- [Performance expectations](PERFORMANCE_EXPECTATIONS.md)
- [Production requirements](PRODUCTION_REQUIREMENTS.md)
