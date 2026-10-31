---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Document Module Features

> Exact model backend not verifiable from final document text alone.

## Overview

The module exposes document store/manager contracts, lifecycle callbacks, schema validation, field-level diff and three-way merge, XDOMEA connector operations, and store-backed round-trip snapshots. The in-memory implementations are reference implementations; this page does not claim durable persistence or production deployment guarantees.

## Implemented surfaces

- CRUD interfaces return `Result` values and optional reads; lifecycle hooks are dispatched synchronously by `InMemoryDocumentManager`.
- The schema reference implementation validates registered required fields and JSON types.
- Diff/merge reads documents through `IDocumentStore`; merge behavior is selected by `MergeStrategy`.
- `StoreBackedRoundTripEditor` stores relay/index snapshots via `IDocumentStore`.
- XDOMEA import/export and in-memory entity JSON helpers are available, with limits described in [README.md](README.md) and [SECURITY.md](SECURITY.md).

## Boundaries

- `InMemoryDocumentStore` is not durable. An `IDocumentStore` adapter must define persistence and its own concurrency guarantees.
- The in-memory XDOMEA implementation does not perform XSD validation.
- The in-memory encrypted entity records key rotation IDs; it does not re-encrypt stored bytes.

Primary references: [module README](README.md), [architecture](ARCHITECTURE.md), [roadmap](ROADMAP.md), [production requirements](PRODUCTION_REQUIREMENTS.md), and [security boundaries](SECURITY.md).
