---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Embedding Boundary

## Scope and applicability

`src/access_model` does not generate, transform, index, or compare vector
embeddings. It contains no embedding model, vector-distance function, vector
dimension contract, or embedding-quality benchmark. This page records that
boundary so that developers do not infer embedding functionality from the
`L3_SEMANTIC` tier name.

## What the module does with values

The abstract `AccessTier` interface accepts a string key and string value for
`get` and `put`; the coordinator's events carry keys, tier labels, access
counts, and time/size metadata. The module can name `L3_SEMANTIC` as a cache
location, but it does not inspect the value or know whether it represents an
embedding, index entry, or other payload.

Any embedding generation and vector retrieval behavior belongs to the
responsible embedding, vector, or RAG subsystem and its API contract. That
subsystem may use an access tier as an integration detail, but must define its
own dimensions, normalization, model versioning, recall/latency measures, and
validation set.

## Evaluation boundary

There are no embedding-specific tests or metrics in
`tests/access_model/` or `benchmarks/access_model/`. Access-model benchmarks
evaluate coordinator/policy latency, event throughput, and memory expectations
listed in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).
Those results cannot establish semantic relevance, embedding parity, or
retrieval quality.

When integrating an embedding-backed tier, test the adapter's serialization,
dimension/model compatibility, missing-key and backend-error behavior, and
retrieval quality in the owning module. Use access-model tests only to verify
that tier labels and notifications are wired as intended.

See [`API.md`](./API.md) for the payload contract and
[`MODULE_GUIDE.md`](./MODULE_GUIDE.md) for subsystem ownership.

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
