> **Architektur-Hinweis:** Klassen/Typen/Namespaces mit aktuellem Sourcecode abgleichen. Symbole, die nicht im Source gefunden werden, mit `<!-- TODO: verify symbol -->` markieren.

# Security Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/security/`

---

## 1. Overview

The Security module implements ThemisDB's defense-in-depth data protection layer:
AES-256-GCM field-level encryption, key management hierarchy (Master Key → Data Encryption
Keys), RBAC with role hierarchy, row-level security, PII redaction, AQL injection
detection, mTLS PKI integration, HSM (hardware security module) support, CMS digital
signing, eIDAS-compatible timestamping, malware scanning, zero-trust policy enforcement,
and VRAM secure clear for GPU privacy.

---

## 2. Design Principles

- **Defense in Depth** – six independent security layers (transport, authn, authz,
  data, audit, threat detection) ensure no single component failure exposes data.
- **Key Hierarchy** – Master Key (HSM/Vault) → Collection Keys → Field Keys;
  data encryption keys are never stored in plaintext.
- **Zero Trust** – `zero_trust_policy_enforcer.cpp` re-verifies every access decision,
  even within the same session.
- **PII by Policy** – PII redaction is policy-driven per collection and per field;
  it is applied at read time so data is always stored unredacted but served redacted.
- **Fail Closed** – if the key provider is unreachable, encryption/decryption fails
  (data not served), not the other way around.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `field_encryption.cpp` | AES-256-GCM field-level encryption/decryption |
| `encrypted_field.cpp` | Encrypted field type: ciphertext + IV + AAD |
| `key_cache.cpp` | In-memory key cache (KEK-wrapped DEKs) |
| `vault_key_provider.cpp` | HashiCorp Vault key provider |
| `hsm_provider.cpp` / `hsm_provider_pkcs11.cpp` | HSM key provider (PKCS#11) |
| `hsm_key_provider_adapter.cpp` | HSM adapter for unified key provider interface |
| `pki_key_provider.cpp` | PKI certificate-based key provider |
| `mock_key_provider.cpp` | Test/dev key provider (in-memory only) |
| `keyprovider_signing.cpp` | Signing operations via key provider |
| `secret_manager.cpp` | Runtime secret storage (API keys, passwords) |
| `rbac.cpp` | Role-Based Access Control with role hierarchy |
| `access_control.cpp` / `access_control_manager.cpp` | RBAC enforcement |
| `row_level_security.cpp` | Per-row access predicates (tenant, owner, group) |
| `pii_redaction_policy.cpp` | PII field masking policy evaluation |
| `query_masking_policy.cpp` | Query result masking based on caller permissions |
| `aql_injection_detector.cpp` | AQL injection pattern detection |
| `malware_scanner.cpp` | Uploaded content malware signature scanning |
| `cms_signing.cpp` | CMS (Cryptographic Message Syntax) digital signing |
| `manifest_signer.cpp` | Plugin/manifest Ed25519 signing |
| `binary_manifest.cpp` | Binary artifact manifest with hash + signature |
| `timestamp_authority.cpp` / `timestamp_authority_openssl.cpp` | RFC 3161 trusted timestamping |
| `vcc_pki_client.cpp` | VCC PKI client for certificate management |
| `usb_admin_authenticator.cpp` | USB token-based admin authentication |
| `zero_trust_policy_enforcer.cpp` | Continuous trust verification |
| `confidential_computing.cpp` | TEE/enclave support (Intel TDX and AMD SEV/SEV-SNP) |
| `vram_secure_clear.cpp` | VRAM zero-fill after GPU computation |
| `user_registration_plugin.cpp` | User registration plugin interface |
| `embedded_user_registration_plugin.cpp` | Built-in user registration |
| `arrow_user_registration_plugin.cpp` | Arrow/Parquet user registration |
| `webdav_user_registration_plugin.cpp` | WebDAV user registration |

### 3.2 Security Layer Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│  Layer 1: Transport (TLS 1.3, mTLS)  →  src/network/ + auth    │
├─────────────────────────────────────────────────────────────────┤
│  Layer 2: Authentication (USB token, PKI, TOTP)  →  src/auth/  │
├─────────────────────────────────────────────────────────────────┤
│  Layer 3: Authorization (RBAC + RLS)                            │
│    rbac.cpp → roles → permissions                               │
│    row_level_security.cpp → per-row predicates                  │
├─────────────────────────────────────────────────────────────────┤
│  Layer 4: Data Protection (field encryption + PII)              │
│    field_encryption.cpp (AES-256-GCM)                           │
│    key hierarchy: HSM/Vault → KEK → DEK                        │
│    pii_redaction_policy.cpp (read-time masking)                 │
├─────────────────────────────────────────────────────────────────┤
│  Layer 5: Audit & Compliance                                    │
│    cms_signing.cpp + timestamp_authority.cpp (eIDAS)           │
│    aql injection detection + malware scanning                   │
├─────────────────────────────────────────────────────────────────┤
│  Layer 6: Zero Trust + GPU Privacy                              │
│    zero_trust_policy_enforcer.cpp                               │
│    vram_secure_clear.cpp                                        │
└─────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Field Encryption (Write)

```
Write: doc.ssn = "123-45-6789"
    │
    ├─ field_encryption.cpp: field marked as ENCRYPTED in schema?
    │       → yes → EncryptedField workflow
    │
    ├─ key_cache.lookup(collection_key_id)
    │       → cache miss → vault_key_provider.unwrapDEK(key_id)
    │
    ├─ AES-256-GCM encrypt(plaintext, DEK, IV=random)
    │       → {ciphertext, IV, AAD=collection+field}
    │
    └─ store encrypted blob → RocksDB
```

### 4.2 PII Redaction (Read)

```
Read: doc.email = "<encrypted_blob>"
    │
    ├─ field_encryption.cpp: decrypt → "user@example.com"
    │
    ├─ pii_redaction_policy.cpp: caller has PII:READ permission?
    │       → yes → return "user@example.com"
    │       → no  → return "u***@example.com" (masked)
    │
    └─ query_masking_policy.cpp: apply any additional query-level masks
```

### 4.3 RBAC Enforcement

```
Request: GET /collection/users  (principal: alice, roles: [analyst])
    │
    ├─ rbac.cpp: analyst has READ on collection "users"?
    │       → check role hierarchy (analyst → data_reader → READ)
    │       → granted
    │
    ├─ row_level_security.cpp: apply tenant predicate
    │       → FILTER doc.tenant_id == alice.tenant_id
    │
    └─ proceed with filtered results
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Called by** | `src/server/` | Per-request RBAC and RLS enforcement |
| **Called by** | `src/storage/` | Field encryption on write |
| **Called by** | `src/auth/` | Token blacklist, TOTP secret encryption |
| **Called by** | `src/content/` | Malware scanning |
| **Called by** | `src/governance/` | PII policy integration |
| **Uses** | `src/observability/` | Security event metrics |

---

## 6. Threading & Concurrency Model

- `FieldEncryption` is stateless and safe for concurrent invocation.
- `KeyCache` uses a read-write lock; key unwrap operations are serialized.
- `RBAC` role evaluation is read-only after initialization (lock-free reads).
- `ZeroTrustPolicyEnforcer` runs synchronously on the request path.
- `VRAMSecureClear` is called from the GPU work completion callback.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| DEK cache | Unwrapped DEKs cached (mlock'd) to avoid per-field Vault round-trips |
| AES-NI | AES-256-GCM uses hardware AES-NI instructions (transparent via OpenSSL) |
| RBAC role cache | Role-permission mappings cached after initial load |
| Lazy PII check | PII policy evaluated only when a PII-annotated field is read |

---

## 8. Security Considerations

- DEKs are never stored in plaintext; always wrapped by KEK (from HSM/Vault).
- DEK cache entries are `mlock`ed to prevent swap to disk.
- VRAM secure clear prevents residual model/query data leaking between tenants on shared GPUs.
- Zero trust policy enforcer re-validates on every sensitive operation.
- AQL injection detector runs on every user-supplied AQL string.
- USB admin authenticator prevents unauthorized admin operations.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `security.encryption.key_provider` | "vault" | Key provider: vault / hsm / mock |
| `security.vault.url` | "" | HashiCorp Vault endpoint |
| `security.key_cache.ttl_s` | 300 | DEK cache TTL |
| `security.rbac.enabled` | true | Enable RBAC |
| `security.rls.enabled` | true | Enable row-level security |
| `security.pii.enabled` | true | Enable PII redaction |
| `security.zero_trust.enabled` | false | Enable zero-trust re-verification |
| `security.vram_clear.enabled` | true | Zero-fill VRAM after GPU use |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Key provider unreachable | Fail closed: reject read/write; log critical |
| DEK cache miss + Vault timeout | Retry with backoff; fail closed after max attempts |
| RBAC role not found | Deny access; log |
| Malware detected | Reject upload; log security event; alert |
| HSM PKCS#11 error | Fall back to Vault (if configured); log warning |

---

## 11. Known Limitations & Future Work

- Confidential computing (Intel TDX and AMD SEV/SEV-SNP enclaves) is implemented with CPUID detection, kernel driver attestation, and AES-256-GCM sealed memory. Software fallback is active on non-TEE hardware.
- Automated key rotation (DEK re-encryption) is in progress.
- PKCS#11 HSM integration supports SoftHSM for testing; production HSM testing needed.
- eIDAS-compatible timestamping is implemented for EU compliance; other jurisdictions
  may require additional adapters.

---

## 12. References

- `src/security/README.md` — module overview
- `docs/security/` — security architecture documentation
- `docs/architecture/security_architecture.md` — layered security diagram
- `ARCHITECTURE.md` (root) — full system architecture
