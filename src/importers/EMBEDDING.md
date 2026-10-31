---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/importers/wikipedia_config.hpp; include/importers/wikipedia_types.hpp; src/importers/wikipedia_transform.cpp; src/importers/wikipedia_project_vector.cpp; src/importers/huggingface_ingest_plugin.cpp; include/importers/huggingface_ingest_plugin.h"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Exact model backend not verifiable from final document text alone.

# Importers Embedding Boundary

This document describes embedding-related importer surfaces for developers
integrating a vector provider. The importer module currently prepares or
projects data for vector work; it does not provide a general-purpose embedding
model runtime. The primary evidence is
[`wikipedia_config.hpp`](../../include/importers/wikipedia_config.hpp),
[`wikipedia_types.hpp`](../../include/importers/wikipedia_types.hpp), and
[`huggingface_ingest_plugin.h`](../../include/importers/huggingface_ingest_plugin.h).

## Wikipedia vector projection

`WikipediaIngestionConfig` exposes an `embedding` configuration with `enabled`,
`provider`, and `model` fields. `WikipediaTransform::buildVectorRecords()`
constructs one `WikipediaVectorRecord` per selected page from its title and up
to the first 512 bytes of canonicalized revision text. It carries the
configured model name and sets `pending_embedding` to the inverse of `enabled`.
`projectVectorDirtyPages()` replaces records for dirty pages in the current
snapshot.

This code constructs projection records; the importer sources do not show a
provider invocation or a vector-index write. `enabled` is therefore metadata
for this projection path, not evidence that inference ran or that an index was
updated. `provider` is a configuration label, not a credential or adapter.

## HuggingFace hook

`HuggingFaceIngestConfig` has an optional `embedding_sink_hook` callback.
The current `projectDocument()` call site invokes it with the document ID and
an empty `std::vector<float>`. It does not compute an embedding. Consumers must
not treat callback invocation as a vector result or as evidence of a working
embedding backend.

## Integration and evaluation boundary

A production provider integration needs an explicit contract for input
normalization, model/version identity, vector dimensions and numeric validity,
provider errors/timeouts, retry and deduplication semantics, and data retention.
None of those are implied by a model-name field or callback type. Keep model
inference outside the importer's canonical record identity so re-embedding does
not silently create duplicate source entities.

Before enabling an adapter, test:

- that only intended page/document text is sent and that it follows the
  deployment's privacy policy;
- deterministic handling of empty text, rejected records, and provider
  failures;
- vector dimension and model-version compatibility with the destination index;
- replay behavior for a resumed import and for a dirty-page projection rebuild.

For current behavior, see
[`wikipedia_project_vector.cpp`](./wikipedia_project_vector.cpp),
[`wikipedia_transform.cpp`](./wikipedia_transform.cpp), and the
[trust model](./TRUST.md). The module does not publish an embedding quality
metric or benchmark; recall, relevance, and model-quality claims require a
separate provider-specific evaluation.
