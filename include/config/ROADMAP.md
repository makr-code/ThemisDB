<!-- Status: current | validated: 2026-04-08 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md -->

# Roadmap — Config Module Public Headers

All public header API milestones for `include/config/`.  
Implementation details and feature roadmap: `src/config/ROADMAP.md`.

## Current Status

Public headers are **production-ready** for all implemented API surfaces.  
All header files in `include/config/` export a stable, documented ABI.

## Completed ✅

- [x] `config_path_resolver.h` — legacy→new path mapping, LRU cache, multi-env overlay, metrics, audit trail
- [x] `config_schema_validator.h` — JSON Schema Draft 7 subset (type, properties, required, allOf/anyOf/oneOf, $ref, format, uniqueItems, not)
- [x] `config_encrypted_store.h` — AES-256-GCM store with key rotation and concurrent-read `shared_mutex`
- [x] `config_audit_log.h` — bounded ring-buffer audit trail
- [x] `config_metrics_exporter.h` — Prometheus text-format and registry-based export
- [x] `config_file_watcher.h` — inotify/kqueue/ReadDirectoryChangesW with 200 ms debounce
- [x] `config_errors.h` — `ConfigNotFoundException`, `MappingNotFoundException`, `InvalidPathException`, `ConfigPermissionException`
- [x] `lru_cache.h` — generic `LRUCacheWithTTL<K,V>` header-only
- [x] `path_mapping_metadata.h` — `PathMappingMetadata` with `deprecated_date`, `removal_date`, `migration_guide_url`
- [x] `config_migration_scanner_impl.h` — `ConfigMigrationScanner` testable inline implementation
- [x] `include/config/` directory created, consistent with other ThemisDB module headers

## Planned Features 📋

- [ ] Complete removal of deprecated legacy path mappings after migration window (Issue #1665)
- [ ] Batch-rename tooling integration once `config_migration_scanner --fix` tooling matures (Issue #1658)

## Production Readiness Checklist

- [x] All public types documented via Doxygen-style comments in header files
- [x] Exception hierarchy stable — no breaking signature changes in minor releases
- [x] Thread safety documented per class (see `include/config/README.md`)
- [x] Environment variable interface documented (`THEMIS_CONFIG_CACHE_SIZE`, `THEMIS_CONFIG_CACHE_TTL`, `THEMIS_CONFIG_ENV`)
- [x] Consistent with other module include directories (`include/analytics/`, `include/auth/`, `include/cache/`)

## Known Issues & Limitations

- `config_migration_scanner_impl.h` is header-only (inline) to allow testing and CLI use without shared library linkage
- `lru_cache.h` is config-module-specific by location; generic consumers should prefer `include/cache/bounded_lru_cache.h`
