# Storage-Modul
<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/storage/ -->

**Stand:** 6. April 2026  
**Version:** 1.1  
**Kategorie:** 💾 Persistenz & Speicher  
**Validated:** 2026-03-10 (Reality-Check gegen Sourcecode; siehe [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md))

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Hauptkomponenten](#hauptkomponenten)
- [Key-Schema](#key-schema)
- [Blob-Storage-Backends](#blob-storage-backends)
- [Backup & Point-in-Time-Recovery](#backup--point-in-time-recovery)
- [Komprimierung & Columnar-Format](#komprimierung--columnar-format)
- [Verschlüsselung & Sicherheit](#verschlüsselung--sicherheit)
- [Tiered Storage](#tiered-storage)
- [Konfiguration](#konfiguration)
- [Bekannte Einschränkungen](#bekannte-einschränkungen)
- [Weiterführende Links](#weiterführende-links)

---

## Übersicht

Das Storage-Modul ist ThemisDBs persistente Datenschicht, aufgebaut auf **RocksDB TransactionDB** (LSM-Tree) mit MVCC, WAL und BlobDB. Es bietet ein einheitliches Key-Schema für alle fünf Datenmodelle (relational, dokument, graph, vektor, timeseries), externe Blob-Backends, Backup/PITR, pluggbare Komprimierung, Feldverschlüsselung und tiered Storage.

**Wichtigste Eigenschaften:**

- **Single RocksDB-Instanz** – alle Datenmodelle teilen eine Instanz mit modellpräfixierten Keys
- **MVCC** – Optimistic Concurrency via RocksDB TransactionDB; Leser blockieren Schreiber nie
- **BlobDB** – große Werte (>4 KB) automatisch in BlobDB ausgelagert
- **Tiered Storage** – automatische Hot/Warm/Cold-Migration nach Alter und Zugriffsmuster
- **DI-fähig** – `StorageEngine` nimmt alle Abhängigkeiten (Encryption, KeyProvider, IndexManager) per Injektion entgegen

---

## Hauptkomponenten

### Source-Code-Referenz (`src/storage/`)

| Datei | Rolle |
|---|---|
| `rocksdb_wrapper.cpp` | RocksDB-Wrapper: MVCC, WAL, BlobDB, Async-I/O |
| `mvcc_store.cpp` | MVCC-Snapshot-Management und Versionsverkettung |
| `wal_storage.cpp` | Write-Ahead-Log-Verwaltung und Replay |
| `key_schema.cpp` | Einheitliches Multi-Modell-Key-Encoding |
| `storage_engine.cpp` | High-Level-Abstraktion mit Dependency Injection |
| `base_entity.cpp` | Gemeinsame Basisklasse für Storage-Entities |
| `batch_write_optimizer.cpp` | Adaptives Write-Batching zur Reduktion der Write-Amplification |
| `compaction_manager.cpp` | Manuelle und geplante RocksDB-Kompaktierung |
| `merge_operators.cpp` | Custom RocksDB-Merge-Operatoren (Counter, Listen) |
| `hlc.cpp` | Hybrid Logical Clock für kausal-konsistente Zeitstempel |
| `raft_mvcc_bridge.cpp` | Integration Raft-Konsenslog ↔ MVCC |
| `history_manager.cpp` | Versionshistorie und Change-Tracking pro Key |
| `index_maintenance.cpp` | Background-Index-Rebuild und Konsistenzprüfungen |
| `disk_space_monitor.cpp` | Echtzeit-Disk-Quota-Monitoring und Alerting |
| `database_connection_manager.cpp` | Connection-Pooling und Lifecycle-Management |
| `transaction_retry_manager.cpp` | Exponentielles Backoff-Retry für fehlgeschlagene Transaktionen |
| `tiered_storage.cpp` | Hot/Warm/Cold-Tiered-Storage mit automatischer Datenmigration |
| `nlp_metadata_extractor.cpp` | Automatische Metadatenextraktion für ingestierte Dokumente |

### Öffentliche Header (`include/storage/`)

| Header | Rolle |
|---|---|
| `storage_engine.h` | Öffentliche `StorageEngine`-API (DI-Konstruktor, CRUD, Scan) |
| `rocksdb_wrapper.h` | `RocksDBWrapper` mit `get/put/del/scan/newIterator` |
| `mvcc_store.h` | `MVCCStore` und Snapshot-Handle |
| `key_schema.h` | `KeySchema::encode/decode` für alle Datenmodelle |
| `base_entity.h` | `BaseEntity` Serialisierung/Deserialisierung |
| `backup_manager.h` | `BackupManager` – Backup-Erstellung und PITR |
| `pitr_manager.h` | `PITRManager` – Point-in-Time-Recovery |
| `transaction_retry_manager.h` | `TransactionRetryManager` und `TransactionRetryConfig` |
| `tiered_storage.h` | `TieredStorageManager`, `AccessTracker`, `TieredStorageConfig` |
| `blob_storage_backend.h` | `IBlobBackend`-Interface (put/get/delete/exists) |
| `blob_storage_manager.h` | `BlobStorageManager` mit automatischer Backend-Auswahl |
| `blob_redundancy_manager.h` | `BlobRedundancyManager` RAID-1-Mirror |
| `security_signature.h` | `SecuritySignature` – AES-256-GCM-Feldverschlüsselung |
| `security_signature_manager.h` | `SecuritySignatureManager` – HMAC-SHA256-Tamper-Detection |
| `storage_audit_logger.h` | `StorageAuditLogger` – strukturiertes Audit-Trail |

---

## Key-Schema

Das Key-Schema (`key_schema.cpp`) kodiert alle Datenmodelle in einem gemeinsamen RocksDB-Keyspace:

```
Relational:    rel:{table}:{pk}
Dokument:      doc:{collection}:{pk}
Graph-Knoten:  node:{pk}
Graph-Kante:   edge:{from_id}:{type}:{to_id}
Vektor:        vec:{index_name}:{pk}
Timeseries:    ts:{series}:{timestamp}:{pk}

Sekundärindex: idx:{table}:{field}:{value}:{pk}
Graph-Index:   gidx:{from_id}:{type}:{to_id}
```

> ⚠️ **Breaking Change (v1.5.0+):** Das Key-Format v1.5.0 (Prefix-basiert) ist nicht rückwärtskompatibel mit Keys, die vor v1.5.0 erstellt wurden. Eine Migration ist erforderlich.

---

## Blob-Storage-Backends

Alle Backends implementieren `IBlobBackend` (`include/storage/blob_storage_backend.h`):

| Backend | Datei | Feature-Flag | Status |
|---|---|---|---|
| Filesystem | `blob_backend_filesystem.cpp` | *(immer aktiv)* | ✅ Produktionsreif |
| Amazon S3 | `blob_backend_s3.cpp` | `THEMIS_ENABLE_S3` | ✅ Produktionsreif |
| Azure Blob | `blob_backend_azure.cpp` | `THEMIS_ENABLE_AZURE` | ✅ Produktionsreif |
| Google GCS | `blob_backend_gcs.cpp` | `THEMIS_ENABLE_GCS` | ✅ Implementiert |
| WebDAV | `blob_backend_webdav.cpp` | *(immer aktiv)* | ✅ Produktionsreif |

**RAID-1-Redundanz** via `BlobRedundancyManager`: Schreibt synchron auf mehrere Backends und liest von dem zuerst antwortenden Backend.

> ℹ️ **Erasure Coding** (Reed-Solomon) ist als ROADMAP-Item geplant (`v1.7.0`), aber noch **nicht implementiert**. Aktuell ist nur RAID-1-Mirror verfügbar.

---

## Backup & Point-in-Time-Recovery

```cpp
BackupManager backup_mgr(config);

// Inkrementelles Backup
auto result = backup_mgr.createBackup("/backups/2026-03-10");

// Alle Backups auflisten
auto backups = backup_mgr.listBackups();

// PITR-Wiederherstellung
PITRManager pitr(config);
pitr.restore(target_timestamp, "/restore/path");
```

> ⚠️ **Einschränkung:** `scheduleBackup()`, `cancelScheduledBackup()`, `uploadBackupToCloud()` und `restoreFromCloud()` sind noch **nicht implementiert** und geben `ERR_UNKNOWN` zurück. Für Scheduling: K8s CronJob oder systemd-Timer verwenden.

---

## Komprimierung & Columnar-Format

```cpp
// Pluggbare Komprimierung pro Column-Family
CompressionStrategy strategy(CompressionConfig{
    .algorithm = CompressionAlgorithm::ZSTD,
    .level = 6
});

// Columnar-Format für analytische Workloads
ColumnarFormat columnar(config);
columnar.writeBatch(rows);
auto result = columnar.scanRange(start, end);
```

Unterstützte Algorithmen: **Snappy**, **Zstd**, **LZ4**, **Brotli**, **None** (konfigurierbar pro Column-Family).

---

## Verschlüsselung & Sicherheit

```cpp
// Feldverschlüsselung (AES-256-GCM)
SecuritySignature sig(key_provider);
auto encrypted = sig.encrypt_field("sensitive_field", plaintext);

// Tamper-Detection (HMAC-SHA256)
SecuritySignatureManager sig_mgr(db, key_provider);
sig_mgr.signRecord(record_key, data);
bool valid = sig_mgr.verifyRecord(record_key, data);
```

**Produktionsmodus-Pflicht:**
```bash
export THEMIS_PRODUCTION_MODE=1
# Verhindert No-Op-Encryption-Defaults; Fehler bei unsicherer Konfiguration
```

---

## Tiered Storage

```cpp
TieredStorageConfig config;
config.hot_tier_path  = "/nvme/data";
config.warm_tier_path = "/sata/data";
config.cold_tier_backend = "s3://archive-bucket";
config.hot_to_warm_days  = 30;
config.warm_to_cold_days = 90;

TieredStorageManager tiered(config);
tiered.put(key, value);             // Schreibt in Hot-Tier
auto val = tiered.get(key);         // Transparenter Lesezugriff
tiered.runMigrationCycle();         // Hintergrundmigration
```

`AccessTracker` verfolgt Schreib- und Lesezugriffe pro Key; `TierMigrationWorker` läuft im Hintergrund.

---

## Konfiguration

```yaml
storage:
  rocksdb:
    path: /data/rocksdb
    block_cache_mb: 512
    write_buffer_mb: 64
    blob_enabled: true
    blob_min_value_size: 4096

  encryption:
    enabled: true
    hsm_endpoint: https://vault:8200

  tiered:
    enabled: true
    hot_to_warm_days: 30
    warm_to_cold_days: 90

  backup:
    path: /backups
    incremental: true
```

---

## Bekannte Einschränkungen

| Einschränkung | Details |
|---|---|
| Erasure Coding | ROADMAP v1.7.0 – nicht implementiert; nur RAID-1-Mirror verfügbar |
| Backup-Scheduling | `scheduleBackup()` / `uploadBackupToCloud()` geben `ERR_UNKNOWN` zurück |
| `RocksDBWrapper::getApproximateSize()` | Gibt immer 0 zurück (TODO in rocksdb_wrapper.cpp) |
| `SecuritySignatureManager` RocksDB-Iteration | Nur In-Memory-Fallback; volle Iteration fehlt (TODO) |
| Distributed 2PC | ROADMAP v1.7.0 – Cross-Shard-Transaktionen nicht implementiert |
| `ColumnarFormat` Parquet-Export | ROADMAP v2.0.0 – noch nicht implementiert |
| Vectorized Execution (SIMD) | ROADMAP v2.0.0 – AVX2-Scan noch nicht implementiert |

Vollständige Details: [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md)

---

## Weiterführende Links

- **Primary Docs:** [`src/storage/README.md`](../../../src/storage/README.md)
- **Architektur:** [`src/storage/ARCHITECTURE.md`](../../../src/storage/ARCHITECTURE.md)
- **Roadmap:** [`src/storage/ROADMAP.md`](../../../src/storage/ROADMAP.md)
- **Geplante Features:** [`src/storage/FUTURE_ENHANCEMENTS.md`](../../../src/storage/FUTURE_ENHANCEMENTS.md)
- **Fehlende Implementierungen:** [MISSING_IMPLEMENTATIONS.md](MISSING_IMPLEMENTATIONS.md)
- **RocksDB Hardening Audit:** [ROCKSDB_HARDENING_AUDIT.md](ROCKSDB_HARDENING_AUDIT.md)
- **Blob-Redundanz (Detail):** [storage_blob_redundancy.md](storage_blob_redundancy.md)
- **Cloud-Backends (Detail):** [storage_cloud_backends.md](storage_cloud_backends.md)
- **Geo-Schema:** [storage_geo_schema.md](storage_geo_schema.md)

