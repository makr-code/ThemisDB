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

# Config Module Production Boundary

## Purpose and status

This page describes code-backed operational boundaries for `src/config`; it
does not certify the module or define deployment-wide production policy. The
previous revision listed mandatory controls (including always-on audit logging,
operator-attributed change records, external retries and unrelated environment
variables) that are not implemented by this module and are removed here.

## Verified implementation behavior

- `ConfigPathResolver` initializes `THEMIS_CONFIG_ENV` to `DEV`, `STAGING`, or
  `PROD`; absent or unrecognized values select `PROD`. Cache defaults are 1,000
  entries and 300 seconds; optional overrides are
  `THEMIS_CONFIG_CACHE_SIZE` and `THEMIS_CONFIG_CACHE_TTL`.
- `ConfigAuditLog` is disabled by default, bounded in memory (10,000 entries by
  default), and records resolution metadata, not operator-attributed
  configuration changes.
- `ConfigFileWatcher` can signal file changes. It does not parse, validate, or
  atomically apply settings.
- `ConfigEncryptedStore` uses a generated in-memory key and AES-256-GCM. Its
  serialized representation includes that key and is therefore sensitive.
- Schema validation returns diagnostics, but the code supports a subset of
  schema behavior. Do not equate parse success or this component's presence
  with production configuration validation.

## Deployment-owner checks

- Confirm the service that consumes the configuration owns authorization,
  reload/apply sequencing, secret delivery, persistence, backups, and incident
  recovery.
- Restrict filesystem access to configuration paths and protect serialized
  store snapshots as secret material.
- Enable the resolver audit log only when its in-memory path records are useful;
  do not rely on it as durable or compliance-grade evidence.
- Test actual service startup, failure handling and reload behavior in the
  deployment environment. This module's focused tests do not establish those
  cross-service properties.

## Explicit gaps

- **TODO:** document deployment-specific production requirements in the owning
  service's canonical runbook; no universal deployment policy is established
  by `src/config`.
- **TODO:** verify enforcement of each bound declared in
  `include/config/config_contract.h` against the implementation and focused
  tests before describing it as a production control.

## Canonical module references

- [`ARCHITECTURE.md`](ARCHITECTURE.md)
- [`IMPLEMENTATION.md`](IMPLEMENTATION.md)
- [`SECURITY.md`](SECURITY.md)
- [`TRUST.md`](TRUST.md)
- `include/config/config_contract.h`
