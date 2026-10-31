---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Vector Embedding Performance

## Scope

This page covers vector representation and nearest-neighbour search code owned
by the performance module. It does not describe text embedding generation,
model selection, or the database's complete vector-index contract. Those
responsibilities belong to their respective model and index modules.

## Implemented paths

- `RaBitQEncoder` learns per-dimension mean and scale from training vectors,
  encodes values into four 2-bit bins, and supports decode and quantized or
  asymmetric distance calculations. `RaBitQIndex` stores encoded vectors and
  currently performs a linear scan for search. Its `MemoryStats` reports the
  packed-vector bytes and an estimated compression ratio.
- `phase3::DiskANNIndex` writes graph nodes to a file, keeps offsets and an
  in-memory cache, and uses a vantage-point tree for an entry point when
  available followed by greedy beam search. Its graph construction checks all
  input pairs to select neighbors, so construction cost grows quadratically
  with vector count in the current implementation.
- `ProductQuantizer` is a helper for splitting vectors, training codebooks,
  and encoding subvector IDs; it is separate from the RaBitQIndex search
  implementation.

This module does not promise that similarly named paper algorithms have the
same properties as their research implementations. In particular, test the
actual search and persistence behavior here before relying on paper-scale
capacity or recall claims.

## Input and lifecycle constraints

The RaBitQ encoder requires a positive dimension no greater than 1,048,576 and
requires an available SIMD capability (SSE2, AVX2, or ARM NEON); otherwise
construction throws. Training data vectors and encoded input vectors must
match the configured dimension. Empty training data leaves the initialized
parameters in place. `ProductQuantizer` requires its dimension to be evenly
divisible by the number of subvectors.

`DiskANNIndex::add()` checks the vector dimension and throws on mismatch.
Search on an empty index returns no results. The index's `flush()` and
`save()`/`load()` methods have different roles: `flush()` flushes graph output,
while `save()` and `load()` operate on the metadata sidecar. Preserve and
validate both files when managing an index.

## Evaluation

For each fixed dataset and dimension, compare exact floating-point ground
truth against the selected search path. Record recall at k, distance error,
query latency percentiles, build time, bytes on disk, resident memory, cache
hits/misses, and disk reads. Vary k, dataset size, cache budget, and search
beam width; include empty input and dimension mismatch cases. Repeat the
measurements on the deployment hardware and feature-flag configuration.

The module benchmark expectations include embedding-cache cases, but those
measure cache behavior and are not, by themselves, evidence of RaBitQ or
DiskANN recall or latency. See
[`bench_embedding_cache_performance.cpp`](../../benchmarks/cache/bench_embedding_cache_performance.cpp),
[PERFORMANCE_EXPECTATIONS.md](PERFORMANCE_EXPECTATIONS.md), and
[`test_rabitq_product_quantizer.cpp`](../../tests/performance/test_rabitq_product_quantizer.cpp).

## Trade-offs

Quantization reduces packed-vector storage but introduces approximation and
requires SIMD support. The current RaBitQIndex linear scan remains
O(number-of-vectors × dimension) per query. DiskANN moves graph data to disk
and uses an approximate traversal, trading build and I/O behavior for search
work; its current graph-building implementation is quadratic. Select neither
path based on a compression or scale claim alone: measure recall and resource
cost for representative application data.

Exact model backend not verifiable from final document text alone.
