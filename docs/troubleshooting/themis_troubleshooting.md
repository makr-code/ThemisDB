# Themis Troubleshooting Guide

The `themis` module is ThemisDB's edition and module management layer, handling edition detection, module dependency resolution, module binary hash verification, and the core wire protocol server entry point.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `EditionManager: edition file not found` | License file missing | Copy edition license to expected path |
| `ModuleHashVerifier: checksum mismatch` | Binary tampered or corrupted | Reinstall ThemisDB packages |
| `ModuleDependencyResolver: cycle` | Circular module dependencies | Check module dependency graph |
| `WireProtocolServer: port already in use` | Port conflict | Change `themis.port` |
| Feature unavailable for edition | Edition does not include feature | Upgrade edition |
| `EditionManager: license expired` | License has expired | Renew license |
| Module not loading despite being enabled | Dependency not satisfied | Check module load order |
| Hash verification too slow | Too many modules | Run verification in background |

## Common Issues

### Issue 1: Edition File Not Found

**Description:** ThemisDB cannot determine the edition because the license file is missing.

**Symptoms:**
- Log: `EditionManager: edition file not found at /etc/themisdb/edition.lic`
- Server defaults to Community Edition with limited features

**Cause:** License file not installed or wrong path.

**Solution:**
```bash
# Check expected path
themisdb-admin edition info | grep license_path

# Copy license file
cp /tmp/themisdb-enterprise.lic /etc/themisdb/edition.lic
chmod 640 /etc/themisdb/edition.lic
chown root:themisdb /etc/themisdb/edition.lic

# Verify edition
themisdb-admin edition info
```
```yaml
themis:
  edition:
    license_file: /etc/themisdb/edition.lic
    default_edition: community    # fallback if license missing
```

---

### Issue 2: Module Binary Hash Verification Fails

**Description:** ThemisDB refuses to load a module because its binary hash does not match the expected value.

**Symptoms:**
- Log: `ModuleHashVerifier: hash mismatch for module 'analytics': expected=abc123 got=def456`
- Module load aborted; feature unavailable

**Cause:** Module binary was modified, corrupted during download, or installed from wrong package.

**Solution:**
```bash
# Verify all module hashes
themisdb-admin themis verify-modules

# Reinstall corrupted module
apt reinstall themisdb-module-analytics

# Or rebuild from source
make MODULE=analytics install
```
```yaml
themis:
  hash_verifier:
    enabled: true
    strict: false                  # warn instead of failing on mismatch
    background_verification: true  # verify asynchronously at startup
```

---

### Issue 3: Module Dependency Cycle

**Description:** Module dependency resolver detects a circular dependency and cannot determine load order.

**Symptoms:**
- Log: `ModuleDependencyResolver: circular dependency detected: auth → security → auth`
- Affected modules fail to load

**Cause:** Module manifest declares circular dependencies.

**Solution:**
```bash
# Show dependency graph
themisdb-admin themis dependency-graph --format dot > deps.dot
dot -Tsvg deps.dot -o deps.svg

# Show cycle details
themisdb-admin themis dependency-cycles
```
*Fix by extracting shared functionality into a `security_core` module that neither `auth` nor `security` depends on.*

---

### Issue 4: License Expired

**Description:** ThemisDB Enterprise edition features become unavailable because the license expired.

**Symptoms:**
- Log: `EditionManager: license expired on 2025-12-31; reverting to community edition`
- Enterprise features return `403 Feature not available`

**Cause:** License file has expired.

**Solution:**
```bash
# Check license expiry
themisdb-admin edition license-info

# Install renewed license
themisdb-admin edition install-license /tmp/themisdb-renewed.lic

# Restart to apply new license
systemctl restart themisdb
```

---

### Issue 5: Wire Protocol Server Port Conflict

**Description:** ThemisDB wire protocol server cannot bind its port.

**Symptoms:**
- Log: `WireProtocolServer: bind failed: port 8080 already in use`
- Server exits at startup

**Cause:** Another process already using port 8080.

**Solution:**
```bash
# Find conflicting process
ss -tlnp | grep 8080
lsof -i :8080

# Change ThemisDB port
```
```yaml
themis:
  port: 8081                      # change to free port
  bind_address: 0.0.0.0
```

---

### Issue 6: Hash Verification Slows Startup

**Description:** Startup takes more than 60 seconds due to module hash verification.

**Symptoms:**
- Log: `ModuleHashVerifier: verifying 45 module binaries...`
- Health check fails during startup (timeout)

**Cause:** Synchronous hash verification of all modules at startup.

**Solution:**
```yaml
themis:
  hash_verifier:
    enabled: true
    background_verification: true  # verify asynchronously after startup
    verify_critical_only: true     # only verify core modules at startup
    critical_modules:
      - storage
      - auth
      - security
```

## Diagnostic Commands

```bash
# Edition info
themisdb-admin edition info

# Module hash verification
themisdb-admin themis verify-modules

# Dependency graph
themisdb-admin themis dependency-graph

# Module load order
themisdb-admin themis module-load-order

# License validity
themisdb-admin edition license-info

# Tail themis logs
journalctl -u themisdb -f | grep -E "themis|edition|module.hash|dependency.resolv"
```

## Configuration Reference

```yaml
themis:
  port: 8080
  bind_address: 0.0.0.0
  edition:
    license_file: /etc/themisdb/edition.lic
    default_edition: community
  hash_verifier:
    enabled: true
    strict: true
    background_verification: false
```

## Known Limitations

- Edition changes require a server restart to take full effect; in-flight requests use the old edition.
- Module hash verification is SHA-256 based; computation time scales linearly with number and size of modules.
- Wire protocol server does not support hot-restart; port changes require a full restart.
- Community Edition feature limits are enforced at the `themis` layer and cannot be bypassed at runtime.

## Related Documentation

- [Themis Module ROADMAP](../../src/themis/ROADMAP.md)
- [Core Troubleshooting](./core_troubleshooting.md)
- [Edition Manager](../EDITION_MANAGER.md)
- [Signature Verification Guide](../de/security/SIGNATURE_VERIFICATION_GUIDE.md)
- [Wire Protocol](../wire-protocol.md)
