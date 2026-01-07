# Phase 1 Implementation Plan: At-Rest Encryption for Vector Embeddings

**Date:** 2025-12-15  
**Status:** Implementation Ready  
**Timeline:** 1-2 weeks  
**Priority:** P0 (Critical Security Gap)

---

## 1. Executive Summary

Implement at-rest encryption for vector embeddings stored in RocksDB using the existing `EncryptedField<T>` infrastructure. This closes the critical security gap identified in the BSI C5 compliance analysis where embeddings are currently stored in plaintext.

**Key Changes:**
- Extend `EncryptedField<T>` to support `std::vector<float>`
- Modify `VectorIndexManager` to encrypt vectors before storage
- Add batch decryption when loading HNSW index into memory
- Maintain backward compatibility with existing unencrypted data

---

## 2. Architecture Overview

### 2.1 Current State (Plaintext)

```
Client → HTTP API → VectorIndexManager
                         ↓
                    BaseEntity (embedding: std::vector<float>)
                         ↓
                    RocksDB (PLAINTEXT) ❌
                         ↓
                    HNSW Index (Memory, PLAINTEXT)
```

### 2.2 Target State (Encrypted)

```
Client → HTTP API → VectorIndexManager
                         ↓
                    EncryptedField<std::vector<float>>
                         ↓
                    BaseEntity (embedding_encrypted: Base64)
                         ↓
                    RocksDB (AES-256-GCM) ✅
                         ↓
                    Batch Decrypt → HNSW Index (Memory, PLAINTEXT)
```

---

## 3. Detailed Implementation

### 3.1 Extend EncryptedField Template

**File:** `include/security/encryption.h`

Add template specialization for `std::vector<float>`:

```cpp
// Template specialization for vector<float>
template<>
class EncryptedField<std::vector<float>> {
public:
    // ... (same interface as other types)
    
    void encrypt(const std::vector<float>& value, const std::string& key_id);
    std::vector<float> decrypt() const;
    
    std::string toBase64() const;
    static EncryptedField<std::vector<float>> fromBase64(const std::string& b64);
    
private:
    // Serialize float vector to bytes
    static std::string serialize(const std::vector<float>& vec);
    
    // Deserialize bytes to float vector
    static std::vector<float> deserialize(const std::string& str);
};
```

**Implementation Details:**
- Serialization: `std::vector<float>` → binary (little-endian floats)
- Format: `[size:uint32_t][float1][float2]...[floatN]`
- No compression in Phase 1 (keep it simple)
- Thread-safe serialization

**File:** `src/security/encrypted_field.cpp`

```cpp
// Serialize vector<float> to binary
template<>
std::string EncryptedField<std::vector<float>>::serialize(
    const std::vector<float>& vec) {
    
    std::string result;
    uint32_t size = static_cast<uint32_t>(vec.size());
    
    // Append size (4 bytes, little-endian)
    result.append(reinterpret_cast<const char*>(&size), sizeof(size));
    
    // Append float data
    result.append(reinterpret_cast<const char*>(vec.data()), 
                  vec.size() * sizeof(float));
    
    return result;
}

// Deserialize binary to vector<float>
template<>
std::vector<float> EncryptedField<std::vector<float>>::deserialize(
    const std::string& str) {
    
    if (str.size() < sizeof(uint32_t)) {
        throw DecryptionException("Invalid vector serialization: too short");
    }
    
    // Read size
    uint32_t size;
    std::memcpy(&size, str.data(), sizeof(size));
    
    // Validate size
    size_t expected_bytes = sizeof(uint32_t) + size * sizeof(float);
    if (str.size() != expected_bytes) {
        throw DecryptionException(
            "Invalid vector serialization: size mismatch");
    }
    
    // Read floats
    std::vector<float> result(size);
    std::memcpy(result.data(), 
                str.data() + sizeof(uint32_t), 
                size * sizeof(float));
    
    return result;
}

// Encrypt method
template<>
void EncryptedField<std::vector<float>>::encrypt(
    const std::vector<float>& value, 
    const std::string& key_id) {
    
    if (!field_encryption_) {
        throw EncryptionException(
            "FieldEncryption not initialized. Call setFieldEncryption().");
    }
    
    std::string serialized = serialize(value);
    blob_ = field_encryption_->encrypt(serialized, key_id);
}

// Decrypt method
template<>
std::vector<float> EncryptedField<std::vector<float>>::decrypt() const {
    if (!field_encryption_) {
        throw DecryptionException(
            "FieldEncryption not initialized. Call setFieldEncryption().");
    }
    
    std::string serialized = field_encryption_->decryptToString(blob_);
    return deserialize(serialized);
}
```

---

### 3.2 Modify VectorIndexManager

**File:** `include/index/vector_index.h`

Add encryption configuration:

```cpp
class VectorIndexManager {
public:
    struct Config {
        // ... existing fields ...
        
        // Encryption configuration
        bool encrypt_vectors = false;  // Feature flag
        std::string vector_key_id = "vector_embeddings";  // Key ID for DEK
        std::shared_ptr<FieldEncryption> field_encryption = nullptr;
    };
    
    // ... existing methods ...
    
private:
    // Encryption helpers
    std::string encryptVector(const std::vector<float>& vec) const;
    std::vector<float> decryptVector(const std::string& encrypted_b64) const;
    
    // Batch operations (parallelized with TBB)
    std::vector<std::vector<float>> decryptVectorBatch(
        const std::vector<std::string>& encrypted_vectors) const;
    
    // Configuration
    bool encrypt_vectors_ = false;
    std::string vector_key_id_ = "vector_embeddings";
    std::shared_ptr<FieldEncryption> field_encryption_;
};
```

**File:** `src/index/vector_index.cpp`

Modify `addEntity` to encrypt vectors:

```cpp
VectorIndexManager::Status VectorIndexManager::addEntity(
    const BaseEntity& e, 
    std::string_view vectorField) {
    
    // Extract vector
    auto vecOpt = e.extractVector(vectorField);
    if (!vecOpt.has_value()) {
        return Status::Error("Entity missing vector field: " + 
                            std::string(vectorField));
    }
    std::vector<float> vec = std::move(*vecOpt);
    
    // Validate dimension
    if (vec.size() != static_cast<size_t>(dim_)) {
        return Status::Error("Vector dimension mismatch");
    }
    
    // Create storage entity
    BaseEntity storage_entity(e.getPrimaryKey());
    
    // PHASE 1: Encrypt vector before storage
    if (encrypt_vectors_ && field_encryption_) {
        EncryptedField<std::vector<float>> enc_vec;
        enc_vec.encrypt(vec, vector_key_id_);
        
        // Store as base64-encoded encrypted blob
        storage_entity.setField("embedding_encrypted", enc_vec.toBase64());
        
        // Mark as encrypted (for migration support)
        storage_entity.setField("_encrypted", int64_t(1));
    } else {
        // Fallback: store plaintext (backward compatibility)
        storage_entity.setField(std::string(vectorField), vec);
    }
    
    // Copy metadata (NEVER encrypt metadata in Phase 1)
    // Metadata encryption is handled separately by HTTP layer
    for (const auto& [field, value] : e.getAllFields()) {
        if (field != std::string(vectorField)) {
            storage_entity.setField(field, value);
        }
    }
    
    // Write to RocksDB
    std::string key = objectName_ + ":" + e.getPrimaryKey();
    auto blob = storage_entity.serialize();
    db_.put(key, blob);
    
    // Update in-memory cache + HNSW index (uses plaintext vec)
    {
        std::unique_lock lock(mutex_);
        pk_to_vec_[e.getPrimaryKey()] = vec;
        
        #ifdef THEMIS_HNSW_ENABLED
        if (useHnsw_ && space_) {
            size_t idx = pk_to_idx_[e.getPrimaryKey()] = next_idx_++;
            idx_to_pk_[idx] = e.getPrimaryKey();
            hnsw_->addPoint(vec.data(), idx);
        }
        #endif
    }
    
    return Status::OK();
}
```

Add batch decryption for index rebuild:

```cpp
VectorIndexManager::Status VectorIndexManager::rebuildFromStorage() {
    THEMIS_INFO("VectorIndexManager::rebuildFromStorage - Started for '{}'", 
                objectName_);
    
    std::unique_lock lock(mutex_);
    pk_to_vec_.clear();
    pk_to_idx_.clear();
    idx_to_pk_.clear();
    next_idx_ = 0;
    
    // Scan RocksDB for all vectors
    std::string prefix = objectName_ + ":";
    std::vector<std::pair<std::string, BaseEntity>> entities;
    
    db_.scan(prefix, [&](const std::string& key, 
                         const std::vector<uint8_t>& value) {
        std::string pk = key.substr(prefix.size());
        BaseEntity e = BaseEntity::deserialize(pk, value);
        entities.emplace_back(std::move(pk), std::move(e));
        return true;  // Continue iteration
    });
    
    THEMIS_INFO("VectorIndexManager::rebuildFromStorage - Found {} entities", 
                entities.size());
    
    // Phase 1: Decrypt vectors (parallelized)
    std::vector<std::vector<float>> decrypted_vectors(entities.size());
    
    #pragma omp parallel for schedule(dynamic)
    for (size_t i = 0; i < entities.size(); ++i) {
        const auto& [pk, entity] = entities[i];
        
        // Check if entity is encrypted
        auto enc_flag = entity.getFieldAsInt("_encrypted");
        bool is_encrypted = enc_flag.has_value() && *enc_flag == 1;
        
        if (is_encrypted && field_encryption_) {
            // Decrypt vector
            auto enc_b64 = entity.getFieldAsString("embedding_encrypted");
            if (!enc_b64.has_value()) {
                THEMIS_WARN("Entity {} marked encrypted but missing field", pk);
                continue;
            }
            
            try {
                auto enc_field = EncryptedField<std::vector<float>>::fromBase64(
                    *enc_b64);
                decrypted_vectors[i] = enc_field.decrypt();
            } catch (const std::exception& ex) {
                THEMIS_ERROR("Failed to decrypt vector for {}: {}", 
                            pk, ex.what());
            }
        } else {
            // Plaintext vector (backward compatibility)
            auto vec_opt = entity.extractVector("embedding");
            if (vec_opt.has_value()) {
                decrypted_vectors[i] = std::move(*vec_opt);
            }
        }
    }
    
    // Phase 2: Build HNSW index (plaintext, in-memory)
    #ifdef THEMIS_HNSW_ENABLED
    if (useHnsw_) {
        initHnsw();  // Creates new HNSW index
        
        for (size_t i = 0; i < entities.size(); ++i) {
            if (decrypted_vectors[i].empty()) continue;
            
            const auto& pk = entities[i].first;
            const auto& vec = decrypted_vectors[i];
            
            pk_to_vec_[pk] = vec;
            size_t idx = pk_to_idx_[pk] = next_idx_++;
            idx_to_pk_[idx] = pk;
            hnsw_->addPoint(vec.data(), idx);
        }
    }
    #endif
    
    THEMIS_INFO("VectorIndexManager::rebuildFromStorage - Complete. "
                "{} vectors indexed", pk_to_vec_.size());
    
    return Status::OK();
}
```

---

### 3.3 Backward Compatibility & Migration

**Strategy:**
1. **Feature Flag:** `encrypt_vectors` (default: false)
2. **Dual Read:** Support both encrypted and plaintext
3. **Lazy Write:** New vectors are encrypted, old remain until updated

**Migration Path:**

```cpp
// Migration utility
VectorIndexManager::Status VectorIndexManager::migrateToEncryption(
    bool dry_run = true) {
    
    THEMIS_INFO("VectorIndexManager::migrateToEncryption - dry_run={}", dry_run);
    
    if (!field_encryption_) {
        return Status::Error("FieldEncryption not configured");
    }
    
    std::string prefix = objectName_ + ":";
    size_t migrated = 0;
    size_t skipped = 0;
    
    db_.scan(prefix, [&](const std::string& key, 
                         const std::vector<uint8_t>& value) {
        std::string pk = key.substr(prefix.size());
        BaseEntity entity = BaseEntity::deserialize(pk, value);
        
        // Check if already encrypted
        auto enc_flag = entity.getFieldAsInt("_encrypted");
        if (enc_flag.has_value() && *enc_flag == 1) {
            ++skipped;
            return true;  // Already encrypted
        }
        
        // Extract plaintext vector
        auto vec_opt = entity.extractVector("embedding");
        if (!vec_opt.has_value()) {
            THEMIS_WARN("Entity {} has no embedding field", pk);
            return true;
        }
        
        if (!dry_run) {
            // Encrypt and update
            EncryptedField<std::vector<float>> enc_vec;
            enc_vec.encrypt(*vec_opt, vector_key_id_);
            
            BaseEntity updated(pk);
            updated.setField("embedding_encrypted", enc_vec.toBase64());
            updated.setField("_encrypted", int64_t(1));
            
            // Copy other fields
            for (const auto& [field, val] : entity.getAllFields()) {
                if (field != "embedding") {
                    updated.setField(field, val);
                }
            }
            
            // Write back
            db_.put(key, updated.serialize());
        }
        
        ++migrated;
        return true;
    });
    
    THEMIS_INFO("VectorIndexManager::migrateToEncryption - "
                "Migrated: {}, Skipped: {}, Dry-run: {}", 
                migrated, skipped, dry_run);
    
    return Status::OK();
}
```

---

### 3.4 Testing

**File:** `tests/test_vector_encryption_phase1.cpp`

```cpp
#include <gtest/gtest.h>
#include "index/vector_index.h"
#include "security/mock_key_provider.h"
#include "security/encryption.h"

TEST(VectorEncryptionPhase1, BasicEncryptDecrypt) {
    // Setup
    auto db = createTestDB();
    auto key_prov = std::make_shared<MockKeyProvider>();
    key_prov->createKey("vector_embeddings", 1);
    
    auto field_enc = std::make_shared<FieldEncryption>(key_prov);
    EncryptedField<std::vector<float>>::setFieldEncryption(field_enc);
    
    VectorIndexManager::Config cfg;
    cfg.object_name = "test_vectors";
    cfg.dimension = 128;
    cfg.encrypt_vectors = true;
    cfg.field_encryption = field_enc;
    
    VectorIndexManager mgr(*db, cfg);
    
    // Insert encrypted vector
    std::vector<float> vec(128, 0.5f);
    BaseEntity e("doc1");
    e.setField("embedding", vec);
    
    auto status = mgr.addEntity(e);
    ASSERT_TRUE(status.ok);
    
    // Verify encrypted storage
    auto stored = db->get("test_vectors:doc1");
    ASSERT_TRUE(stored.has_value());
    
    BaseEntity loaded = BaseEntity::deserialize("doc1", *stored);
    EXPECT_TRUE(loaded.hasField("embedding_encrypted"));
    EXPECT_FALSE(loaded.hasField("embedding"));  // Plaintext removed
    EXPECT_EQ(loaded.getFieldAsInt("_encrypted").value_or(0), 1);
    
    // Verify search works (decrypted in-memory)
    std::vector<float> query(128, 0.5f);
    auto [search_status, results] = mgr.searchKnn(query, 1);
    ASSERT_TRUE(search_status.ok);
    ASSERT_EQ(results.size(), 1);
    EXPECT_EQ(results[0].pk, "doc1");
}

TEST(VectorEncryptionPhase1, BackwardCompatibility) {
    // Setup with encryption disabled
    auto db = createTestDB();
    
    VectorIndexManager::Config cfg;
    cfg.object_name = "test_vectors";
    cfg.dimension = 128;
    cfg.encrypt_vectors = false;  // Disabled
    
    VectorIndexManager mgr(*db, cfg);
    
    // Insert plaintext vector
    std::vector<float> vec(128, 0.5f);
    BaseEntity e("doc1");
    e.setField("embedding", vec);
    mgr.addEntity(e);
    
    // Verify plaintext storage
    auto stored = db->get("test_vectors:doc1");
    BaseEntity loaded = BaseEntity::deserialize("doc1", *stored);
    EXPECT_TRUE(loaded.hasField("embedding"));
    EXPECT_FALSE(loaded.hasField("embedding_encrypted"));
    
    // Search still works
    auto [status, results] = mgr.searchKnn(vec, 1);
    ASSERT_TRUE(status.ok);
    EXPECT_EQ(results[0].pk, "doc1");
}

TEST(VectorEncryptionPhase1, Migration) {
    // Phase 1: Create plaintext data
    auto db = createTestDB();
    VectorIndexManager::Config cfg;
    cfg.object_name = "test";
    cfg.dimension = 64;
    cfg.encrypt_vectors = false;
    
    VectorIndexManager mgr1(*db, cfg);
    
    for (int i = 0; i < 100; ++i) {
        std::vector<float> vec(64, static_cast<float>(i));
        BaseEntity e("doc" + std::to_string(i));
        e.setField("embedding", vec);
        mgr1.addEntity(e);
    }
    
    // Phase 2: Enable encryption + migrate
    auto key_prov = std::make_shared<MockKeyProvider>();
    key_prov->createKey("vector_embeddings", 1);
    auto field_enc = std::make_shared<FieldEncryption>(key_prov);
    
    cfg.encrypt_vectors = true;
    cfg.field_encryption = field_enc;
    
    VectorIndexManager mgr2(*db, cfg);
    
    // Dry-run migration
    auto status = mgr2.migrateToEncryption(/*dry_run=*/true);
    ASSERT_TRUE(status.ok);
    
    // Actual migration
    status = mgr2.migrateToEncryption(/*dry_run=*/false);
    ASSERT_TRUE(status.ok);
    
    // Verify all vectors encrypted
    mgr2.rebuildFromStorage();
    std::vector<float> query(64, 50.0f);
    auto [search_status, results] = mgr2.searchKnn(query, 10);
    ASSERT_TRUE(search_status.ok);
    EXPECT_EQ(results.size(), 10);
}

TEST(VectorEncryptionPhase1, PerformanceBenchmark) {
    // Measure encryption overhead
    auto db = createTestDB();
    auto key_prov = std::make_shared<MockKeyProvider>();
    key_prov->createKey("vector_embeddings", 1);
    auto field_enc = std::make_shared<FieldEncryption>(key_prov);
    
    VectorIndexManager::Config cfg;
    cfg.object_name = "bench";
    cfg.dimension = 768;
    cfg.encrypt_vectors = true;
    cfg.field_encryption = field_enc;
    
    VectorIndexManager mgr(*db, cfg);
    
    // Benchmark: Insert 1000 vectors
    auto start = std::chrono::steady_clock::now();
    
    for (int i = 0; i < 1000; ++i) {
        std::vector<float> vec(768, 0.5f);
        BaseEntity e("doc" + std::to_string(i));
        e.setField("embedding", vec);
        mgr.addEntity(e);
    }
    
    auto elapsed = std::chrono::steady_clock::now() - start;
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
    
    std::cout << "1000 inserts (encrypted): " << ms.count() << " ms\n";
    std::cout << "Per-vector: " << (ms.count() / 1000.0) << " ms\n";
    
    // Target: < 1ms per vector (including encryption)
    EXPECT_LT(ms.count(), 1500);
}
```

---

## 4. Configuration & Deployment

### 4.1 Server Configuration

**File:** `config/server.yaml`

```yaml
database:
  encryption:
    enabled: true
    key_provider: "vault"
    vault:
      address: "https://vault.example.com"
      token_path: "/etc/themis/vault_token"
    
  vector:
    encrypt_embeddings: true  # Phase 1 feature flag
    vector_key_id: "vector_embeddings"
```

### 4.2 HTTP API Changes

**File:** `src/server/http_server.cpp`

Modify vector insertion endpoint:

```cpp
void HttpServer::handleVectorInsert(const httplib::Request& req, 
                                   httplib::Response& res) {
    // Parse request
    auto json = nlohmann::json::parse(req.body);
    std::string collection = json["collection"];
    
    // Get VectorIndexManager
    auto mgr = getVectorIndexManager(collection);
    
    // Create entity
    BaseEntity entity(json["id"]);
    entity.setField("embedding", json["embedding"].get<std::vector<float>>());
    
    // Encryption is handled internally by VectorIndexManager
    auto status = mgr->addEntity(entity);
    
    if (status.ok) {
        res.set_content(R"({"status":"ok"})", "application/json");
    } else {
        res.status = 500;
        res.set_content(R"({"error":")" + status.message + R"("})", 
                       "application/json");
    }
}
```

---

## 5. Performance Analysis

### 5.1 Overhead Estimates

**Single Vector (768-dim):**
- Serialization: ~0.01 ms
- AES-256-GCM Encryption: ~0.3 ms
- Base64 Encoding: ~0.05 ms
- **Total:** ~0.4 ms per vector

**Batch Decryption (1M vectors):**
- Single-threaded: 400 seconds (0.4 ms × 1M)
- Parallelized (8 cores): ~50 seconds
- HNSW build: ~120 seconds
- **Total Index Load:** ~170 seconds

**Comparison:**
- Without Encryption: 120 seconds (HNSW build only)
- **Overhead:** +50 seconds (+40%)

**Acceptable for Production:** ✅ Yes
- Cold start penalty: Once per server startup
- Warm start (encrypted HNSW in Phase 2): ~5 seconds

### 5.2 Storage Overhead

**Per Vector:**
- Plaintext: 768 × 4 bytes = 3,072 bytes
- Encrypted:
  - Ciphertext: 3,072 bytes
  - IV: 12 bytes
  - Tag: 16 bytes
  - Metadata: ~50 bytes (key_id, version)
  - **Total:** ~3,150 bytes

**Overhead:** +2.5% storage

---

## 6. Security Analysis

### 6.1 Attack Surface Reduction

**Before Phase 1:**
- Disk: ❌ Plaintext vectors in RocksDB
- Network: ❌ No encryption (assume HTTPS)
- Memory: ❌ Plaintext vectors in HNSW
- **Risk:** High (3/3 attack vectors)

**After Phase 1:**
- Disk: ✅ Encrypted vectors (AES-256-GCM)
- Network: ✅ TLS 1.3
- Memory: ⚠️ Plaintext vectors in HNSW (unavoidable)
- **Risk:** Low (1/3 attack vectors)

**Risk Reduction:** 66%

### 6.2 BSI C5 Compliance

**Before:**
- CRY-03 (Data-at-Rest): ⚠️ Conditionally Compliant

**After Phase 1:**
- CRY-03 (Data-at-Rest): ✅ **Fully Compliant**

**Justification:**
- Disk encryption: ✅ Implemented
- Memory-only risk: ✅ Documented and accepted
- State-of-the-art: ✅ Matches industry standards

---

## 7. Rollout Plan

### Week 1: Implementation
- **Day 1-2:** Extend `EncryptedField<std::vector<float>>`
- **Day 3-4:** Modify `VectorIndexManager` (encrypt, decrypt, batch)
- **Day 5:** Unit tests

### Week 2: Integration & Testing
- **Day 1-2:** Integration tests
- **Day 3:** Performance benchmarks
- **Day 4:** Migration tool
- **Day 5:** Documentation + Code review

### Deployment:
- **Staging:** Week 3
- **Production:** Week 4 (phased rollout)

---

## 8. Risks & Mitigations

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|-----------|
| Performance regression | Medium | High | Benchmark before merge; feature flag for rollback |
| Encryption bugs | Low | Critical | Comprehensive unit tests; code review |
| Key rotation issues | Medium | Medium | Test key rotation separately |
| Migration failures | Low | High | Dry-run mode; backup before migration |
| Backward compatibility | Low | Medium | Dual-read support; gradual migration |

---

## 9. Success Criteria

- [ ] `EncryptedField<std::vector<float>>` implemented and tested
- [ ] All vectors stored encrypted in RocksDB
- [ ] Search performance < 5% degradation
- [ ] Index load time < 3 minutes for 1M vectors
- [ ] 100% test coverage for encryption code
- [ ] BSI C5 CRY-03 compliance achieved
- [ ] Migration tool validated on production-size dataset
- [ ] Documentation complete
- [ ] Code review passed
- [ ] Security team approval

---

## 10. Next Steps (Phase 2)

After Phase 1 is complete and validated:

1. **Phase 2 (Weeks 3-6):** HNSW index encryption at-rest
2. **Phase 3 (Months 3-6):** Differential Privacy (optional)
3. **Phase 4 (Month 12+):** Homomorphic Encryption (research)

---

## References

- BSI C5 Compliance Analysis: `docs/security/BSI_C5_COLUMN_ENCRYPTION_COMPLIANCE.md`
- Embedding Reversibility: `docs/security/EMBEDDING_REVERSIBILITY_ANALYSIS.md`
- Symmetric Encryption Approaches: `docs/security/SYMMETRIC_ENCRYPTION_APPROACHES.md`
- Existing Encryption: `src/security/field_encryption.cpp`

---

**Status:** Ready for implementation  
**Approvals Required:** Security Team, Engineering Lead  
**Review Date:** 2025-12-16
