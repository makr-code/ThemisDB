<!-- Status: current | validated: 2026-04-06 -->

# Utils — Include Changelog

> Public header changes only. For implementation changes see [`../../src/utils/CHANGELOG.md`](../../src/utils/CHANGELOG.md).

## [Unreleased]

## [1.5.0] — 2026-03-12

### Added
- `lossless_vector_compression.h` — lossless float-vector compression interface
- `lossless_vector_integration.h` — integration adapter for vector compression
- `hkdf_cache.h` — cached HKDF derived-key storage
- `saga_logger.h` — distributed saga transaction logger
- `self_awareness.h` — runtime capability/health introspection header

### Changed
- `pii_detection_engine.h` — extended API for NER-based entity types
- `simd_distance.h` — added cosine distance alongside L2; scalar fallback documented
- `thread_pool_manager.h` — configurable queue depth added to constructor

### Fixed
- `safe_cast.h` — corrected overflow boundary for signed→unsigned narrowing cast

## [1.4.x] and earlier
See [`../../src/utils/CHANGELOG.md`](../../src/utils/CHANGELOG.md) for historical entries.
