# Utilities Module

Utility functions and shared components for ThemisDB.

## Module Purpose

Provides shared utility functions and cross-cutting infrastructure components used across all ThemisDB modules, including audit logging, PII detection, text processing, cryptographic helpers, tracing, compression, and pagination.

## Subsystem Scope

**In scope:** Audit logger, cursor/pagination, HKDF key derivation, LEK manager, structured logger, text normalizer, PII detection and pseudonymization, PKI client, retention manager, SAGA logger, serialization helpers, stemmer/stopwords, distributed tracing, ZSTD codec, geospatial utilities.

**Out of scope:** Business logic, module-specific data models, high-level orchestration.

## Relevant Interfaces

- `audit_logger.cpp` — structured audit trail
- `pii_detector.cpp` — PII detection and pseudonymization
- `hkdf_helper.cpp` — HKDF key derivation
- `tracing.cpp` — distributed trace propagation
- `zstd_codec.cpp` — ZSTD compression
- `stemmer.cpp` — text stemming
- `lek_manager.cpp` — Local Encryption Key management

## Current Delivery Status

**Maturity:** 🟡 Beta — All core utilities operational; streaming PII pipeline and tamper-evident audit chain in progress.

## Components

- Audit logger
- Cursor/pagination
- HKDF key derivation helper
- LEK (Local Encryption Key) manager
- Logger
- Normalizer
- PII detection and pseudonymization
- PKI client
- Retention manager
- SAGA logger
- Serialization
- Text processing (stemmer, stopwords)
- Tracing
- ZSTD codec
- Geospatial utilities

## Features

- Comprehensive logging infrastructure
- Audit trail generation
- PII detection and redaction
- Text normalization and stemming
- Compression utilities
- Distributed tracing

## Documentation

### Observability Overview
For comprehensive observability and monitoring documentation, see:
- **[Observability & Monitoring Overview](../../docs/observability/README.md)** - Central hub for logging, tracing, metrics, and alerting

### Component Documentation
For detailed utilities documentation, see:
- [Audit Logger](../../docs/src/utils/audit_logger.cpp.md)
- [Cursor](../../docs/src/utils/cursor.cpp.md)
- [HKDF Helper](../../docs/src/utils/hkdf_helper.cpp.md)
- [LEK Manager](../../docs/src/utils/lek_manager.cpp.md)
- [Logger](../../docs/src/utils/logger.cpp.md)
- [PII Detector](../../docs/src/utils/pii_detector.cpp.md)
- [Stemmer](../../docs/src/utils/stemmer.cpp.md)
- [Tracing](../../docs/src/utils/tracing.cpp.md)
- [ZSTD Codec](../../docs/src/utils/zstd_codec.cpp.md)
- And many more in [docs/src/utils/](../../docs/src/utils/)
