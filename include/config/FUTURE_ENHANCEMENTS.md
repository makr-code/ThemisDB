# Config Module — Future Enhancements (Public API)

**Version:** 2.0.0
**Status:** 📋 Planned
**Last Updated:** 2026-04-08

This document covers planned **public API** additions to `include/config/`.
For implementation-level enhancements see `src/config/FUTURE_ENHANCEMENTS.md`.

---

## Planned Public API Additions

| Enhancement | Target Version | Status |
|-------------|---------------|--------|
| `ConfigPathResolver::resolveAll(pattern)` — glob-pattern multi-path resolution | v2.1.0 | `[ ]` |
| `ConfigSchemaValidator::validateDirectory(dir, schema)` — batch validation | v2.1.0 | `[ ]` |
| `ConfigEncryptedStore::exportEncrypted(master_key)` — master-key-wrapped snapshot | v2.1.0 | `[ ]` |
| Remove deprecated legacy path mappings from `config_path_resolver.h` (Issue #1665) | post-migration | `[I]` |

## ABI Stability Policy

- `[ ]` No existing public method in `config_path_resolver.h` may be removed or have its
  signature changed in a minor release without a deprecation cycle of at least one major version.
- `[ ]` `config_errors.h` exception class names and base-class hierarchy are frozen in the
  stable ABI; message text may change only in patch releases with changelog notice.
- `[ ]` `LRUCacheWithTTL<K,V>` in `lru_cache.h` must remain generic (no config-specific logic).

## References

- `src/config/FUTURE_ENHANCEMENTS.md` — detailed implementation notes and performance targets
- `src/config/ROADMAP.md` — full feature roadmap with phase breakdown
