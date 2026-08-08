# Crypto and Key Management Cross-Module Architecture

**Level:** L4 - Publication-Oriented Cross-Module Context  
**Last Updated:** 2026-08-08  
**Source:** L0-L3 documentation, ARCHITECTURE.md  
**SOT Domain:** architecture-governance, crypto-key-management  
**Audience:** Product architects, platform engineers, stakeholders  

---

## 1. Overview

### Purpose
ThemisDB's cryptographic infrastructure provides deterministic key derivation (HKDF) and daily key rotation (LEK) for secure data encryption across all modules. This document provides cross-module visibility into key management contracts, integration patterns, and operational workflows.

### Scope
- HKDF-based key derivation (RFC 5869)
- Daily LEK (Log Encryption Key) rotation
- Annual KEK (Key Encryption Key) management
- Multi-module key sharing and versioning
- Audit trail and compliance integration
- Threat model and security guarantees

### Key Metrics
| Metric | Target | Achieved |
|--------|--------|----------|
| Key Derivation Latency (cache hit) | <1ms | ✓ |
| Cache Hit Rate | >80% | ✓ |
| LEK Rotation Uptime | 99.9% | ✓ |
| Key Revocation Time to Enforcement | <100ms | ✓ |

---

## 2. System Architecture

### 2.1 Cryptographic Stack

```
┌─────────────────────────────────────────────────────────┐
│  Applications (Auth, Process, Storage, Query Modules)    │
├─────────────────────────────────────────────────────────┤
│  L2: Integration Guide (CRYPTO_INTEGRATION_GUIDE.md)     │
├─────────────────────────────────────────────────────────┤
│  L3: Lifecycle Contracts (KEY_LIFECYCLE.md)              │
├─────────────────────────────────────────────────────────┤
│  L1: Implementation (hkdf_cache.cpp, lek_manager.cpp)    │
├─────────────────────────────────────────────────────────┤
│  L0: API Contracts (hkdf_cache.h, lek_manager.h)         │
├─────────────────────────────────────────────────────────┤
│  OpenSSL 3.0+: EVP_KDF, AES-GCM                          │
│  RocksDB: Persistent LEK storage                         │
│  PKI Client: KEK derivation from certificates            │
└─────────────────────────────────────────────────────────┘
```

### 2.2 Key Hierarchy

```
┌──────────────────────────────────────────────────────────┐
│  PKI Certificate (from external CA)                      │
└──────────────────────────────────────────────────────────┘
                      │
                      │ HKDF-SHA256
                      │ ("lek-kek.v1")
                      ↓
┌──────────────────────────────────────────────────────────┐
│  KEK (Key Encryption Key)                                │
│  - Derived on startup from certificate                   │
│  - 256-bit AES key                                       │
│  - Used to encrypt/decrypt all LEKs                      │
│  - Rotated annually or on incident                       │
└──────────────────────────────────────────────────────────┘
                      │
          ┌───────────┼───────────┐
          │           │           │
          ↓           ↓           ↓
    ┌─────────┐ ┌─────────┐ ┌─────────┐
    │ LEK-08- │ │ LEK-08- │ │ LEK-08- │
    │   07    │ │   08    │ │   09    │
    │ (old)   │ │ (today) │ │ (future)│
    └─────────┘ └─────────┘ └─────────┘
        │           │           │
        │       Stores in       │
        │       RocksDB:        │
        │   lek:encrypted:      │
        │   YYYY-MM-DD          │
        │                       │
        ├─ Can decrypt (old)    │
        ├─ Can encrypt/decrypt  ├─ Will create at
        │  (current)            │  UTC midnight
        │
        └─ Revoke after max_age_days
```

### 2.3 Module Integration Matrix

| Module | Uses | Contract | Frequency |
|--------|------|----------|-----------|
| **Auth** | HKDF for session keys | Derive per-session | Per login |
| | LEK for token storage | getCurrentLEK() | Continuous |
| **Process** | LEK for state encryption | Rotate daily | Per-day |
| | HKDF for inter-process keys | Derive per-request | High frequency |
| **Storage** | LEK for at-rest encryption | Multi-key decryption | Per-read |
| | HKDF for backup keys | Unique per-backup | Per-backup |
| **Query** | LEK for result encryption | Version-aware decrypt | Per-query |
| | HKDF for index keys | Domain-specific derive | Per-index |

---

## 3. Operational Workflows

### 3.1 Startup Workflow

```
System Startup
    │
    ├─ Load PKI certificate
    │  │
    │  └─ Error: Cannot start
    │
    ├─ Derive KEK via HKDF
    │  │
    │  └─ Error: Cannot start
    │
    ├─ Initialize LEKManager
    │  │
    │  ├─ Load LEK for today from RocksDB
    │  │  │
    │  │  └─ If missing: Generate new LEK
    │  │
    │  └─ Error: Log warning, use degraded mode
    │
    ├─ Initialize HKDFCache (per thread)
    │  │
    │  └─ Create 16-shard LRU cache
    │
    ├─ Start background rotation worker
    │  │
    │  └─ Wake every check_interval seconds
    │
    └─ System Ready
        │
        └─ Emit STARTUP_COMPLETE audit event
```

### 3.2 Daily Rotation Workflow

```
Rotation Worker (runs every hour)
    │
    ├─ Check if date changed (crosses midnight UTC)
    │
    ├─ If date changed:
    │  │
    │  ├─ Generate new LEK
    │  │
    │  ├─ Encrypt with KEK
    │  │
    │  ├─ Store in RocksDB: lek:encrypted:<YYYY-MM-DD>
    │  │
    │  ├─ Update in-memory cache
    │  │
    │  ├─ Emit KEY_ROTATED audit event
    │  │  ├─ old_key_id: "lek_2026-08-07"
    │  │  ├─ new_key_id: "lek_2026-08-08"
    │  │  └─ reason: "automatic"
    │  │
    │  └─ Clear HKDF cache (force re-derivation)
    │
    ├─ Check for expired keys
    │  │
    │  └─ For keys older than max_age_days:
    │      │
    │      ├─ Revoke key
    │      │
    │      ├─ Store lek_revoked:<key_id>
    │      │
    │      ├─ Remove from in-memory LEK cache
    │      │
    │      └─ Emit KEY_REVOKED audit event
    │
    └─ Resume sleep until next check
```

### 3.3 Encryption Workflow

```
Application needs to encrypt data
    │
    ├─ Request current LEK
    │  │
    │  └─ LEKManager::getCurrentLEK()
    │      │
    │      ├─ Check cache for today's key
    │      │
    │      ├─ If missing:
    │      │  │
    │      │  ├─ Check RocksDB
    │      │  │
    │      │  ├─ If missing:
    │      │  │  │
    │      │  │  └─ Generate new LEK
    │      │  │
    │      │  └─ Decrypt LEK with KEK if stored
    │      │
    │      └─ Return key_id
    │
    ├─ Encrypt data with LEK
    │  │
    │  └─ Use FieldEncryption module
    │      │
    │      └─ AES-GCM encrypt
    │
    ├─ Store encrypted data
    │
    └─ Emit KEY_USED audit event
```

### 3.4 Decryption Workflow

```
Application needs to decrypt data
    │
    ├─ Determine original encryption key
    │  │
    │  ├─ From metadata (key_version)
    │  │
    │  └─ If unknown: try current + recent keys
    │
    ├─ Request LEK for specific date
    │  │
    │  └─ LEKManager::getLEKForDate(date_str)
    │      │
    │      ├─ Check revocation list
    │      │  │
    │      │  └─ If revoked: Return empty
    │      │
    │      ├─ Check in-memory cache
    │      │
    │      ├─ If missing: Load from RocksDB
    │      │  │
    │      │  └─ Decrypt with KEK
    │      │
    │      └─ Return key_id
    │
    ├─ Decrypt data with LEK
    │  │
    │  └─ Use FieldEncryption module
    │      │
    │      └─ AES-GCM decrypt (validates authentication tag)
    │
    ├─ Return plaintext or error
    │
    └─ Emit KEY_USED audit event
```

### 3.5 Key Rotation (Incident Response)

```
Security incident detected
    │
    ├─ Alert: LEK_COMPROMISE
    │
    ├─ Force immediate rotation
    │  │
    │  └─ LEKManager::rotate()
    │      │
    │      └─ Create new LEK immediately (not at midnight)
    │
    ├─ Revoke compromised key
    │  │
    │  └─ LEKManager::revokeKey("2026-08-08")
    │      │
    │      ├─ Update revocation list
    │      │
    │      └─ Emit KEY_REVOKED audit event
    │
    ├─ Clear HKDF cache
    │  │
    │  └─ Force re-derivation with new key material
    │
    ├─ Re-encrypt sensitive data
    │  │
    │  ├─ For data encrypted with compromised key:
    │  │  │
    │  │  └─ Decrypt with revoked key (allowed for recovery)
    │  │
    │  └─ Encrypt with new key
    │
    └─ Emit INCIDENT_RESPONSE audit event
```

---

## 4. Consumer Module Contracts

### 4.1 Auth Module

**Key Management Responsibilities:**
- Derive unique session keys per login
- Encrypt session tokens with current LEK
- Support decryption of historical tokens
- Clean up keys on session termination

**Integration Example:**
```cpp
// Session creation
class SessionManager {
public:
    std::string createSession(const std::string& user_id) {
        auto session_key = hkdf_cache_->derive_cached(
            master_key_,
            user_id + "_" + timestamp,
            "auth.session.v1",
            32
        );
        
        auto lek_id = lek_manager_->getCurrentLEK();
        auto encrypted_token = encrypt_token(user_id, session_key, lek_id);
        
        return encrypted_token;
    }
};
```

**Contracts:**
- ✓ Derive unique session key per login
- ✓ Encrypt session state with current LEK
- ✓ Support session replay with previous day's LEK
- ✓ Clean up keys on session logout
- ✓ Audit: Log KEY_USED events

### 4.2 Process Module

**Key Management Responsibilities:**
- Encrypt process state for at-rest security
- Support multi-version state decryption
- Re-encrypt on key rotation
- Track process encryption keys in audit log

**Integration Pattern:**
```cpp
// Process state encryption
class ProcessManager {
public:
    std::string serializeState(const Process& p) {
        auto lek_id = lek_manager_->getCurrentLEK();
        auto state_json = json::dumps(p);
        return field_encryption_->encryptField(
            "process_state", state_json, lek_id
        );
    }
    
    Process deserializeState(const std::string& encrypted) {
        // Try current key first
        auto lek_id = lek_manager_->getCurrentLEK();
        auto decrypted = field_encryption_->decryptField(
            encrypted, lek_id
        );
        
        // Fall back to previous key if needed
        if (decrypted.empty()) {
            auto prev_date = getPreviousDate(
                lek_manager_->getCurrentDateString()
            );
            lek_id = lek_manager_->getLEKForDate(prev_date);
            if (!lek_id.empty()) {
                decrypted = field_encryption_->decryptField(
                    encrypted, lek_id
                );
            }
        }
        
        return json::parse(decrypted);
    }
};
```

**Contracts:**
- ✓ Encrypt all persisted process state
- ✓ Support decryption with LEK from creation date
- ✓ Support multi-key decryption (current + recent)
- ✓ Re-encrypt after key rotation (optional, for compliance)
- ✓ Audit: Log process state encryption events

### 4.3 Storage Module

**Key Management Responsibilities:**
- Encrypt data at rest with current LEK
- Track encryption key version with stored data
- Support decryption with version-specific LEK
- Handle key expiration for old records

**Integration Pattern:**
```cpp
// At-rest encryption
class StorageEngine {
public:
    void putRecord(const std::string& key, const Record& record) {
        auto lek_id = lek_manager_->getCurrentLEK();
        
        // Store with metadata
        StorageRecord storage{
            .data = field_encryption_->encryptField(
                "record", record.serialize(), lek_id
            ),
            .key_version = extract_version(lek_id),
            .created_at = now()
        };
        
        db_->put(key, storage.serialize());
    }
    
    Record getRecord(const std::string& key) {
        auto storage = db_->get(key);
        auto lek_id = lek_manager_->getLEKForDate(
            extract_date(storage.key_version)
        );
        
        if (lek_id.empty() && is_revoked(storage.key_version)) {
            throw std::runtime_error("Record encrypted with revoked key");
        }
        
        auto decrypted = field_encryption_->decryptField(
            storage.data, lek_id
        );
        
        return Record::deserialize(decrypted);
    }
};
```

**Contracts:**
- ✓ Store key version with encrypted data
- ✓ Support decryption with historical keys
- ✓ Detect and reject revoked key usage
- ✓ Support batch re-encryption for key rotation
- ✓ Audit: Log all data encryption/decryption events

### 4.4 Query Module

**Key Management Responsibilities:**
- Encrypt query results with current LEK
- Decrypt results for consumer
- Track result encryption keys
- Support encrypted index operations

**Integration Pattern:**
```cpp
// Query result encryption
class QueryExecutor {
public:
    QueryResult executeQuery(const QueryRequest& req) {
        auto results = query_engine_->execute(req);
        
        auto lek_id = lek_manager_->getCurrentLEK();
        QueryResult encrypted_results;
        
        for (const auto& row : results) {
            encrypted_results.rows.push_back({
                .encrypted_data = field_encryption_->encryptField(
                    "row", row.serialize(), lek_id
                ),
                .key_version = extract_version(lek_id)
            });
        }
        
        return encrypted_results;
    }
};
```

**Contracts:**
- ✓ Encrypt result sets before transmission
- ✓ Include key version in result metadata
- ✓ Support client-side decryption with version info
- ✓ Audit: Log query execution and encryption events

---

## 5. Audit Trail and Compliance

### 5.1 Audit Events

**Mandatory Events:**

```
KEY_GENERATED:
  Trigger: New LEK created
  Fields: key_id, algorithm, size, reason
  Frequency: Once per day (daily rotation)
  
KEY_USED:
  Trigger: Key used for encrypt/decrypt
  Fields: key_id, operation, data_size, module
  Frequency: Per operation
  
KEY_ROTATED:
  Trigger: Automatic or manual rotation
  Fields: old_key_id, new_key_id, reason, auto/manual
  Frequency: Once per day (automatic)
  
KEY_REVOKED:
  Trigger: Key revocation (incident or policy)
  Fields: key_id, reason, incident_id
  Frequency: On demand (incident response)
  
KEY_EXPIRED:
  Trigger: Key exceeds max_age_days
  Fields: key_id, age_days, action_taken
  Frequency: Once per expiration
```

### 5.2 Compliance Integration

**Requirements:**
- All key lifecycle events must be auditable
- Audit trail must be immutable and append-only
- Support retention policies (e.g., 7 years)
- Support evidence export for compliance audits
- Integration with AuditLogger framework

**Configuration:**
```cpp
// In LeaderBoard startup
auto lek_manager = std::make_shared<LEKManager>(...);
lek_manager->setAuditLogger(audit_logger);
lek_manager->startAutoRotation(
    std::chrono::seconds(3600),
    30  // max_age_days
);
```

---

## 6. Performance Characteristics

### 6.1 Latency Profile

| Operation | Typical | P99 | P99.9 |
|-----------|---------|-----|-------|
| HKDF cache hit | <100µs | <500µs | <1ms |
| HKDF cache miss | 1-2ms | 5ms | 10ms |
| LEK cache lookup | <50µs | <200µs | <500µs |
| Key rotation | 10-50ms | 100ms | 500ms |

### 6.2 Memory Usage

| Component | Memory | Notes |
|-----------|--------|-------|
| HKDF cache | ~1-10MB | 16 shards × ~64 entries |
| LEK cache | ~1KB | 30 entries × 32 bytes max |
| Revocation list | <1KB | In-memory set |
| Audit buffer | ~1MB | Recent events (configurable) |

### 6.3 Concurrency

| Metric | Value | Notes |
|--------|-------|-------|
| HKDF cache shards | 16 | Reduces lock contention |
| Rotation worker threads | 1 | Per LEKManager instance |
| Max concurrent readers | Unlimited | Sharded mutex design |
| Max concurrent writers | 1 | Per-shard serialization |

---

## 7. Failure Modes and Recovery

### 7.1 Key Derivation Failure

**Scenario:** OpenSSL HKDF returns error

**Impact:**
- Cannot derive session/encryption keys
- Dependent operations fail
- No automatic fallback

**Recovery:**
```cpp
try {
    auto key = cache.derive_cached(...);
} catch (const std::runtime_error& e) {
    LOG(ERROR) << "Key derivation failed: " << e.what();
    // Alert: requires admin intervention
    // Option 1: Retry with backoff
    // Option 2: Use degraded mode (no encryption)
    // Option 3: Fail request to client
}
```

### 7.2 LEK Rotation Failure

**Scenario:** Cannot generate or store new LEK at midnight

**Impact:**
- Old LEK remains in use
- Encryption continues with day-old key
- Risk window extends beyond normal daily rotation

**Recovery:**
```cpp
// Rotation worker detects failure
LOG(ERROR) << "LEK rotation failed at midnight";
emit_alert("LEK_ROTATION_FAILURE");

// Retry on next check_interval
// After 3 failures: alert escalation
// Manual rotation available: rotate() API
```

### 7.3 Key Revocation Failure

**Scenario:** Cannot persist revocation to RocksDB

**Impact:**
- Revocation exists in-memory only
- Revocation lost on process restart
- Key could be reused after restart

**Recovery:**
```cpp
try {
    lek_manager->revokeKey("2026-08-08");
} catch (const std::runtime_error& e) {
    LOG(ERROR) << "Revocation failed: " << e.what();
    // Keep in-memory revocation list
    // Retry persistence
    // If persistent failure: requires admin intervention
}
```

### 7.4 Certificate Expiration

**Scenario:** PKI certificate expires; cannot derive KEK

**Impact:**
- LEK decryption fails
- All encrypted data inaccessible
- System cannot start

**Recovery:**
```
Required action: Certificate renewal
1. Obtain new certificate from CA
2. Restart ThemisDB with new certificate
3. New KEK derived from new certificate
4. Old LEKs must be re-encrypted with new KEK
```

---

## 8. Testing and Validation

### 8.1 Unit Tests

**HKDF Tests:**
- [x] Determinism: Same inputs → same output
- [x] Domain separation: Different info → different output
- [x] Output length: Correct byte count
- [x] Error handling: Invalid parameters throw

**LEK Tests:**
- [x] Rotation: Triggers at date boundary
- [x] Revocation: Prevents decryption
- [x] Expiration: Detected correctly
- [x] Multi-key: Fallback to previous key works

**Cache Tests:**
- [x] Hit rate: >80% on typical workload
- [x] Eviction: LRU replacement works
- [x] TTL: Keys evicted after TTL
- [x] Thread safety: Concurrent access safe

### 8.2 Integration Tests

- [x] Multi-module key sharing
- [x] Rotation across all consumers
- [x] Audit logging end-to-end
- [x] Performance under load
- [x] Memory safety (valgrind, ASan)

### 8.3 Security Tests

- [x] No key material in logs
- [x] No key material left on heap
- [x] Revocation enforced immediately
- [x] KEK derivation deterministic
- [x] Timing attack resistance

---

## 9. Operational Runbook

### 9.1 Daily Health Check

```bash
# Check rotation status
themis-cli key-status

# Expected output:
# Current LEK: lek_2026-08-08
# Previous LEKs: 1 (2026-08-07)
# Revoked LEKs: 0
# Cache hit rate: 87%

# Check audit events
themis-cli audit-events --filter="KEY_ROTATED" --days=1

# Expected: 1 KEY_ROTATED event at midnight
```

### 9.2 Incident Response

**Key Compromise:**
```bash
# 1. Force immediate rotation
themis-cli lek rotate

# 2. Revoke compromised key
themis-cli lek revoke "2026-08-08"

# 3. Verify revocation
themis-cli lek status "2026-08-08"
# Expected: REVOKED

# 4. Monitor audit events
themis-cli audit-events --filter="KEY_REVOKED"
```

**Certificate Expiration:**
```bash
# 1. Obtain new certificate
# ... CA renewal process ...

# 2. Restart with new certificate
systemctl restart themis-db

# 3. Verify new KEK derived
themis-cli key-info kek
# Verify derivation timestamp is recent
```

### 9.3 Performance Tuning

**Low Cache Hit Rate:**
```bash
# Check current stats
themis-cli cache-stats

# If hit rate < 75%:
themis-cli cache-config --capacity 512 --ttl 600

# Monitor improvement
watch -n 60 'themis-cli cache-stats'
```

---

## 10. References

### Standards and RFCs
- **RFC 5869**: HKDF - HMAC-based Extract-and-Expand Key Derivation Function
- **NIST SP 800-38D**: Recommendation for Block Cipher Modes of Operation (GCM)
- **NIST SP 800-56A**: Recommendation for Pair-Wise Key Establishment Schemes
- **NIST SP 800-57**: Recommendation for Key Management

### Internal Documentation
- `SECURITY.md`: Security policy and incident response
- `CRYPTO_INTEGRATION_GUIDE.md` (L2): Developer integration patterns
- `KEY_LIFECYCLE.md` (L3): Detailed lifecycle contracts
- `include/utils/hkdf_cache.h` (L0): API contract
- `include/utils/lek_manager.h` (L0): API contract

### Tools and Libraries
- OpenSSL 3.0+: EVP_KDF, AES-GCM
- RocksDB: Persistent storage
- spdlog: Structured logging
- AuditLogger: Compliance event logging

---

**Canonical Sources:**
- `include/utils/` - L0 API contracts
- `src/utils/` - L1 implementations
- `docs/utils/KEY_LIFECYCLE.md` - L3 detailed contracts
- `docs/utils/CRYPTO_INTEGRATION_GUIDE.md` - L2 usage patterns

**Last Security Review:** 2026-08-08  
**Next Review Due:** 2027-08-08  
**Last Update:** 2026-08-08
