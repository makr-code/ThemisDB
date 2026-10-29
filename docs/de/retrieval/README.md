# Retrieval Module

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: ../../../src/retrieval/README.md · ../../../include/retrieval/README.md -->

## Overview

The `retrieval` module provides ranking, candidate selection, and evidence-oriented fetch logic used across query, RAG, and document-grounding flows in ThemisDB.

## Primary sources

- `src/retrieval/` — retrieval engine and evaluation logic
- `include/retrieval/` — public retrieval interfaces and contracts
- `src/retrieval/ARCHITECTURE.md` — retrieval pipeline and data flow
- `src/retrieval/EMBEDDING.md` — embedding and candidate generation guidance

## Key responsibilities

- document and vector retrieval orchestration
- candidate scoring and ranking flow
- validator and quality gate integration
- retrieval metadata and feedback preparation

## Related documentation

- [src/retrieval/README.md](../../../src/retrieval/README.md)
- [src/retrieval/ARCHITECTURE.md](../../../src/retrieval/ARCHITECTURE.md)
- [include/retrieval/README.md](../../../include/retrieval/README.md)

## Validation

This overview keeps the German module index aligned with the retrieval engine’s runtime implementation and public API surface.
