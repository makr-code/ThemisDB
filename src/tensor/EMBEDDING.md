---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "unknown"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Tensor Embedding and Similarity Model

## Representation

Tensor embeddings in this module are represented in TT form, not as raw dense arrays only. The canonical home of the representation is `storage::TTTrain` in `include/storage/tensor_train_decomposer.h`.

Each TT train is composed of cores:

- `TTCore.r_left` = left bond dimension
- `TTCore.n` = mode size
- `TTCore.r_right` = right bond dimension
- `TTCore.data` = flattened row-major core data

The module expects each core to maintain dimensional consistency across the chain. Malformed or empty cores are rejected before insertion into the graph.

## Fingerprint embedding

The fingerprint graph does not store a full TT train for every candidate; it stores a compact summary derived from the first core:

- `columnMeans(G0.data, n_rows, n_cols)` computes mean values across the first TT core
- `fingerprint_sq_norm` stores the squared L2 norm for fast approx queries
- `first_core_norm` stores the Frobenius norm for normalization and validation

This keeps the approximate similarity path fast while limiting the memory footprint of the graph itself.

## Similarity semantics

Two modes exist:

### 1. Approximate fingerprint path

`TensorFingerprintGraph::findSimilarByFingerprint()` compares a query fingerprint to each candidate fingerprint using a cosine-similarity path with zero-padding semantics. Missing dimensions are treated as zeros, and near-zero-norm vectors immediately return `0.0`.

Key invariants from the code:

- `a.empty() || b.empty()` -> score `0.0`
- `a_sq_norm < 1e-12f || b_sq_norm < 1e-12f` -> score `0.0`
- result list is sorted descending by score and truncated to `k`

### 2. Exact registered-train path

`TensorFingerprintGraph::findSimilar()` computes the exact similarity of registered TT trains by using `TensorTrainDecomposer::innerProduct()` and normalizing by the product of the Frobenius norms. When a custom exact similarity bridge is injected through `setExactSimilarityFn()`, the module delegates to that override instead of computing the internal TT product itself.

This makes the module compatible with experimental or backend-specific exact similarity implementations while preserving an explicit fallback path.

## Edge cases the implementation considers

- zero-norm query or candidate embeddings
- dimension mismatch between query and candidate fingerprints
- roots with invalid or NaN self inner products
- invalid exact similarity function callbacks
- candidate train entries missing from the graph
- duplicate adapter keys replacing prior entries

The code intentionally prevents silent results in these cases by returning empty results and emitting diagnostics when the state is malformed.

## Operational significance

The embedding model is designed for fast retrieval on large candidate sets while preventing invalid or ambiguous math from silently producing garbage scores. In practice, that means the module prioritizes explicit filtering, bounded comparisons, and deterministic result ordering over unconstrained approximate ranking.
