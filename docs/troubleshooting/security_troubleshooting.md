# Security Troubleshooting Guide

The `security` module provides ThemisDB's comprehensive security layer, including field-level encryption, HSM/PKCS#11 integration, RBAC/ABAC access control, row-level security, AQL injection detection, malware scanning, and zero-trust policy enforcement.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `AccessControlManager: permission denied` | User/role missing required privilege | Grant privilege via `themisdb-admin rbac grant` |
| `FieldEncryption: key not found for key_id=xxx` | KEK rotation without updating field metadata | Re-encrypt fields with new key; update `key_id` |
| `HsmProvider: slot not found` | Wrong PKCS#11 slot index | Run `pkcs11-tool --list-slots` and update config |
| `AqlInjectionDetector: query blocked` | Legitimate query matched injection pattern | Add query to allowlist; tune sensitivity |
| `MalwareScanner: scan timeout` | ClamAV daemon not running or overloaded | Restart `clamd`; check socket path |
| `ConfidentialComputing: enclave init failed` | TDX/SEV not available or kernel driver absent | Verify TDX/SEV support via CPUID; ensure `/dev/tdx_guest` or `/dev/sev-guest` is accessible |
| Row-level security blocks admin user | Admin role not in RLS bypass list | Add admin role to `security.rls.bypass_roles` |
| HSM response latency >500ms | HSM under heavy load or network issue | Enable HSM key cache; check HSM connectivity |
| `BinaryManifest: signature verification failed` | Plugin binary tampered or wrong key | Re-sign plugin; update trust anchor |
| `KeyCache: eviction before TTL` | Cache too small for number of key IDs | Increase `security.key_cache.max_entries` |

## Common Issues

### Issue 1: RBAC Permission Denied for Valid Role

**Description:** A user with the correct role still receives `403 permission denied`.

**Symptoms:**
- Log: `AccessControlManager: user=alice role=analyst denied action=query:write on collection=orders`
- User is visibly in the `analyst` role

**Cause:** Privilege was granted on the wrong resource scope (database vs. collection level), or role inheritance is not configured.

**Solution:**
```bash
# Check effective privileges for user
themisdb-admin rbac effective-permissions --user alice

# Grant missing privilege
themisdb-admin rbac grant \
  --role analyst \
  --privilege query:write \
  --resource collection/orders

# List role hierarchy
themisdb-admin rbac role-tree --role analyst
```

---

### Issue 2: Field Encryption Key Not Found

**Description:** Reads of encrypted fields fail because the encryption key has been rotated.

**Symptoms:**
- Log: `FieldEncryption: key_id=dek-2024-01 not found in key provider`
- Queries on encrypted fields return errors instead of plaintext

**Cause:** The Data Encryption Key (DEK) was rotated but the encrypted field metadata still references the old `key_id`.

**Solution:**
```bash
# List available key IDs
themisdb-admin security keys list

# Re-encrypt a collection's encrypted fields with the current key
themisdb-admin security reencrypt \
  --collection users \
  --fields email,ssn \
  --old-key-id dek-2024-01 \
  --new-key-id dek-2025-01
```
```yaml
security:
  field_encryption:
    key_rotation:
      auto_reencrypt: true     # automatically re-encrypt on key rotation
      batch_size: 1000
```

---

### Issue 3: HSM PKCS#11 Slot Not Found

**Description:** ThemisDB cannot connect to the HSM because the configured slot index does not exist.

**Symptoms:**
- Log: `HsmProviderPkcs11: C_OpenSession failed: CKR_SLOT_ID_INVALID (slot=2)`
- Server refuses to start when `security.hsm.required: true`

**Cause:** HSM firmware update changed slot numbering, or the PKCS#11 library path is wrong.

**Solution:**
```bash
# List HSM slots
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so --list-slots

# Test key operations
pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
  --login --pin $HSM_PIN --list-objects
```
```yaml
security:
  hsm:
    enabled: true
    pkcs11_library: /usr/lib/softhsm/libsofthsm2.so
    slot: 0                    # update to correct slot
    pin: ${HSM_PIN}
    label: themisdb-master-key
    required: true
```

---

### Issue 4: AQL Injection Detector Blocks Legitimate Queries

**Description:** The injection detector flags valid queries from application code.

**Symptoms:**
- Log: `AqlInjectionDetector: blocked query from user=app_user score=0.87`
- Application receives `403` with `{"error": "query_injection_detected"}`

**Cause:** Detector sensitivity is too high; the query contains patterns that resemble injection (e.g., dynamic field names).

**Solution:**
```yaml
security:
  aql_injection_detector:
    enabled: true
    sensitivity: medium          # high | medium | low
    block_threshold: 0.90        # increase from 0.80
    log_threshold: 0.50
    allowlist_queries:
      - "FOR doc IN @@col FILTER doc.@field == @value RETURN doc"
```
```bash
# Check injection score for a specific query
themisdb-admin security injection-check \
  --query "FOR u IN users FILTER u.status == @s RETURN u"
```

---

### Issue 5: Row-Level Security Blocks Admin Operations

**Description:** An admin user cannot query a collection because RLS is applied unexpectedly.

**Symptoms:**
- Admin queries return empty result sets
- Log: `AccessControl: RLS policy 'tenant_isolation' applied to user=admin`

**Cause:** Admin role is not in the RLS bypass list.

**Solution:**
```yaml
security:
  rls:
    enabled: true
    bypass_roles:
      - admin
      - system
      - monitoring
    default_policy: deny        # deny if no matching policy
```

---

### Issue 6: Malware Scanner Timeout

**Description:** File uploads stall because the ClamAV scanner cannot respond in time.

**Symptoms:**
- Log: `MalwareScanner: scan timed out after 5000ms for file=upload_abc.dat`
- Large file uploads always fail

**Cause:** ClamAV daemon socket is unavailable or the scan timeout is too short for large files.

**Solution:**
```bash
# Check ClamAV daemon status
systemctl status clamav-daemon
clamdscan --ping

# Test scan manually
clamdscan /tmp/test-upload.dat
```
```yaml
security:
  malware_scanner:
    enabled: true
    backend: clamav
    clamav_socket: /var/run/clamav/clamd.sock
    scan_timeout_ms: 30000       # increase from 5000
    max_file_size_mb: 100        # skip files larger than this
    quarantine_dir: /var/lib/themisdb/quarantine
```

---

### Issue 7: CMS Signing Fails with `Key not found`

**Description:** Signed artifacts cannot be produced because the signing key is unavailable.

**Symptoms:**
- Log: `CmsSigning: signing key 'code-sign-2025' not found in key provider`
- Plugin packaging fails

**Cause:** Signing key was deleted or the key provider configuration changed.

**Solution:**
```bash
# List signing keys
themisdb-admin security signing-keys list

# Import a signing key
themisdb-admin security signing-keys import \
  --name code-sign-2025 \
  --key-file /etc/themisdb/signing.key \
  --cert-file /etc/themisdb/signing.crt
```

---

### Issue 8: Key Cache Evicts Keys Under Load

**Description:** Under high query load, decryption operations slow because the key cache is too small.

**Symptoms:**
- Log: `KeyCache: evicting key_id=dek-001 before TTL (cache full)`
- Decryption latency spikes to >50ms

**Cause:** `security.key_cache.max_entries` is too small for the number of distinct DEKs in use.

**Solution:**
```yaml
security:
  key_cache:
    max_entries: 1024           # increase from default 128
    ttl_seconds: 3600
    refresh_before_expiry_seconds: 300
```

---

### Issue 9: Confidential Computing Enclave Init Fails

**Description:** Intel TDX or AMD SEV/SEV-SNP kernel driver is unavailable; ThemisDB falls back to software mode.

**Symptoms:**
- Log: `ConfidentialComputing: /dev/tdx_guest unavailable (...); TDX detected via CPUID only`
- Log: `ConfidentialComputing: /dev/sev-guest not accessible (...); returning software-mode report`

**Cause:** TDX/SEV hardware present (CPUID confirms) but the Linux kernel driver is absent or inaccessible (kernel < 6.2 for TDX, kernel < 5.19 for SEV-SNP, or insufficient permissions).

**Solution:**
```bash
# Check Intel TDX support (kernel ≥ 6.2)
cpuid -l 0x21 -s 0          # should show "IntelTDX    " in EBX/EDX/ECX
ls -la /dev/tdx_guest        # must exist and be accessible

# Check AMD SEV-SNP support (kernel ≥ 5.19)
cpuid -l 0x8000001f          # EAX bit 4 = SEV-SNP supported
ls -la /dev/sev-guest        # must exist and be accessible

# Disable if TEE hardware is not available
```
```yaml
security:
  confidential_computing:
    enabled: false      # disable if TDX/SEV not available
    fallback_to_software: true
```

---

### Issue 10: Binary Manifest Verification Fails After Update

**Description:** Plugin or binary refuses to load because manifest signature is invalid.

**Symptoms:**
- Log: `BinaryManifest: HMAC verification failed for plugin=my_plugin.so`
- Plugin load is rejected even though binary is unchanged

**Cause:** The trust anchor certificate was rotated but the plugin was not re-signed.

**Solution:**
```bash
# Re-sign a plugin binary
themisdb-admin security manifest sign \
  --binary /usr/lib/themisdb/plugins/my_plugin.so \
  --key-id code-sign-2025 \
  --output /usr/lib/themisdb/plugins/my_plugin.so.sig

# Verify signature
themisdb-admin security manifest verify \
  --binary /usr/lib/themisdb/plugins/my_plugin.so
```

## Diagnostic Commands

```bash
# Check RBAC effective permissions for a user
themisdb-admin rbac effective-permissions --user alice

# List encryption keys
themisdb-admin security keys list

# Show HSM health
themisdb-admin security hsm-health

# Run injection detector on a query
themisdb-admin security injection-check --query "FOR u IN users RETURN u"

# Check RLS policies
themisdb-admin security rls list-policies

# Audit log for security events
themisdb-admin audit log --filter security --last 100

# Live security metrics
curl -s http://localhost:9100/metrics | grep themisdb_security

# Tail security logs
journalctl -u themisdb -f | grep -E "security|access_control|encryption|hsm|injection"
```

## Configuration Reference

```yaml
security:
  rbac:
    enabled: true
    default_action: deny
  rls:
    enabled: true
    bypass_roles: [admin, system]
  field_encryption:
    enabled: true
    algorithm: AES-256-GCM
    key_provider: hsm              # "hsm" | "vault" | "local"
  hsm:
    enabled: false
    pkcs11_library: /usr/lib/softhsm/libsofthsm2.so
    slot: 0
    pin: ${HSM_PIN}
    required: false
  key_cache:
    max_entries: 256
    ttl_seconds: 3600
  aql_injection_detector:
    enabled: true
    sensitivity: medium
    block_threshold: 0.90
  malware_scanner:
    enabled: false
    backend: clamav
    scan_timeout_ms: 10000
  confidential_computing:
    enabled: false
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `rls.bypass_roles` | `[]` | Include `admin`, `system` |
| `hsm.required` | `true` without HSM | `false` until HSM is provisioned |
| `aql_injection_detector.block_threshold` | `0.5` | `0.85–0.95` |
| `key_cache.max_entries` | `16` | `≥ 256` in production |

## Known Limitations

- PKCS#11 HSM operations are synchronous; high-throughput encryption workloads may bottleneck on the HSM.
- ClamAV malware scanner does not support scanning of encrypted blob content.
- Row-level security policies are evaluated in-process; complex policies with many rules impact query latency.
- AQL injection detector uses heuristic pattern matching and may produce false positives on dynamic query templates.
- SGX confidential computing support requires specific hardware (Intel Xeon E/SP series with SGX enabled).

## Related Documentation

- [Security Module ROADMAP](../../src/security/ROADMAP.md)
- [Encryption Strategy](../security/encryption_strategy.md)
- [HSM Production Setup](../security/HSM_PRODUCTION_SETUP.md)
- [Zero Trust Policy Enforcer](../security/zero_trust_policy_enforcer.md)
- [Access Control Framework](../security/access_control_framework.md)
- [Security Executive Summary](../de/security/SECURITY_EXECUTIVE_SUMMARY.md)
