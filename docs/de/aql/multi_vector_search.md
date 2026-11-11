[docs](../../README.md) > [de](../README.md) > [aql](./README.md) > [multi-vector-search](./multi_vector_search.md)

**Author:** ThemisDB Contributors
**Created:** 2026-11-11
**Last Updated:** 2026-11-11
**Status:** review
**Datum:** 2026-11-11
**Primary (Quelle der Wahrheit):** `src/vector_search/API.md`, `include/index/multi_vector_search.h`, `src/index/multi_vector_search.cpp`
**Bezug / Reference:** [Issue #7791](https://github.com/makr-code/ThemisDB/issues/7791)
**AI_Used:** true
**AI_Models:** unknown
**AI_Contributor:** GitHub Copilot coding agent
**Human_Review:** false
**Human_Author:** unknown

> Exact model backend not verifiable from final document text alone.

# Multi-Vector Search Guide

`MultiVectorSearch` is an aggregation layer in the shared `index` subsystem. It
combines results from vector queries, query expansions, or vector and keyword
scores. It does not own storage or create field-specific indexes.

## Supported entry points

The public interface provides `search()`, `searchMultiField()`,
`searchWithExpansion()`, `hybridSearch()`, `batchSearch()`, and
`optimizeWeights()`. Fusion strategies include linear combination, max/min/mean
score, rank fusion, reciprocal-rank fusion, and `LEARNED_FUSION`.

`LEARNED_FUSION` applies caller-provided or optimized weights; it is not a
trained-model inference path.

## Current limitations

- `searchMultiField()` requires a non-empty field list but repeats the same
  query vector through the shared manager; it does not route to field-specific
  indexes.
- `MultiQuery::field_names` and `require_all_fields` are not consumed by
  `search()`.
- `SearchConfig::index_name` and `ef_search` are not read by the current
  implementation. ANN tuning belongs to `VectorIndexManager`.
- No production performance result is asserted by this guide. See the module's
  benchmark evidence limits before making latency or throughput claims.

## References

- [Canonical API contract](../../../src/vector_search/API.md)
- [Module overview](../../../src/vector_search/README.md)
- [`MultiVectorSearch` public header](../../../include/index/multi_vector_search.h)
- [Implementation](../../../src/index/multi_vector_search.cpp)
