# Core Troubleshooting Guide

The `core` module is the foundational database engine for ThemisDB, responsible for security layer initialisation, core operation dispatch, and the bootstrap sequence that brings all other modules online.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| Server fails to start | Security init error or missing config | Check logs: `journalctl -u themisdb -n 100` |
| `SecurityLayer: key provider unavailable` | HSM or Vault unreachable at startup | Check key provider; set `core.startup.skip_key_provider: false` |
| Core dumps on startup | Incompatible library version | Check `ldd /usr/bin/themisdb`; reinstall |
| `ConcernsContext: module not registered` | Module failed to register | Check module load order in logs |
| High CPU on idle | Security initialization loop | Check `core.security.rekey_interval_ms` |
| `core: assertion failed in security_init` | Missing or invalid certificate | Regenerate server certificate |
| Module dependency cycle | Wrong module load order | Review `core.module_load_order` |
| `core: feature flag disabled` | Edition feature not available | Check edition limits |

## Common Issues

### Issue 1: Server Fails to Start Due to Security Initialisation

**Description:** ThemisDB exits immediately during the security layer bootstrap.

**Symptoms:**
- Log: `SecurityLayerBuilder: failed to initialise key provider: connection refused`
- Exit code 1 immediately after startup

**Cause:** The configured key provider (Vault, HSM) is unavailable at startup time.

**Solution:**
```bash
# Check key provider connectivity
curl -s http://vault:8200/v1/sys/health

# Start with local key provider for development
```
```yaml
core:
  security:
    key_provider: local           # "local" | "vault" | "hsm"
    startup:
      skip_key_provider: false    # set true only in dev to bypass
      key_provider_timeout_ms: 10000
      retry_count: 3
```

---

### Issue 2: Module Registration Fails at Boot

**Description:** A required module fails to register during the startup sequence.

**Symptoms:**
- Log: `ConcernsContext: module 'storage' failed to register`
- Subsequent modules that depend on storage also fail

**Cause:** Module initialization error (missing config, port conflict, etc.).

**Solution:**
```bash
# Enable verbose startup logging
themisdb --log-level debug --log-module core

# Check module registration order
themisdb-admin core module-status

# Attempt startup with specific modules disabled (for diagnosis)
themisdb --disable-module=gpu --disable-module=llm
```
```yaml
core:
  module_load_order:
    - config
    - storage
    - security
    - auth
    - network
    - api
  startup:
    fail_fast: true                # exit on first module failure
    module_timeout_ms: 30000
```

---

### Issue 3: Core Dump on Startup

**Description:** ThemisDB crashes with a segfault during the startup sequence.

**Symptoms:**
- `systemctl status themisdb` shows `signal=SIGSEGV`
- Core dump in `/var/crash/` or `/tmp/`

**Cause:** Incompatible shared library version linked at runtime vs. compile time.

**Solution:**
```bash
# Check shared library dependencies
ldd /usr/bin/themisdb

# Check for version conflicts
ldconfig -p | grep -E "librocksdb|libssl"

# Run with address sanitizer (development builds)
ASAN_OPTIONS=abort_on_error=0 themisdb --log-level debug 2>&1 | head -100

# Reinstall dependencies
apt reinstall themisdb librocksdb-dev libssl-dev
```

---

### Issue 4: High CPU Idle Due to Re-keying Loop

**Description:** ThemisDB uses excessive CPU even without any queries.

**Symptoms:**
- `top` shows `themisdb` at 30% CPU with no connections
- Log: `SecurityInit: re-keying in progress`

**Cause:** `rekey_interval_ms` is too short; continuous key rotation consuming CPU.

**Solution:**
```yaml
core:
  security:
    rekey_interval_ms: 86400000    # once per day instead of every minute
    background_rekey: true
    rekey_batch_size: 100
```

---

### Issue 5: Edition Feature Flag Blocks Expected Functionality

**Description:** A feature that should be available in the current edition is disabled.

**Symptoms:**
- Log: `core: feature 'multi_gpu' disabled for edition=community`
- API returns `{"error": "feature_not_available_in_edition"}`

**Cause:** Feature is restricted to a higher edition.

**Solution:**
```bash
# Check current edition and available features
themisdb-admin core edition-info

# List all feature flags
themisdb-admin core feature-flags list
```

---

### Issue 6: Assertion Failure in Security Init

**Description:** A failed assertion in the security layer terminates the process.

**Symptoms:**
- Log: `core: ASSERTION FAILED: cert != nullptr at security_initialization.cpp:45`
- Process terminates with `abort()`

**Cause:** TLS certificate file missing or empty.

**Solution:**
```bash
# Check certificate files
ls -la /etc/themisdb/tls/
openssl x509 -in /etc/themisdb/tls/server.crt -noout -subject

# Regenerate self-signed certificate for development
openssl req -x509 -newkey rsa:4096 -keyout /etc/themisdb/tls/server.key \
  -out /etc/themisdb/tls/server.crt -days 365 -nodes \
  -subj "/CN=themisdb-server"
```

## Diagnostic Commands

```bash
# Full startup diagnostic
themisdb --log-level debug --dry-run 2>&1 | head -200

# Module status
themisdb-admin core module-status

# Edition info
themisdb-admin core edition-info

# Security layer health
themisdb-admin core security-health

# Tail core logs
journalctl -u themisdb -f | grep -E "core|startup|security_init|module"
```

## Configuration Reference

```yaml
core:
  edition: community               # "community" | "enterprise"
  security:
    key_provider: local
    rekey_interval_ms: 86400000
  startup:
    fail_fast: true
    module_timeout_ms: 30000
  module_load_order: []            # empty = default order
```

## Known Limitations

- The `core` module cannot be disabled; it is always the first module initialised.
- Security layer re-keying is a blocking operation; do not set `rekey_interval_ms` below 3600000 in production.
- Edition limits are enforced at the `core` level and cannot be overridden at runtime.

## Related Documentation

- [Architecture Overview](../../ARCHITECTURE.md)
- [Setup Guide](../../SETUP.md)
- [Security Framework Implementation](../SECURITY_FRAMEWORK_IMPLEMENTATION.md)
- [Production Readiness Review](../implementation-history/reviews/PRODUCTION_READINESS_REVIEW.md)
