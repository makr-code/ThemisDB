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

> Exact model backend not verifiable from final document text alone.

# Distributed Knowledge Embedding Contract

## Purpose and ownership

This page is for teams producing or consuming embeddings at federation
boundaries. `distributed_knowledge` transports embedding-bearing records; it
does not define or run an embedding model, tokenizer, or vector index. The
producer remains responsible for model selection, preprocessing, versioning,
and semantic compatibility. See
[cross-shard feedback API](../../include/distributed_knowledge/cross_shard_feedback_sync.h).

## Feedback summary representation

`FeedbackSummary` contains a `reason_embedding` vector of `float`, a feedback
type label, an opaque summary ID, a target RLAIF round, a timestamp, and a
`shard_origin`. The default configured dimension is 384. With
`validate_embedding_dim` enabled, publishing rejects a **non-empty** vector
whose dimension differs from `max_embedding_dim`; an empty vector is not
rejected by that check. `FeedbackSyncConfig::isValid()` requires positive
dimension and dedup-cache capacity.

The publisher overwrites `shard_origin` with `"ANON"` and creates a new
summary ID and timestamp before serializing to JSON. This does not strip or
transform the embedding or feedback label. The data therefore remains
potentially sensitive even when the origin field is anonymized.

## Compatibility invariants

- Configure the same embedding model/version and dimension at all producers
  and consumers that compare or aggregate vectors.
- A dimension match is only a shape check; it does not establish compatible
  model spaces, normalization, finite values, or privacy properties.
- Keep any model/version metadata in the calling integration if it is needed
  to reject incompatible vectors; the current summary type has no embedding
  model-version field.
- Do not put raw user text into `reason_embedding` metadata or labels. The
  type is intended to carry semantic reason vectors, not raw prompt content.
- Treat serialization as transport encoding, not encryption. Secure the
  gossip transport and authorize peers at the caller boundary.

## Validation and tests

Exercise correct-dimension publish, wrong-dimension rejection, empty-vector
handling, JSON round-trip, policy rejection, duplicate IDs, and failed gossip
dispatch. Existing coverage is in
`tests/distributed_knowledge/test_cross_shard_feedback_sync_focused.cpp`.
For changes to dimension semantics or the serialized schema, update the API
contract and the test suite together. No embedding-quality benchmark is
defined in this module; model-space evaluation belongs to the embedding
producer.
