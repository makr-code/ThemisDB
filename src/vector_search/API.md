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

# Vector Search API Contract

## Scope

This page documents the public contract visible in the shared index layer, not a separate code path under `src/vector_search/`. The authoritative interfaces are in:

- `include/index/vector_index.h`
- `include/index/multi_vector_search.h`

## `VectorIndexManager`

### Lifecycle

`VectorIndexManager` is constructed from a `RocksDBWrapper&` and then initialized with an object namespace and vector dimension.

Core lifecycle functions and declarations:

- `Status init(std::string_view objectName, int dim, Metric metric = COSINE, int M = 16, int efConstruction = 200, int efSearch = 64, const std::string& savePath = "")`
- `Status setAdvancedIndexConfig(const AdvancedIndexConfig& config)` (configure before `init`)
- `Status setEfSearch(int efSearch)`
- `Status rebuildFromStorage()`
- `std::pair<Status, IncrementalReindexStats> incrementalReindex(float rebuild_threshold = 0.20f, std::string_view vectorField = "embedding")`
- `Status shutdown()`
- `Status saveIndex(const std::string& directory) const`
- `Status loadIndex(const std::string& directory)`

Entity mutation is available as direct-commit, `WriteBatchWrapper`, and
`TransactionWrapper` overloads of `addEntity`, `updateEntity`, and `removeByPk`.
Each returns `Status`. The direct-commit `addEntity` overload currently checks
that the manager is initialized and that the vector field can be extracted, but
does not check the vector length against the configured dimension. The batch and
MVCC `addEntity` overloads do check that length.

### Status and search model

`VectorIndexManager::Status` is the canonical error/status object. It contains:

- `ok` (boolean)
- `message` (string)

`searchKnn(const std::vector<float>& query, size_t k, const std::vector<std::string>* whitelistPks = nullptr)`
returns `std::pair<Status, std::vector<Result>>`. A query whose dimension differs
from the initialized index returns a failed status. `Result` contains the
primary key and distance; lower distances rank first. L2 returns squared L2,
COSINE returns `1 - cosine`, and DOT returns the negative dot product.

### Metric model

`VectorIndexManager::Metric` is an enum with:

- `L2`
- `COSINE`
- `DOT`

The metric is selected at initialization and determines distance semantics.
Search calls do not accept a separate metric argument, so there is no
per-query metric mismatch to validate.

### Result model

`VectorIndexManager::Result` is a struct with:

- `pk`
- `distance`

This keeps the search result narrow and predictable. It is a ranking object, not a hidden stateful result container.

## `MultiVectorSearch`

`MultiVectorSearch` is in namespace `themis::vector` and aggregates results
from one or more calls to the shared `VectorIndexManager`.

### Core search types

- `Result<MultiSearchResult> search(const MultiQuery&, const SearchConfig&)`
- `Result<MultiSearchResult> searchMultiField(const std::vector<float>&, const std::vector<std::string>&, const SearchConfig&)`
- `Result<MultiSearchResult> searchWithExpansion(const std::vector<std::vector<float>>&, const SearchConfig&)`
- `Result<MultiSearchResult> hybridSearch(const std::vector<float>&, const std::unordered_map<std::string, float>&, const SearchConfig&)`
- `Result<std::vector<MultiSearchResult>> batchSearch(const std::vector<MultiQuery>&, const SearchConfig&)`
- `Result<std::vector<float>> optimizeWeights(const std::vector<MultiQuery>&, const std::vector<std::vector<std::string>>&)`

### Configuration surface

`SearchConfig` contains:

- `fusion` strategy,
- `weights`,
- `top_k`,
- `rrf_k`,
- `normalize_scores`,
- `index_name`,
- `ef_search`.

The current implementation uses `fusion`, `weights`, `top_k`, `rrf_k`, and
`normalize_scores`. It does not read `index_name` or `ef_search`; ANN tuning is
controlled on `VectorIndexManager` instead.

### Fusion strategies

The implementation documents the following strategies:

- `LINEAR_COMBINATION`
- `MAX_SCORE`
- `MIN_SCORE`
- `AVG_SCORE`
- `RANK_FUSION`
- `RECIPROCAL_RANK`
- `LEARNED_FUSION` (uses weights from `optimizeWeights()` or caller-supplied weights
  with linear combination; this is not a trained model inference path)

`searchMultiField()` currently validates that `field_names` is non-empty, then
repeats the same query vector and delegates to `search()`. It does not route
searches to field-specific indexes. `MultiQuery::field_names` and
`require_all_fields` are not consumed by `search()`.

### Result envelope

`MultiSearchResult` returns:

- vector of `SearchResult` entries,
- the strategy used,
- total candidate count,
- computation time,
- the weights actually applied.

## Contract invariants

The API expects the following invariants to hold:

- each index must have a fixed dimension,
- each query must match the initialized index dimension; metric semantics are
  selected on the index,
- invalid states surface as status failures,
- backend availability and fallback behavior depend on the selected backend and
  build configuration,
- reindex, rebuild, and persistence paths remain explicit and recoverable.

## Edge cases and failure expectations

A caller should treat the following as expected failure cases rather than as hidden non-failures:

- query dimension mismatch,
- vector-dimension mismatch (query and batch/MVCC entity-write overloads),
- malformed input vectors,
- unsupported advanced backend configuration (not every unavailable backend
  fails initialization; for example, unavailable DiskANN falls back to the
  standard indexing path),
- storage drift requiring rebuild or reindex,
- resource exhaustion under high cardinality or sustained concurrency.

The implementation does not currently establish that NaN/Inf values are
rejected. Treat finite-value validation as unverified. The unchecked dimension
in the direct-commit `addEntity` overload is an open implementation gap.

## Conclusion

These descriptions reflect the current shared-index implementation, including
the field-routing and direct-commit dimension-validation gaps above; they are
not guarantees of production readiness.
