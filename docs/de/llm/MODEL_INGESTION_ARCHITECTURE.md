# Model Ingestion Architecture

## Overview

ThemisDB provides a robust model ingestion pipeline for uploading, storing, and distributing large language models across a distributed cluster. The architecture supports models up to 50 GB, automatic Raft replication, versioning, and memory-mapped loading for optimal performance.

**Key Features**:
- Chunked streaming upload (handles large files)
- RocksDB blob storage with Raft replication
- URN-based addressing for versioning
- Memory-mapped zero-copy loading
- Automatic deduplication
- Cross-shard distribution

## Why Not Regular Entity Adapter?

| Feature | Entity Adapter | Blob Store (Recommended) |
|---------|----------------|--------------------------|
| **Max Size** | ~100 MB | 50+ GB |
| **Streaming** | ✗ No | ✅ Yes |
| **Memory Efficiency** | ✗ Load to RAM | ✅ Memory-mapped |
| **Replication** | Manual | ✅ Automatic (Raft) |
| **Versioning** | Basic | ✅ Full history |
| **Deduplication** | ✗ No | ✅ Automatic |
| **Performance** | Medium | ✅ High (zero-copy) |

**Conclusion**: Entity Adapter is unsuitable for LLM models (6-50 GB). Blob Store is specifically designed for large binary data.

## Ingestion Strategies

### Strategy 1: File-Based Loading (Development)

**Use Case**: Local development, small deployments

**Method**: Direct file path reference

```bash
# Via HTTP API
curl -X POST http://localhost:8080/api/v1/llm/models/load \
  -H "Authorization: Bearer $TOKEN" \
  -H "Content-Type: application/json" \
  -d '{
    "model_id": "mistral-7b",
    "path": "/models/mistral-7b.gguf",
    "options": {"n_gpu_layers": 32}
  }'
```

```aql
-- Via AQL
LLM MODEL LOAD 'mistral-7b'
  FROM '/models/mistral-7b.gguf'
  OPTIONS {n_gpu_layers: 32};
```

**Pros**:
- Simple, fast (no upload)
- Good for development

**Cons**:
- No replication (manual deployment to all shards)
- No versioning
- File must exist on each node
- Not suitable for production

### Strategy 2: Blob Storage Ingestion (Recommended for Production)

**Use Case**: Production deployments, multi-shard clusters

**Method**: Upload to RocksDB blob store with automatic Raft replication

#### HTTP Upload

```bash
# Chunked upload (handles large files)
curl -X POST http://localhost:8080/api/v1/llm/models/ingest \
  -H "Authorization: Bearer $TOKEN" \
  -F "model_id=llama-3-8b" \
  -F "file=@/local/llama-3-8b.gguf" \
  -F 'metadata={
    "version": "v1.0",
    "description": "Llama 3 8B Q4 quantized",
    "shard_affinity": "legal",
    "replicate": true
  }'
```

#### Python SDK

```python
from themis import ThemisClient

client = ThemisClient(url="http://localhost:8080", token="...")

# Upload with progress tracking
response = client.llm.ingest_model(
    model_id="llama-3-8b",
    source="/local/llama-3-8b.gguf",
    version="v1.0",
    description="Llama 3 8B Q4 quantized",
    shard_affinity="legal",
    replicate=True,
    progress_callback=lambda pct: print(f"Upload: {pct}%")
)

print(f"URN: {response.urn}")
print(f"Checksum: {response.checksum}")
print(f"Replicated to {response.shards_replicated}/{response.total_shards} shards")
```

#### AQL

```aql
LLM MODEL INGEST 'llama-3-8b'
  FROM BLOB '/local/llama-3-8b.gguf'
  VERSION 'v1.0'
  REPLICATE TO ALL;
```

**Pros**:
- ✅ Automatic Raft replication to all shards
- ✅ Versioning and history
- ✅ URN addressing: `urn:themis:model:llama-3-8b:v1`
- ✅ Deduplication (shared base models)
- ✅ Centralized management

**Cons**:
- Upload time (mitigated by chunking)
- Storage overhead (mitigated by deduplication)

### Strategy 3: External Storage Integration (Enterprise)

**Use Case**: Large-scale deployments, CDN distribution

**Method**: S3/MinIO/Azure Blob integration with local caching

```yaml
# Configuration
llm:
  model_storage:
    type: s3
    endpoint: https://s3.amazonaws.com
    bucket: themis-models
    region: us-east-1
    access_key: AKIAIOSFODNN7EXAMPLE
    secret_key: wJalrXUtnFEMI/K7MDENG/bPxRfiCYEXAMPLEKEY
    cache_local: true
    cache_size_gb: 100
```

```aql
-- Pull from S3 on-demand
LLM MODEL LOAD 'llama-3-8b'
  FROM 's3://themis-models/llama-3-8b.gguf'
  OPTIONS {n_gpu_layers: 32};
```

**Pros**:
- ✅ Centralized storage (no per-shard duplication)
- ✅ CDN distribution (fast downloads)
- ✅ Cost-effective (object storage pricing)
- ✅ Automatic versioning (S3 versioning)

**Cons**:
- Network dependency (first load)
- Additional complexity

## Blob Storage Architecture

### Data Flow

```
┌─────────────────┐
│  Client Upload  │
│  (Chunked HTTP) │
└────────┬────────┘
         │
         ▼
┌─────────────────────────────────────┐
│  Model Ingestion Service             │
│  - Validate GGUF format              │
│  - Calculate checksum (SHA-256)      │
│  - Chunk into 4 MB blocks            │
│  - Generate URN                      │
└────────┬────────────────────────────┘
         │
         ▼
┌─────────────────────────────────────┐
│  RocksDB Blob Store (Leader)        │
│  - Key: model:blob:{id}:{version}   │
│  - Value: Binary chunks              │
│  - Metadata: model:meta:{id}         │
└────────┬────────────────────────────┘
         │
         ▼ Raft Replication
┌─────────────────────────────────────┐
│  Follower Shards (1..N)              │
│  - Automatic replication             │
│  - Eventually consistent             │
│  - Local blob store                  │
└────────┬────────────────────────────┘
         │
         ▼ On-demand
┌─────────────────────────────────────┐
│  LazyModelLoader                     │
│  - Memory-mapped loading             │
│  - Zero-copy from blob store         │
│  - LRU caching                       │
└─────────────────────────────────────┘
```

### RocksDB Key-Value Schema

#### Model Metadata

**Key**: `model:metadata:{model_id}`

**Value** (JSON):
```json
{
  "model_id": "llama-3-8b",
  "version": "v1.0",
  "urn": "urn:themis:model:llama-3-8b:v1",
  "format": "GGUF",
  "size_bytes": 8500000000,
  "checksum": "sha256:abc123def456...",
  "chunk_size": 4194304,
  "total_chunks": 2024,
  "architecture": "llama",
  "n_layers": 32,
  "n_embd": 4096,
  "uploaded_at": "2024-01-15T10:30:00Z",
  "uploaded_by": "user@example.com",
  "description": "Llama 3 8B Q4 quantized",
  "tags": ["llama", "8b", "q4"],
  "shard_affinity": "legal"
}
```

#### Model Blob Data

**Key**: `model:blob:{model_id}:{version}:{chunk_index}`

**Value**: Binary data (4 MB chunks)

Example keys:
- `model:blob:llama-3-8b:v1:0000`
- `model:blob:llama-3-8b:v1:0001`
- `model:blob:llama-3-8b:v1:0002`
- ...
- `model:blob:llama-3-8b:v1:2023`

#### Model Index (Per Shard)

**Key**: `model:index:{shard_id}`

**Value** (JSON):
```json
{
  "shard_id": "shard-1",
  "models": [
    {
      "model_id": "llama-3-8b",
      "version": "v1.0",
      "status": "replicated",
      "local_path": null,
      "blob_available": true,
      "last_accessed": "2024-01-15T12:45:30Z"
    },
    {
      "model_id": "mistral-7b",
      "version": "v2.0",
      "status": "replicated",
      "blob_available": true
    }
  ]
}
```

#### Version History

**Key**: `model:versions:{model_id}`

**Value** (JSON):
```json
{
  "model_id": "llama-3-8b",
  "versions": [
    {
      "version": "v1.0",
      "urn": "urn:themis:model:llama-3-8b:v1",
      "uploaded_at": "2024-01-15T10:30:00Z",
      "size_bytes": 8500000000,
      "status": "active"
    },
    {
      "version": "v1.1",
      "urn": "urn:themis:model:llama-3-8b:v1.1",
      "uploaded_at": "2024-02-01T14:20:00Z",
      "size_bytes": 8520000000,
      "status": "active"
    }
  ],
  "latest": "v1.1"
}
```

## Ingestion Process (Step-by-Step)

### 1. Client Initiates Upload

```python
# Python SDK
client.llm.ingest_model(
    model_id="llama-3-8b",
    source="/local/llama-3-8b.gguf",
    version="v1.0"
)
```

### 2. Server-Side Validation

```cpp
// Model Ingestion Service (pseudo-code)
class ModelIngestionService {
    Status validateUpload(const UploadRequest& req) {
        // Check format
        if (!isValidGGUF(req.file)) {
            return Status::INVALID_FORMAT;
        }
        
        // Check size
        if (req.file_size > MAX_MODEL_SIZE) {
            return Status::TOO_LARGE;
        }
        
        // Check authentication
        if (!hasPermission(req.token, Permission::UPLOAD_MODEL)) {
            return Status::UNAUTHORIZED;
        }
        
        // Check duplicate
        if (modelExists(req.model_id, req.version)) {
            return Status::ALREADY_EXISTS;
        }
        
        return Status::OK;
    }
};
```

### 3. Chunked Streaming Upload

```cpp
class ModelIngestionService {
    Status processChunkedUpload(const std::string& model_id,
                                 const std::string& version,
                                 std::istream& file_stream) {
        const size_t CHUNK_SIZE = 4 * 1024 * 1024;  // 4 MB
        std::vector<char> buffer(CHUNK_SIZE);
        
        size_t chunk_index = 0;
        SHA256 checksum;
        
        while (file_stream.read(buffer.data(), CHUNK_SIZE) || file_stream.gcount() > 0) {
            size_t bytes_read = file_stream.gcount();
            
            // Update checksum
            checksum.update(buffer.data(), bytes_read);
            
            // Store chunk in RocksDB
            std::string key = fmt::format("model:blob:{}:{}:{:04d}",
                                         model_id, version, chunk_index);
            
            rocksdb_->Put(WriteOptions(),
                         key,
                         Slice(buffer.data(), bytes_read));
            
            chunk_index++;
            
            // Report progress
            reportProgress(chunk_index, total_chunks);
        }
        
        // Store metadata
        storeMetadata(model_id, version, chunk_index, checksum.finalize());
        
        return Status::OK;
    }
};
```

### 4. Raft Replication

```cpp
// Automatic replication via Raft
class RaftReplicator {
    void replicateModelBlob(const std::string& model_id,
                           const std::string& version) {
        // Get all blob chunks
        auto chunks = getModelChunks(model_id, version);
        
        // Replicate to all follower shards
        for (const auto& chunk : chunks) {
            // Raft log entry
            RaftLogEntry entry{
                .type = LogEntryType::MODEL_BLOB,
                .key = chunk.key,
                .value = chunk.value
            };
            
            // Append to Raft log (automatically replicated)
            raft_->appendEntry(entry);
        }
        
        // Update replication status
        updateReplicationStatus(model_id, version,
                               ReplicationStatus::COMPLETE);
    }
};
```

### 5. Memory-Mapped Loading

```cpp
// LazyModelLoader uses memory-mapped I/O
class LazyModelLoader {
    void* loadModelFromBlobStore(const std::string& model_id,
                                  const std::string& version) {
        // Get metadata
        auto metadata = getModelMetadata(model_id, version);
        
        // Create temporary file for mmap
        std::string temp_path = fmt::format("/tmp/themis-model-{}-{}.gguf",
                                           model_id, version);
        
        // Reassemble chunks into temporary file
        std::ofstream temp_file(temp_path, std::ios::binary);
        
        for (size_t i = 0; i < metadata.total_chunks; i++) {
            std::string key = fmt::format("model:blob:{}:{}:{:04d}",
                                         model_id, version, i);
            
            std::string chunk_data;
            rocksdb_->Get(ReadOptions(), key, &chunk_data);
            
            temp_file.write(chunk_data.data(), chunk_data.size());
        }
        temp_file.close();
        
        // Memory-map the file (zero-copy)
        int fd = open(temp_path.c_str(), O_RDONLY);
        void* mapped = mmap(nullptr, metadata.size_bytes,
                           PROT_READ, MAP_PRIVATE, fd, 0);
        
        // Mark for deletion on close
        unlink(temp_path.c_str());
        
        return mapped;
    }
};
```

## Deduplication Strategy

### Base Model Sharing

Multiple LoRAs can share the same base model:

```
Shard 1:
  - mistral-7b:v1 (6 GB) ← Shared
  - legal-qa LoRA (20 MB)
  - medical-qa LoRA (20 MB)
  Total: 6.04 GB (vs 12 GB without sharing)
```

### Implementation

```cpp
class ModelDeduplicator {
    std::string getOrCreateBaseModel(const std::string& base_model_id) {
        // Check if base model already exists
        if (modelExists(base_model_id)) {
            // Return existing URN
            return getModelURN(base_model_id);
        }
        
        // Model doesn't exist, trigger ingestion
        return ingestBaseModel(base_model_id);
    }
};
```

## Versioning

### URN Format

```
urn:themis:model:{model_id}:{version}
```

Examples:
- `urn:themis:model:llama-3-8b:v1`
- `urn:themis:model:llama-3-8b:v1.1`
- `urn:themis:model:mistral-7b:v2.0`

### Version Management

```aql
-- Load specific version
LLM MODEL LOAD 'llama-3-8b'
  FROM BLOB 'urn:themis:model:llama-3-8b:v1';

-- Load latest version (default)
LLM MODEL LOAD 'llama-3-8b'
  FROM BLOB 'urn:themis:model:llama-3-8b:latest';

-- List all versions
FOR version IN model_versions('llama-3-8b')
  RETURN {
    version: version.version,
    size_gb: version.size_bytes / 1e9,
    uploaded: version.uploaded_at
  };
```

## Shard Affinity

Route specific models to specific shards:

```python
# Upload with shard affinity
client.llm.ingest_model(
    model_id="legal-specialist",
    source="/local/legal-specialist.gguf",
    version="v1.0",
    shard_affinity="legal",  # Only replicate to "legal" shard
    replicate=False
)
```

**Use Case**: Domain-specific models stay on domain shards

```
Cluster:
  Shard "legal": legal-specialist model
  Shard "medical": medical-specialist model
  Shard "finance": finance-specialist model
  Shard "general": mistral-7b (replicated to all)
```

## Performance Optimizations

### 1. Chunked Upload

- **Chunk Size**: 4 MB (optimal for network + storage)
- **Parallel Uploads**: Multiple chunks in flight
- **Resume Support**: Continue from last chunk on failure

### 2. Memory-Mapped Loading

- **Zero-Copy**: No data copied to RAM
- **Lazy Loading**: OS loads pages on-demand
- **Shared Memory**: Multiple processes share same pages

### 3. Compression

```python
# Upload with compression
client.llm.ingest_model(
    model_id="llama-3-8b",
    source="/local/llama-3-8b.gguf",
    compress=True,  # Use zstd compression
    compression_level=3
)
```

**Savings**: 20-30% reduction in storage and transfer time

### 4. Delta Updates

For model updates, only transfer changed chunks:

```python
# Update model (delta upload)
client.llm.ingest_model(
    model_id="llama-3-8b",
    source="/local/llama-3-8b-updated.gguf",
    version="v1.1",
    delta_from="v1.0"  # Only upload changed chunks
)
```

## Monitoring & Metrics

### Ingestion Metrics

```python
# Get ingestion statistics
stats = client.llm.get_ingestion_stats()

print(f"Total models: {stats.total_models}")
print(f"Total size: {stats.total_size_gb:.1f} GB")
print(f"Avg upload time: {stats.avg_upload_time_seconds:.1f}s")
print(f"Replication success rate: {stats.replication_success_rate:.1%}")
```

### Replication Status

```python
# Check replication status
status = client.llm.get_replication_status("llama-3-8b", "v1.0")

print(f"Status: {status.status}")
print(f"Replicated: {status.shards_replicated}/{status.total_shards}")
print(f"Progress: {status.progress_percent}%")
```

## Security Considerations

### 1. Authentication

All uploads require Bearer Token authentication:

```bash
curl -X POST http://localhost:8080/api/v1/llm/models/ingest \
  -H "Authorization: Bearer $TOKEN" \
  -F "file=@model.gguf"
```

### 2. Authorization

Permissions required:
- `llm:model:upload` - Upload models
- `llm:model:replicate` - Trigger replication
- `llm:model:delete` - Delete models

### 3. Validation

- GGUF format validation (magic bytes)
- Checksum verification (SHA-256)
- Size limits (max 50 GB)
- Malware scanning (optional)

### 4. Encryption

- TLS for upload (HTTPS/gRPC with TLS)
- At-rest encryption (RocksDB encryption)
- Checksum integrity verification

## Best Practices

1. **Use Blob Storage for Production** - Automatic replication, versioning
2. **Enable Compression** - 20-30% storage savings
3. **Set Shard Affinity** - Keep domain models on domain shards
4. **Version Your Models** - Easy rollback and A/B testing
5. **Monitor Replication** - Ensure all shards have the model
6. **Use Delta Updates** - Faster updates for model refinements
7. **Clean Old Versions** - Remove unused versions to save space
8. **Memory-Map Loading** - Zero-copy performance

## Troubleshooting

### Upload Failures

```python
try:
    response = client.llm.ingest_model(...)
except UploadError as e:
    print(f"Upload failed: {e}")
    print(f"Chunks uploaded: {e.chunks_uploaded}/{e.total_chunks}")
    print(f"Last chunk: {e.last_chunk_index}")
    
    # Resume upload
    response = client.llm.resume_ingest(
        model_id="llama-3-8b",
        from_chunk=e.last_chunk_index + 1
    )
```

### Replication Issues

```bash
# Check replication status
curl -X GET http://localhost:8080/api/v1/llm/models/llama-3-8b/replication \
  -H "Authorization: Bearer $TOKEN"

# Manually trigger replication
curl -X POST http://localhost:8080/api/v1/llm/models/llama-3-8b/replicate \
  -H "Authorization: Bearer $TOKEN" \
  -d '{"shards": ["shard-2", "shard-3"]}'
```

### Storage Space

```python
# Check storage usage
stats = client.llm.get_storage_stats()

print(f"Used: {stats.used_gb:.1f} GB")
print(f"Total: {stats.total_gb:.1f} GB")
print(f"Available: {stats.available_gb:.1f} GB")

# Clean up old versions
client.llm.delete_model_version("llama-3-8b", "v1.0")
```

## Future Enhancements

- **P2P Replication**: Peer-to-peer model distribution (BitTorrent-style)
- **Model Registry**: Central catalog of available models
- **Auto-Scaling**: Automatic model distribution based on load
- **Multi-Region**: Geo-distributed model replication
- **CDN Integration**: Edge caching for faster downloads
- **Model Marketplace**: Share and discover models
