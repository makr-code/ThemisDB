# Storage Module Documentation

**Version:** 1.0.0  
**Date:** February 9, 2026  
**Category:** 💾 Storage

---

## 📑 Table of Contents

- [Overview](#overview)
- [Core Components](#core-components)
- [Storage Architecture](#storage-architecture)
- [Blob Storage](#blob-storage)
- [Backup & Recovery](#backup--recovery)
- [Index Maintenance](#index-maintenance)
- [Audit Reports](#audit-reports)
- [Related Documentation](#related-documentation)

---

## Overview

The Storage Module provides the foundational data persistence layer for ThemisDB, built on RocksDB with advanced features for entity storage, blob management, backup/recovery, and index maintenance.

## Core Components

### BaseEntity

The `BaseEntity` abstraction provides a unified interface for storing and retrieving objects in ThemisDB.

**Key Features:**
- Serialization/deserialization of entities
- MVCC (Multi-Version Concurrency Control) support
- Schema versioning
- Type safety and validation

**Documentation:**
- [BaseEntity Architecture (DE)](../de/architecture/architecture_base_entity.md) - Architectural overview
- [BaseEntity Source Documentation (DE)](../de/src/storage/base_entity.cpp.md) - Implementation details

**Source Files:**
- Header: `include/storage/base_entity.h`
- Implementation: `src/storage/base_entity.cpp`

### KeySchema

Key schema management for encoding and organizing keys in RocksDB.

**Key Features:**
- Hierarchical key prefixes for logical separation
- Entity keys: `entity:<table>:<pk>`
- Secondary indexes: `idx:<table>:<column>:<value>:<pk>`
- Range indexes: `ridx:<table>:<column>:<value>:<pk>`
- Graph adjacency: `graph:out:<from_pk>:<edge_id>`, `graph:in:<to_pk>:<edge_id>`
- Vector indexes: `vector:<table>:<pk>`
- Time-series: `ts:<metric>:<timestamp>:<tags>`

**Documentation:**
- [KeySchema Source Documentation (DE)](../de/src/storage/key_schema.cpp.md)

**Source Files:**
- Header: `include/storage/key_schema.h`
- Implementation: `src/storage/key_schema.cpp`

### RocksDB Wrapper

Wrapper around RocksDB TransactionDB providing:
- Transaction management
- Column family management
- Snapshot isolation
- Write-ahead logging (WAL)
- Compaction strategies

**Documentation:**
- [RocksDB Wrapper Source Documentation (DE)](../de/src/storage/rocksdb_wrapper.cpp.md)
- [RocksDB Storage Layout](rocksdb_layout.md) - Key prefixes and physical layout
- [RocksDB Storage Operations (DE)](../de/storage/storage_rocksdb.md) - Detailed operations guide
- [RocksDB Optimization Guide (EN)](../en/storage/ROCKSDB_OPTIMIZATION_GUIDE.md)

**Source Files:**
- Header: `include/storage/rocksdb_wrapper.h`
- Implementation: `src/storage/rocksdb_wrapper.cpp`

## Storage Architecture

### Column Families

ThemisDB uses RocksDB column families to logically separate different data types:
- `cf_entities` - Primary entity storage
- `cf_indexes` - Secondary and composite indexes
- `cf_graph` - Graph adjacency lists
- `cf_changefeed` - Change data capture events
- `cf_ts` - Time-series data
- `cf_vector` - Vector index metadata

### Write-Ahead Log (WAL)

The WAL ensures durability and crash recovery:
- All writes are first logged to WAL
- Configurable sync strategies for latency vs. durability trade-offs
- Automatic WAL pruning after checkpoints

### Snapshots & MVCC

Snapshot isolation provides consistent reads:
- Read operations see a fixed point-in-time view
- No read locks required
- Supports long-running read transactions

## Blob Storage

ThemisDB supports multiple blob storage backends for large binary data.

### Cloud Blob Backends

Support for cloud storage providers:

**Documentation:**
- [Cloud Blob Storage Backends](CLOUD_BLOB_BACKENDS.md) - Consolidated overview of all blob backends
- [Cloud Blob Storage Backends (DE)](../de/storage/storage_cloud_backends.md) - S3 and Azure integration
- [Blob Redundancy Management (DE)](../de/storage/storage_blob_redundancy.md) - RAID-like redundancy

**Supported Backends:**
- **AWS S3** - Server-side encryption, multipart uploads, cross-region replication
- **Azure Blob Storage** - Blob tiers, lifecycle management
- **Filesystem** - Local and NFS storage
- **WebDAV** - HTTP-based storage access

**Source Files:**
- Manager: `include/storage/blob_storage_manager.h`, `src/storage/blob_storage_manager.cpp`
- Backends:
  - `include/storage/blob_storage_backend.h` (interface)
  - `include/storage/blob_backend_filesystem.h`, `src/storage/blob_backend_filesystem.cpp`
  - `src/storage/blob_backend_s3.cpp`
  - `src/storage/blob_backend_azure.cpp`
  - `src/storage/blob_backend_webdav.cpp`
- Redundancy: `include/storage/blob_redundancy_manager.h`, `src/storage/blob_redundancy_manager.cpp`

## Backup & Recovery

Comprehensive backup and point-in-time recovery capabilities.

### Backup Types

- **Full Backup** - Complete database snapshot using RocksDB checkpoint API
- **Incremental Backup** - Changes since last backup
- **Differential Backup** - Changes since last full backup

### Point-in-Time Recovery (PITR)

Git-like recovery with named snapshots:
- Create semantic tags at important points
- Restore to any previous sequence, tag, or timestamp
- Dry-run preview before applying
- Selective table restoration

**Documentation:**
- [Backup & Recovery System](../backup_recovery_system.md) - Complete backup guide
- [Point-in-Time Recovery (EN)](../en/features/features_pitr.md) - PITR usage guide
- [RAID5 Backup Features (EN)](../en/features/features_raid5_backup.md)
- [RAID5 Backup Features (DE)](../de/features/features_raid5_backup.md)

**Source Files:**
- Backup Manager: `include/storage/backup_manager.h`, `src/storage/backup_manager.cpp`
- PITR Manager: `include/storage/pitr_manager.h`, `src/storage/pitr_manager.cpp`

## Index Maintenance

Index defragmentation, rebuild, and maintenance operations.

**Key Features:**
- Index statistics collection
- Defragmentation and compaction
- Index rebuilding
- Maintenance scheduling

**Documentation:**
- [Index Maintenance (DE)](../de/features/features_index_maintenance.md) - Maintenance operations guide
- [Index Backup (DE)](../de/features/features_index_backup.md)

**Source Files:**
- Header: `include/storage/index_maintenance.h`
- Implementation: `src/storage/index_maintenance.cpp`

## Audit Reports

### RocksDB Wrapper Audit

Systematic security and correctness analysis of the RocksDB wrapper implementation.

**Documentation:**
- [RocksDB Wrapper Audit Report](../Audit/ROCKSDB_WRAPPER_AUDIT_REPORT.md) - Comprehensive audit findings

**Report Contents:**
- Critical security issues identified
- Memory safety violations
- Transaction consistency issues
- Performance optimizations
- Recommended fixes

## Related Documentation

### Architecture
- [Architecture Overview](../../ARCHITECTURE.md)
- [Storage External Analysis (DE)](../de/storage/storage_external_analysis.md)
- [Geo Schema Storage (DE)](../de/storage/storage_geo_schema.md)

### Features
- [Replication Module (EN)](../en/storage/README.md) - Leader-follower and multi-master replication
- [MVCC & Transactions](../en/features/README.md)

### Implementation Summaries
- [Cloud Storage Implementation Summary](../../CLOUD_STORAGE_IMPLEMENTATION_SUMMARY.md)
- [LoRA Storage Backend Completion](../implementation-history/LORA_STORAGE_BACKEND_COMPLETION.md)

### Build & Operations
- [Dependencies](../../DEPENDENCIES.md)
- [Build Guide](../../README.md#building)

---

**Version:** 1.0.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
