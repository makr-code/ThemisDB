---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Embedding boundary

This page is for developers deciding whether temporal storage owns vector or
machine-learning embedding behavior. It does not: the temporal module exposes
versioning, time-range, query, retention, snapshot, CDC, and conflict
interfaces. Its public headers do not define embedding generation, vector
dimensions, similarity metrics, model selection, or embedding lifecycle APIs.

## What temporal versioning does

The temporal payload is a `nlohmann::json` document. A caller may choose to
store arbitrary fields in that payload, but the temporal module treats them as
document data; it does not encode text, validate vector shape, compute
similarity, or manage model versions. The interval and temporal indexes operate
on temporal keys/ranges, not embedding coordinates.

## Integration constraints

- Keep embedding production, vector indexing, and similarity search in their
  owning subsystem; pass temporal records or identifiers through an explicit
  integration contract.
- If an embedding is stored in a versioned payload, associate its model and
  schema version in application data. Temporal history preserves payload
  versions but does not guarantee semantic compatibility between model
  revisions.
- Define retention and audit requirements before purging versions that contain
  embeddings or source material. `RetentionManager` can physically remove
  historical versions.
- Do not infer embedding support from JSON serialization or temporal CDC
  payloads. CDC transports the record fields supplied to it and may expose
  before/after values to subscribers.

## Validation boundary

There are no temporal-module embedding benchmarks or embedding-specific tests
in the module test and benchmark inventories. Validate embedding correctness,
dimension compatibility, and retrieval quality in the owning embedding/vector
subsystem. Validate only temporal version selection, payload preservation, and
retention effects in
[`tests/temporal/`](../../tests/temporal/).

Exact model backend not verifiable from final document text alone.
