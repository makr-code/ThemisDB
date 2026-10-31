---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: level1
SOT_Domain: module-behavior
Canonical_Sources:
  - "include/index/vector_index.h"
  - "src/index/vector_index.cpp"
  - "include/index/index_manifest_v1.h"
  - "src/index/index_manifest_v1.cpp"
  - "src/index/INDEX_MANIFEST_V1_SCHEMA.json"
  - "tests/index/test_index_manifest_v1.cpp"
  - "src/index/ROADMAP.md"
Related: "Issue #7125; DOC-WEEKLY-2026-44"
---

Exact model backend not verifiable from final document text alone.

# Embedding inputs and index compatibility

## Purpose and reader

This document defines the boundary between embedding producers and the index
module for engineers integrating ingestion, retrieval, and reindex workflows.
It covers vector inputs and the compatibility metadata represented by the
index-manifest types. The index module consumes embeddings; it does not
generate text embeddings, choose an embedding model, or own chunk creation.

## Input contract

`VectorIndexManager::init()` establishes an index namespace, vector dimension,
and distance metric. Entity operations read a configured vector field (default
`embedding`). Supply vectors whose dimension matches the index, whose values
and ordering are stable for the producer's model contract, and whose metric
interpretation is consistent between indexing and querying. Dimension
mismatches, absent or malformed vector fields, and backend failures must be
handled through the returned `Status`; do not treat a failed insert or rebuild
as a successful index update.

Storage and in-memory indexing are distinct: on the raw serialization path, the
entity containing the producer vector is serialized and stored before a separate
cache copy is normalized. For COSINE with vector encryption disabled, that
in-memory copy is L2-normalized before caching and, when HNSW is active and
unencrypted, before use as its input; the COSINE query is also L2-normalized on
the alternative ANN backend path. Other storage paths may encode the stored
vector differently. Callers should verify these implementation and
encryption-dependent details in
[`vector_index.cpp`](vector_index.cpp), rather than infer normalization from
the metric name.

## Version and compatibility metadata

`IndexVersionInfo` represents an embedding model identifier and dimension,
chunking profile ID, index schema version, document count, index size estimate,
and indexing timestamps. `VersionHistoryEntry` captures version/action/reason
and document additions/removals. `ReindexTrackingInfo` records status,
timestamps, error text, and rollback availability flags. These fields are
serialization metadata; they do not themselves generate embeddings, detect a
model change, perform a reindex, or restore an earlier index.

`IndexManifestV1::to_json()` emits the nested embedding, chunking, and schema
metadata objects implemented in
[`index_manifest_v1.cpp`](index_manifest_v1.cpp). The class serializes JSON to
a string and parses it back; it does not itself open RocksDB or perform a
column-family write. Do not copy the header's illustrative physical-layout
comments into deployment assumptions without checking the actual storage
integration.

The adjacent
[`INDEX_MANIFEST_V1_SCHEMA.json`](INDEX_MANIFEST_V1_SCHEMA.json) is a schema
artifact, but the C++ serializer does not validate against it. The code and
schema are not fully aligned: for example, the C++ default manifest version is
`1.0` while the schema's pattern requires three numeric components; several
schema-required metadata fields are not emitted by the C++ structs. A consumer
must not claim schema conformance based solely on successful C++ serialization.
Treat alignment/validation as a follow-up contract issue before enforcing this
schema at a persistence boundary.

## Reindex decision and migration safety

When an upstream embedding model, output dimension, chunking profile, or
index-schema assumption changes, treat existing vectors and their index as a
potentially incompatible generation. Before serving the new generation,
confirm compatibility from producer metadata, persist the relevant version
facts through the owning integration, build/reindex from authoritative source
records, and compare representative results. Manifest rollback flags only
report metadata conditions (`atomic_rollback_available`,
`previous_version_accessible`); the manifest class does not implement atomic
index rollback.

History is held in an in-memory vector and `add_version_history()` keeps the
most recent 30 entries by FIFO rotation. Do not interpret that limit as a
30-day retention policy. The manifest implementation is not thread-safe;
callers must synchronize access as stated by the header.

## Validation

Use [`test_index_manifest_v1.cpp`](../../tests/index/test_index_manifest_v1.cpp)
to verify JSON round trips, string serialization/parsing, FIFO history
rotation, and rollback-availability predicates. These tests do not prove
RocksDB placement, schema validation, embedding generation, or a full reindex
workflow. Validate a producer/index integration separately for dimension
agreement, version metadata, recall against the previous generation, and
failure behavior before switching traffic.

## Canonical sources

- [Vector input and lifecycle API](../../include/index/vector_index.h)
- [Manifest C++ types and thread-safety note](../../include/index/index_manifest_v1.h)
- [Manifest serializer implementation](index_manifest_v1.cpp)
- [Manifest schema artifact](INDEX_MANIFEST_V1_SCHEMA.json)
- [Manifest unit tests](../../tests/index/test_index_manifest_v1.cpp)
- [Index roadmap and unresolved gates](ROADMAP.md)
