---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/importers/importer_interface.h; include/importers/flatfile_importer.h; src/importers/README.md; tests/importers/CMakeLists.txt"
Related_Reference: "makr-code/ThemisDB#7353"
---

> Exact model backend not verifiable from final document text alone.

# Importers in ThemisDB

## Overview

Importer source files cover relational, document, stream, flat-file, object,
Wikipedia, and specialized ingestion/helper paths. This list describes source
surfaces, not a guarantee that every connector is enabled, has equivalent
behavior, or is production-ready. See the [API guide](./API.md) and
[architecture](./ARCHITECTURE.md).

## Source-backed feature areas

- Database and document connectors: see the concrete public headers under
  [`include/importers/`](../../include/importers/) and implementations under
  this directory. Runtime support can depend on configuration and optional
  dependencies.
- Flat files: `FlatFileImporter` declares CSV, TSV, JSONL/NDJSON, and Parquet;
  Parquet code is conditional on `ARROW_ENABLED`.
- Shared processing: schema inference/validation, conflict resolution, audit,
  data quality, MDM, and related helper components have corresponding headers
  and sources. Connector use of these features must be confirmed individually.
- Wikipedia and HuggingFace paths have distinct lifecycle and persistence
  boundaries; see [implementation](./IMPLEMENTATION.md) and
  [embedding boundary](./EMBEDDING.md).

## Evidence boundary

The presence of source files or CMake test registrations is not a successful
build, passing test run, production-readiness assessment, or measured performance
result. Current local registration and the evidence limits are summarized in
[`BUILD_STATUS.md`](./BUILD_STATUS.md). TODO: obtain a fresh full build and
connector-level integration evidence before making broader readiness claims.

## References

[Module README](./README.md) · [API](./API.md) · [Roadmap](./ROADMAP.md) ·
[Security](./SECURITY.md) · [Production guidance](./PRODUCTION_REQUIREMENTS.md)
