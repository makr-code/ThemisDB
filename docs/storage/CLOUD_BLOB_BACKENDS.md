# Cloud Blob Storage Backends

**Version:** 1.0.0  
**Date:** February 9, 2026  
**Category:** 💾 Storage

---

## Overview

ThemisDB supports multiple cloud blob storage backends for storing large binary objects (BLOBs) such as images, videos, documents, and backups. The plugin architecture allows seamless integration with various cloud providers and storage protocols.

## Supported Backends

### 1. AWS S3

**Amazon Simple Storage Service (S3)** - Industry-standard object storage.

#### Features

- **Server-side encryption**: AES-256, KMS, SSE-C
- **Automatic retry**: Exponential backoff for transient failures
- **Multipart upload**: Efficient handling of large files (>5GB)
- **Storage classes**: Standard, Intelligent-Tiering, Glacier, Deep Archive
- **Cross-region replication**: Automatic data redundancy
- **Lifecycle policies**: Automatic tier transitions and expiration

#### Configuration

```cpp
BlobStorageConfig config;
config.enable_s3 = true;
config.s3_bucket = "themisdb-blobs";
config.s3_region = "eu-central-1";
config.s3_prefix = "production/";
config.s3_storage_class = "STANDARD_IA";  // Infrequent Access
```

#### Authentication

AWS credentials can be provided via:
1. Environment variables: `AWS_ACCESS_KEY_ID`, `AWS_SECRET_ACCESS_KEY`
2. AWS credentials file: `~/.aws/credentials`
3. IAM role (EC2, ECS, EKS)
4. Instance metadata service

#### Dependencies

- **aws-sdk-cpp** >= 1.11.0
- **OpenSSL** >= 1.1.1

#### Example Usage

```cpp
auto s3_backend = std::make_unique<BlobBackendS3>(config);
auto blob_id = s3_backend->store(data, size, metadata);
auto retrieved_data = s3_backend->retrieve(blob_id);
```

---

### 2. Azure Blob Storage

**Microsoft Azure Blob Storage** - Enterprise-grade cloud object storage.

#### Features

- **Blob tiers**: Hot, Cool, Archive
- **Soft delete**: Configurable retention for deleted blobs
- **Versioning**: Automatic blob version tracking
- **Lifecycle management**: Automated tier transitions
- **Geo-redundancy**: LRS, ZRS, GRS, RA-GRS
- **Immutable storage**: WORM (Write Once, Read Many) compliance

#### Configuration

```cpp
BlobStorageConfig config;
config.enable_azure = true;
config.azure_account_name = "themisdbstorage";
config.azure_container = "blobs";
config.azure_tier = "Cool";
```

#### Authentication

- **Connection string**: Complete authentication in one string
- **Shared Access Signature (SAS)**: Time-limited, scoped access
- **Azure AD**: Service principal or managed identity

#### Dependencies

- **azure-storage-cpp** >= 7.5.0
- **cpprestsdk** >= 2.10.0

#### Example Usage

```cpp
auto azure_backend = std::make_unique<BlobBackendAzure>(config);
azure_backend->store(blob_id, data, size, {{"tier", "Archive"}});
```

---

### 3. Filesystem Backend

**Local or NFS filesystem** - Simple, portable storage option.

#### Features

- **Local storage**: Fast access for single-node deployments
- **NFS support**: Network-attached storage for multi-node setups
- **Directory hierarchy**: Automatic organization by prefix
- **Atomic writes**: Rename-based atomic operations
- **Compression**: Optional transparent compression

#### Configuration

```cpp
BlobStorageConfig config;
config.enable_filesystem = true;
config.filesystem_root = "/var/lib/themisdb/blobs";
config.filesystem_compression = true;
```

#### Directory Structure

```
/var/lib/themisdb/blobs/
├── 00/
│   ├── 00123456.blob
│   └── 00789abc.blob
├── 01/
│   └── 01fedcba.blob
└── metadata/
    └── index.db
```

#### Use Cases

- Development and testing
- Single-node production deployments
- Edge computing scenarios
- Air-gapped environments

---

### 4. WebDAV Backend

**Web Distributed Authoring and Versioning (WebDAV)** - HTTP-based file access.

#### Features

- **HTTP/HTTPS**: Standard web protocol
- **Authentication**: Basic, Digest, Bearer token
- **Versioning**: Optional server-side versioning
- **Locking**: Exclusive and shared locks
- **Metadata**: Extended attributes via properties

#### Configuration

```cpp
BlobStorageConfig config;
config.enable_webdav = true;
config.webdav_url = "https://webdav.example.com/themisdb/";
config.webdav_username = "admin";
config.webdav_password = "secret";
```

#### Use Cases

- Integration with existing WebDAV servers (Nextcloud, ownCloud)
- Corporate file servers
- Content management systems

---

## Blob Storage Manager

The `BlobStorageManager` provides a unified interface across all backends.

### Key Operations

```cpp
class BlobStorageManager {
public:
    // Store a blob, returns blob_id
    std::string store(const void* data, size_t size, 
                     const std::map<std::string, std::string>& metadata = {});
    
    // Retrieve a blob by id
    std::vector<uint8_t> retrieve(const std::string& blob_id);
    
    // Delete a blob
    bool remove(const std::string& blob_id);
    
    // Check if blob exists
    bool exists(const std::string& blob_id);
    
    // Get blob metadata
    std::map<std::string, std::string> getMetadata(const std::string& blob_id);
    
    // List blobs with prefix
    std::vector<std::string> list(const std::string& prefix = "");
};
```

### Backend Selection

Backends are selected based on configuration priority:

1. Primary backend (configurable)
2. Fallback backends (for redundancy)
3. Read-only backends (for migration)

## Redundancy Management

The `BlobRedundancyManager` provides RAID-like redundancy across backends.

### Redundancy Strategies

#### RAID-1 (Mirroring)

Store identical copies on multiple backends:

```cpp
BlobRedundancyConfig redundancy;
redundancy.strategy = RedundancyStrategy::MIRROR;
redundancy.backends = {"s3_primary", "azure_backup"};
redundancy.min_write_confirmations = 2;
```

#### Erasure Coding

Store data with parity for space efficiency:

```cpp
BlobRedundancyConfig redundancy;
redundancy.strategy = RedundancyStrategy::ERASURE_CODING;
redundancy.data_shards = 4;
redundancy.parity_shards = 2;  // Can lose any 2 shards
```

### Auto-Recovery

- **Detection**: Periodic integrity checks
- **Repair**: Automatic reconstruction from redundant copies
- **Alerting**: Notifications for degraded states

## Performance Considerations

### Caching

- **In-memory cache**: LRU cache for frequently accessed blobs
- **Disk cache**: SSD-backed cache for medium-sized blobs
- **Tiered caching**: Memory → SSD → Cloud

### Compression

- **Transparent compression**: Automatic for text and JSON
- **Selective compression**: Based on content type
- **Algorithms**: LZ4 (fast), Zstd (balanced), LZMA (high ratio)

### Async Operations

```cpp
// Async store
auto future = blob_manager->storeAsync(data, size);
auto blob_id = future.get();

// Batch operations
auto blob_ids = blob_manager->storeBatch(blob_vector);
```

## Security

### Encryption

- **At-rest encryption**: Backend-provided or client-side
- **In-transit encryption**: TLS/HTTPS for all transfers
- **Key management**: Integration with KMS (AWS KMS, Azure Key Vault)

### Access Control

- **Backend policies**: IAM roles, RBAC
- **Client-side validation**: Signature verification
- **Audit logging**: All access logged for compliance

## Monitoring

### Metrics

- **Throughput**: Bytes uploaded/downloaded per second
- **Latency**: P50, P95, P99 operation times
- **Error rates**: Failed operations by type
- **Storage usage**: Total size per backend

### Health Checks

```cpp
auto health = blob_manager->getHealth();
// {
//   "s3": {"status": "healthy", "latency_ms": 45},
//   "azure": {"status": "degraded", "latency_ms": 230}
// }
```

## See Also

- [Blob Storage Source Code](../../src/storage/)
  - `blob_storage_manager.cpp`
  - `blob_backend_s3.cpp`
  - `blob_backend_azure.cpp`
  - `blob_backend_filesystem.cpp`
  - `blob_backend_webdav.cpp`
  - `blob_redundancy_manager.cpp`
- [Cloud Blob Backends (DE)](../de/storage/storage_cloud_backends.md) - Detailed German documentation
- [Blob Redundancy (DE)](../de/storage/storage_blob_redundancy.md) - Redundancy strategies
- [Storage Module Overview](README.md)

---

**Version:** 1.0.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
