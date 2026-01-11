---
title: "Implement LLM Model Loading to RocksDB Blob Store"
labels: llm, storage, blob-store, priority-medium, v1.4.0
milestone: v1.4.0
---

## 📋 Summary

LLM models are currently loaded from the file system, but the GGUF loader has a **TODO to implement loading from RocksDB Blob Store**. This would enable models to be stored in the database itself, improving portability and management.

**Type**: Feature Implementation  
**Priority**: MEDIUM  
**Effort**: 1-2 weeks  
**Status**: ❌ Not Implemented (TODO verified)

## 🔍 Verification

**File**: `src/llm/gguf_loader.cpp:173`

```cpp
bool GGUFLoader::loadToRocksDB(const std::string& model_path, const std::string& blob_key) {
    // TODO: Implement actual loading to RocksDB Blob Store
    // ...
    return false;  // ← Currently always fails
}
```

## 🎯 Problem Statement

### Current State
- ✅ Models can be loaded from file system
- ❌ Models cannot be stored in RocksDB Blob Store
- ❌ No unified model management
- ❌ Models tied to specific filesystem paths

### Desired State
- ✅ Models stored in database as blobs
- ✅ Models portable with database backup
- ✅ Multi-version model storage
- ✅ Centralized model management

### Benefits
1. **Portability**: Models move with database backups
2. **Management**: Unified storage for data + models
3. **Versioning**: Store multiple model versions
4. **Distribution**: Easier to replicate models across shards
5. **Cleanup**: Automatic garbage collection of unused models

## 🏗️ Proposed Implementation

### Architecture

```
┌──────────────────────────────────────┐
│      GGUFLoader                       │
├──────────────────────────────────────┤
│ loadFromFile(path) → Model     ✅    │
│ loadFromBlob(key) → Model      ❌    │
│ saveToBlob(model, key) → bool  ❌    │
└────────────┬─────────────────────────┘
             │
             ├─→ RocksDB Blob Store
             │   (Large Binary Objects)
             │
             └─→ Model Metadata Collection
                 (model_id, version, size, etc.)
```

### Implementation Steps

#### Step 1: Blob Store Integration

```cpp
bool GGUFLoader::loadToRocksDB(const std::string& model_path, const std::string& blob_key) {
    // 1. Read GGUF file
    std::ifstream file(model_path, std::ios::binary);
    if (!file) {
        return false;
    }
    
    // 2. Get file size
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    
    // 3. Read in chunks to avoid memory exhaustion
    const size_t CHUNK_SIZE = 64 * 1024 * 1024; // 64 MB chunks
    std::vector<uint8_t> buffer(CHUNK_SIZE);
    
    rocksdb::WriteBatch batch;
    size_t offset = 0;
    
    while (file.read(reinterpret_cast<char*>(buffer.data()), CHUNK_SIZE)) {
        size_t bytes_read = file.gcount();
        
        // Store chunk with key: blob:<blob_key>:chunk:<offset>
        std::string chunk_key = fmt::format("blob:{}:chunk:{}", blob_key, offset);
        batch.Put(chunk_key, rocksdb::Slice(reinterpret_cast<char*>(buffer.data()), bytes_read));
        
        offset += bytes_read;
    }
    
    // 4. Store metadata
    ModelMetadata metadata;
    metadata.blob_key = blob_key;
    metadata.total_size = file_size;
    metadata.chunk_count = (file_size + CHUNK_SIZE - 1) / CHUNK_SIZE;
    metadata.created_at = getCurrentTimestamp();
    
    batch.Put(fmt::format("blob:{}:metadata", blob_key), serializeMetadata(metadata));
    
    // 5. Commit batch
    rocksdb::WriteOptions opts;
    auto status = db_->Write(opts, &batch);
    
    return status.ok();
}
```

#### Step 2: Loading from Blob Store

```cpp
std::unique_ptr<Model> GGUFLoader::loadFromBlob(const std::string& blob_key) {
    // 1. Read metadata
    std::string metadata_key = fmt::format("blob:{}:metadata", blob_key);
    std::string metadata_value;
    auto status = db_->Get(rocksdb::ReadOptions(), metadata_key, &metadata_value);
    if (!status.ok()) {
        return nullptr;
    }
    
    auto metadata = deserializeMetadata(metadata_value);
    
    // 2. Read chunks and reconstruct file
    std::vector<uint8_t> model_data;
    model_data.reserve(metadata.total_size);
    
    for (size_t i = 0; i < metadata.chunk_count; ++i) {
        std::string chunk_key = fmt::format("blob:{}:chunk:{}", blob_key, i * CHUNK_SIZE);
        std::string chunk_data;
        
        status = db_->Get(rocksdb::ReadOptions(), chunk_key, &chunk_data);
        if (!status.ok()) {
            return nullptr;
        }
        
        model_data.insert(model_data.end(), chunk_data.begin(), chunk_data.end());
    }
    
    // 3. Load model from memory buffer
    return loadFromMemory(model_data);
}
```

#### Step 3: Model Metadata Management

```json
{
  "blob_key": "llama-2-7b-chat-v1",
  "model_name": "Llama-2-7B-Chat",
  "version": "1.0.0",
  "total_size": 13476770816,
  "chunk_count": 201,
  "chunk_size": 67108864,
  "format": "GGUF",
  "quantization": "Q4_K_M",
  "created_at": "2026-01-11T15:00:00Z",
  "accessed_at": "2026-01-11T15:30:00Z",
  "access_count": 42,
  "sha256": "abc123..."
}
```

## 📝 Implementation Tasks

### Milestone 1: Core Blob Storage (Week 1)

- [ ] Implement chunked blob writing to RocksDB
- [ ] Implement chunked blob reading from RocksDB
- [ ] Implement metadata storage/retrieval
- [ ] Add compression option (zstd)
- [ ] Add unit tests for blob storage

### Milestone 2: GGUF Integration (Week 1-2)

- [ ] Implement `loadToRocksDB(path, key)`
- [ ] Implement `loadFromBlob(key)`
- [ ] Implement `deleteBlob(key)`
- [ ] Implement `listBlobs()`
- [ ] Add integration tests with real GGUF files

### Milestone 3: Model Management (Week 2)

- [ ] Add REST API endpoints:
  - `POST /api/v1/models/upload` - Upload model to blob store
  - `GET /api/v1/models` - List models
  - `GET /api/v1/models/{key}` - Get model metadata
  - `DELETE /api/v1/models/{key}` - Delete model
- [ ] Add model versioning support
- [ ] Add model garbage collection
- [ ] Add documentation

### Milestone 4: Performance & Optimization (Week 2)

- [ ] Implement LRU cache for frequently used models
- [ ] Implement streaming load for large models
- [ ] Optimize chunk size based on benchmarks
- [ ] Add progress reporting for uploads
- [ ] Performance benchmarks

## 🔗 Dependencies & Related Issues

### Related
- RocksDB Blob Store already exists
- GGUF Loader partially implemented
- Model loading from file system works

### Future Enhancements
- Distributed model storage across shards
- Model replication
- Automatic model compression

## 📊 Success Criteria

### Functional Requirements
- ✅ Models can be stored in RocksDB Blob Store
- ✅ Models can be loaded from Blob Store
- ✅ Models can be deleted from Blob Store
- ✅ Metadata accurately tracks model info

### Technical Metrics
- ✅ Upload speed > 100 MB/s
- ✅ Load speed > 100 MB/s  
- ✅ Memory overhead < 100 MB during load
- ✅ Supports models up to 100 GB

### Quality Gates
- ✅ Unit tests > 85% coverage
- ✅ Integration tests with real GGUF files (1B, 7B, 13B params)
- ✅ Performance benchmarks meet targets
- ✅ No memory leaks
- ✅ Code review approved

## 📅 Timeline Estimate

| Milestone | Duration | Deliverable |
|-----------|----------|-------------|
| Core Blob Storage | 3-4 days | Chunked read/write |
| GGUF Integration | 3-4 days | Load/save GGUF |
| Model Management | 3-4 days | REST API + versioning |
| Performance | 2-3 days | Optimization + benchmarks |
| **Total** | **1-2 weeks** | **Feature complete** |

## ✅ Definition of Done

- [ ] `GGUFLoader::loadToRocksDB()` fully implemented
- [ ] `GGUFLoader::loadFromBlob()` fully implemented
- [ ] Model metadata storage working
- [ ] REST API endpoints implemented
- [ ] Unit tests passing
- [ ] Integration tests with real models passing
- [ ] Performance benchmarks meet targets
- [ ] Documentation updated
- [ ] Code review approved
- [ ] TODO comment removed from `gguf_loader.cpp:173`

---

**Created**: 2026-01-11  
**Verified**: 2026-01-11 (TODO confirmed in `gguf_loader.cpp:173`)  
**Target Version**: v1.4.0  
**Priority**: MEDIUM  
**Component**: LLM / Storage
