# Storage Module

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Storage

---

## Übersicht

Das Storage-Modul bildet das Fundament von ThemisDB und bietet eine hochperformante Abstraktionsschicht über RocksDB mit ACID-Transaktionen (MVCC), Kompression und BlobDB-Unterstützung.

## Source-Code Referenz

| Komponente | Header | Source | LOC | Beschreibung |
|------------|--------|--------|-----|--------------|
| RocksDBWrapper | `rocksdb_wrapper.h` | `rocksdb_wrapper.cpp` | ~1,600 | RocksDB Abstraction |
| BaseEntity | `base_entity.h` | `base_entity.cpp` | ~800 | Entity Storage |
| BlobRedundancyManager | `blob_redundancy_manager.h` | `blob_redundancy_manager.cpp` | ~700 | RAID-like Redundanz |
| KeySchema | `key_schema.h` | `key_schema.cpp` | ~400 | Key-Format-Definition |

**Gesamt:** 9 Header, 10 Source-Dateien, ~4,600 LOC

## RocksDBWrapper

### Konfiguration

```cpp
RocksDBWrapper::Config config;
config.db_path = "./data/rocksdb";
config.memtable_size_mb = 256;
config.block_cache_size_mb = 1024;
config.enable_wal = true;
config.enable_blobdb = true;
config.blob_size_threshold = 4096;  // >4KB → BlobDB
config.compression_default = "lz4";
config.compression_bottommost = "zstd";
```

### Features

- **MVCC Transactions:** Snapshot Isolation via RocksDB TransactionDB
- **LSM-Tree Tuning:** Konfigurierbare Memtable, Block Cache, Bloom Filter
- **WAL:** Write-Ahead Log für Durability
- **BlobDB:** Separate Speicherung großer Objekte (>4KB default)
- **Kompression:** LZ4 (Level 0-5), ZSTD (Bottommost Level)
- **Multi-Path:** Mehrere NVMe-Mounts für SSTable-Distribution

### Kompression

| Algorithmus | Kompressionsrate | Write Speed | Read Speed | Empfehlung |
|-------------|------------------|-------------|------------|------------|
| **None** | 1.0x | ⚡ Schnell | ⚡ Schnell | Nur Entwicklung |
| **LZ4** | 2-3x | ⚡ Schnell | ⚡ Schnell | ✅ Level 0-5 |
| **ZSTD** | 3-5x | Mittel | Schnell | ✅ Bottommost |
| **Snappy** | 2-2.5x | Schnell | Schnell | Alternative |

### API

```cpp
RocksDBWrapper db(config);

// Transaction API
auto txn = db.beginTransaction();
txn->put("key1", "value1");
txn->put("key2", "value2");
txn->commit();

// Snapshot Read
auto snapshot = db.getSnapshot();
auto value = db.get("key1", snapshot);

// WriteBatch (Atomic multi-put)
auto batch = db.createWriteBatch();
batch.put("k1", "v1");
batch.put("k2", "v2");
db.write(batch);

// Backup
db.createCheckpoint("/backup/path");
db.restoreFromCheckpoint("/backup/path");
```

## BaseEntity

Unified Entity Storage für alle Datenmodelle:

```cpp
struct BaseEntity {
    std::string pk;          // Primary Key (collection:uuid)
    uint64_t version;        // Optimistic Locking Version
    std::string hash;        // Content Hash
    json data;               // Entity Data (JSON)
    std::vector<uint8_t> binary_blob;  // Optional Binary
};
```

### Key Format

| Modell | Key Format | Beispiel |
|--------|------------|----------|
| Relational | `table:pk` | `users:12345` |
| Document | `collection:pk` | `orders:abc-123` |
| Graph Node | `node:pk` | `node:person-1` |
| Graph Edge | `edge:pk` | `edge:follows-1` |
| Vector | `object:pk` | `object:embedding-1` |

## BlobRedundancyManager

RAID-ähnliche Redundanz für große Objekte:

```cpp
BlobRedundancyManager::Config config;
config.redundancy_mode = RedundancyMode::MIRROR;  // oder STRIPE, PARITY, GEO_MIRROR
config.storage_paths = {"/data/nvme1", "/data/nvme2"};

BlobRedundancyManager mgr(config);
mgr.store("blob-id", data);
auto retrieved = mgr.retrieve("blob-id");
```

### Redundanz-Modi

| Modus | Beschreibung | Speichereffizienz |
|-------|--------------|-------------------|
| **NONE** | Keine Redundanz | 100% |
| **MIRROR** | Vollständige Kopie | 50% |
| **STRIPE** | Striping ohne Parität | 100% |
| **PARITY** | RAID-5-ähnlich | ~75% |
| **GEO_MIRROR** | Geo-Replikation | 50% |

## Verwandte Dokumentation

- [storage_rocksdb.md](storage_rocksdb.md) - RocksDB Tuning
- [storage_cloud_backends.md](storage_cloud_backends.md) - Cloud Storage Backends
- [storage_blob_redundancy.md](storage_blob_redundancy.md) - Blob Redundanz Details
