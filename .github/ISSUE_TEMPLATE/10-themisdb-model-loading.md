---
name: "🗄️ ThemisDB Blob Store Integration für LLM"
about: Model Loading aus ThemisDB Blob Store implementieren (Kritisch - P0)
title: "[LLM] Implement Model Loading from ThemisDB Blob Store"
labels: priority:P0, type:feature, area:llm, area:storage, effort:large, phase:production
assignees: ''

---

## 📋 Beschreibung / Description

**DE**: Implementierung des Model Loadings aus dem ThemisDB Blob Store. Dies ist eine kritische Lücke, die die "Native LLM Integration" Kernfunktion blockiert.

**EN**: Implement model loading from ThemisDB Blob Store. This is a critical gap blocking the "Native LLM Integration" core feature.

**Related Analysis**: `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md` §2.1, §4  
**Current Status**: `src/llm/llamacpp_inference_engine.cpp:58-61` (stub returns false)  
**Blocker**: ❌ **PRODUKTIONSBLOCKER** - Modelle können nicht aus der Datenbank geladen werden

## 🎯 Ziele / Goals

- [ ] `loadModelFromThemisDB()` vollständig implementieren
- [ ] Blob Store Integration für GGUF-Modelle
- [ ] Streaming von großen Modellen (>5GB)
- [ ] Encryption/Decryption Support
- [ ] Model Metadata Management
- [ ] Comprehensive Testing

## 📝 Aufgaben / Tasks

### 1. Blob Store Query Implementation
**Priorität**: P0 - Kritisch

- [ ] Query `LLMModelStorage` für Model Metadata by `model_id`
- [ ] Retrieve blob reference from metadata
- [ ] Handle model not found errors gracefully
- [ ] Add logging for debugging

**File**: `src/llm/llamacpp_inference_engine.cpp`  
**Function**: `loadModelFromThemisDB()`  
**Lines**: 58-61 (current stub)

**Implementation Steps**:
```cpp
// 1. Query metadata
auto metadata = model_storage_->getModelMetadata(model_id);
if (!metadata) {
    spdlog::error("Model {} not found in metadata store", model_id);
    return false;
}

// 2. Get blob reference
std::string blob_ref = metadata->blob_reference;
if (blob_ref.empty()) {
    spdlog::error("Model {} has no blob reference", model_id);
    return false;
}
```

---

### 2. GGUF Model Streaming
**Priorität**: P0 - Kritisch

- [ ] Implement streaming reader for Blob Store
- [ ] Handle large models (chunked reading)
- [ ] Memory-efficient buffer management
- [ ] Progress tracking for large downloads
- [ ] Error recovery (retry on network failures)

**File**: `src/llm/llamacpp_inference_engine.cpp`  
**New Helper**: `streamModelFromBlobStore()`

**Requirements**:
- Support models up to 100GB
- Chunk size: 64MB (configurable)
- Retry logic: 3 attempts with exponential backoff
- Progress callback for UI

**ThemisDB Integration**:
```cpp
// Use existing Blob Store API
auto blob_stream = blob_store_->openStream(blob_ref);
size_t chunk_size = 64 * 1024 * 1024; // 64MB chunks
std::vector<uint8_t> buffer(chunk_size);

while (!blob_stream->eof()) {
    size_t bytes_read = blob_stream->read(buffer.data(), chunk_size);
    // Process chunk...
}
```

---

### 3. llama.cpp Integration
**Priorität**: P0 - Kritisch

- [ ] Initialize llama.cpp from stream/buffer
- [ ] Support both file-based and memory-based loading
- [ ] Handle GGUF format validation
- [ ] Verify model compatibility (architecture, quantization)
- [ ] Clean up resources on failure

**File**: `src/llm/llamacpp_inference_engine.cpp`  
**Dependencies**: llama.cpp API, GGUF loader

**Implementation Options**:
1. **Option A (File-based)**: Stream to temporary file, then load
   - Pros: Simple, llama.cpp supports directly
   - Cons: Disk I/O overhead, temp file cleanup
   
2. **Option B (Memory-based)**: Load to memory, use memory buffer
   - Pros: Faster, no disk I/O
   - Cons: Memory constraints for large models

**Recommended**: Option A for models >10GB, Option B for smaller models

---

### 4. Encryption/Decryption Support
**Priorität**: P0 - Kritisch

- [ ] Integrate with `FieldEncryption` for encrypted models
- [ ] Support AES-256-GCM decryption during streaming
- [ ] Key retrieval from Vault/HSM (not MockKeyProvider)
- [ ] Handle decryption errors gracefully
- [ ] Performance optimization (decrypt in chunks)

**File**: `src/llm/llamacpp_inference_engine.cpp`  
**Dependencies**: `include/security/field_encryption.h`

**Security Requirements**:
- Use production Vault/HSM (not mock)
- Never store decryption keys in memory longer than necessary
- Audit log all decryption attempts
- Support key rotation

**Implementation**:
```cpp
// Get key from Vault (not MockKeyProvider!)
auto key_provider = VaultKeyProvider::create(vault_config);
auto dek = key_provider->getDEK(model_id);

// Decrypt during streaming
auto encrypted_stream = blob_store_->openStream(blob_ref);
auto decrypting_stream = std::make_unique<DecryptingStream>(
    std::move(encrypted_stream), 
    dek
);
```

---

### 5. Model Metadata Management
**Priorität**: P1 - Hoch

- [ ] Store model fingerprint (hash) in metadata
- [ ] Verify model integrity after loading
- [ ] Cache model paths for performance
- [ ] Update last_loaded timestamp
- [ ] Track model usage statistics

**File**: New: `src/llm/llm_model_storage.cpp`  
**Schema**:
```
LLMModelMetadata {
    model_id: string
    blob_reference: string
    model_name: string
    architecture: string (e.g., "llama", "mistral")
    quantization: string (e.g., "Q4_K_M")
    size_bytes: uint64
    sha256_hash: string
    created_at: timestamp
    last_loaded_at: timestamp
    load_count: uint64
    encryption_enabled: bool
    encryption_key_id: string (Vault key ID)
}
```

**Storage**: RocksDB Column Family `llm_model_metadata`

---

### 6. Testing & Validation
**Priorität**: P0 - Kritisch

- [ ] Unit tests for model loading
- [ ] Integration test with real Blob Store
- [ ] Test with encrypted models
- [ ] Test with large models (>10GB)
- [ ] Test error scenarios (not found, corrupted, permission denied)
- [ ] Performance benchmarks

**Test File**: `tests/integration/llm/test_model_loading_from_themisdb.cpp`

**Test Cases**:
1. Load small model (<1GB) successfully
2. Load large model (>10GB) with streaming
3. Load encrypted model with decryption
4. Handle model not found error
5. Handle corrupted model (integrity check failure)
6. Handle decryption key not available
7. Handle blob store connection failure
8. Concurrent model loading (thread safety)

**Performance Targets**:
- Small model (<1GB): Load in <10 seconds
- Large model (10GB): Load in <2 minutes
- Streaming overhead: <5% vs file loading

---

## 🔗 Abhängigkeiten / Dependencies

### ThemisDB Components (Must Use)
- ✅ `BlobStoreManager` - For blob storage operations
- ✅ `FieldEncryption` - For encryption/decryption
- ✅ `RocksDBWrapper` - For metadata storage
- ⚠️ **Replace** `MockKeyProvider` with `VaultKeyProvider` or `HSMProvider`

### External Dependencies
- ✅ llama.cpp - Already integrated
- ✅ OpenSSL - For encryption (already available)
- ⚠️ HashiCorp Vault SDK - If using Vault (optional)

### Blocked By
- None - All dependencies available

### Blocks
- ✅ LoRa Adapter Training (needs base models from DB)
- ✅ Model Serving (needs to load models from DB)
- ✅ Multi-tenant model sharing

---

## ✅ Akzeptanzkriterien / Acceptance Criteria

### Functional Requirements
- [ ] `loadModelFromThemisDB()` returns true for valid models
- [ ] Models can be loaded from Blob Store successfully
- [ ] Encrypted models are decrypted correctly
- [ ] Large models (>10GB) stream without memory exhaustion
- [ ] Model metadata is updated after loading

### Non-Functional Requirements
- [ ] Loading time <10s for small models, <2min for large
- [ ] Memory usage during loading <2x model size
- [ ] All tests pass (unit + integration)
- [ ] No security vulnerabilities (CodeQL clean)
- [ ] Proper error handling and logging

### Production Readiness
- [ ] No MockKeyProvider usage (production keys only)
- [ ] Audit logging for all model loads
- [ ] Graceful degradation on failures
- [ ] Documentation complete
- [ ] Monitoring metrics exposed (load time, success rate)

---

## 📊 Aufwand / Effort

**Geschätzte Zeit**: 2-3 Wochen (10-15 Arbeitstage)

**Breakdown**:
- Blob Store Integration: 3-4 Tage
- Streaming Implementation: 2-3 Tage
- Encryption/Decryption: 2-3 Tage
- Metadata Management: 1-2 Tage
- Testing & Validation: 3-4 Tage
- Documentation: 1 Tag

**Complexity**: Hoch - Erfordert Kenntnis von:
- ThemisDB Blob Store API
- llama.cpp Model Loading
- Encryption/Security
- RocksDB Metadata Storage

---

## 🧪 Test-Strategie / Test Strategy

### Unit Tests (New)
```cpp
TEST(LlamaCppInferenceEngine, LoadModelFromThemisDB_Success) {
    // Load small model successfully
}

TEST(LlamaCppInferenceEngine, LoadModelFromThemisDB_Encrypted) {
    // Load and decrypt encrypted model
}

TEST(LlamaCppInferenceEngine, LoadModelFromThemisDB_NotFound) {
    // Handle model not found error
}

TEST(LlamaCppInferenceEngine, LoadModelFromThemisDB_LargeModel) {
    // Stream large model (>10GB)
}
```

### Integration Tests (New)
```cpp
TEST(LLMIntegration, EndToEnd_LoadAndInference) {
    // 1. Store model in Blob Store
    // 2. Load model from ThemisDB
    // 3. Run inference
    // 4. Verify output
}
```

### Performance Tests
- Benchmark loading time for various model sizes
- Compare file vs blob store loading performance
- Measure memory usage during streaming

---

## 📚 Referenzen / References

**Investigation Report**:
- `INVESTIGATION_GAPS_SIMULATIONS_THEMISDB.md` - Section 2.1 (Critical)

**ThemisDB Documentation**:
- `docs/de/storage/blob_store.md` - Blob Store API
- `docs/de/security/encryption.md` - Field Encryption
- `docs/de/llm/model_management.md` - Model Storage

**Implementation Guides**:
- `LORA_TRAINING_IMPLEMENTATION_STATUS.md` - Related LoRa work
- `docs/analysis/IMPLEMENTATION_GUIDE.md` - Architecture

**External References**:
- llama.cpp API: https://github.com/ggerganov/llama.cpp
- GGUF Specification: https://github.com/ggerganov/ggml/blob/master/docs/gguf.md

---

## 💡 Implementation Notes

### Architecture Decision: Streaming vs File-based

**Recommendation**: Hybrid approach
- Models <5GB: Load to memory buffer
- Models 5-20GB: Stream to temporary file
- Models >20GB: Progressive streaming with partial loading

### Security Considerations

⚠️ **CRITICAL**:
- **NEVER** use `MockKeyProvider` in production
- **ALWAYS** use Vault/HSM for production keys
- **AUDIT** all model loads (who, when, which model)
- **ROTATE** encryption keys periodically

### Performance Optimization

1. **Connection Pooling**: Reuse Blob Store connections
2. **Prefetching**: Start loading metadata while streaming
3. **Compression**: Consider on-the-fly decompression
4. **Caching**: Cache frequently-used model paths

---

## 🏁 Definition of Done

- [ ] All tasks completed
- [ ] All tests passing (unit + integration)
- [ ] Code review approved
- [ ] Security scan passed (CodeQL)
- [ ] Documentation updated
- [ ] No MockKeyProvider in production code
- [ ] Performance targets met
- [ ] Monitoring metrics exposed
- [ ] Production deployment verified

---

**Priority**: 🔴 **P0 - CRITICAL PRODUCTION BLOCKER**  
**Impact**: Unblocks core "native LLM integration" feature  
**Timeline**: Start immediately, complete in 2-3 weeks  
**Dependencies**: None (all components available)

---

**Erstellt**: 15. Januar 2026  
**Status**: 🚧 Ready for Implementation  
**Related Issues**: #[LoRa Training], #[Production Readiness]
