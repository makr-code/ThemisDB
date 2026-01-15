---
name: "💾 LoRa Storage Backend für ThemisDB"
about: Vollständige LoRa Storage Backend Implementation (Kritisch - P0)
title: "[LoRa] Complete LoRa Storage Backend for ThemisDB and S3"
labels: priority:P0, type:feature, area:llm, area:storage, effort:large, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Vervollständigung der LoRa Storage Backends für ThemisDB und S3. Aktuell funktioniert nur das Filesystem-Backend, alle anderen returnen false.

**EN**: Complete LoRa storage backends for ThemisDB and S3. Currently only filesystem backend works, all others return false.

**Related Analysis**: `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md` §1.1.2, §4  
**Current Status**: `src/llm/lora_framework/lora_storage_service.cpp:43,57,71,91` (4x TODO)  
**Blocker**: ❌ **PRODUKTIONSBLOCKER** - Adapter können nicht persistent in DB gespeichert werden

## 🎯 Ziele / Goals

- [ ] ThemisDB backend vollständig implementieren (save/load/delete/metadata)
- [ ] S3 backend vollständig implementieren
- [ ] Encryption/Decryption Support (kein MockKeyProvider)
- [ ] Blob Reference Management
- [ ] Adapter Lifecycle Management
- [ ] Comprehensive Testing

## 📝 Aufgaben / Tasks

### 1. ThemisDB Backend - saveAdapter()
**Priorität**: P0 - Kritisch

**Current Code** (Lines 43-45):
```cpp
case StorageBackend::THEMISDB:
    // TODO: Implement ThemisDB and S3 backends
    return false;
```

**Implementation**:
- [ ] Serialize adapter weights to binary format
- [ ] Store weights in Blob Store with compression
- [ ] Store metadata in RocksDB column family `lora_adapters`
- [ ] Handle encryption if enabled
- [ ] Generate unique adapter ID
- [ ] Return blob reference

**File**: `src/llm/lora_framework/lora_storage_service.cpp`  
**Function**: `saveAdapter()`  
**Lines**: 43-45

**Requirements**:
```cpp
bool LoRAStorageService::saveAdapter(
    const std::string& adapter_id,
    const LoRAAdapterWeights& weights,
    const AdapterMetadata& metadata
) {
    if (backend_ == StorageBackend::THEMISDB) {
        // 1. Serialize weights
        std::vector<uint8_t> serialized = serializeWeights(weights);
        
        // 2. Compress (optional but recommended)
        auto compressed = compressData(serialized);
        
        // 3. Encrypt if enabled
        if (metadata.encryption_enabled) {
            auto key_provider = getProductionKeyProvider(); // NOT MockKeyProvider!
            compressed = encryptData(compressed, key_provider);
        }
        
        // 4. Store in Blob Store
        std::string blob_ref = blob_store_->put(adapter_id, compressed);
        
        // 5. Store metadata in RocksDB
        AdapterMetadataRecord record {
            .adapter_id = adapter_id,
            .blob_reference = blob_ref,
            .rank = metadata.rank,
            .alpha = metadata.alpha,
            .target_modules = metadata.target_modules,
            .size_bytes = compressed.size(),
            .created_at = std::chrono::system_clock::now(),
            .encrypted = metadata.encryption_enabled
        };
        
        return metadata_store_->putAdapter(adapter_id, record);
    }
    return false;
}
```

---

### 2. ThemisDB Backend - loadAdapter()
**Priorität**: P0 - Kritisch

**Current Code** (Lines 57-59):
```cpp
case StorageBackend::THEMISDB:
    // TODO: Implement ThemisDB and S3 backends
    return std::nullopt;
```

**Implementation**:
- [ ] Retrieve metadata from RocksDB
- [ ] Get blob reference from metadata
- [ ] Load encrypted blob from Blob Store
- [ ] Decrypt if encrypted (using production keys)
- [ ] Decompress data
- [ ] Deserialize to LoRAAdapterWeights
- [ ] Validate integrity (checksum)

**File**: `src/llm/lora_framework/lora_storage_service.cpp`  
**Function**: `loadAdapter()`  
**Lines**: 57-59

**Requirements**:
```cpp
std::optional<LoRAAdapterWeights> LoRAStorageService::loadAdapter(
    const std::string& adapter_id
) {
    if (backend_ == StorageBackend::THEMISDB) {
        // 1. Get metadata
        auto metadata = metadata_store_->getAdapter(adapter_id);
        if (!metadata) {
            spdlog::error("Adapter {} not found in metadata", adapter_id);
            return std::nullopt;
        }
        
        // 2. Load blob
        auto blob_data = blob_store_->get(metadata->blob_reference);
        if (!blob_data) {
            spdlog::error("Blob {} not found", metadata->blob_reference);
            return std::nullopt;
        }
        
        // 3. Decrypt if encrypted
        if (metadata->encrypted) {
            auto key_provider = getProductionKeyProvider();
            blob_data = decryptData(*blob_data, key_provider);
            if (!blob_data) {
                spdlog::error("Decryption failed for adapter {}", adapter_id);
                return std::nullopt;
            }
        }
        
        // 4. Decompress
        auto decompressed = decompressData(*blob_data);
        
        // 5. Deserialize
        auto weights = deserializeWeights(decompressed);
        
        // 6. Validate integrity
        if (!validateChecksum(weights, metadata->checksum)) {
            spdlog::error("Checksum validation failed for {}", adapter_id);
            return std::nullopt;
        }
        
        return weights;
    }
    return std::nullopt;
}
```

---

### 3. ThemisDB Backend - getAdapterMetadata()
**Priorität**: P0 - Kritisch

**Current Code** (Lines 71-73):
```cpp
case StorageBackend::THEMISDB:
    // TODO: Implement ThemisDB and S3 backends
    return std::nullopt;
```

**Implementation**:
- [ ] Query RocksDB for adapter metadata
- [ ] Return AdapterMetadata struct
- [ ] Handle not found cases

**File**: `src/llm/lora_framework/lora_storage_service.cpp`  
**Function**: `getAdapterMetadata()`  
**Lines**: 71-73

---

### 4. ThemisDB Backend - deleteAdapter()
**Priorität**: P0 - Kritisch  
**Currently Broken in**: `lora_storage_service_themisdb.cpp:120`

**Current Code**:
```cpp
// TODO: Get blob ref and delete
// Currently only deletes metadata, not blob data
```

**Implementation**:
- [ ] Get blob reference from metadata
- [ ] Delete blob from Blob Store
- [ ] Delete metadata from RocksDB
- [ ] Atomic operation (both or neither)
- [ ] Handle errors gracefully

**Files**: 
- `src/llm/lora_framework/lora_storage_service.cpp:91-93`
- `src/llm/lora_framework/lora_storage_service_themisdb.cpp:120`

**Requirements**:
```cpp
bool LoRAStorageService::deleteAdapter(const std::string& adapter_id) {
    if (backend_ == StorageBackend::THEMISDB) {
        // 1. Get metadata (includes blob reference)
        auto metadata = metadata_store_->getAdapter(adapter_id);
        if (!metadata) {
            spdlog::warn("Adapter {} not found, nothing to delete", adapter_id);
            return true; // Idempotent
        }
        
        // 2. Delete blob from Blob Store
        bool blob_deleted = blob_store_->remove(metadata->blob_reference);
        if (!blob_deleted) {
            spdlog::error("Failed to delete blob {}", metadata->blob_reference);
            return false;
        }
        
        // 3. Delete metadata from RocksDB
        bool meta_deleted = metadata_store_->deleteAdapter(adapter_id);
        if (!meta_deleted) {
            spdlog::error("Failed to delete metadata for {}", adapter_id);
            // Blob already deleted, but metadata remains (inconsistent state!)
            // TODO: Consider compensating transaction
            return false;
        }
        
        spdlog::info("Adapter {} deleted successfully", adapter_id);
        return true;
    }
    return false;
}
```

**⚠️ Atomicity Concern**: 
- If blob deletion succeeds but metadata deletion fails → orphaned blob
- If metadata deletion succeeds but blob deletion fails → dangling reference
- **Solution**: Use 2-Phase Commit or Compensating Transaction

---

### 5. S3 Backend Implementation
**Priorität**: P1 - Hoch

**Implementation**:
- [ ] AWS SDK C++ integration
- [ ] S3 bucket configuration
- [ ] Multi-part upload for large adapters
- [ ] Pre-signed URLs for secure access
- [ ] Versioning support
- [ ] Lifecycle policies (optional)

**File**: `src/llm/lora_framework/lora_storage_service.cpp`  
**Dependencies**: AWS SDK for C++, S3 credentials

**Configuration**:
```yaml
lora_storage:
  backend: s3
  s3:
    bucket: themisdb-lora-adapters
    region: us-east-1
    access_key_id: ${AWS_ACCESS_KEY_ID}
    secret_access_key: ${AWS_SECRET_ACCESS_KEY}
    encryption: AES256  # Server-side encryption
```

**Requirements**:
- Support multi-part upload for adapters >5MB
- Use pre-signed URLs for temporary access
- Enable versioning for audit trail
- Implement retry logic (3 attempts, exponential backoff)

---

### 6. Encryption Integration (Production Keys)
**Priorität**: P0 - Kritisch

**Current Issue**: `lora_storage_service_themisdb.cpp:29` uses `MockKeyProvider`

**Replace MockKeyProvider**:
```cpp
// OLD (Line 29) - DO NOT USE IN PRODUCTION!
auto key_provider = std::make_shared<themis::security::MockKeyProvider>();

// NEW - Production-ready
auto key_provider = createProductionKeyProvider(security_config_);
// Options:
// 1. VaultKeyProvider - HashiCorp Vault integration
// 2. HSMProvider - Hardware Security Module
// 3. KMSProvider - AWS KMS, Azure Key Vault, GCP KMS
```

**Implementation**:
- [ ] Remove all `MockKeyProvider` usage
- [ ] Integrate with Vault/HSM/KMS
- [ ] Implement key rotation support
- [ ] Add audit logging for all key accesses
- [ ] Handle key not found errors

**File**: `src/llm/lora_framework/lora_storage_service_themisdb.cpp`  
**Line**: 29 (and all similar usages)

**Security Requirements**:
- Never log encryption keys
- Audit all key retrievals
- Support key rotation without data migration
- Use envelope encryption (KEK + DEK pattern)

---

### 7. Blob Reference Management
**Priorität**: P0 - Kritisch

**Implementation**:
- [ ] Add `BlobReference` abstraction
- [ ] Track blob lifecycle (created, accessed, deleted)
- [ ] Implement garbage collection for orphaned blobs
- [ ] Reference counting for shared adapters
- [ ] Blob expiration policies

**New File**: `src/llm/lora_framework/lora_blob_manager.cpp`

**Features**:
```cpp
class LoRABlobManager {
public:
    // Create blob and get reference
    BlobReference createBlob(const std::vector<uint8_t>& data);
    
    // Get blob data by reference
    std::optional<std::vector<uint8_t>> getBlob(const BlobReference& ref);
    
    // Delete blob (decrements ref count)
    bool deleteBlob(const BlobReference& ref);
    
    // Garbage collection
    size_t collectOrphanedBlobs();
    
    // Blob statistics
    BlobStats getStats();
};
```

---

### 8. Testing & Validation
**Priorität**: P0 - Kritisch

- [ ] Unit tests for each backend operation
- [ ] Integration tests with real ThemisDB
- [ ] Integration tests with real S3 (or LocalStack)
- [ ] Test encryption/decryption roundtrip
- [ ] Test blob deletion (no orphans)
- [ ] Test concurrent access (thread safety)
- [ ] Performance benchmarks

**Test Files**:
- `tests/test_lora_storage_service.cpp` (unit tests)
- `tests/integration/lora/test_lora_storage_backends.cpp` (integration)

**Test Cases**:
1. Save adapter to ThemisDB successfully
2. Load adapter from ThemisDB successfully
3. Save and load encrypted adapter
4. Delete adapter (both blob and metadata removed)
5. Handle adapter not found errors
6. Handle blob store connection failures
7. Concurrent save/load operations
8. Large adapter handling (>100MB)
9. S3 backend operations
10. Blob garbage collection

**Performance Targets**:
- Save adapter (<10MB): <1 second
- Load adapter (<10MB): <500ms
- Delete adapter: <500ms
- S3 operations: <2 seconds (with network)

---

## 🔗 Abhängigkeiten / Dependencies

### ThemisDB Components (Must Use)
- ✅ `BlobStoreManager` - Blob storage operations
- ✅ `RocksDBWrapper` - Metadata storage
- ✅ `FieldEncryption` - Encryption/decryption
- ⚠️ **Replace** `MockKeyProvider` with production provider

### External Dependencies
- ⚠️ AWS SDK for C++ (for S3 backend) - Add to vcpkg.json
- ✅ HashiCorp Vault SDK (optional, for VaultKeyProvider)
- ✅ OpenSSL - Already available

### Blocked By
- None - All dependencies available or can be added

### Blocks
- ✅ LoRa Training (needs to save adapters)
- ✅ LoRa Inference (needs to load adapters)
- ✅ LoRa Orchestrator (needs full CRUD)

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

### Functional Requirements
- [ ] All backend operations (save/load/metadata/delete) work for ThemisDB
- [ ] S3 backend fully functional
- [ ] Encrypted adapters work correctly
- [ ] Blob deletion removes both blob and metadata
- [ ] No MockKeyProvider in production code

### Non-Functional Requirements
- [ ] Save/load times meet performance targets
- [ ] Thread-safe concurrent operations
- [ ] No memory leaks (valgrind clean)
- [ ] No security vulnerabilities (CodeQL clean)
- [ ] Proper error handling and logging

### Production Readiness
- [ ] Production key providers only (Vault/HSM/KMS)
- [ ] Audit logging for all operations
- [ ] Monitoring metrics exposed
- [ ] Documentation complete
- [ ] All tests passing

---

## 📊 Aufwand / Effort

**Geschätzte Zeit**: 2-3 Wochen (10-15 Arbeitstage)

**Breakdown**:
- ThemisDB Backend (save/load/metadata): 4-5 Tage
- ThemisDB Backend (delete mit atomicity): 2 Tage
- S3 Backend Implementation: 3-4 Tage
- Encryption Integration (production keys): 2-3 Tage
- Blob Reference Management: 2 Tage
- Testing & Validation: 3-4 Tage
- Documentation: 1 Tag

**Complexity**: Hoch - Erfordert Kenntnis von:
- ThemisDB Blob Store API
- RocksDB Column Families
- Encryption/Security
- AWS S3 (if implementing S3 backend)
- Concurrency/Thread Safety

---

## 🧪 Test-Strategie / Test Strategy

### Unit Tests
```cpp
TEST(LoRAStorageService, SaveAdapter_ThemisDB_Success) {
    // Save adapter to ThemisDB backend
}

TEST(LoRAStorageService, LoadAdapter_ThemisDB_Success) {
    // Load adapter from ThemisDB backend
}

TEST(LoRAStorageService, DeleteAdapter_ThemisDB_Success) {
    // Delete adapter (blob + metadata)
}

TEST(LoRAStorageService, SaveLoadEncrypted_ThemisDB) {
    // Roundtrip with encryption
}
```

### Integration Tests
```cpp
TEST(LoRAStorageIntegration, EndToEnd_ThemisDB) {
    // 1. Save adapter
    // 2. Verify blob exists
    // 3. Load adapter
    // 4. Verify weights match
    // 5. Delete adapter
    // 6. Verify blob and metadata removed
}

TEST(LoRAStorageIntegration, S3Backend) {
    // Test S3 operations (or LocalStack)
}
```

### Stress Tests
- Concurrent save/load operations (100 threads)
- Large adapter handling (1GB adapter)
- Blob garbage collection under load

---

## 📚 Referenzen / References

**Investigation Report**:
- `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md` - Section 1.1.2 (Critical)

**ThemisDB Documentation**:
- `docs/de/storage/blob_store.md` - Blob Store API
- `docs/de/security/encryption.md` - Field Encryption

**Implementation Status**:
- `LORA_TRAINING_IMPLEMENTATION_STATUS.md` - Current status

**External References**:
- AWS S3 SDK: https://aws.amazon.com/sdk-for-cpp/
- HashiCorp Vault: https://www.vaultproject.io/api-docs

---

## 💡 Implementation Notes

### Storage Backend Priority

**Recommended Implementation Order**:
1. ThemisDB backend (P0) - Core feature
2. Encryption integration (P0) - Security requirement
3. Blob reference management (P0) - Prevents data loss
4. S3 backend (P1) - Cloud storage option

### Security Best Practices

⚠️ **CRITICAL**:
- **NEVER** use `MockKeyProvider` in production
- **ALWAYS** audit all key accesses
- **ROTATE** encryption keys quarterly
- **ENCRYPT** all adapters containing sensitive data

### Atomicity Pattern for Delete

**Option 1: 2-Phase Commit**
```
Phase 1: Mark metadata as "deleting"
Phase 2a: Delete blob
Phase 2b: Delete metadata
Rollback: Unmark metadata if blob deletion fails
```

**Option 2: Compensating Transaction**
```
Try: Delete blob
On Success: Delete metadata
On Failure: Keep both (no inconsistency)

Background job: Cleanup orphaned blobs
```

**Recommendation**: Option 2 (simpler, eventual consistency)

---

## 🏁 Definition of Done

- [ ] All backend operations implemented (ThemisDB + S3)
- [ ] All tests passing (unit + integration)
- [ ] Code review approved
- [ ] Security scan passed (CodeQL)
- [ ] No MockKeyProvider in production code
- [ ] Documentation updated
- [ ] Performance targets met
- [ ] Monitoring metrics exposed
- [ ] Production deployment verified

---

**Priority**: 🔴 **P0 - CRITICAL PRODUCTION BLOCKER**  
**Impact**: Enables persistent LoRa adapter storage in database  
**Timeline**: Start immediately, complete in 2-3 weeks  
**Dependencies**: None (all components available)

---

**Erstellt**: 15. Januar 2026  
**Status**: 🚧 Ready for Implementation  
**Related Issues**: #[Model Loading], #[LoRa Training]
