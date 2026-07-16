# Storage Module — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-04-06 -->
<!-- Primärdokumentation: ../../../src/storage/ -->

Dieser Report dokumentiert Funktionen, die in `src/storage/ROADMAP.md`,
`src/storage/ARCHITECTURE.md` oder anderen Primary-Docs als implementiert oder geplant
beschrieben werden, jedoch bei der Reality-Check-Prüfung als **nicht vollständig
umgesetzt** befunden wurden.

Prüfstand: 2026-03-20 | Branch: `copilot/update-documentation-sync-storage`

---

## 1. BackupManager — Scheduling und Cloud-Upload ✅ BEHOBEN

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/backup_manager.cpp` (Methoden-Signaturen und Header) |
| **Erwartet** | `scheduleBackup()`, `cancelScheduledBackup()`, `uploadBackupToCloud()`, `restoreFromCloud()` als nutzbare Produktionsmethoden |
| **Status** | ✅ **Behoben** in PR `copilot/implement-backup-manager-scheduling` |
| **Behobene Befunde** | `scheduleBackup()` nutzt jetzt ein thread-sicheres In-Memory-Registry und gibt eine eindeutige Schedule-ID zurück (`sched_<YYYYMMDD_HHMMSS>_<counter>`). `cancelScheduledBackup()` entfernt den Eintrag und gibt `ERR_BACKUP_NOT_FOUND` zurück wenn nicht vorhanden. `listScheduledBackups()` gibt die registrierten Einträge zurück. `uploadBackupToCloud()` und `restoreFromCloud()` validieren Inputs und delegieren über `#if defined(THEMIS_ENABLE_S3\|AZURE\|GCS)` Compile-Time-Gates an die Cloud-Hilfsmethoden. |
| **Evidence** | `src/storage/backup_manager.cpp` Zeilen 1430–1621; `include/storage/backup_manager.h` (ScheduledBackupEntry, scheduler_mutex_, schedule_counter_) |
| **Behoben durch** | GitHub Copilot, 2026-03-11 |

---

## 2. BlobRedundancyManager — RocksDB EventListener-Integration ✅ BEHOBEN

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/blob_redundancy_manager.cpp` (Methode `createRocksDBListener()`) |
| **Erwartet** | Liefert einen `rocksdb::EventListener`, der bei SSTable-Flush/-Compaction-Events automatisch Blob-Replikation auslöst |
| **Status** | ✅ **Behoben** |
| **Behobene Befunde** | `createRocksDBListener()` gibt jetzt einen funktionierenden `RocksDBBlobListener` zurück (Zeile 882). `notifySSTFileDeleted()` markiert betroffene Locations als unhealthy. CI: `blob-redundancy-event-listener-ci.yml`. Tests: `tests/test_raid_redundancy.cpp` (BlobRedundancyEventListenerFocusedTests). |
| **Evidence** | `src/storage/blob_redundancy_manager.cpp` Zeilen 878–886 |
| **Behoben durch** | GitHub Copilot |

---

## 3. RocksDBWrapper — `getApproximateSize()` ✅ BEHOBEN

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/rocksdb_wrapper.cpp` Zeile 1445 (TODO-Kommentar) |
| **Erwartet** | `getApproximateSize()` liefert eine sinnvolle Schätzung der RocksDB-Datenbankgröße (für Monitoring und Quota-Enforcement) |
| **Status** | ✅ **Behoben** |
| **Behobene Befunde** | `getApproximateSize()` nutzt jetzt `rocksdb.total-sst-files-size`-Property (primär) mit Fallback auf `GetApproximateSizes(INCLUDE_FILES)`. CI: `rocksdb-size-calculation-ci.yml`. Tests: `tests/test_rocksdb_size_calculation.cpp` (RocksDBSizeCalculationFocusedTests). |
| **Evidence** | `src/storage/rocksdb_wrapper.cpp` Zeilen 1520–1542 |
| **Behoben durch** | GitHub Copilot |

---

## 4. SecuritySignatureManager — RocksDB-Iteration ✅ BEHOBEN

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/security_signature_manager.cpp` Zeile 110 (TODO-Kommentar) |
| **Erwartet** | `SecuritySignatureManager` iteriert über alle RocksDB-Einträge und prüft/listet Signaturen |
| **Claim-Quelle** | `src/storage/security_signature_manager.cpp` Zeile 110 (TODO-Kommentar) |
| **Erwartet** | `SecuritySignatureManager` iteriert über alle RocksDB-Einträge und prüft/listet Signaturen |
| **Status** | ✅ **Behoben** |
| **Behobene Befunde** | `listAllSignatures()` iteriert jetzt via `db_->iterateRange(start_key, end_key, ...)` über den vollständigen Signatur-Schlüsselbereich. In-Memory-Fallback bleibt für Tests ohne echte DB. CI: `security-signature-rocksdb-iteration-ci.yml`. Tests: `tests/test_security_signature_rocksdb_iteration.cpp`. |
| **Evidence** | `src/storage/security_signature_manager.cpp` Zeilen 105–135 |
| **Behoben durch** | GitHub Copilot |

---

## 5. BlobRedundancyManager — Erasure Coding ✅ BEHOBEN

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/ROADMAP.md` §"Planned Features – Short-term" |
| **Erwartet** | Reed-Solomon(k,m) Erasure Coding in `BlobRedundancyManager` |
| **Status** | ✅ **Behoben** |
| **Behobene Befunde** | `RedundancyMode::PARITY` mit `ErasureCodingBackend` (RS(k,m)) implementiert. Unterstützt RS(10,4), RS(6,3), RS(4,2) und beliebige (k,m)-Konfigurationen. `ErasureCodingBackend` in `src/storage/erasure_coding_backend.cpp`. CI: `erasure-coding-blob-storage-ci.yml`. Tests: `tests/test_erasure_coding_backend.cpp` (ErasureCodingFocusedTests). |
| **Evidence** | `src/storage/blob_redundancy_manager.cpp` Zeilen 485–615; `include/storage/erasure_coding_backend.h` |
| **Behoben durch** | GitHub Copilot |

---

## 6. Distributed Transactions (2PC) ✅ BEHOBEN

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/ROADMAP.md` §"Planned Features – Long-term" |
| **Erwartet** | `DistributedTransactionManager` mit Two-Phase-Commit (2PC) für Cross-Shard-Atomarität |
| **Status** | ✅ **Behoben** |
| **Behobene Befunde** | `DistributedTransactionManager` + `IDistributedShardParticipant` + `DistributedTransaction` implementiert in `src/storage/distributed_transaction_manager.cpp`. `ManagerSharedState` shared ownership für thread-sicheren Lebenszyklus. 27 Unit-Tests in `tests/test_distributed_transactions.cpp`. |
| **Evidence** | `src/storage/distributed_transaction_manager.cpp`; `include/storage/distributed_transaction_manager.h` |
| **Behoben durch** | GitHub Copilot |

---

## 7. ColumnarFormat — Parquet-Export nicht implementiert (ROADMAP v2.0.0)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/ROADMAP.md` §"Planned Features – Long-term" |
| **Erwartet** | `parquet_exporter.cpp` – Export von `ColumnarFormat`-Tabellen als Apache-Parquet-Dateien (kompatibel mit Spark, DuckDB, Pandas) |
| **Beobachtet** | Kein `parquet_exporter.cpp` oder entsprechender Header; `ColumnarFormat` hat keine Export-Methoden für Parquet |
| **Evidence (geprüfte Pfade)** | `src/storage/columnar_format.cpp` (keine Parquet-Referenz); `include/storage/columnar_format.h` |
| **ROADMAP-Status** | `[ ]` geplant für v2.0.0 |
| **Issue-Titelvorschlag** | `[storage] Implement native Parquet export from ColumnarFormat (v2.0.0)` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `area:storage`, `area:analytics`, `milestone:v2.0.0` |

---

## 8. ColumnarFormat — Vectorized Execution (AVX2 SIMD) nicht implementiert (ROADMAP v2.0.0)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/ROADMAP.md` §"Planned Features – Long-term" |
| **Erwartet** | `simd_filter.cpp` – AVX2-SIMD-Batchverarbeitung in `ColumnarFormat` für ≥4× Scan-Throughput vs. skalarer Baseline |
| **Beobachtet** | Kein `simd_filter.cpp`; `ColumnarFormat` führt ausschließlich skalare Verarbeitung durch |
| **Evidence (geprüfte Pfade)** | `src/storage/columnar_format.cpp` (keine SIMD-Referenz) |
| **ROADMAP-Status** | `[ ]` geplant für v2.0.0 |
| **Issue-Titelvorschlag** | `[storage] Implement AVX2 SIMD vectorized execution in ColumnarFormat (v2.0.0)` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `area:storage`, `area:performance`, `milestone:v2.0.0` |

---

## Zusammenfassung

| # | Feature | Quelle | Kritikalität | Status |
|---|---|---|---|---|
| 1 | BackupManager Scheduling / Cloud-Upload | backup_manager.cpp | **Mittel** | ✅ Behoben (2026-03-11) |
| 2 | BlobRedundancyManager RocksDB EventListener | blob_redundancy_manager.cpp | Niedrig | ✅ Behoben |
| 3 | RocksDBWrapper::getApproximateSize() | rocksdb_wrapper.cpp | Niedrig | ✅ Behoben |
| 4 | SecuritySignatureManager RocksDB-Iteration | security_signature_manager.cpp | Mittel | ✅ Behoben |
| 5 | Erasure Coding in BlobRedundancyManager | ROADMAP v1.7.0 | Mittel | ✅ Behoben (ErasureCodingBackend PARITY mode) |
| 6 | Distributed 2PC Transactions | ROADMAP v1.7.0 | **Hoch** | ✅ Behoben (DistributedTransactionManager) |
| 7 | ColumnarFormat Parquet-Export | ROADMAP v2.0.0 | Niedrig | `[ ]` geplant (v2.0.0) |
| 8 | ColumnarFormat AVX2 SIMD | ROADMAP v2.0.0 | Niedrig | `[ ]` geplant (v2.0.0) |
| 9 | 9 Source-Dateien fehlten im CMake-Build | cmake/CMakeLists.txt, cmake/ModularBuild.cmake | **Hoch** | ✅ Behoben |
| 10 | NVMe Optimierungen (NVMeManager) | ROADMAP v1.6.0 | Mittel | ✅ Behoben (nvme_manager.cpp, CI: nvme-manager-ci.yml) |

*Alle ROADMAP-Einträge für Phase 1–6 (v1.x–v1.8.0) sind vollständig implementiert.
Verbleibende offene Punkte betreffen ausschließlich v2.0.0-Features (Parquet-Export, AVX2 SIMD).*

---

## 9. Build-System: 9 Storage-Quelldateien fehlten in cmake/CMakeLists.txt und cmake/ModularBuild.cmake

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/README.md` §"Relevant Interfaces", `src/storage/ARCHITECTURE.md` §"Key Components" |
| **Erwartet** | Alle `src/storage/*.cpp`-Dateien sind in `cmake/CMakeLists.txt` (THEMIS_CORE_SOURCES) und `cmake/ModularBuild.cmake` (THEMIS_STORAGE_SOURCES) registriert |
| **Beobachtet** | Folgende Dateien fehlten in beiden Build-Definitionen: `blob_backend_azure.cpp`, `blob_backend_filesystem.cpp`, `blob_backend_gcs.cpp`, `blob_backend_s3.cpp`, `blob_backend_webdav.cpp`, `compressed_storage.cpp`, `compression_strategy.cpp`, `index_maintenance.cpp`, `storage_engine.cpp` |
| **Evidence (geprüfte Pfade)** | `cmake/CMakeLists.txt` THEMIS_CORE_SOURCES (Zeilen 1431–1443); `cmake/ModularBuild.cmake` THEMIS_STORAGE_SOURCES (Zeilen 212–238) |
| **Status** | ✅ **Behoben** in diesem PR – alle 9 Dateien zu `cmake/CMakeLists.txt` und `cmake/ModularBuild.cmake` hinzugefügt |
| **Issue-Titelvorschlag** | `[storage] Register all blob backends, compressed_storage, compression_strategy, index_maintenance, storage_engine in cmake build` |
| **Label-Vorschläge** | `type:bug`, `priority:high`, `area:storage`, `area:build`, `status:resolved` |
