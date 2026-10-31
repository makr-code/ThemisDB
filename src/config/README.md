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

# ThemisDB Config Module

## Purpose and ownership

`src/config` implements configuration path resolution, JSON/YAML loading and
schema checks, file-change notification, resolver metrics/audit, and an
in-memory authenticated-encryption key/value store. This page is for maintainers
working on the module; it does not claim that the module owns deployment
configuration rollout or durable secrets/audit services.

## Runtime flow

1. `ConfigPathResolver::tryResolve()` normalizes a path, applies a known legacy
   mapping and environment overlay logic, checks the shared TTL/LRU cache, and
   probes candidate paths. `resolve()` turns an absent result into
   `ConfigNotFoundException`.
2. `ConfigSchemaValidator` parses JSON/YAML, then evaluates the implemented
   schema keywords and returns `ValidationResult`; a successful parse is not a
   schema pass.
3. `ConfigFileWatcher` debounces supported filesystem events. The resolver's
   watcher callback clears cached paths; callers still own reloading, validating
   and applying settings.
4. `ConfigMetricsExporter` exposes resolver/cache counters. The resolver audit
   log records successful path resolution only when enabled and retains entries
   in process memory.
5. `ConfigEncryptedStore` encrypts values in memory with OpenSSL AES-256-GCM.
   Serialization includes key bytes and must be treated as secret material.

## Main implementation files

| Concern | Implementation | Public interface |
|---|---|---|
| Mapping, resolution, environment, cache, audit hooks | `config_path_resolver.cpp` | `config_path_resolver.h` |
| JSON/YAML parsing and schema evaluation | `config_schema_validator.cpp` | `config_schema_validator.h` |
| Platform filesystem notifications | `config_file_watcher.cpp` | `config_file_watcher.h` |
| Encrypted in-memory values and rotation | `config_encrypted_store.cpp` | `config_encrypted_store.h` |
| Resolver metrics and bounded audit entries | `config_metrics_exporter.cpp`, `config_audit_log.cpp` | corresponding headers |

The migration-scanner helper in `include/config/config_migration_scanner_impl.h`
is a testable utility used by `tools/config_migration_scanner.cpp`, not a
configuration-runtime service.

## Developer entry points

- **Quickstart:** read [`API.md`](API.md) for method contracts, then use
  [`MODULE_GUIDE.md`](MODULE_GUIDE.md) to select the focused test target for
  the code path you intend to change. For example, the resolver target is
  `test_config_path_resolver_focused`, registered as
  `ConfigPathResolverFocusedTests`.
- **Build/test guidance:** configure with a supported root preset, build the
  selected focused target, and run only its registered CTest name. Exact
  commands are in [`MODULE_GUIDE.md`](MODULE_GUIDE.md); they are guidance and
  were not executed for this documentation update.
- API details and error caveats: [`API.md`](API.md)
- Runtime composition and failure boundaries: [`ARCHITECTURE.md`](ARCHITECTURE.md)
- Implementation invariants and current gaps: [`IMPLEMENTATION.md`](IMPLEMENTATION.md)
- Focused test/build names: [`MODULE_GUIDE.md`](MODULE_GUIDE.md)
- Benchmark inventory and evidence limits: [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md)
- Public declarations: [`../../include/config/README.md`](../../include/config/README.md)

`include/config/config_contract.h` states the intended v1.x contract. Where it
differs from implementation or tests, document the discrepancy as a gap; do not
present the contract text alone as proof of runtime enforcement.