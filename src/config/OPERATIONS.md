---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Config Module Operations

## Boundary

This guide covers the module-level APIs only. `src/config` resolves paths,
parses/validates data, emits file-change notifications, exports resolver
counters, retains optional in-memory path audit entries, and stores encrypted
values in process memory. It does not own application rollout, reload/apply
transactions, external secret management, or durable audit storage.

## Environment and resolver cache

- `THEMIS_CONFIG_ENV` accepts `dev`, `staging`, or `prod` (case-normalized).
  Absent, invalid-character, or unknown values select `prod`.
- Cache defaults are 1,000 entries and 300 seconds. Optional process-start
  variables are `THEMIS_CONFIG_CACHE_SIZE` (accepted range 10–100,000) and
  `THEMIS_CONFIG_CACHE_TTL` (1–86,400 seconds). Invalid values fall back to
  defaults. These are read during static initialization; set them before
  process startup.
- `setEnvironment()` clears the shared resolver cache. Do not assume changing
  an environment variable after initialization changes the active environment.

## Signals and observability

- `ConfigMetricsExporter::collect()` emits resolver/cache counters and gauges
  in Prometheus text format. Registry integration is provided when available
  in the build.
- Resolver audit logging is disabled by default; its default maximum is
  10,000 in-memory successful-resolution records. Entries include requested and
  resolved paths, timestamp, legacy-fallback and cache-hit flags, not operator
  identity or a durable change history.
- Watcher startup takes a directory path and debounce interval (default
  200 ms). `startHotReload()` returns false on failure; a successful start only
  enables notification/cache clearing.
- `registerSighupHandler()` is a separate POSIX API and must be called by the
  application. SIGHUP clears the resolver cache on a subsequent resolution;
  it does not reload settings. Windows treats registration as a no-op.

## Safe handling

- Validate actual files with the consuming service's schema/configuration
  rules; do not infer service readiness from resolver success.
- Restrict access to filesystem config paths and redact sensitive values from
  logs and tickets.
- Treat `ConfigEncryptedStore::serialize()` output as secret key material: the
  serialized JSON contains the store's key bytes.
- Confirm settings were loaded, validated and applied by the owner after a
  watcher event or cache clear.

For symptom-driven steps see [`RUNBOOK.md`](RUNBOOK.md); for data boundaries
see [`TRUST.md`](TRUST.md).
