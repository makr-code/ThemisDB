---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
---

# Embeddings and the Timeseries Module

This page is for developers deciding whether the timeseries module can own an
embedding workflow. Source inspection finds no embedding model, vector
representation, embedding generation, or vector-index API in
`include/timeseries/` or `src/timeseries/`.

Exact model backend not verifiable from final document text alone.

## Current module contract

The module stores time-ordered numeric samples, identified by metric and
entity, with JSON tags and metadata. The public point type is
`TSStore::DataPoint` in `include/timeseries/tsstore.h`. Queries are time-range
and exact-tag filters; the module's aggregation and downsampling APIs derive
time-window statistics.

The `metadata` JSON member can carry application-defined fields, but that does
not make those fields a supported embedding contract. The module does not
validate vector dimensions, generate vectors, calculate similarity, or provide
vector retrieval. Do not document such behavior as native timeseries
capability.

## Integration boundary

An application that needs embeddings should use the owning embedding/vector
subsystem for generation, validation, indexing, and similarity queries. It may
associate an external identifier or application metadata with a time-series
point, but must define its own schema and lifecycle; this module does not
guarantee that arbitrary metadata is retained per sample when samples are
stored in one Gorilla batch.

## Validation and change criteria

Any future embedding feature would require an explicit owner and public
contract before implementation: vector dimensions and numeric format,
timestamp/series association, indexing/query semantics, persistence and
retention behavior, error handling, and tests for malformed or incompatible
vectors. No such interface is currently declared in the timeseries headers.

## Evidence

- [Point and query types](../../include/timeseries/tsstore.h)
- [Gorilla numeric codec](../../include/timeseries/gorilla.h)
- [Module public-header inventory](../../include/timeseries/ARCHITECTURE.md)
- [Module boundary summary](README.md)
