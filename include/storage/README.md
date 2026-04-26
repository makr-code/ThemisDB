> **Build:** `cmake --preset release && cmake --build build/release`

# ThemisDB Storage Module Headers

## Module Purpose

The Storage headers define the public interfaces and abstractions for ThemisDB's persistent data layer. These headers expose the storage engine's capabilities for LSM-tree key-value operations, blob management, backup/recovery, compression strategies, security features, and multi-model data access without requiring clients to depend on implementation details or RocksDB internals.

## Scope

**In Scope:**
- Storage engine interface definitions and abstractions
- RocksDB wrapper API for MVCC transactions and LSM-tree operations
- Multi-model key schema interfaces (relational, document, graph, vector, timeseries)
- Blob storage backend interfaces (filesystem, S3, Azure, WebDAV, GCS)
- Backup and PITR manager interfaces
- Compression strategy and columnar format APIs
- Security signature and field encryption interfaces
- Index maintenance and transaction retry APIs
- Database connection pooling interfaces
- Disk space monitoring interfaces

**Out of Scope:**
- RocksDB implementation details (internal to wrapper)
- Concrete blob backend implementations (see src/storage)
- Wire format specifications (handled by protocol modules)
- Query execution logic (handled by query module)
- Network transport mechanisms (handled by server module)

## Key Components

### Storage Engine Interface
**Location:** `storage_engine.h`

Primary abstraction for storage operations with dependency injection support.

**Purpose:**
- Provides pluggable storage engine implementation
- Enables dependency injection for query evaluation, encryption, and indexing
- Decouples storage from query and security modules
- Supports testing via mock implementations

**Key Features:**
```cpp
class StorageEngine : public IStorageEngine {
public:
    // Dependency injection constructor
    StorageEngine(
        IExpressionEvaluatorPtr evaluator,    // Query filtering
        IFieldEncryptionPtr encryption,       // Field-level encryption
        IKeyProviderPtr key_provider,         // Key management
        IIndexManagerPtr index_manager        // Index coordination
    );

    // Factory for backward compatibility (uses defaults)
    static std::shared_ptr<StorageEngine> createDefault();

    // Basic operations
    Result<void> put(const std::string& key, const std::string& value);
    Result<std::string> get(const std::string& key);
    Result<void> del(const std::string& key);

    // Filter-aware operations
    bool apply_filter(const std::string& filter_expr, const void* context);

    // Encryption operations
    std::vector<uint8_t> encrypt_field(const std::string& field_name,
                                        const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> decrypt_field(const std::string& field_name,
                                        const std::vector<uint8_t>& ciphertext);
};
```

**Design Patterns:**
- **Dependency Injection**: Constructor receives interfaces, not implementations
- **Factory Pattern**: `createDefault()` for quick setup
- **Strategy Pattern**: Pluggable encryption and evaluation strategies
- **Interface Segregation**: Separate concerns via focused interfaces

**Thread Safety:** All operations are thread-safe (delegates to RocksDBWrapper)

### RocksDB Wrapper
**Location:** `rocksdb_wrapper.h`

High-level wrapper around RocksDB TransactionDB providing MVCC, WAL, and BlobDB integration.

**Purpose:**
- Abstracts RocksDB complexity for application code
- Provides transactional MVCC operations
- Manages LSM-tree configuration and tuning
- Coordinates WAL and BlobDB
- Supports read-only mode for archives/backups

**Configuration Structure:**
```cpp
struct Config {
    std::string db_path = "./data/rocksdb";
    std::string wal_dir;                    // Separate WAL directory
    std::vector<DbPath> db_paths;           // Multi-device SSTable distribution

    bool read_only = false;                 // Read-only mode (v1.4.0+)
    size_t memtable_size_mb = 512;          // Write-amp optimization
    size_t block_cache_size_mb = 1024;      // Block cache size
    int block_cache_shard_bits = -1;        // Auto-sharding (-1 = auto)

    bool enable_wal = true;
    bool enable_blobdb = true;
    bool enable_statistics = true;
    size_t blob_size_threshold = 4096;      // >4KB → BlobDB

    int max_background_jobs = 4;
    int max_background_compactions = -1;    // Auto
    int max_background_flushes = -1;        // Auto

    // Async I/O optimization (v1.3.0+)
    bool enable_async_io = false;
    size_t async_io_readahead_size_mb = 0;

    // CPU prefetch hints (v1.4.1+)
    bool enable_cpu_prefetch = false;
    int cpu_prefetch_distance = 64;
};
```

**Key Operations:**
```cpp
class RocksDBWrapper {
public:
    // Lifecycle
    RocksDBWrapper(const Config& config);
    Result<void> open();
    void close();

    // Basic operations
    Result<void> put(const std::string& key, const std::string& value);
    Result<std::string> get(const std::string& key);
    Result<void> del(const std::string& key);

    // Batch operations
    Result<void> writeBatch(WriteBatch& batch);

    // Transactions
    std::unique_ptr<Transaction> beginTransaction();

    // Iterators (range queries)
    std::unique_ptr<Iterator> createIterator();
    std::unique_ptr<Iterator> createIterator(const ReadOptions& opts);

    // Snapshots (MVCC)
    const Snapshot* getSnapshot();
    void releaseSnapshot(const Snapshot* snapshot);

    // Statistics
    std::string getStatistics() const;
    void resetStatistics();
};
```

**Thread Safety Model:**
```cpp
/// @thread_safety
/// - **Read-safe**: Multiple concurrent readers
/// - **Write-safe**: Internal locking for concurrent writes
/// - **NOT move-safe**: Move only during init/teardown
/// - **NOT copyable**: Copy operations deleted
/// - **Iterator-safe**: Reference counting prevents use-after-free
/// - **close() waits**: Waits for active operations before shutdown
```

**Performance Characteristics:**
- Point reads: 10-50μs (cached), 100-500μs (disk)
- Sequential scans: 100K-500K keys/sec
- Writes: 50K-200K ops/sec (WAL enabled)
- Write amplification: 10-30x (LSM-tree characteristic)
- Space amplification: 1.2-2.0x

### Key Schema System
**Location:** `key_schema.h`

Unified key encoding scheme supporting all ThemisDB data models.

**Purpose:**
- Enable multi-model storage in single RocksDB instance
- Provide natural ordering for range queries
- Support efficient prefix scans per table/collection
- Simplify backup (single key space)

**Key Formats:**
```
Relational:  rel:table_name:pk_value
Document:    doc:collection_name:pk_value
Graph Node:  node:pk_value
Graph Edge:  edge:pk_value
Vector:      vec:object_name:pk_value
Timeseries:  ts:series_name:timestamp:pk_value

Secondary Index:  idx:table_name:field_name:field_value:pk_value
Graph Index:      gidx:from_id:edge_type:to_id
```

**API:**
```cpp
class KeySchema {
public:
    // Encoding
    static std::string encodeRelationalKey(const std::string& table,
                                           const std::string& pk);
    static std::string encodeDocumentKey(const std::string& collection,
                                         const std::string& pk);
    static std::string encodeGraphNodeKey(const std::string& node_id);
    static std::string encodeGraphEdgeKey(const std::string& edge_id);
    static std::string encodeVectorKey(const std::string& object_name,
                                       const std::string& pk);
    static std::string encodeTimeseriesKey(const std::string& series,
                                           uint64_t timestamp,
                                           const std::string& pk);

    // Decoding
    static std::tuple<DataModel, std::string, std::string> decodeKey(
        const std::string& key);

    // Prefix generation for range scans
    static std::string collectionPrefix(const std::string& name);
    static std::string tablePrefix(const std::string& name);

    // Validation
    static bool isValidKey(const std::string& key);
    static DataModel extractModel(const std::string& key);
};
```

### Blob Storage System

#### Blob Storage Manager
**Location:** `blob_storage_manager.h`

Orchestrates multiple blob storage backends with automatic size-based selection.

**Backend Selection Strategy:**
```
< inline_threshold (1KB)           → INLINE (RocksDB value)
< rocksdb_blob_threshold (1MB)     → ROCKSDB_BLOB (.blob files)
>= rocksdb_blob_threshold          → External backend (S3/Azure/etc)
```

**Supported Backends:**

| Backend | Header | Use Case |
|---------|--------|----------|
| INLINE | N/A | Small values (<1KB) |
| ROCKSDB_BLOB | N/A | Medium files (1KB-1MB) |
| FILESYSTEM | `blob_backend_filesystem.h` | Local disk, development |
| S3 | `blob_storage_backend.h` | AWS production |
| AZURE_BLOB | `blob_storage_backend.h` | Azure production |
| WEBDAV | `blob_storage_backend.h` | WebDAV servers |
| GCS | `blob_storage_backend.h` | Google Cloud |

**BlobRef Structure:**
```cpp
struct BlobRef {
    std::string blob_id;                     // Unique identifier
    BlobStorageType type;                    // Backend type
    std::string uri;                         // Backend-specific URI
    size_t size_bytes;                       // Blob size
    std::string sha256_hash;                 // Integrity verification
    std::string compression;                 // Compression algorithm
    std::map<std::string, std::string> metadata;  // Custom metadata

    std::string serialize() const;
    static BlobRef deserialize(const std::string& data);
};
```

**API:**
```cpp
class BlobStorageManager {
public:
    BlobStorageManager(const BlobStorageConfig& config);

    // Backend registration
    void registerBackend(BlobStorageType type,
                        std::shared_ptr<IBlobStorageBackend> backend);

    // Blob operations (automatic backend selection)
    BlobRef put(const std::string& blob_id,
                const std::vector<uint8_t>& data);
    Result<std::vector<uint8_t>> get(const BlobRef& ref);
    Result<void> del(const BlobRef& ref);

    // Statistics
    std::map<BlobStorageType, size_t> getBackendStats() const;
};
```

### Backup & Recovery

#### Backup Manager
**Location:** `backup_manager.h`

Incremental backup system with versioning and validation.

**Features:**
- Incremental backups (only changed SSTables)
- Full backups on demand
- Backup verification with checksums
- Restore to specific backup ID
- Automatic cleanup of old backups

**API:**
```cpp
class BackupManager {
public:
    BackupManager(std::shared_ptr<RocksDBWrapper> db);

    // Backup operations
    Result<uint32_t> createBackup(bool full = false);
    Result<void> restoreFromBackup(uint32_t backup_id,
                                   const std::string& restore_path);
    Result<void> verifyBackup(uint32_t backup_id);

    // Management
    std::vector<BackupInfo> listBackups() const;
    Result<void> deleteBackup(uint32_t backup_id);
    Result<void> cleanupOldBackups(size_t keep_count);

    // Statistics
    BackupStatistics getStatistics() const;
};
```

#### PITR Manager
**Location:** `pitr_manager.h`

Point-in-time recovery with snapshot management.

**API:**
```cpp
class PITRManager {
public:
    PITRManager(std::shared_ptr<RocksDBWrapper> db);

    // Snapshot management
    Result<std::string> createSnapshot();
    std::vector<SnapshotInfo> listSnapshots() const;
    Result<void> deleteSnapshot(const std::string& snapshot_id);

    // Recovery
    Result<void> restoreToTimestamp(
        std::chrono::system_clock::time_point timestamp);
    Result<void> restoreToSnapshot(const std::string& snapshot_id);

    // Configuration
    void setRetentionPolicy(std::chrono::hours retention);
    void enableAutoSnapshot(std::chrono::minutes interval);
};
```

### Security Components

#### Security Signature Manager
**Location:** `security_signature_manager.h`

Digital signatures for data integrity verification.

**Features:**
- HMAC-SHA256 signatures
- Signature verification on read
- Tamper detection
- Key management integration

**API:**
```cpp
class SecuritySignatureManager {
public:
    SecuritySignatureManager(std::shared_ptr<IKeyProvider> key_provider);

    // Signature operations
    std::string sign(const std::string& data);
    bool verify(const std::string& data, const std::string& signature);

    // Key management
    void rotateKey();
    std::string getCurrentKeyId() const;
};
```

## Architecture

### Header Organization

```
include/storage/
├── storage_engine.h              # Main storage abstraction
├── rocksdb_wrapper.h             # RocksDB MVCC wrapper
├── key_schema.h                  # Multi-model key encoding
│
├── blob_storage_manager.h        # Blob orchestration
├── blob_storage_backend.h        # Backend interface
├── blob_backend_filesystem.h     # Filesystem backend
├── blob_backend_gcs.h            # Google Cloud Storage backend
├── blob_redundancy_manager.h     # RAID-like redundancy
├── encrypted_blob_backend.h      # Encryption-at-rest blob backend
├── erasure_coding_backend.h      # Erasure-coded blob backend
├── zero_copy_blob_transfer.h     # Zero-copy blob transfer
│
├── backup_manager.h              # Incremental backups
├── pitr_manager.h                # Point-in-time recovery
├── history_manager.h             # Historical version management
│
├── compression_strategy.h        # Compression algorithms
├── columnar_format.h             # Columnar storage
├── columnar_cache.h              # Columnar read cache
├── batch_write_optimizer.h       # Write batching
├── gpu_compression.h             # GPU-accelerated compression
│
├── security_signature.h          # Field signatures
├── security_signature_manager.h  # Signature management
│
├── index_maintenance.h           # Index operations
├── transaction_retry_manager.h   # Transaction retries
├── distributed_transaction_manager.h # Distributed 2PC coordinator
├── database_connection_manager.h # Connection pooling
├── disk_space_monitor.h          # Disk monitoring
├── merge_operators.h             # RocksDB merge ops
├── base_entity.h                 # Entity abstraction
├── compressed_storage.h          # Compression layer
├── nlp_metadata_extractor.h      # NLP metadata
│
├── mvcc_store.h                  # MVCC storage layer
├── mvcc_chain_pruner.h           # MVCC version GC
├── raft_mvcc_bridge.h            # Raft/MVCC integration
├── hlc.h                         # Hybrid Logical Clock
├── wal_storage.h                 # Write-ahead log storage
│
├── adaptive_compaction.h         # Adaptive LSM compaction policy
├── compaction_manager.h          # Compaction lifecycle manager
├── concurrent_write_controller.h # Concurrent write coordination
├── nvme_manager.h                # NVMe device management
├── online_schema_migration.h     # Live schema migration
├── schema_dead_weight_detector.h # Unused schema detection
├── simd_filter.h                 # SIMD-accelerated row filtering
├── storage_audit_logger.h        # Storage operation audit log
├── storage_layout_advisor.h      # Storage layout recommendations
├── storage_parquet_exporter.h    # Parquet export
├── streaming_ingest_manager.h    # Streaming data ingestion
├── tiered_storage.h              # Hot/warm/cold tiering
├── vector_index_backend.h        # Vector index storage backend
└── wom_tree.h                    # Write-optimized merge tree
```

### Dependency Graph

```
Application Layer
       ↓
storage_engine.h (+ DI interfaces)
       ↓
┌──────────────┬─────────────────┬───────────────┐
↓              ↓                 ↓               ↓
rocksdb_     blob_storage_    backup_        security_
wrapper.h    manager.h         manager.h     signature.h
↓              ↓                 ↓               ↓
key_schema.h   ↓                 ↓               ↓
             blob_storage_     pitr_          compression_
             backend.h         manager.h      strategy.h
```

## Integration Points

### With Core Module
```cpp
// Accept ConcernsContext for observability
void StorageEngine::setConcerns(std::shared_ptr<ConcernsContext> concerns);
```

### With Query Module
```cpp
// Storage accepts IExpressionEvaluator
StorageEngine storage(query_evaluator, encryption, keys, index_manager);
```

### With Security Module
```cpp
// Field encryption integration
StorageEngine storage(evaluator, field_encryption, key_provider, index_manager);
```

## API/Usage Examples

### Basic Storage Setup

```cpp
#include "storage/storage_engine.h"
#include "storage/rocksdb_wrapper.h"

// Configure RocksDB
RocksDBWrapper::Config config;
config.db_path = "./data";
config.memtable_size_mb = 512;
config.block_cache_size_mb = 1024;
config.enable_blobdb = true;

auto db = std::make_shared<RocksDBWrapper>(config);
db->open();

// Create storage with dependency injection
auto storage = std::make_shared<StorageEngine>(
    expression_evaluator,  // Query filtering
    field_encryption,      // Field encryption
    key_provider,          // Key management
    index_manager          // Index coordination
);
```

### Multi-Model Data Storage

```cpp
#include "storage/key_schema.h"

// Relational data
std::string rel_key = KeySchema::encodeRelationalKey("customers", "cust-123");
storage->put(rel_key, row_data);

// Document data
std::string doc_key = KeySchema::encodeDocumentKey("orders", "order-456");
storage->put(doc_key, json_document);

// Graph data
std::string node_key = KeySchema::encodeGraphNodeKey("person-789");
storage->put(node_key, node_properties);

// Vector data
std::string vec_key = KeySchema::encodeVectorKey("images", "img-001");
storage->put(vec_key, embedding_vector);
```

## Dependencies

### Internal Dependencies
- **themis/base/interfaces**: Core interface definitions
- **utils/expected**: Result types for error handling
- **utils/tracing**: Tracing utilities

### External Dependencies
- **RocksDB**: LSM-tree storage engine (required)
- **nlohmann/json**: JSON handling (optional, for entities)
- **OpenSSL**: Cryptographic operations (optional, for signatures)

## Performance Characteristics

### RocksDB Wrapper Performance

- **Point reads**: 10-50μs (cached), 100-500μs (disk)
- **Sequential scans**: 100K-500K keys/sec
- **Writes**: 50K-200K ops/sec (WAL enabled)
- **Transactions**: 10K-50K tx/sec
- **Write amplification**: 10-30x (LSM-tree)
- **Space amplification**: 1.2-2.0x

### Blob Storage Performance

- **Inline**: Same as RocksDB value (no overhead)
- **BlobDB**: 1.5-2x slower reads, 0.8x write throughput
- **S3**: 50-200ms latency per blob
- **Local filesystem**: 1-5ms per blob

## Known Limitations

1. **RocksDB Constraints**
   - Single-node only (no distributed transactions)
   - No built-in replication
   - Limited cross-shard transactions

2. **Key Schema**
   - Fixed prefix format (cannot change after data stored)
   - No automatic key migration

3. **Blob Storage**
   - No automatic tiering between backends
   - No erasure coding (mirroring only)

4. **Backup & Recovery**
   - Restore requires downtime
   - No online verification during backup

5. **Thread Safety**
   - RocksDBWrapper not move-safe during operation
   - Iterator invalidation on concurrent writes

## Status

**Production Ready** (as of v1.5.0)

✅ **Stable Interfaces:**
- StorageEngine with dependency injection
- RocksDBWrapper MVCC operations
- Key schema encoding/decoding
- Blob storage manager
- Backup and PITR managers
- Compression strategies

⚠️ **Beta Interfaces:**
- Columnar format API
- NLP metadata extractor
- Adaptive index maintenance

🔬 **Experimental:**
- Distributed transaction support
- Erasure coding for blobs
- GPU-accelerated compression

## Related Documentation

- [Storage Implementation](../../src/storage/README.md) - Implementation details
- [RocksDB Layout](../../docs/storage/rocksdb_layout.md) - Physical key layout
- [Blob Storage Backends](../../docs/storage/CLOUD_BLOB_BACKENDS.md) - Backend implementations
- [Core Module](../core/README.md) - Cross-cutting concerns
- [Server Module](../server/README.md) - Network protocols

## Contributing

When contributing to storage headers:

1. **Maintain ABI Stability**
   - Do not change existing public method signatures
   - Add new methods, don't modify existing ones

2. **Document Thread Safety**
   - Clearly state thread-safety guarantees
   - Document move/copy semantics

3. **Use Forward Declarations**
   - Minimize include dependencies
   - Keep headers lightweight

4. **Follow Naming Conventions**
   - Interfaces start with `I` (e.g., `IStorageEngine`)
   - Manager classes end with `Manager`

For detailed guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

## See Also

- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned storage header improvements
- [Storage Implementation README](../../src/storage/README.md) - Implementation guide
- [Server Headers](../server/README.md) - Server interface documentation

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
