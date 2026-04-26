> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · docs/de/storage/README.md -->

# ThemisDB Storage Module

## Module Purpose

The Storage module provides ThemisDB's persistent data layer, built on RocksDB for high-performance LSM-tree storage with MVCC support. It handles all aspects of data persistence including key-value storage, blob management, backup/recovery, compression, encryption, and transaction coordination.

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `rocksdb_wrapper.cpp` | RocksDB wrapper with MVCC, WAL, BlobDB, and async I/O |
| `mvcc_store.cpp` | Multi-version concurrency control snapshot management |
| `wal_storage.cpp` | Write-Ahead Log management and replay |
| `backup_manager.cpp` | Backup creation and point-in-time recovery |
| `pitr_manager.cpp` | Point-in-time recovery via WAL replay and snapshot restore |
| `storage_engine.cpp` | Storage engine with dependency injection (`storage_engine.h` public API) |
| `key_schema.cpp` | Unified multi-model key encoding (relational/doc/graph/vector/timeseries) |
| `base_entity.cpp` | Base type for all storage-layer entities |
| `batch_write_optimizer.cpp` | Adaptive write batching to reduce write amplification |
| `compaction_manager.cpp` | Manual and scheduled RocksDB compaction control |
| `compression_strategy.cpp` | Pluggable per-table compression (Snappy, Zstd, LZ4, Brotli) |
| `compressed_storage.cpp` | Transparent compression/decompression layer |
| `columnar_format.cpp` | Columnar storage for analytical workloads |
| `blob_backend_filesystem.cpp` | Local filesystem blob backend |
| `blob_backend_s3.cpp` | Amazon S3 blob backend |
| `blob_backend_azure.cpp` | Azure Blob Storage backend |
| `blob_backend_gcs.cpp` | Google Cloud Storage blob backend (requires `THEMIS_ENABLE_GCS`) |
| `blob_backend_webdav.cpp` | WebDAV blob backend |
| `blob_redundancy_manager.cpp` | RAID-1 mirror redundancy across multiple backends |
| `database_connection_manager.cpp` | Connection pooling and lifecycle management |
| `disk_space_monitor.cpp` | Real-time disk quota monitoring and alerting |
| `index_maintenance.cpp` | Background index rebuild, optimize, and consistency checks |
| `history_manager.cpp` | Version history and change tracking per key |
| `hlc.cpp` | Hybrid Logical Clock for causally consistent timestamps |
| `merge_operators.cpp` | Custom RocksDB merge operators (counters, list appends) |
| `raft_mvcc_bridge.cpp` | Integration between Raft consensus log and MVCC storage |
| `security_signature.cpp` | Field-level AES-GCM encryption primitives |
| `security_signature_manager.cpp` | HMAC-SHA256 tamper detection and signature management |
| `storage_audit_logger.cpp` | Structured audit trail for all storage operations |
| `tiered_storage.cpp` | Hot/warm/cold tiered storage with automatic data migration |
| `transaction_retry_manager.cpp` | Exponential backoff retry for failed transactions |
| `nlp_metadata_extractor.cpp` | Automatic metadata extraction for ingested documents |
| `adaptive_compaction.cpp` | Machine-learning-based adaptive compaction scheduling |
| `columnar_cache.cpp` | LRU in-memory columnar segment cache for analytics acceleration |
| `concurrent_write_controller.cpp` | Bounded FIFO write-concurrency semaphore (`ConcurrentWriteController`) |
| `distributed_transaction_manager.cpp` | Two-phase commit coordinator for cross-shard atomicity |
| `encrypted_blob_backend.cpp` | AES-256-GCM encryption wrapper for any blob backend |
| `erasure_coder_factory.cpp` | Factory for Reed-Solomon erasure codec selection |
| `erasure_coding_backend.cpp` | Reed-Solomon erasure coding backend (PARITY mode) |
| `gpu_compression.cpp` | GPU-accelerated compression via CUDA/ROCm (nvCOMP, fallback to CPU) |
| `mvcc_chain_pruner.cpp` | MVCC version-chain pruning for old snapshot cleanup |
| `nvme_manager.cpp` | NVMe device management: io_uring, multi-queue, ZNS, Direct I/O |
| `online_schema_migration.cpp` | Zero-downtime online DDL via `SchemaMigrator` |
| `schema_dead_weight_detector.cpp` | Detects unused schema fields and stale indexes; GDPR field registry |
| `simd_filter.cpp` | AVX-512/AVX2/NEON/scalar SIMD-accelerated columnar predicate filtering |
| `storage_layout_advisor.cpp` | Recommends optimal storage layout based on access patterns |
| `storage_parquet_exporter.cpp` | Native Apache Parquet v2 export from columnar format |
| `streaming_ingest_manager.cpp` | Ring-buffer streaming ingest with configurable flush thread |
| `vector_index_backend.cpp` | Vector index storage backend (`IVectorIndexBackend`, `InMemoryVectorIndex`) |
| `wom_tree.cpp` | Write-Optimized Merge (WOM/Bε) Tree for write-heavy workloads |
| `zero_copy_blob_transfer.cpp` | Zero-copy blob transfer via mmap and sendfile |

## Scope

**In Scope:**
- LSM-tree key-value storage (RocksDB wrapper and configuration)
- Multi-model key schema (relational, document, graph, vector, timeseries)
- Large object storage (BlobDB and external blob backends)
- Backup and point-in-time recovery (PITR)
- Compression strategies and columnar formats
- Field-level encryption and security signatures
- Index maintenance and optimization
- Transaction management and retry logic
- Storage engine abstraction with dependency injection

**Out of Scope:**
- Query parsing and execution (handled by query module)
- Network protocols and APIs (handled by server module)
- Authentication and authorization (handled by auth module)
- Specific application logic (handled by higher layers)

## Key Components

### RocksDBWrapper
**Location:** `rocksdb_wrapper.cpp`, `../include/storage/rocksdb_wrapper.h`

High-level wrapper around RocksDB TransactionDB providing MVCC, WAL, and BlobDB integration.

**Features:**
- **MVCC Support**: Multi-version concurrency control via RocksDB transactions
- **LSM-Tree Tuning**: Configurable memtable size, block cache, compaction
- **BlobDB Integration**: Automatic offloading of large values (>4KB by default)
- **WAL Management**: Write-ahead logging with separate directory support
- **Multi-Path SSTables**: Distribute SSTable files across multiple NVMe drives
- **Read-Only Mode**: Safe read access without WAL updates (v1.4.0+)
- **Async I/O**: Prefetching for 2-5x scan performance improvement (v1.3.0+)
- **CPU Prefetch Hints**: Software prefetch for random access patterns (v1.4.1+)

**Thread Safety:**
- Read-safe: Multiple concurrent readers
- Write-safe: Internal locking for concurrent writes
- Iterator-safe: Reference counting prevents use-after-free
- NOT move-safe: Move only during initialization/teardown

**Configuration Example:**
```cpp
RocksDBWrapper::Config config;
config.db_path = "./data/rocksdb";
config.wal_dir = "./data/wal";           // Separate WAL directory
config.memtable_size_mb = 512;            // 512MB memtable
config.block_cache_size_mb = 1024;        // 1GB block cache
config.enable_blobdb = true;              // Enable BlobDB
config.blob_size_threshold = 4096;        // Files >4KB go to BlobDB
config.max_background_jobs = 4;           // Compaction threads
config.enable_async_io = true;            // Enable async I/O
config.async_io_readahead_size_mb = 128;  // 128MB readahead

auto db = std::make_unique<RocksDBWrapper>(config);
db->open();
```

**Performance Characteristics:**
- Point reads: 10-50μs (with cache)
- Sequential scans: 100K-500K keys/sec
- Writes: 50K-200K ops/sec (with WAL)
- Write amplification: 10-30x (LSM-tree characteristic)
- Space amplification: 1.2-2.0x (depends on compaction)

### Key Schema System
**Location:** `key_schema.cpp`, `../include/storage/key_schema.h`

Unified key encoding scheme supporting all data models in a single RocksDB instance.

**Key Formats (v1.5.0+):**
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

**Benefits:**
- Single RocksDB instance for all data models
- Efficient prefix scans per table/collection
- Natural ordering for range queries
- Simple backup (entire key space)

**Usage:**
```cpp
// Encode a key
std::string key = KeySchema::encodeRelationalKey("users", "user123");

// Decode a key
auto [model, table, pk] = KeySchema::decodeKey(key);

// Iterate over a collection
auto it = db->createIterator();
it->seek(KeySchema::collectionPrefix("users"));
while (it->valid() && it->key().starts_with(KeySchema::collectionPrefix("users"))) {
    // Process document
    it->next();
}
```

### Blob Storage System

#### BlobStorageManager
**Location:** See `../include/storage/blob_storage_manager.h`

Orchestrates multiple blob storage backends with automatic selection based on size.

**Selection Strategy:**
```
< inline_threshold (default: 1KB)        → INLINE (store in RocksDB value)
< rocksdb_blob_threshold (default: 1MB)  → ROCKSDB_BLOB (BlobDB)
>= rocksdb_blob_threshold                → External backend (S3/Azure/Filesystem/WebDAV)
```

**Supported Backends:**

| Backend | Location | Use Case |
|---------|----------|----------|
| **INLINE** | RocksDB value | Small data (<1KB) |
| **ROCKSDB_BLOB** | BlobDB (.blob files) | Medium files (1KB-1MB) |
| **FILESYSTEM** | Local disk | Large files, development |
| **S3** | AWS S3 | Production, distributed storage |
| **AZURE_BLOB** | Azure Blob Storage | Azure deployments |
| **WEBDAV** | WebDAV server | Custom storage, NextCloud/OwnCloud |
| **GCS** | Google Cloud Storage | GCP deployments |

**BlobRef Structure:**
```cpp
struct BlobRef {
    std::string blob_id;        // Unique identifier
    BlobStorageType type;       // Storage backend type
    std::string uri;            // Backend-specific URI
    size_t size_bytes;          // Blob size
    std::string sha256_hash;    // Integrity check
    std::string compression;    // Compression algorithm
    std::map<std::string, std::string> metadata;  // Custom metadata
};
```

**Usage:**
```cpp
BlobStorageConfig config;
config.enable_s3 = true;
config.s3_bucket = "my-bucket";
config.inline_threshold_bytes = 1024;
config.rocksdb_blob_threshold_bytes = 1024 * 1024;

BlobStorageManager manager(config);
manager.registerBackend(BlobStorageType::S3, std::make_shared<S3Backend>(s3_config));

// Store blob (automatic backend selection)
std::vector<uint8_t> data = /* ... */;
BlobRef ref = manager.put("blob-123", data);

// Retrieve blob
auto result = manager.get(ref);
```

#### Blob Redundancy Manager
**Location:** `blob_redundancy_manager.cpp`

RAID-like redundancy across multiple blob backends for high availability.

**Redundancy Modes:**
- **Mirror (RAID-1)**: Write to multiple backends, read from any
- **Erasure Coding**: Future enhancement for space-efficient redundancy

**Example:**
```cpp
BlobRedundancyManager redundancy;
redundancy.addBackend(s3_backend);
redundancy.addBackend(azure_backend);
redundancy.addBackend(filesystem_backend);

// Automatically writes to all backends
redundancy.putWithRedundancy("blob-123", data);

// Reads from first available backend
auto result = redundancy.getWithRedundancy("blob-123");
```

### Storage Engine
**Location:** `storage_engine.cpp`, `../include/storage/storage_engine.h`

High-level storage abstraction with dependency injection for query evaluation, encryption, and indexing.

**Dependency Injection Pattern:**
```cpp
class StorageEngine : public IStorageEngine {
public:
    StorageEngine(
        IExpressionEvaluatorPtr evaluator,    // Query filtering
        IFieldEncryptionPtr encryption,       // Field-level encryption
        IKeyProviderPtr key_provider,         // Key management
        IIndexManagerPtr index_manager        // Index coordination
    );

    // Storage operations
    Result<void> put(const std::string& key, const std::string& value);
    Result<std::string> get(const std::string& key);
    Result<void> del(const std::string& key);

    // Filter-aware operations
    bool apply_filter(const std::string& filter_expr, const void* context);

    // Encryption operations
    std::vector<uint8_t> encrypt_field(const std::string& field_name,
                                        const std::vector<uint8_t>& plaintext);
};
```

**Production Mode Safety:**
```bash
export THEMIS_PRODUCTION_MODE=1
# Prevents use of default (no-op) encryption/key providers
# Throws error if insecure defaults are detected
```

**Factory Method (for backward compatibility):**
```cpp
// Creates StorageEngine with default implementations
// WARNING: Not production-safe! Use DI constructor instead.
auto storage = StorageEngine::createDefault();
```

## Backup & Recovery

### Backup Manager
**Location:** `backup_manager.cpp`, `../include/storage/backup_manager.h`

Incremental backup system with versioning and validation.

**Features:**
- Incremental backups (only changed SSTables)
- Full backups on demand
- Backup verification with checksums
- Restore to specific backup ID
- Automatic cleanup of old backups

**Usage:**
```cpp
BackupManager backup(db);

// Create incremental backup
backup.createBackup(false);  // false = incremental

// Create full backup
backup.createBackup(true);   // true = full

// List available backups
auto backups = backup.listBackups();

// Restore from backup
backup.restoreFromBackup(backup_id, restore_path);

// Cleanup old backups (keep last N)
backup.cleanupOldBackups(keep_count);
```

#### Point-in-Time Recovery (PITR) Manager
**Location:** `pitr_manager.cpp`, `../include/storage/pitr_manager.h`

Snapshot-based point-in-time recovery with configurable retention.

**Features:**
- Automatic snapshot creation
- Restore to any past timestamp
- Configurable snapshot retention policy
- WAL-based recovery between snapshots

**Usage:**
```cpp
PITRManager pitr(db);

// Create snapshot
pitr.createSnapshot();

// Restore to timestamp
auto timestamp = std::chrono::system_clock::now() - std::chrono::hours(24);
pitr.restoreToTimestamp(timestamp);

// List available snapshots
auto snapshots = pitr.listSnapshots();
```

### Compression & Optimization

#### Compression Strategy
**Location:** `compression_strategy.cpp`, `../include/storage/compression_strategy.h`

Pluggable compression algorithms for different data types.

**Supported Algorithms:**
- **None**: No compression (fast access)
- **Snappy**: Fast compression/decompression
- **Zstd**: High compression ratio
- **LZ4**: Ultra-fast compression
- **Brotli**: Web-optimized compression

**Per-Table Configuration:**
```cpp
CompressionStrategy strategy;
strategy.setTableCompression("users", CompressionType::SNAPPY);
strategy.setTableCompression("logs", CompressionType::ZSTD);
strategy.setTableCompression("cache", CompressionType::NONE);
```

#### Columnar Format
**Location:** `columnar_format.cpp`, `../include/storage/columnar_format.h`

Columnar storage for analytical workloads.

**Benefits:**
- Better compression (similar values together)
- Faster scans (read only needed columns)
- Vectorized processing support

**Usage:**
```cpp
ColumnarFormat columnar;
columnar.writeColumnar(table_name, rows);
auto result = columnar.scanColumn(table_name, column_name, predicate);
```

#### Batch Write Optimizer
**Location:** `batch_write_optimizer.cpp`, `../include/storage/batch_write_optimizer.h`

Automatic batching of writes to reduce write amplification.

**Strategies:**
- Group writes by key prefix
- Merge operations to same key
- Delay flushes to accumulate writes
- Adaptive batch sizes based on load

### Security Components

#### Field Encryption
**Location:** `security_signature.cpp`, `../include/storage/security_signature.h`

Field-level encryption before storage.

**Features:**
- Per-field encryption configuration
- AES-GCM encryption
- Key rotation support
- Selective field encryption

#### Security Signature Manager
**Location:** `security_signature_manager.cpp`, `../include/storage/security_signature_manager.h`

Digital signatures for data integrity.

**Features:**
- HMAC-SHA256 signatures
- Signature verification on read
- Tamper detection
- Key management integration

### Additional Components

#### Database Connection Manager
**Location:** `database_connection_manager.cpp`

Connection pooling and lifecycle management for database connections.

#### Disk Space Monitor
**Location:** `disk_space_monitor.cpp`

Real-time disk space monitoring with quota enforcement.

#### Index Maintenance
**Location:** `index_maintenance.cpp`

Background index rebuilding, optimization, and consistency checks.

#### Transaction Retry Manager
**Location:** `transaction_retry_manager.cpp`

Automatic retry logic for failed transactions with exponential backoff.

#### Merge Operators
**Location:** `merge_operators.cpp`

Custom RocksDB merge operators for efficient counter updates and list appends.

## Architecture

### Layered Architecture

```
┌───────────────────────────────────────────────────────────────┐
│                    Storage Engine API                          │
│  (High-level abstraction with dependency injection)           │
└───────────────────────────────────────────────────────────────┘
                              ↓
┌───────────────────────────────────────────────────────────────┐
│                    Key Schema Layer                            │
│  (Multi-model key encoding: rel:, doc:, node:, vec:, etc.)   │
└───────────────────────────────────────────────────────────────┘
                              ↓
┌───────────────────────────────────────────────────────────────┐
│                   RocksDB Wrapper Layer                        │
│  (MVCC, WAL, BlobDB, Transactions, Iterators)                │
└───────────────────────────────────────────────────────────────┘
                              ↓
┌───────────────────────────────────────────────────────────────┐
│                      RocksDB Engine                            │
│  (LSM-Tree, Compaction, MemTable, SSTables)                   │
└───────────────────────────────────────────────────────────────┘
                              ↓
┌────────────────┬────────────────────────┬─────────────────────┐
│  Local Disk    │   BlobDB (.blob files) │  External Blobs     │
│  (SSTables)    │   (large values)       │  (S3/Azure/WebDAV)  │
└────────────────┴────────────────────────┴─────────────────────┘
```

### Data Flow

**Write Path:**
```
1. Application writes key-value
2. StorageEngine encrypts fields (if configured)
3. Key Schema encodes key (e.g., "doc:users:user123")
4. RocksDBWrapper writes to memtable
5. WAL logs write for durability
6. Memtable flush to L0 SSTable
7. Background compaction to L1-L6
8. Large values offloaded to BlobDB/S3
9. Backup Manager captures incremental changes
```

**Read Path:**
```
1. Application requests key
2. Key Schema encodes lookup key
3. RocksDBWrapper checks block cache
4. If miss, check memtable
5. If miss, check SSTables (L0-L6)
6. If BlobRef, fetch from blob storage
7. StorageEngine decrypts fields
8. Return value to application
```

### Thread Safety Model

**RocksDBWrapper:**
- Multiple concurrent readers (thread-safe)
- Multiple concurrent writers (internal mutex)
- Iterator reference counting (prevents use-after-free)
- Move/copy not thread-safe (initialization only)

**BlobStorageManager:**
- Thread-safe backend registration
- Thread-safe put/get operations
- Internal mutex for backend map

**StorageEngine:**
- Thread-safe operations (delegates to RocksDBWrapper)
- Dependency injection during construction only

## Integration Points

### With Core Module
Uses ConcernsContext for observability:
```cpp
storage.setConcerns(concerns_context);
// Enables logging, tracing, metrics for storage operations
```

### With Query Module
Provides IExpressionEvaluator interface for query filtering:
```cpp
StorageEngine storage(query_evaluator, encryption, keys, index_manager);
// Query engine evaluates WHERE clauses during scans
```

### With Index Module
Coordinates with index manager for secondary indexes:
```cpp
storage.put("doc:users:user123", data);
// Automatically updates secondary indexes via IIndexManager
```

### With Security Module
Integrates field-level encryption:
```cpp
StorageEngine storage(evaluator, field_encryption, key_provider, index_manager);
// Automatically encrypts sensitive fields before storage
```

## API/Usage Examples

### Basic Storage Operations

```cpp
#include "storage/storage_engine.h"
#include "storage/rocksdb_wrapper.h"

// Create RocksDB wrapper
RocksDBWrapper::Config config;
config.db_path = "./data";
config.enable_blobdb = true;

auto db = std::make_unique<RocksDBWrapper>(config);
db->open();

// Create storage engine with dependencies
auto storage = std::make_shared<StorageEngine>(
    expression_evaluator,
    field_encryption,
    key_provider,
    index_manager
);

// Store data
storage->put("doc:users:user123", R"({"name":"Alice","email":"alice@example.com"})");

// Retrieve data
auto result = storage->get("doc:users:user123");
if (result) {
    std::cout << "User data: " << *result << "\n";
}

// Delete data
storage->del("doc:users:user123");
```

### Transaction Example

```cpp
// Begin transaction
auto tx = db->beginTransaction();

// Write operations
tx->put("account:alice", "balance:1000");
tx->put("account:bob", "balance:500");

// Transfer money
auto alice_balance = tx->get("account:alice");
auto bob_balance = tx->get("account:bob");
tx->put("account:alice", "balance:900");
tx->put("account:bob", "balance:600");

// Commit atomically
tx->commit();
```

### Blob Storage Example

```cpp
#include "storage/blob_storage_manager.h"

BlobStorageConfig config;
config.enable_s3 = true;
config.s3_bucket = "my-bucket";

BlobStorageManager blob_manager(config);
blob_manager.registerBackend(BlobStorageType::S3, s3_backend);

// Store large file
std::vector<uint8_t> file_data = readFile("document.pdf");
BlobRef ref = blob_manager.put("document-123", file_data);

// Store ref in RocksDB
storage->put("doc:documents:123", ref.serialize());

// Later, retrieve file
auto ref_str = storage->get("doc:documents:123");
BlobRef ref = BlobRef::deserialize(*ref_str);
auto file_data = blob_manager.get(ref);
```

### Backup & Recovery Example

```cpp
#include "storage/backup_manager.h"

BackupManager backup(db);

// Create daily backups
backup.createBackup(false);  // Incremental

// Disaster recovery
backup.restoreFromBackup(backup_id, "./restore");
```

## Dependencies

### Internal Dependencies
- **themis/base/interfaces**: Storage interface definitions
- **core/concerns**: Logging, tracing, metrics
- **utils/expected**: Result types
- **utils/tracing**: Tracing utilities

### External Dependencies
- **RocksDB** (required): LSM-tree storage engine
- **fmt** (required): String formatting
- **spdlog** (optional): Logging
- **AWS SDK** (optional): S3 backend
- **Azure SDK** (optional): Azure Blob backend
- **libcurl** (optional): WebDAV backend

### Build Configuration
```cmake
# Link storage module
target_link_libraries(my_app themis-storage)

# Dependencies
find_package(RocksDB REQUIRED)
find_package(fmt REQUIRED)

# Optional blob backends
option(THEMIS_ENABLE_S3 "Enable S3 blob backend" ON)
option(THEMIS_ENABLE_AZURE "Enable Azure blob backend" ON)
option(THEMIS_ENABLE_WEBDAV "Enable WebDAV blob backend" ON)
```

## Performance Characteristics

### RocksDB Performance
- **Point reads**: 10-50μs (cached), 100-500μs (disk)
- **Sequential scans**: 100K-500K keys/sec
- **Writes**: 50K-200K ops/sec (with WAL), 500K+ ops/sec (no WAL)
- **Transactions**: 10K-50K tx/sec

### Write Amplification
- **LSM-tree characteristic**: 10-30x write amplification
- **Tuning**:
  - Larger memtables → less write-amp (fewer flushes)
  - Universal compaction → less write-amp
  - Level-based compaction → better read performance

### Space Amplification
- **LSM-tree characteristic**: 1.2-2.0x space usage
- **Compaction reduces space over time**
- **BlobDB**: ~1.0x (minimal overhead)

### Memory Usage
- **Memtable**: 512MB default (configurable)
- **Block cache**: 1GB default (configurable)
- **Write buffers**: 2GB total across all CFs
- **Total**: ~3.5GB minimum for production

### Tuning Recommendations
See [PERFORMANCE_TIPS.md](../../docs/PERFORMANCE_TIPS.md) for detailed tuning guide.

## Known Limitations

1. **LSM-Tree Characteristics**
   - High write amplification (10-30x)
   - Compaction CPU overhead
   - Point updates slower than key-value inserts

2. **RocksDB Constraints**
   - No distributed transactions (single-node only)
   - No built-in replication (use external replication)
   - Limited secondary index support (manual management)

3. **Blob Storage**
   - External backends require network latency
   - No automatic blob migration between backends
   - No erasure coding (mirroring only)

4. **Backup & Recovery**
   - Backups are point-in-time (not continuous)
   - Restore requires downtime
   - No online backup verification

5. **Encryption**
   - Field-level only (not full-database encryption)
   - No automatic key rotation
   - Performance overhead (5-15% for encryption)

6. **Default Implementations**
   - Default StorageEngine factory uses no-op encryption
   - Production mode prevents insecure defaults
   - Must use DI constructor for production

## Status

**Production Ready** (as of v2.0.0)

✅ **Stable Features:**
- RocksDB wrapper with MVCC, WAL, BlobDB
- Multi-model key schema
- BlobDB and external blob storage (S3, Azure, GCS, WebDAV, Filesystem)
- Backup and PITR
- Compression strategies (Snappy, Zstd, LZ4, Brotli)
- Transaction support with distributed 2PC
- Tiered storage (hot/warm/cold)
- Erasure coding for blob storage (Reed-Solomon RS(k,m))
- GPU-accelerated compression (CUDA/ROCm with CPU fallback)
- NVMe optimizations (io_uring, multi-queue, ZNS, Direct I/O)
- Write-Optimized Merge (WOM) Tree
- Streaming ingest pipeline (`StreamingIngestManager`)
- Columnar cache for analytics (`ColumnarCache`)
- SIMD-accelerated columnar filtering (`SIMDColumnFilter`)
- Native Parquet export from columnar format
- Online schema migration (`SchemaMigrator`)
- Zero-copy blob transfer (`ZeroCopyBlobTransfer`)
- Encrypted blob backend wrapper (`EncryptedBlobBackend`)
- Concurrent write controller (`ConcurrentWriteController`)
- Adaptive compaction scheduling (`AdaptiveCompactionScheduler`)

⚠️ **Beta Features:**
- `WomTree` as drop-in LSM replacement for write-heavy workloads
- Automatic index maintenance
- `StorageLayoutAdvisor` layout recommendations

🔬 **Experimental:**
- Schema dead-weight detector (`SchemaDeadWeightDetector`)
- MVCC chain pruner (`MVCCChainPruner`)
- Vector index backend (`IVectorIndexBackend` / `InMemoryVectorIndex`)

## Related Documentation

### Quick Links

- **Core Components:**
  - [Base Entity](../../docs/de/src/storage/base_entity.cpp.md) - Entity storage abstractions
  - [Key Schema](../../docs/de/src/storage/key_schema.cpp.md) - Key encoding and organization
  - [RocksDB Wrapper](../../docs/de/src/storage/rocksdb_wrapper.cpp.md) - RocksDB integration
- **Architecture:**
  - [RocksDB Layout](../../docs/storage/rocksdb_layout.md) - Key prefixes and physical layout
  - [RocksDB Storage Operations (DE)](../../docs/de/storage/storage_rocksdb.md) - Detailed operations guide
  - [Base Entity Architecture (DE)](../../docs/de/architecture/architecture_base_entity.md) - Architectural overview
  - [Storage Module Index](../../docs/storage/README.md) - Comprehensive storage documentation
- **Blob Storage:**
  - [Cloud Blob Backends](../../docs/storage/CLOUD_BLOB_BACKENDS.md) - S3, Azure, WebDAV, Filesystem
  - [Blob Redundancy (DE)](../../docs/de/storage/storage_blob_redundancy.md) - RAID-like redundancy
- **Operations:**
  - [Backup & Recovery](../../docs/backup_recovery_system.md) - Backup strategies and PITR
  - [Index Maintenance (DE)](../../docs/de/features/features_index_maintenance.md) - Index operations
  - [Performance Tips](../../docs/PERFORMANCE_TIPS.md) - RocksDB tuning guide
- **Audit Reports:**
  - [RocksDB Wrapper Audit](../../docs/ROCKSDB_WRAPPER_AUDIT_REPORT.md) - Security and correctness analysis

## Contributing

When contributing to the storage module:

1. Maintain thread safety guarantees
2. Add performance tests for new features
3. Update key schema documentation for new data types
4. Test backup/recovery with new features
5. Consider write/space amplification impact

For detailed contribution guidelines, see [CONTRIBUTING.md](../../CONTRIBUTING.md).

## See Also

- [ARCHITECTURE.md](ARCHITECTURE.md) - Component architecture and data flow diagrams
- [ROADMAP.md](ROADMAP.md) - Implementation phases and planned features
- [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md) - Planned storage improvements
- [Secondary Docs (docs/de)](../../../docs/de/storage/README.md) - German-language overview
- [Missing Implementations Report](../../../docs/de/storage/missing-implementations.md) - Reality-check findings
- [Core Module](../core/README.md) - Cross-cutting concerns
- [Server Module](../server/README.md) - Network protocols and APIs

## Scientific References

1. O'Neil, P., Cheng, E., Gawlick, D., & O'Neil, E. (1996). **The Log-Structured Merge-Tree (LSM-tree)**. *Acta Informatica*, 33(4), 351–385. https://doi.org/10.1007/s002360050048

2. Dong, S., Callaghan, M., Galanis, L., Borthakur, D., Savor, T., & Strum, M. (2017). **Optimizing Space Amplification in RocksDB**. *Proceedings of the 8th Biennial Conference on Innovative Data Systems Research (CIDR)*. https://www.cidrdb.org/cidr2017/papers/p82-dong-cidr17.pdf

3. Rosenblum, M., & Ousterhout, J. K. (1992). **The Design and Implementation of a Log-Structured File System**. *ACM Transactions on Computer Systems*, 10(1), 26–52. https://doi.org/10.1145/146941.146943

4. Reed, D. P. (1978). **Naming and Synchronization in a Decentralized Computer System** (Doctoral dissertation, MIT). https://dspace.mit.edu/handle/1721.1/14965

5. Graefe, G. (2010). **A Survey of B-Tree Locking Techniques**. *ACM Transactions on Database Systems*, 35(3), 16:1–16:26. https://doi.org/10.1145/1806907.1806908

## Installation

This module is built as part of ThemisDB. See the root `CMakeLists.txt` for build configuration.
