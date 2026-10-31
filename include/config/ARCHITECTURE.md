---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Public API Architecture

## Boundary

`include/config/` declares the public-facing C++ interfaces used by
`src/config/`. It does not describe the runtime implementation as header-only,
does not establish ABI guarantees, and does not own application-level config
rollout. The implementation flow is documented in
[`../../src/config/ARCHITECTURE.md`](../../src/config/ARCHITECTURE.md).

## Interface groups

1. **Path resolution:** `ConfigPathResolver` exposes optional/throwing
   resolution, legacy mapping metadata, environment selection, cache controls,
   optional SIGHUP/file watcher registration, metrics and audit controls.
2. **Parse and validate:** `ConfigSchemaValidator` exposes file/string parsing
   and `ValidationResult` diagnostics.
3. **Change notification:** `ConfigFileWatcher` owns its worker lifecycle and
   invokes a debounced callback; callers decide what to reload.
4. **Metrics and audit:** `ConfigMetricsExporter` exports resolver/cache
   counters; `ConfigAuditLog` stores bounded in-memory path records.
5. **Encrypted storage:** `ConfigEncryptedStore` provides CRUD, key rotation
   and serialization. Snapshots contain key bytes.
6. **Supporting types:** error classes, `LRUCacheWithTTL`, path-mapping
   metadata, and the migration scanner utility.

## Contract interpretation

`config_contract.h` records intended v1.x cross-component constraints. Compare
its claims with implementation and focused test coverage before treating each
one as enforced. The previous header architecture assigned the module to
"Graph" and described the watcher as inotify-only; those claims are not
supported by the current API and implementation inventory and are removed.

For versioning requirements, consult the root
[`../../VERSIONING.md`](../../VERSIONING.md). No independent ABI or platform
compatibility matrix was verified for this update.
