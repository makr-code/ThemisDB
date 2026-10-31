---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Module Capabilities

## Reader and scope

For developers integrating or changing `src/config`. The module is a set of
runtime support components; it does not own application-level rollout or
consumer authorization.

## Implemented capability surfaces

- **Path lookup:** `ConfigPathResolver` normalizes paths, applies entries from
  its legacy mapping table, checks environment candidates, optionally caches
  results, and exposes throwing (`resolve`) and optional (`tryResolve`) forms.
- **Parsing and schema evaluation:** `ConfigSchemaValidator` loads JSON/YAML,
  evaluates the schema keywords implemented in its source, and returns
  diagnostics. It is not a complete Draft 7 implementation.
- **Change notification:** `ConfigFileWatcher` debounces supported filesystem
  events. The resolver's watcher callback clears its path cache; settings reload
  and application remain the caller's responsibility.
- **Observability:** `ConfigMetricsExporter` exports resolver/cache metrics.
  `ConfigAuditLog` optionally stores bounded successful-resolution records in
  process memory; it is not durable compliance evidence.
- **Encrypted storage:** `ConfigEncryptedStore` stores in-memory values using
  AES-256-GCM and can re-encrypt entries on key rotation. Its serialized
  snapshot includes the current key bytes.

## Engineering and operational relevance

The caller can use the resolver, validator, notification, metrics, and storage
interfaces independently. This makes failure handling explicit at those
boundaries, but does not make reloads atomic or establish end-to-end service
availability; those properties depend on the consuming service.

## Engineering limits and trade-offs

- `tryResolve()` returns no value for both invalid and unavailable paths; use
  diagnostics or the throwing API when the distinction matters.
- `tryGet()` returns `std::nullopt` only for an absent key and propagates
  decryption failures as `ConfigEncryptionException`.
- Watch notifications are not a reload transaction and can be debounced.
- Schema contract limits and full keyword conformance need implementation/test
  verification; see [`MODULE_GAPS.md`](MODULE_GAPS.md).
- Serialized store output must be protected as key material.

## Sources

- [`README.md`](README.md), [`ARCHITECTURE.md`](ARCHITECTURE.md),
  [`API.md`](API.md)
- `include/config/config_path_resolver.h`
- `include/config/config_schema_validator.h`
- `include/config/config_file_watcher.h`
- `include/config/config_encrypted_store.h`
- Implementations under `src/config/`
