# Storage Module — Fehlende / Unvollständige Implementierungen

<!-- Status: current | validated: 2026-03-11 -->
<!-- Primärdokumentation: ../../../src/storage/ -->

Dieser Report dokumentiert Funktionen, die in `src/storage/ROADMAP.md`,
`src/storage/ARCHITECTURE.md` oder anderen Primary-Docs als implementiert oder geplant
beschrieben werden, jedoch bei der Reality-Check-Prüfung als **nicht vollständig
umgesetzt** befunden wurden.

Prüfstand: 2026-03-11 | Branch: `copilot/implement-backup-manager-scheduling`

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

## 2. BlobRedundancyManager — RocksDB EventListener-Integration nicht implementiert

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/blob_redundancy_manager.cpp` (Methode `createRocksDBListener()`) |
| **Erwartet** | Liefert einen `rocksdb::EventListener`, der bei SSTable-Flush/-Compaction-Events automatisch Blob-Replikation auslöst |
| **Beobachtet** | `createRocksDBListener()` gibt sofort `ERR_STORAGE_REDUNDANCY_FAILED` mit *"RocksDB listener not implemented"* zurück. Keine EventListener-Implementierung vorhanden |
| **Evidence (geprüfte Pfade)** | `src/storage/blob_redundancy_manager.cpp` Zeile 768 |
| **ROADMAP-Status** | Nicht als separater ROADMAP-Eintrag |
| **Issue-Titelvorschlag** | `[storage] Implement BlobRedundancyManager RocksDB EventListener integration` |
| **Label-Vorschläge** | `type:feature`, `priority:low`, `area:storage`, `status:open` |

---

## 3. RocksDBWrapper — `getApproximateSize()` gibt immer 0 zurück

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/rocksdb_wrapper.cpp` Zeile 1445 (TODO-Kommentar) |
| **Erwartet** | `getApproximateSize()` liefert eine sinnvolle Schätzung der RocksDB-Datenbankgröße (für Monitoring und Quota-Enforcement) |
| **Beobachtet** | Methode gibt immer `0` zurück; enthält `// TODO: Implement proper size calculation` |
| **Evidence (geprüfte Pfade)** | `src/storage/rocksdb_wrapper.cpp` Zeilen 1443–1450 |
| **ROADMAP-Status** | Nicht als separater ROADMAP-Eintrag; Datei-Header meldet `TODOs: 1` |
| **Issue-Titelvorschlag** | `[storage] Implement RocksDBWrapper::getApproximateSize using RocksDB SizeApproximation API` |
| **Label-Vorschläge** | `type:bug`, `priority:low`, `area:storage`, `status:open` |

---

## 4. SecuritySignatureManager — RocksDB-Iteration als In-Memory-Fallback

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/security_signature_manager.cpp` Zeile 110 (TODO-Kommentar) |
| **Erwartet** | `SecuritySignatureManager` iteriert über alle RocksDB-Einträge und prüft/listet Signaturen |
| **Beobachtet** | RocksDB-Iteration nicht implementiert; aktueller Pfad endet am TODO-Kommentar *"Implement proper RocksDB iteration when RocksDBWrapper supports it"* und gibt leere Ergebnisse zurück |
| **Evidence (geprüfte Pfade)** | `src/storage/security_signature_manager.cpp` Zeilen 105–115 |
| **ROADMAP-Status** | Nicht als separater ROADMAP-Eintrag; Datei-Header meldet `TODOs: 1` |
| **Issue-Titelvorschlag** | `[storage] Implement SecuritySignatureManager RocksDB-wide signature iteration` |
| **Label-Vorschläge** | `type:bug`, `priority:medium`, `area:storage`, `area:security`, `status:open` |

---

## 5. BlobRedundancyManager — Erasure Coding nicht implementiert (ROADMAP v1.7.0)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/ROADMAP.md` §"Planned Features – Short-term" |
| **Erwartet** | `RedundancyMode::ERASURE_CODING` mit Reed-Solomon(k,m); `k=4, m=2` Default (1,5× Speicher-Overhead vs. 3× bei RAID-1) |
| **Beobachtet** | Nur RAID-1-Mirror implementiert (`BlobRedundancyManager`); kein `erasure_coding.cpp`, kein RS-Encode/Decode, kein `ERASURE_CODING`-Enum-Wert |
| **Evidence (geprüfte Pfade)** | `src/storage/blob_redundancy_manager.cpp` (kein Erasure-Coding-Pfad); `include/storage/blob_redundancy_manager.h` (kein `ERASURE_CODING`-Enum) |
| **ROADMAP-Status** | `[ ]` geplant für v1.7.0 |
| **Issue-Titelvorschlag** | `[storage] Implement Reed-Solomon erasure coding in BlobRedundancyManager (v1.7.0)` |
| **Label-Vorschläge** | `type:feature`, `priority:medium`, `area:storage`, `milestone:v1.7.0` |

---

## 6. Distributed Transactions (2PC) nicht implementiert (ROADMAP v1.7.0)

| Feld | Wert |
|---|---|
| **Claim-Quelle** | `src/storage/ROADMAP.md` §"Planned Features – Long-term" |
| **Erwartet** | `DistributedTransactionManager` mit Two-Phase-Commit (2PC) und Raft-Koordination für Cross-Shard-Atomarität |
| **Beobachtet** | Kein `distributed_transaction_manager.cpp` vorhanden; `TransactionRetryManager` deckt nur lokale Retry-Logik ab |
| **Evidence (geprüfte Pfade)** | `src/storage/` (kein `distributed_transaction_manager.*`); `include/storage/` (kein entsprechender Header) |
| **ROADMAP-Status** | `[ ]` geplant für v1.7.0 |
| **Issue-Titelvorschlag** | `[storage] Implement DistributedTransactionManager with 2PC and Raft coordination (v1.7.0)` |
| **Label-Vorschläge** | `type:feature`, `priority:high`, `area:storage`, `area:transaction`, `milestone:v1.7.0` |

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
| 2 | BlobRedundancyManager RocksDB EventListener | blob_redundancy_manager.cpp:768 | Niedrig | Stub |
| 3 | RocksDBWrapper::getApproximateSize() | rocksdb_wrapper.cpp:1445 | Niedrig | TODO / gibt 0 |
| 4 | SecuritySignatureManager RocksDB-Iteration | security_signature_manager.cpp:110 | Mittel | TODO |
| 5 | Erasure Coding in BlobRedundancyManager | ROADMAP v1.7.0 | Mittel | `[ ]` geplant |
| 6 | Distributed 2PC Transactions | ROADMAP v1.7.0 | **Hoch** | `[ ]` geplant |
| 7 | ColumnarFormat Parquet-Export | ROADMAP v2.0.0 | Niedrig | `[ ]` geplant |
| 8 | ColumnarFormat AVX2 SIMD | ROADMAP v2.0.0 | Niedrig | `[ ]` geplant |
| 9 | 9 Source-Dateien fehlten im CMake-Build | cmake/CMakeLists.txt, cmake/ModularBuild.cmake | **Hoch** | ✅ Behoben in diesem PR |

*Alle anderen ROADMAP-Einträge (Phase 1–4 sowie Tiered Storage und GCS Backend) sind durch
vorhandene Implementierungsdateien auf `develop` belegt.*

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
