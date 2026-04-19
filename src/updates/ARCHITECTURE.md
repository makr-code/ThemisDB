# Updates Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/updates/`

---

## 1. Overview

The Updates module provides ThemisDB's update and migration system: zero-downtime hot-reload
engine, schema migration framework, release manifest management with digital signature
verification, automatic backup/rollback, canary rollout, and delta update delivery.

---

## 2. Design Principles

- **Atomic File Replacement** – hot-reload updates use atomic `rename()` with `fsync`
  guarantees so no partial updates are visible to the running server.
- **Backup First** – a rollback point is always created before any update is applied;
  no update proceeds without a successful backup.
- **Signature Verification** – CMS/PKCS#7 signatures are verified against X.509
  certificates before any binary or manifest is applied.
- **Canary Rollout** – `canary_rollout.cpp` routes a configurable percentage of traffic
  to the new version before full rollout.
- **Schema Migration as Code** – migrations are registered in `migration_registry.cpp`
  with explicit dependency ordering and rollback scripts.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `hot_reload_engine.cpp` | Atomic binary update, signature verify, backup, rollback |
| `schema_migration_tester.cpp` | Dry-run migration testing |
| `in_place_schema_migrator.cpp` | In-place schema migration execution |
| `delta_update_engine.cpp` | Delta-only update delivery (send changed files only) |
| `canary_rollout.cpp` | Progressive traffic shifting for new versions |
| `manifest_database.cpp` | Installed version and release manifest registry |
| `release_manifest.cpp` | Release manifest parsing and validation |
| `update_state_machine.cpp` | State machine: IDLE → DOWNLOADING → VERIFYING → APPLYING → DONE |
| `updates_config.cpp` | Update configuration loading |

### 3.2 Component Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│         Update Trigger (manual, auto-check, scheduler)          │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                  UpdateStateMachine                              │
│  IDLE → DOWNLOADING → SIGNATURE_VERIFY → BACKUP → APPLYING → DONE │
└──────────────────────────┬──────────────────────────────────────┘
                           │
┌──────────────────────────▼──────────────────────────────────────┐
│                  HotReloadEngine                                 │
│                                                                  │
│  1. release_manifest.verify(signature)                          │
│  2. create rollback point (backup current files)                │
│  3. atomic file replace (download → temp → rename → fsync)      │
│  4. in_place_schema_migrator.run(pending migrations)            │
│  5. canary_rollout.start(percentage: 10%)                       │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Hot-Reload Update

```
trigger: new release 1.6.0 available
    │
    ├─ download_release("1.6.0") → delta_update_engine: only changed files
    │
    ├─ release_manifest.verify(CMS_signature, X.509_cert) → OK
    │
    ├─ create_backup() → /var/lib/themisdb/rollback/v1.5.3/
    │
    ├─ atomic_replace(files):
    │       for each file: write to .tmp → rename() → fsync()
    │
    ├─ schema_migrator.run(pending_migrations: [m_002, m_003])
    │
    ├─ canary_rollout.start(5%) → monitor metrics 10 minutes
    │       → metrics OK → 25% → 50% → 100%
    │       → regression → rollback()
    │
    └─ update complete; manifest_database.record("1.6.0")
```

### 4.2 Schema Migration

```
in_place_schema_migrator.run("migration_002_add_created_at")
    │
    ├─ migration_registry.getDependencies("migration_002") → [migration_001]
    ├─ check migration_001 already applied → OK
    ├─ schema_migration_tester.dryRun("migration_002") → no errors
    │
    ├─ apply: ALTER COLLECTION users ADD FIELD created_at TIMESTAMP DEFAULT now()
    │
    └─ version_tracker.recordApplied("migration_002")
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/storage/` | Backup creation and schema mutation |
| **Uses** | `src/scheduler/` | Scheduled update checks |
| **Uses** | `src/security/` | CMS signature verification |
| **Provides to** | `src/server/` | Update API endpoints |
| **Provides to** | `src/themis/` | Version tracking |

---

## 6. Threading & Concurrency Model

- Hot-reload is strictly single-threaded; no concurrent updates.
- `UpdateStateMachine` uses a state mutex; concurrent state queries are safe.
- `CanaryRollout` traffic routing uses an atomic percentage counter.
- Schema migrations run under an exclusive schema lock.

---

## 7. Security Considerations

- All releases are CMS/PKCS#7 signed; no unsigned updates are applied.
- Backup is created atomically before any change; rollback is always available.
- Admin API access to trigger updates requires admin role + USB token authentication.
- Download integrity is verified by manifest checksum before installation.

---

## 8. Configuration

| Parameter | Default | Description |
|---|---|---|
| `updates.auto_check.enabled` | false | Enable automatic update checks |
| `updates.auto_check.interval_h` | 24 | Check interval |
| `updates.verify_signatures` | true | Require signature verification |
| `updates.create_backup` | true | Always backup before update |
| `updates.canary.enabled` | true | Enable canary rollout |
| `updates.canary.initial_pct` | 5 | Initial canary traffic % |
| `updates.dry_run` | false | Dry-run mode |

---

## 9. Error Handling

| Error Type | Strategy |
|---|---|
| Download failure | Abort; clean temp files; retry configurable times |
| Signature verification failure | Abort; log security alert; do not apply |
| Backup failure | Abort update; do not apply (backup first policy) |
| Migration failure | Rollback to backup; log error; alert operator |
| Canary regression | Auto-rollback to previous version |

---

## 10. Known Limitations & Future Work

- Online schema evolution (add column without restart) is in progress.
- Multi-node coordinated updates (rolling restart) are planned.
- Plugin-based migration scripting (Python/AQL) is planned.

---

## 11. References

- `src/updates/README.md` — module overview
- `docs/updates/` — update and migration guides
- `ARCHITECTURE.md` (root) — full system architecture
