# ThemisDB – RocksDB Nutzung & Härtung (Audit)

**Datum:** 2026-02-10 12:39:02  
**Repo:** makr-code/ThemisDB  
**Ziel:** Konkrete Analyse, wie RocksDB in ThemisDB verwendet wird, und welche zusätzlichen Härtungsmaßnahmen (Integrity/Durability/Recovery) verfügbar sind.  
**Scope:** RocksDB (LSM), SST/WAL/MANIFEST/CURRENT, TransactionDB/MVCC, Sharding-Durability, Checkpoints, Verifikation.

> Hinweis: Dieses Dokument ist ein *Engineering Audit* (Ist-Zustand + Lücken + Maßnahmen), nicht die endgültige Betriebsdoku.

---

## 1. Überblick: RocksDB-Dateien & Fehlerklassen

### 1.1 Relevante RocksDB-Dateien
- **WAL:** `000123.log` (Write-Ahead Log)
- **SST:** `000456.sst` (immutable tables, checksummed blocks)
- **MANIFEST:** `MANIFEST-000789` (DB-Versionierung/Metadaten)
- **CURRENT:** Pointer auf aktives MANIFEST
- **OPTIONS-***, **LOCK**, **LOG**

### 1.2 Typische Korruptions-/Fehlerbilder
- Silent data corruption (bit flips; I/O liefert „falsche richtige“ Daten)
- I/O errors / bad sectors (read() schlägt fehl)
- torn/partial writes bei Powerloss (insb. WAL/MANIFEST)
- Locking/FSync-Semantik inkonsistent (Cache/Controller „lügt“)

---

## 2. Ist-Zustand in ThemisDB (konkrete Code-Stellen)

### 2.1 Wrapper & Konfiguration
**`RocksDBWrapper::Config`** definiert Pfade (db_path, wal_dir, db_paths) sowie Integritäts-Schalter.

- Config-Definition (Auszug):  
  https://github.com/makr-code/ThemisDB/blob/2bf8de8306eeae671a126fff29c155ca7ee642a8/include/storage/rocksdb_wrapper.h#L66-L91

- Data Integrity & Robustness Felder (Auszug):  
  https://github.com/makr-code/ThemisDB/blob/2bf8de8306eeae671a126fff29c155ca7ee642a8/include/storage/rocksdb_wrapper.h#L169-L188

### 2.2 Options Wiring in `configureOptions()`
In `RocksDBWrapper::configureOptions()` werden u. a. paranoid checks und read checksums aktiviert.

- Implementation-Ausschnitt:  
  https://github.com/makr-code/ThemisDB/blob/2bf8de8306eeae671a126fff29c155ca7ee642a8/src/storage/rocksdb_wrapper.cpp#L355-L368

**Beobachtung (Gap):** `verify_checksums_in_compaction` ist im Code kommentiert („not available in RocksDB 8.9“), obwohl Config-Feld und Doku es als verfügbar darstellen.

### 2.3 WAL-/Multi-SSD Pfade (wal_dir / db_paths)
- Doku: WAL auf dedizierter SSD, SSTables auf mehreren Pfaden:  
  https://github.com/makr-code/ThemisDB/blob/2bf8de8306eeae671a126fff29c155ca7ee642a8/docs/en/performance/WAL_MULTI_SSD_CONFIGURATION.md#L82-L98

- Code: Erzeugung von WAL-Verzeichnissen (Best effort):  
  https://github.com/makr-code/ThemisDB/blob/2bf8de8306eeae671a126fff29c155ca7ee642a8/src/storage/rocksdb_wrapper.cpp#L411-L430

### 2.4 Durability/Recovery Komponenten (Sharding)
Es existiert eine Durability-Schicht mit Checkpoint-/Recovery-Logik und WAL-Sync.

- ShardDurability (Initialize, syncWAL, createCheckpoint, performRecovery …):  
  https://github.com/makr-code/ThemisDB/blob/2bf8de8306eeae671a126fff29c155ca7ee642a8/src/sharding/shard_durability.cpp#L18-L231

---

## 3. Doku ↔ Code Konsistenzcheck (aktueller Befund)

### 3.1 Checksums in Compaction
- **Doku sagt:** implementiert (`verify_checksums_in_compaction`)  
  https://github.com/makr-code/ThemisDB/blob/2bf8de8306eeae671a126fff29c155ca7ee642a8/docs/DATABASE_FILE_ROBUSTNESS.md#L76-L92
- **Code zeigt:** Option auskommentiert (nicht verfügbar in RocksDB 8.9)  
  https://github.com/makr-code/ThemisDB/blob/2bf8de8306eeae671a126fff29c155ca7ee642a8/src/storage/rocksdb_wrapper.cpp#L355-L368

**Risiko:** Korruption wird evtl. erst bei Produktions-Reads entdeckt statt proaktiv während Compactions.

**Action:** Doku korrigieren *oder* Alternative implementieren (siehe Abschnitt 4.2/4.3).

---

## 4. Härtungs-Potenziale (konkret ableitbar)

### 4.1 Durability Contract (WAL fsync / sync policy)
**Ziel:** klar definierte Durability-Stufen (async / group-commit / strict fsync) und exaktes Mapping auf RocksDB-Optionen.

**Zu prüfen/zu dokumentieren:**
- Wird `WriteOptions.sync` nur global gesetzt oder pro-Commit?  
- Gibt es `DBOptions::use_fsync`?  
- Werden `wal_bytes_per_sync` / `bytes_per_sync` / `max_total_wal_size` gesetzt?

**Empfehlung (typisch best practice):**
- `sync=true` für „commit“ (oder kritische Writes), ansonsten group commit.
- WAL getrenntes Device (`wal_dir`) für Performance + isolierte Fehlerdomäne.
- Saubere Limits/Monitoring für WAL-Size.

### 4.2 Background Verification / Scrubbing (DataIntegrityManager)
Im Robustness-Dokument existiert ein Plan für Background Verification/Manager, aber anscheinend noch nicht vollständig umgesetzt.

- Plan:  
  https://github.com/makr-code/ThemisDB/blob/2bf8de8306eeae671a126fff29c155ca7ee642a8/docs/DATABASE_FILE_ROBUSTNESS.md#L545-L570

**Empfehlung:**
- Low-priority Verifikations-Thread, der SSTs zyklisch prüft (oder stichprobenartig).
- Metrics/Alerting (Prometheus/OpenTelemetry): verified_bytes_total, corruption_detected_total, wal_verify_failures_total.

### 4.3 Recovery-/Isolation-Runbook (ohne Backup)
**Ziel:** definierter Ablauf bei Korruptionssignal:
1) freeze writes / read-only open
2) verifizieren (WAL + SST + Manifest soweit möglich)
3) Quarantäne betroffener Files
4) Restore via Checkpoint / rebuild via full scan/export

**Hinweis:** Ohne redundante Kopie ist „vollständige Reparatur“ nicht garantiert; Fokus: isolieren + konsistente Teilwiederherstellung.

### 4.4 CF-Trennung als Fehlerdomänen-Strategie
Die Doku erwähnt, dass Themis standardmäßig Default-CF nutzt und CF-Trennung optional ist:
- `docs/de/storage/storage_rocksdb.md`  
  https://github.com/makr-code/ThemisDB/blob/2bf8de8306eeae671a126fff29c155ca7ee642a8/docs/de/storage/storage_rocksdb.md#L16-L63

**Potenzial:** Sekundärstrukturen (Indizes, Changefeed, TS) in separate CFs, damit bei Korruption gezieltes Rebuild/Drop möglich ist (Primärdaten unangetastet).

---

## 5. Offene Punkte (gezielt nachzurecherchieren im Repo)
1) **RocksDB-Version**: Welche Version wird tatsächlich gelinkt (vcpkg/system package)?  
2) **Option-Wiring Vollständigkeit**:
   - `allow_mmap_reads/writes` wirklich gesetzt?
   - XXH3 checksum wirklich aktiviert (table options)?
   - `use_fsync`, `wal_bytes_per_sync`, `bytes_per_sync`, `max_total_wal_size` irgendwo gesetzt?
3) **Admin/CLI Tools**:
   - existiert `themisdb-admin wal verify` wirklich als Code?
   - wie wird „point_in_time recovery“ implementiert oder nur dokumentiert?

---

## 6. Nächste Schritte (konkret)
- [ ] Repo-weit nach `use_fsync`, `wal_bytes_per_sync`, `bytes_per_sync`, `max_total_wal_size` suchen und Ergebnisse hier verlinken.
- [ ] ThemisDB CLI/Admin Commands für WAL verify / recover im Code verifizieren und dokumentieren.
- [ ] Entscheidung: Doku anpassen vs. Feature-Implementierung für Compaction-Verification / Background Scrub.

---

## Anhang: Suchlinks (GitHub UI)
- Code search: https://github.com/makr-code/ThemisDB/search?q=use_fsync&type=code
- Code search: https://github.com/makr-code/ThemisDB/search?q=wal_bytes_per_sync&type=code
- Code search: https://github.com/makr-code/ThemisDB/search?q=max_total_wal_size&type=code
- Code search: https://github.com/makr-code/ThemisDB/search?q=verify_checksums_in_compaction&type=code
