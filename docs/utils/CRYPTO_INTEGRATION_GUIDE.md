# Crypto and Key-Management Integration Guide

**Level:** L2 - Developer Integration Patterns  
**Last Updated:** 2026-08-08  
**Source:** Level 1 implementation (src/utils/hkdf_helper.cpp, src/utils/lek_manager.cpp)  
**SOT Domain:** crypto-key-management  
**Audience:** Module developers, integration engineers, security reviewers  

---

## 1. Quick Start

### 1.1 HKDF Key Derivation

```cpp
#include "utils/hkdf_cache.h"
#include "utils/hkdf_helper.h"
#include <openssl/crypto.h>

// Method 1: Using thread-local cached HKDF (recommended for hot paths)
{
    auto& cache = themis::utils::HKDFCache::threadLocal();
    
    std::vector<uint8_t> master_key = /* load from secure storage */;
    std::vector<uint8_t> salt = /* random or derived */;
    std::string context = "auth.session.v1";
    
    auto session_key = cache.derive_cached(master_key, salt, context, 32);
    // Use session_key...
    
    // Cleanup (important for sensitive keys)
    OPENSSL_cleanse(session_key.data(), session_key.size());
}

// Method 2: Direct derivation (one-time key generation)
{
    auto key = themis::utils::HKDFHelper::derive(
        master_key,
        salt,
        "encryption.data.v1",
        32  // 256-bit AES key
    );
    // Use key...
    OPENSSL_cleanse(key.data(), key.size());
}

// Method 3: String-based (for passphrases)
{
    auto key = themis::utils::HKDFHelper::deriveFromString(
        "my-passphrase",
        "key.for.something.specific",
        32
    );
}
```

### 1.2 LEK Manager Usage

```cpp
#include "utils/lek_manager.h"

// Initialization (typically in startup)
{
    auto lek_manager = std::make_shared<themis::utils::LEKManager>(
        rocksdb_wrapper,      // RocksDB storage
        pki_client,           // PKI for key derivation
        key_provider          // Encryption provider
    );
    
    // Start automatic daily rotation
    lek_manager->startAutoRotation(
        std::chrono::seconds(3600),  // Check every hour
        30                           // Revoke keys older than 30 days
    );
}

// Encryption: Get current LEK and encrypt
{
    std::string lek_id = lek_manager->getCurrentLEK();
    
    // Use with FieldEncryption
    field_encryption->encryptField(
        "user_email",
        "alice@example.com",
        lek_id
    );
}

// Decryption: Get LEK for historical date
{
    std::string old_lek = lek_manager->getLEKForDate("2026-08-01");
    if (!old_lek.empty()) {
        auto decrypted = field_encryption->decryptField(
            encrypted_data,
            old_lek
        );
    }
}
```

### 1.3 Key Rotation Patterns

```cpp
// Automatic daily rotation (recommended)
lek_manager->startAutoRotation(
    std::chrono::seconds(3600),  // Wake every hour
    30                           // Force rotation after 30 days
);

// Manual rotation (security incident)
lek_manager->rotate();  // Creates new LEK for today

// Revocation (compromise response)
bool revoked = lek_manager->revokeKey("2026-08-05");
if (revoked) {
    LOG(WARNING) << "LEK for 2026-08-05 revoked due to incident";
}

// Check revocation status
if (lek_manager->isRevoked("2026-08-05")) {
    LOG(WARNING) << "Key is revoked; cannot use for encryption";
}
```

---

## 2. API Reference

### 2.1 HKDF Cache API

#### `HKDFCache::derive_cached()`
```cpp
std::vector<uint8_t> derive_cached(
    const std::vector<uint8_t>& ikm,      // Input key material
    const std::vector<uint8_t>& salt,     // Salt (can be empty)
    const std::string& info,               // Context string
    size_t output_length                   // Desired key length
);
```

**Performance:**
- Cache hit: O(1) lookup + return (microseconds)
- Cache miss: HKDF derivation (milliseconds)
- Hit rate: 90%+ in typical workloads

**Thread-Safety:** Yes, all calls thread-safe

**Example:**
```cpp
auto& cache = HKDFCache::threadLocal();
auto key = cache.derive_cached(
    master,
    salt,
    "purpose.v1",
    32
);
```

#### `HKDFCache::clear()`
```cpp
void clear();
```

Securely wipes all cached keys. Use after key rotation.

#### `HKDFCache::setCapacity()`
```cpp
void setCapacity(size_t cap);  // per-shard capacity
```

Tuning guidance:
- Default: 64 entries per shard (~1000 total)
- High-throughput services: 256-512 per shard
- Memory-constrained: 32-64 per shard

#### `HKDFCache::stats()`
```cpp
HKDFCacheStats stats() const;
```

Returns: `{hits, misses, evictions}`

Useful for tuning:
```cpp
auto stats = cache.stats();
double hit_rate = 100.0 * stats.hits / (stats.hits + stats.misses);
if (hit_rate < 0.8) {
    cache.setCapacity(256);  // Increase cache
}
```

### 2.2 LEK Manager API

#### `LEKManager::getCurrentLEK()`
```cpp
std::string getCurrentLEK();
```

Returns today's LEK key_id (creates if needed).

**Guarantees:**
- Always returns valid key_id
- Idempotent: same key_id on repeated calls
- Creates LEK automatically if missing

#### `LEKManager::getLEKForDate()`
```cpp
std::string getLEKForDate(const std::string& date_str);
```

Returns LEK for specific date or empty string if not found.

**Format:** "YYYY-MM-DD"

#### `LEKManager::rotate()`
```cpp
void rotate();
```

Force immediate rotation (creates new LEK for today).

**Use cases:**
- Security incident: force key replacement
- Compliance: manual key change
- Testing: verify rotation behavior

#### `LEKManager::revokeKey()`
```cpp
bool revokeKey(const std::string& date_str);
```

Revokes a key permanently. Prevents future use.

**Returns:** true if key was present and revoked, false if not found.

#### `LEKManager::isRevoked()`
```cpp
bool isRevoked(const std::string& date_str) const;
```

Checks revocation status.

#### `LEKManager::isExpired()`
```cpp
static bool isExpired(
    const std::string& date_str,
    int max_age_days = 30
);
```

Checks if key is older than max_age_days.

---

## 3. Common Integration Patterns

### 3.1 Single Key Derivation (Low-Frequency)

```cpp
// Derive key for one-time use (e.g., password reset token)
auto key = themis::utils::HKDFHelper::derive(
    master_key,           // From secure storage
    random_salt,          // Per-request randomness
    "password.reset.v1",  // Context
    32                    // AES-256
);

// Use key immediately
auto encrypted_token = aes_encrypt(token_data, key);

// Cleanup
OPENSSL_cleanse(key.data(), key.size());
```

### 3.2 Bulk Key Derivation (High-Frequency)

```cpp
// Cache-based pattern for multiple requests
class SessionManager {
public:
    std::vector<uint8_t> getSessionKey(const std::string& session_id) {
        // ThreadLocal cache provides ~90% hit rate
        auto& cache = HKDFCache::threadLocal();
        
        return cache.derive_cached(
            master_key_,
            session_id,
            "session.encryption",
            32
        );
    }
    
private:
    std::vector<uint8_t> master_key_;
};
```

### 3.3 Key Rotation During Request

```cpp
// Scenario: Key rotated mid-request
std::vector<uint8_t> old_key = /* from previous cache fetch */;
std::vector<uint8_t> new_key = cache.derive_cached(
    new_master_key,
    salt,
    "encryption.data.v2",  // Version bump
    32
);

// Decrypt with old key
auto decrypted = decrypt(encrypted_data, old_key);

// Re-encrypt with new key
auto re_encrypted = encrypt(decrypted, new_key);

// Cleanup
OPENSSL_cleanse(old_key.data(), old_key.size());
OPENSSL_cleanse(new_key.data(), new_key.size());
```

### 3.4 Fallback to Previous Key Version

```cpp
// Try current key first, then fall back to previous version
auto lek_id = lek_manager_->getCurrentLEK();
auto decrypted = field_encryption_->decryptField(data, lek_id);

if (!decrypted.empty()) {
    return decrypted;
}

// Fallback: try key from yesterday
auto yesterday_date = getPreviousDate(getCurrentDateString());
auto old_lek_id = lek_manager_->getLEKForDate(yesterday_date);

if (!old_lek_id.empty()) {
    return field_encryption_->decryptField(data, old_lek_id);
}

LOG(ERROR) << "Failed to decrypt with any available key";
return "";
```

### 3.5 Error Handling

```cpp
try {
    auto key = cache.derive_cached(ikm, salt, info, 32);
    // Use key...
} catch (const std::invalid_argument& e) {
    LOG(ERROR) << "Invalid derivation parameters: " << e.what();
    // Handle: output_length too large
} catch (const std::runtime_error& e) {
    LOG(ERROR) << "OpenSSL HKDF failed: " << e.what();
    // Handle: system error, retry with backoff
} catch (...) {
    LOG(ERROR) << "Unexpected error during key derivation";
    // Handle: graceful degradation
}

try {
    lek_manager_->rotate();
} catch (const std::runtime_error& e) {
    LOG(ERROR) << "Key rotation failed: " << e.what();
    // Alert: requires admin intervention
}
```

---

## 4. Consumer Checklist

Use this checklist when integrating crypto helpers:

- [ ] **Key Size Validation**
  - [ ] Derive keys of correct size (e.g., 32 for AES-256)
  - [ ] Verify output_length matches algorithm requirements
  - [ ] Document key size expectations in consuming module

- [ ] **Rotation Interval Configuration**
  - [ ] Configure LEK rotation check_interval (recommended: 3600s = 1 hour)
  - [ ] Set max_age_days appropriately (recommended: 30 days)
  - [ ] Test rotation trigger behavior
  - [ ] Verify keys rotate at midnight UTC

- [ ] **Error Handling**
  - [ ] Catch std::invalid_argument for invalid parameters
  - [ ] Catch std::runtime_error for OpenSSL failures
  - [ ] Handle empty returns from getLEKForDate()
  - [ ] Log all errors with context
  - [ ] Implement graceful degradation

- [ ] **Audit Logging**
  - [ ] Attach AuditLogger to LEK manager
  - [ ] Verify KEY_ROTATED events are logged
  - [ ] Verify KEY_REVOKED events are logged
  - [ ] Monitor audit trail for anomalies

- [ ] **Timeouts**
  - [ ] Set request timeout for key derivation
  - [ ] Configure timeout for rotation operations
  - [ ] Test timeout behavior
  - [ ] Handle timeout errors gracefully

- [ ] **Memory Cleanup**
  - [ ] Use OPENSSL_cleanse() or volatile_free() for sensitive data
  - [ ] Cleanup keys after use in hot paths
  - [ ] Verify no key material left on stack/heap
  - [ ] Test with valgrind/sanitizers

- [ ] **Thread Safety**
  - [ ] Use threadLocal() for multi-threaded services
  - [ ] Verify no race conditions in key access
  - [ ] Test high-concurrency scenarios
  - [ ] Monitor lock contention

- [ ] **Performance**
  - [ ] Monitor cache hit rate (target: >80%)
  - [ ] Measure key derivation latency (target: <1ms cache hit)
  - [ ] Profile rotation worker overhead
  - [ ] Implement cache tuning based on stats()

---

## 5. Troubleshooting

### Issue: Low Cache Hit Rate

**Symptoms:** Cache hit rate < 50%

**Diagnosis:**
```cpp
auto stats = cache.stats();
LOG(INFO) << "Hit rate: " 
    << (100.0 * stats.hits / (stats.hits + stats.misses)) << "%";
LOG(INFO) << "Evictions: " << stats.evictions;
```

**Solutions:**
1. Increase capacity: `cache.setCapacity(256)`
2. Increase TTL: `HKDFCacheConfig{.ttl = std::chrono::seconds(600)}`
3. Use stable salts (avoid per-request randomness)
4. Review key derivation pattern (may indicate design issue)

### Issue: Key Rotation Not Happening

**Symptoms:** getCurrentLEK() returns same key_id after midnight

**Diagnosis:**
```cpp
bool running = lek_manager_->isAutoRotationRunning();
auto date = LEKManager::getCurrentDateString();
auto lek_id = lek_manager_->getCurrentLEK();
```

**Solutions:**
1. Verify startAutoRotation() was called
2. Check background worker thread is running
3. Verify system clock is correct (crosses midnight)
4. Check for exceptions in rotation worker logs

### Issue: "Invalid Derivation Parameters"

**Symptoms:** std::invalid_argument exception

**Diagnosis:**
```cpp
// output_length exceeds RFC 5869 limit
size_t max_length = 255 * 32;  // 8160 for SHA-256
if (output_length > max_length) {
    LOG(ERROR) << "Output length too large: " << output_length;
}
```

**Solutions:**
1. Reduce output_length to ≤ 8160 bytes
2. Split into multiple derivations if needed
3. Verify algorithm requirements (AES-256 = 32 bytes)

### Issue: OpenSSL HKDF Failed

**Symptoms:** std::runtime_error "EVP_KDF_fetch failed"

**Diagnosis:**
- OpenSSL 3.0+: HKDF provider not loaded
- Missing OpenSSL headers during build
- OpenSSL library mismatch at runtime

**Solutions:**
1. Verify OpenSSL version: `openssl version`
2. Rebuild with `-DOPENSSL_LIB_PATH=/path/to/openssl`
3. Check CMakeLists.txt linking
4. Review OpenSSL provider availability

---

## 6. Performance Tuning

### Cache Configuration Guidance

| Use Case | max_entries | ttl | Rationale |
|----------|-------------|-----|-----------|
| API Gateway | 1000 | 300s | High hit rate, frequent reuse |
| Session Handler | 500 | 600s | Medium reuse, user sessions |
| Encryption Pipeline | 256 | 120s | Lower reuse, varies by content |
| Testing | 64 | 60s | Low reuse, quick feedback |

### Monitoring Key Metrics

```cpp
// Daily health check
void monitor_crypto_health() {
    auto& cache = HKDFCache::threadLocal();
    auto stats = cache.stats();
    
    double hit_rate = 100.0 * stats.hits / (stats.hits + stats.misses);
    
    LOG(INFO) << "HKDF Cache Health:";
    LOG(INFO) << "  Hit Rate: " << hit_rate << "%";
    LOG(INFO) << "  Evictions: " << stats.evictions;
    
    // Alert if hit rate degraded
    if (hit_rate < 0.75) {
        cache.setCapacity(512);  // Increase cache
        LOG(WARNING) << "Cache hit rate low; increased capacity";
    }
    
    // Check LEK rotation
    auto lek_id = lek_manager_->getCurrentLEK();
    LOG(INFO) << "Current LEK: " << lek_id;
    
    auto revoked = lek_manager_->getRevokedKeys();
    LOG(INFO) << "Revoked keys: " << revoked.size();
}
```

---

## 7. References

- **RFC 5869**: HKDF - HMAC-based Extract-and-Expand Key Derivation Function
  - https://tools.ietf.org/html/rfc5869
  
- **NIST SP 800-38D**: Recommendation for Block Cipher Modes of Operation: Galois/Counter Mode (GCM)
  - https://nvlpubs.nist.gov/nistpubs/Legacy/SP/nistspecialpublication800-38d.pdf

- **OpenSSL EVP_KDF**: Key Derivation Functions
  - https://www.openssl.org/docs/man3.0/man3/EVP_KDF.html

- **ThemisDB Security Policy**: SECURITY.md
  - See root repo for security incident response procedures

---

## 8. Common Questions

**Q: Should I use HKDFCache or HKDFHelper directly?**  
A: Use HKDFCache.threadLocal() for hot paths (>100 req/s). Use HKDFHelper directly for one-time derivations.

**Q: What's the difference between salt and info?**  
A: Salt randomizes the extract phase (prevents rainbow tables). Info provides domain separation (different purposes get different keys).

**Q: Can I use the same master key with different salts?**  
A: Yes! Different salts → different derived keys. Useful for deriving multiple keys from single master.

**Q: What happens if I don't call OPENSSL_cleanse()?**  
A: Key material remains in memory, potentially recoverable via memory dumps or cold boot attacks. Always cleanup sensitive keys.

**Q: How often should I rotate keys?**  
A: Daily rotation is standard (LEK), yearly for master keys. Adjust based on compliance requirements.

**Q: Can I disable automatic rotation?**  
A: Yes, just don't call startAutoRotation(). Manually call rotate() as needed.

---

**Canonical Source:** `src/utils/hkdf_helper.cpp`, `src/utils/lek_manager.cpp`  
**Related Issues:** [Link to GitHub issues]  
**Last Security Review:** 2026-08-08
