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

**Maturity:** 🟢 Production-Ready — Core utility stack (audit, PII detection/pseudonymization, tracing, compression, crypto helpers, pagination, text tooling) is operational.

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

## Scientific References

1. Josuttis, N. M. (2012). **The C++ Standard Library: A Tutorial and Reference (2nd ed.)**. Addison-Wesley. ISBN: 978-0-321-62321-8

2. Knuth, D. E. (1998). **The Art of Computer Programming, Vol. 3: Sorting and Searching (2nd ed.)**. Addison-Wesley. ISBN: 978-0-201-89685-5

3. Chazelle, B. (2001). **The Soft Heap: An Approximate Priority Queue with Optimal Error Rate**. *Journal of the ACM*, 47(6), 1012–1027. https://doi.org/10.1145/355541.355542

4. Agner Fog. (2023). **Instruction Tables: Lists of Instruction Latencies, Throughputs and Micro-operation Breakdowns for Intel, AMD and VIA CPUs**. Technical University of Denmark. https://www.agner.org/optimize/instruction_tables.pdf
