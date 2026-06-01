> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/config/ARCHITECTURE.md -->

# Config Module — Public Header Architecture

**Module Path:** `include/config/`  
**Implementation:** `../../src/config/`  
**Canonical architecture doc:** [`../../src/config/ARCHITECTURE.md`](../../src/config/ARCHITECTURE.md)

---

## 1. Overview

`include/config/` defines the **public runtime configuration management, validation, encryption, hot-reload, audit logging, and metrics export API contract** for ThemisDB.

For runtime composition and implementation internals see:
→ [`../../src/config/ARCHITECTURE.md`](../../src/config/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Config

| Header | Public Type | Purpose |
|--------|------------|---------|
| `config_file_watcher.h` | `ConfigFileWatcher` | Inotify-based hot-reload watcher |
| `config_schema_validator.h` | `ConfigSchemaValidator` | JSON-schema configuration validation |
| `config_path_resolver.h` | `ConfigPathResolver` | Multi-root config path resolution |
| `config_migration_scanner_impl.h` | `ConfigMigrationScannerImpl` | Legacy config migration scanner |
| `lru_cache.h` | `LRUCache` | Generic LRU cache for config lookups |
| `path_mapping_metadata.h` | `PathMappingMetadata` | Path-alias metadata registry |
### 2.2 Security and Auditing

| Header | Public Type | Purpose |
|--------|------------|---------|
| `config_encrypted_store.h` | `ConfigEncryptedStore` | Envelope-encrypted config store |
| `config_audit_log.h` | `ConfigAuditLog` | Change audit log for config mutations |
### 2.3 Observability

| Header | Public Type | Purpose |
|--------|------------|---------|
| `config_metrics_exporter.h` | `ConfigMetricsExporter` | Prometheus metrics export for config system |
| `config_errors.h` | `ConfigError` | Typed config error codes |

---

## 3. Namespace Layout

All public types reside in the `themis::config` namespace (or a sub-namespace).

---

## 4. Contract Notes

- Headers in `include/config/` expose the **stable public API**; internal types live in `src/config/`.
- Clients depend only on types declared here; implementation details in `src/` may change without notice.
- For breaking-change policy see [`../../VERSIONING.md`](../../VERSIONING.md).
- Layer association: **Graph**.
