---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# Config Module Operations

## Purpose and operational boundary

This page describes routine deployment and monitoring for config-module
surfaces. The module resolves paths, validates data, reports file changes,
exports counters, and stores encrypted values in memory. It does not manage
deployment rollout, guarantee that a consumer applied a reload, or provide a
durable external audit backend.

## Startup and configuration

- Set `THEMIS_CONFIG_ENV` explicitly for deployments. Supported values are
  `dev`, `staging`, and `prod`; the default and invalid-value fallback is
  `prod`.
- Confirm expected files and schemas are readable by the service identity and
  pass validation before enabling dependent services.
- Resolver cache defaults are 1,000 entries with a 300-second TTL. The
  implementation also recognizes `THEMIS_CONFIG_CACHE_SIZE` and
  `THEMIS_CONFIG_CACHE_TTL`; inspect startup logs for invalid or out-of-range
  values.
- If file-change reload is needed, start the watcher on an existing directory
  and check the returned status. The default debounce is 200 ms. Registering
  SIGHUP is a separate POSIX mechanism.

## Health signals

- Monitor resolver resolution hits/misses, legacy fallbacks, unmapped requests,
  and cache hits/misses. A rising fallback count is a migration signal; an
  unmapped path or growing miss rate may indicate bad deployment configuration.
- `ConfigMetricsExporter::collect()` provides Prometheus exposition. When
  registering collectors, use a stable registry and avoid unbounded label
  values.
- Enable the resolver audit log only when operationally required. It is
  disabled by default, retains at most 10,000 entries by default, and records
  path-level resolution details in process memory; it is not durable audit
  evidence.
- Watcher logs indicate start failures and callback errors. A successful watcher
  only establishes change notification and cache invalidation, not successful
  service reload.

## Recovery and rollback

For missing paths or invalid schemas, correct or roll back the deployment
configuration and validate it before the service consumes it. For watcher
failure, use the owning service's supported restart/reload procedure or the
POSIX SIGHUP cache-clear mechanism where applicable; verify the new config was
actually loaded. For encrypted-store errors, stop unsafe writes and restore only
from a protected, verified snapshot. Do not delete or replace key material
without preserving recoverability.

Use [`RUNBOOK.md`](RUNBOOK.md) for incident steps and
[`PRODUCTION_REQUIREMENTS.md`](PRODUCTION_REQUIREMENTS.md) for the module's
production requirements.
