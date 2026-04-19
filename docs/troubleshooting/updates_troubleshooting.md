# Updates Troubleshooting Guide

The `updates` module handles schema migrations, canary rollouts, hot-reload of engine components, in-place schema migrations, and release manifest management for ThemisDB.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| Migration fails midway | Incompatible schema change | Use `--dry-run` first; check migration plan |
| `CanaryRollout: traffic split not applied` | Routing rules not propagated | Check `updates.canary.routing_backend` |
| Hot reload engine fails to apply update | Update validation failed | Check update manifest checksum |
| `InPlaceSchemaMigrator: lock timeout` | Another migration running | Wait or kill stuck migration lock |
| `ReleaseManifest: signature invalid` | Update package tampered | Re-download and verify package signature |
| Schema migration tester reports false failure | Test data mismatch | Update test fixtures in migration |
| Canary rollout stuck at 10% | Automated progression disabled | Enable `canary.auto_progress: true` |
| `DeltaUpdateEngine: delta too large` | Full rebuild needed | Use full update instead of delta |
| Update state machine in FAILED state | Migration error not handled | Check migration logs; manually advance state |
| `ManifestDatabase: version conflict` | Two concurrent updates | Serialise updates; check coordinator lock |

## Common Issues

### Issue 1: Schema Migration Fails Midway

**Description:** A schema migration fails after partially applying changes.

**Symptoms:**
- Log: `InPlaceSchemaMigrator: migration step 3 failed: column type mismatch`
- Collection is left in partially migrated state

**Cause:** Incompatible field type change attempted without data transformation.

**Solution:**
```bash
# Always dry-run first
themisdb-admin updates migration dry-run \
  --migration add_index_v2 \
  --collection users

# Check migration plan
themisdb-admin updates migration plan --migration add_index_v2

# Roll back to previous state
themisdb-admin updates migration rollback \
  --migration add_index_v2 \
  --to-version pre-migration
```
```yaml
updates:
  migration:
    timeout_ms: 300000
    rollback_on_failure: true
    verify_after: true
    backup_before: true
```

---

### Issue 2: Canary Rollout Stuck at Low Percentage

**Description:** Canary rollout is deployed but traffic percentage does not increase over time.

**Symptoms:**
- Canary has been at 10% for 24 hours with no errors
- Expected: automatic progression to 25%, 50%, 100%

**Cause:** Auto-progression is disabled; manual approval required.

**Solution:**
```yaml
updates:
  canary:
    enabled: true
    auto_progress: true
    stages:
      - percentage: 10
        duration_minutes: 30
        error_threshold_pct: 1.0
      - percentage: 25
        duration_minutes: 60
      - percentage: 50
        duration_minutes: 120
      - percentage: 100
    routing_backend: api_gateway    # "api_gateway" | "nginx" | "envoy"
```
```bash
# Manually advance canary stage
themisdb-admin updates canary advance --deployment v1.5.0

# Check canary health
themisdb-admin updates canary status --deployment v1.5.0
```

---

### Issue 3: Hot Reload Engine Fails to Apply

**Description:** Hot reload of an engine component fails validation.

**Symptoms:**
- Log: `HotReloadEngine: validation failed for engine component 'query_optimizer'`
- Server continues with old component version

**Cause:** New component has incompatible API or failed checksum validation.

**Solution:**
```yaml
updates:
  hot_reload:
    enabled: true
    validate_before_apply: true
    checksum_verification: true
    rollback_on_failure: true
    reload_timeout_ms: 30000
```
```bash
# Validate update package before applying
themisdb-admin updates hot-reload validate \
  --package /tmp/query_optimizer_v2.so

# Force reload (skip validation – use with caution)
themisdb-admin updates hot-reload apply \
  --component query_optimizer \
  --package /tmp/query_optimizer_v2.so \
  --skip-validation
```

---

### Issue 4: Migration Lock Timeout

**Description:** Schema migration cannot acquire the migration lock because another migration is running.

**Symptoms:**
- Error: `InPlaceSchemaMigrator: failed to acquire migration lock after 60000ms`
- Another migration process is stuck

**Cause:** A previous migration failed without releasing the lock.

**Solution:**
```bash
# Check migration lock status
themisdb-admin updates migration lock-status

# Force release lock (only if migration process is dead)
themisdb-admin updates migration release-lock --force

# List running migrations
themisdb-admin updates migration list --state running
```

---

### Issue 5: Release Manifest Signature Invalid

**Description:** An update package is rejected because its manifest signature cannot be verified.

**Symptoms:**
- Log: `ReleaseManifest: signature verification failed for package=themisdb-1.5.1.tar.gz`
- Update installation aborted

**Cause:** Package was tampered, downloaded incorrectly, or trust anchor is outdated.

**Solution:**
```bash
# Re-download the package
wget https://releases.themisdb.com/themisdb-1.5.1.tar.gz
wget https://releases.themisdb.com/themisdb-1.5.1.tar.gz.sig

# Verify signature manually
themisdb-admin updates verify-signature \
  --package themisdb-1.5.1.tar.gz \
  --signature themisdb-1.5.1.tar.gz.sig

# Update trust anchor if needed
themisdb-admin updates update-trust-anchor \
  --cert /etc/themisdb/trust/release-signing.crt
```

---

### Issue 6: Schema Migration Tester Reports False Failures

**Description:** Migration tests fail even though the migration logic is correct.

**Symptoms:**
- Log: `SchemaMigrationTester: test 'users_v2' failed: expected field 'email' not found`
- Migration works correctly in production

**Cause:** Test fixture data is stale and does not reflect current collection schema.

**Solution:**
```bash
# Regenerate test fixtures from current collection
themisdb-admin updates migration generate-fixtures \
  --collection users \
  --output tests/fixtures/users_v2.json

# Run migration tests with updated fixtures
themisdb-admin updates migration test \
  --migration add_email_field \
  --fixtures tests/fixtures/users_v2.json
```

---

### Issue 7: Delta Update Too Large for Delta Engine

**Description:** Delta update calculation fails because the change set is too large.

**Symptoms:**
- Log: `DeltaUpdateEngine: delta size=2.1GB exceeds max_delta_size=500MB`
- Update falls back to full rebuild

**Cause:** Too many changes accumulated since last full build.

**Solution:**
```yaml
updates:
  delta:
    enabled: true
    max_delta_size_mb: 2048        # increase from 500MB
    fallback_to_full: true         # automatically fall back to full update
    compress_delta: true
    delta_algorithm: bsdiff        # "bsdiff" | "xdelta3" | "simple"
```

---

### Issue 8: Update State Machine Stuck in FAILED State

**Description:** An update's state machine is stuck and cannot be retried.

**Symptoms:**
- `themisdb-admin updates list` shows update in `FAILED` state
- Retry attempts return `409 Conflict`

**Cause:** Unhandled error left the state machine in a terminal FAILED state.

**Solution:**
```bash
# Reset update state machine to allow retry
themisdb-admin updates reset-state \
  --update-id upd-abc123 \
  --to-state PENDING

# Or abort and clean up
themisdb-admin updates abort \
  --update-id upd-abc123 \
  --cleanup
```

## Diagnostic Commands

```bash
# List all updates
themisdb-admin updates list

# Migration history
themisdb-admin updates migration history --collection users

# Canary rollout status
themisdb-admin updates canary status

# Pending updates
themisdb-admin updates list --state pending

# Live update metrics
curl -s http://localhost:9100/metrics | grep themisdb_updates

# Tail updates logs
journalctl -u themisdb -f | grep -E "updates|migration|canary|hot.reload|manifest"
```

## Configuration Reference

```yaml
updates:
  migration:
    rollback_on_failure: true
    verify_after: true
    backup_before: true
    timeout_ms: 300000
  canary:
    enabled: true
    auto_progress: true
    routing_backend: api_gateway
  hot_reload:
    enabled: true
    rollback_on_failure: true
  delta:
    enabled: true
    max_delta_size_mb: 1024
    fallback_to_full: true
```

## Known Limitations

- In-place schema migrations do not support renaming collections; use create-and-copy strategy.
- Canary rollouts require a load balancer with weight-based routing; not all backends support this.
- Delta updates require the exact previous version to be installed; skip-version updates require full rebuild.
- Schema migration tester runs against a copy of the collection; large collections may require significant disk space.

## Related Documentation

- [Updates Module ROADMAP](../../src/updates/ROADMAP.md)
- [Migration Guide](../MIGRATION_GUIDE.md)
- [Canary Rollout Strategy](../ci-cd/branching-release-history/BRANCHING_STRATEGY.md)
- [Schema Migration Tester](../TESTING_AND_BENCHMARKING_GUIDE.md)
- [Signature Verification Guide](../de/security/SIGNATURE_VERIFICATION_GUIDE.md)
