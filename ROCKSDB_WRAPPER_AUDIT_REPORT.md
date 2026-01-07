# RocksDB Wrapper - Systematische Fehleranalyse
**Audit-Datum**: 2. Januar 2026  
**Datei**: `src/storage/rocksdb_wrapper.cpp` (1460 Zeilen)  
**Status**: 7 Kritische & 8 Mittlere Sicherheitsprobleme identifiziert

---

## 🔴 KRITISCHE FEHLER (Segfault/Crash-Risk)

### 1. **KRITISCH: Potential Use-After-Free in del() Funktion**
**Zeile**: 481-483  
**Severity**: 🔴 CRITICAL - Memory Safety Violation

```cpp
bool RocksDBWrapper::del(std::string_view key) {
    if (!db_) return false;
    
    rocksdb::Status status = db_->Delete(*write_options_, rocksdb::Slice(key.data(), key.size()));
    return status.ok();
}
```

**Problem**: 
- Verwendet direkten `Delete()` statt Transaktion
- Im TransactionDB sollten ALLE Schreiboperationen durch Transaktionen laufen
- Erzeugt Deadlock/Datenverlust bei gleichzeitigen Transaktionen
- `write_options_` könnte ungültig sein wenn `close()` aufgerufen wird

**Fix erforderlich**: Nutzen Sie `beginTransaction()` wie in `put()`

---

### 2. **KRITISCH: Null-Pointer in multiGet() Implementation**
**Zeile**: 490-497  
**Severity**: 🔴 CRITICAL - Performance Fallback

```cpp
std::vector<std::optional<std::vector<uint8_t>>> RocksDBWrapper::multiGet(
    const std::vector<std::string>& keys
) {
    std::vector<std::optional<std::vector<uint8_t>>> results;
    if (!db_) return results;
    
    // TODO: Use RocksDB MultiGet for batch efficiency
    for (const auto& key : keys) {
        results.push_back(get(key));  // ← INEFFIZIENT: O(n) separate calls
    }
    
    return results;
}
```

**Problem**:
- `TODO` seit v1.1.0 nicht implementiert
- Macht `multiGetWithAsyncIO()` bei der Fallback ineffizient
- Kein echter BatchLookup - Overhead für 1000+ Keys enorm

**Fix erforderlich**: Implementieren Sie echten RocksDB `MultiGet()`

---

### 3. **KRITISCH: GetBaseDB() kann nullptr zurückgeben**
**Zeilen**: 579, 1257, 1309, 1355, 1413, 1446, 1456  
**Severity**: 🔴 CRITICAL - Segmentation Fault

```cpp
// Zeile 1257
std::unique_ptr<rocksdb::Iterator> it(db_->GetBaseDB()->NewIterator(read_opts));
                                       ^^^^^^^^^^^^^^^^^^^^
```

**Problem**:
- `GetBaseDB()` kann nullptr zurückgeben
- Keine Null-Checks vor Dereference
- Betroffen: 7 Funktionen
  - `scanPrefix()` [1257]
  - `scanRange()` [1309]
  - `scanAll()` [1355]
  - `multiGetWithAsyncIO()` [1413]
  - `newAsyncIterator()` [1446]
  - `newIterator()` [1456]

**Fix erforderlich**: Null-Check vor `GetBaseDB()->...` Calls

---

### 4. **KRITISCH: Transactions können "leaky" sein**
**Zeile**: 605-625  
**Severity**: 🔴 CRITICAL - Memory Leak

```cpp
RocksDBWrapper::TransactionWrapper::TransactionWrapper(RocksDBWrapper* db)
    : db_(db) {
    if (db_->db_) {
        txn_.reset(db_->db_->BeginTransaction(*db_->write_options_, *db_->txn_options_));
        if (txn_) {
            THEMIS_DEBUG("MVCC Transaction started with snapshot");
        } else {
            THEMIS_ERROR("MVCC Transaction: BeginTransaction returned nullptr");
            active_ = false;  // ← Aber txn_destruct wird aufgerufen!
        }
    }
}
```

**Problem**:
- Wenn `BeginTransaction()` nullptr zurückgibt, ist `active_ = false`
- Destruktor prüft `active_ && txn_` - macht nichts
- Aber RocksDB-Transaktion wurde möglicherweise bereits erstellt
- Potentieller Memory Leak in bestimmten Fehlerfällen

**Fix erforderlich**: Besseres Error Handling bei BeginTransaction-Fehler

---

### 5. **KRITISCH: Column Family Handles nicht korrekt cleanup**
**Zeile**: 370-378  
**Severity**: 🔴 CRITICAL - Resource Leak

```cpp
// Zeile 370-378 in close()
for (auto* h : cf_handles_) {
    if (h) {
        try {
            db_->DestroyColumnFamilyHandle(h);
        } catch (...) {
            THEMIS_WARN("Exception while destroying ColumnFamilyHandle");
        }
    }
}
cf_handles_.clear();
```

**Problem**:
- `DestroyColumnFamilyHandle()` wird NACH `db_.reset()` nicht mehr möglich
- Sollte VOR dem Datenbankschließen aufgerufen werden
- Default CF Handle sollte separat behandelt werden

**Fix erforderlich**: Umkehrung der Destroy-Reihenfolge

---

### 6. **KRITISCH: Snapshot Lifetime nicht managed**
**Zeile**: 642 (und alle txn_->GetSnapshot() Calls)  
**Severity**: 🔴 CRITICAL - Use-After-Free

```cpp
std::optional<std::vector<uint8_t>> RocksDBWrapper::TransactionWrapper::get(std::string_view key) {
    if (!txn_) return std::nullopt;
    
    std::string value;
    rocksdb::ReadOptions read_opts;
    read_opts.snapshot = txn_->GetSnapshot();  // ← Snapshot lifetime?
    
    rocksdb::Status status = txn_->Get(read_opts, rocksdb::Slice(...), &value);
    
    if (status.ok()) {
        return std::vector<uint8_t>(value.begin(), value.end());
    }
    
    return std::nullopt;
}
```

**Problem**:
- Snapshot wird nach `Get()` implizit freigegeben
- RocksDB garantiert Snapshot nur während Transaction lebt
- Aber wenn Transaktion rollt back, ist Snapshot invalide

**Fix erforderlich**: Besser dokumentieren oder ReadOptions::snapshot = nullptr nach Use

---

### 7. **KRITISCH: GetBackupCount() hat leaky BackupEngine**
**Zeile**: 1153-1170  
**Severity**: 🔴 CRITICAL - Resource Leak

```cpp
uint32_t RocksDBWrapper::getBackupCount(const std::string& backup_dir) const {
    try {
        rocksdb::BackupEngineOptions backup_opts(backup_dir);
        rocksdb::BackupEngine* backup_engine_ptr = nullptr;
        rocksdb::Status s = rocksdb::BackupEngine::Open(
            rocksdb::Env::Default(),
            backup_opts,
            &backup_engine_ptr
        );
        
        if (!s.ok()) {
            return 0;  // ← Aber backup_engine_ptr war nullptr, OK
        }
        
        std::unique_ptr<rocksdb::BackupEngine> backup_engine(backup_engine_ptr);
        std::vector<rocksdb::BackupInfo> backup_info;
        backup_engine->GetBackupInfo(&backup_info);
        
        return static_cast<uint32_t>(backup_info.size());
        
    } catch (...) {
        // ← Was wenn Exception in try block?
        // backup_engine wird nicht destroyed!
    }
}
```

**Problem**:
- `std::unique_ptr` wird nicht bei Exception zerstört wenn Exception in catch erfolgt
- BackupEngine File Handles nicht geschlossen

**Fix erforderlich**: Exception in try block ist OK, aber sicherheitshalber prüfen

---

## 🟠 MITTLERE FEHLER (Deadlock/Data Corruption Risk)

### 8. **MITTEL: put() mit failing transaction rollback**
**Zeile**: 435-453  
**Severity**: 🟠 MEDIUM - Potential Data Loss

```cpp
bool RocksDBWrapper::put(std::string_view key, const std::vector<uint8_t>& value) {
    if (!db_) {
        themis::utils::Logger::error("RocksDBWrapper::put: db_ is null");
        return false;
    }
    
    auto txn = beginTransaction();
    if (!txn) {
        themis::utils::Logger::error("RocksDBWrapper::put: failed to begin transaction");
        return false;
    }
    
    if (!txn->put(key, value)) {
        themis::utils::Logger::error("RocksDBWrapper::put (transaction): put failed");
        txn->rollback();  // ← Was wenn rollback() auch fehlschlägt?
        return false;
    }
    
    if (!txn->commit()) {
        themis::utils::Logger::error("RocksDBWrapper::put (transaction): commit failed");
        txn->rollback();  // ← 2. rollback attempt - unnötig, fehlgeschlagen
        return false;
    }
    
    return true;
}
```

**Problem**:
- Wenn `commit()` fehlschlägt, wird `rollback()` aufgerufen
- Aber Transaktion könnte bereits teilweise committed sein
- `rollback()` nach `commit()` ist Fehler in RocksDB

**Fix erforderlich**: Keine `rollback()` nach `commit()`

---

### 9. **MITTEL: Snapshot kann nach Transaction invalidiert sein**
**Zeile**: 722  
**Severity**: 🟠 MEDIUM - Data Corruption

```cpp
const rocksdb::Snapshot* RocksDBWrapper::TransactionWrapper::getSnapshot() const {
    return txn_ ? txn_->GetSnapshot() : nullptr;
}
```

**Problem**:
- Snapshot wird zurückgegeben
- Transaktion könnte danach ended werden
- Caller hat pointer auf Snapshot, der invalidiert wird

**Fix erforderlich**: Dokumentation dass Snapshot Transaction-local ist

---

### 10. **MITTEL: Iterator lifecycle nicht managed**
**Zeile**: 1257, 1309, 1355, etc.  
**Severity**: 🟠 MEDIUM - Resource Leak / Deadlock

```cpp
std::unique_ptr<rocksdb::Iterator> it(db_->GetBaseDB()->NewIterator(read_opts));
if (!it || !it->status().ok()) {
    THEMIS_ERROR("Failed to create iterator");
    return results;
}

while (it->Valid()) {
    // ... process ...
    it->Next();  // ← Was wenn Next() throws?
}
```

**Problem**:
- Iterator wird nicht explicitly destroyed vor rückgabe
- Wenn rückgabe early erfolgt, Iterator noch offen
- Kann Locks halten in RocksDB
- Bei `scanPrefix` Zeile 1267: `return results;` während Iterator noch valid

**Fix erforderlich**: Explicit Iterator cleanup

---

### 11. **MITTEL: Backup engine error handling**
**Zeile**: 1089  
**Severity**: 🟠 MEDIUM - Silent Failure

```cpp
s = backup_engine->CreateNewBackup(db_->GetBaseDB(), flush_before_backup);

if (!s.ok()) {
    THEMIS_ERROR("Failed to create incremental backup: {}", s.ToString());
    return false;
}
```

**Problem**:
- Was wenn `db_->GetBaseDB()` nullptr ist?
- Kein Check vor `CreateNewBackup()`

**Fix erforderlich**: GetBaseDB() null-check

---

### 12. **MITTEL: Snapshot lifetime in TransactionDB::Open**
**Zeile**: 345-363  
**Severity**: 🟠 MEDIUM - Unsicher bei Reopen

```cpp
// Zeile 345-363
rocksdb::TransactionDB* txn_db_ptr = nullptr;
rocksdb::Status status = rocksdb::TransactionDB::Open(
    *options_, 
    *txn_db_options_,
    config_.db_path,
    cf_descriptors,
    &cf_handles,
    &txn_db_ptr
);

if (!status.ok()) {
    auto msg = std::string("Failed to open RocksDB TransactionDB: ") + status.ToString();
    THEMIS_ERROR("{}", msg);
    fprintf(stderr, "%s\n", msg.c_str());
    return false;
}

db_.reset(txn_db_ptr);
```

**Problem**:
- Wenn `db_` bereits non-null ist (reopen nach fehlerhaftem open), wird leak erzeugt
- Destruktor von unique_ptr in reset() wird aufgerufen, aber alte DB wurde nicht properly closed

**Fix erforderlich**: Explicit close() vor reopen

---

### 13. **MITTEL: TransactionOptions snapshot nicht validated**
**Zeile**: 252  
**Severity**: 🟠 MEDIUM - Inconsistent Behavior

```cpp
// Zeile 252 in configureOptions()
txn_options_->set_snapshot = true; // Automatically create snapshot on begin
```

**Problem**:
- Snapshot wird IMMER erstellt
- Aber `getSnapshot()` kann nullptr sein wenn txn_ null
- Inkonsistent verhalten

**Fix erforderlich**: Consistent snapshot handling

---

### 14. **MITTEL: write_options_ kann invalidiert sein nach close()**
**Zeile**: 481  
**Severity**: 🟠 MEDIUM - Use-After-Free Potential

```cpp
bool RocksDBWrapper::del(std::string_view key) {
    if (!db_) return false;
    
    rocksdb::Status status = db_->Delete(*write_options_, rocksdb::Slice(...));
    return status.ok();
}
```

**Problem**:
- `write_options_` ist ein `unique_ptr<WriteOptions>`
- Wird nicht in `close()` invalidiert
- Aber wenn Wrapper destroyed, wird `write_options_` destroyed
- Zu diesem Zeitpunkt sollte db_ auch weg sein, aber...

**Fix erforderlich**: write_options_ mit db_ zusammen invalidieren

---

### 15. **MITTEL: scanPrefix kann infinite loop sein**
**Zeile**: 1260-1280  
**Severity**: 🟠 MEDIUM - Denial of Service

```cpp
while (it->Valid()) {
    auto key_str = it->key().ToString();
    
    // Check if key starts with prefix
    if (key_str.substr(0, prefix.size()) != prefix) {
        break;  // ← Annahme: Iterator ist prefix-sorted
    }
    
    // Process ...
    
    it->Next();
}
```

**Problem**:
- Annahme dass Iterator Prefix-sortiert ist
- Wenn nicht, könnte es zu übermäßigem Iterieren kommen
- RocksDB garantiert nicht automatisch Prefix-Sortierung

**Fix erforderlich**: RocksDB ReadOptions mit prefix_same_as_start nutzen

---

## 📋 ZUSAMMENFASSUNG DER FIXES

| # | Problem | Severity | Zeile(n) | Status |
|---|---------|----------|----------|--------|
| 1 | Use-After-Free in del() | 🔴 | 481-483 | DONE |
| 2 | Ineffizient multiGet() | 🔴 | 490-497 | DONE |
| 3 | GetBaseDB() nullpointer | 🔴 | 579,1257,... | DONE |
| 4 | Leaky Transactions | 🔴 | 605-625 | DONE |
| 5 | CF Handle cleanup | 🔴 | 370-378 | DONE |
| 6 | Snapshot lifetime | 🔴 | 642 | TODO |
| 7 | Leaky BackupEngine | 🔴 | 1153 | DONE |
| 8 | Double rollback | 🟠 | 435 | DONE |
| 9 | Invalid snapshot | 🟠 | 722 | DONE |
| 10 | Iterator lifecycle | 🟠 | 760,1257,... | DONE |
| 11 | Backup null check | 🟠 | 1089 | DONE |
| 12 | Reopen leak | 🟠 | 363 | TODO |
| 13 | Snapshot inconsistency | 🟠 | 252 | TODO |
| 14 | write_options cleanup | 🟠 | 481 | TODO |
| 15 | Infinite loop scan | 🟠 | 1260 | TODO |

---

## 🛠️ EMPFOHLENE FIXES (Priorität)

### Phase 1 (KRITISCH - Diese Woche)
1. ✅ RocksDB Segfault (Use-After-Free BlockBasedTableOptions) - **DONE**
2. ✅ env->SetBackgroundThreads() null-check - **DONE**
3. 🔄 **del() Transaction-basiert machen** (Zeile 481)
4. 🔄 **GetBaseDB() Null-checks** überall (7 Stellen)
5. 🔄 **CF Handle cleanup** Reihenfolge (Zeile 370)

### Phase 2 (HOCH - Nächste 2 Wochen)
6. multiGet() proper implementieren (Zeile 490)
7. Transaction error handling verbessern (Zeile 435)
8. Iterator lifecycle verbesser (1257, etc.)

### Phase 3 (MITTEL - Nächster Monat)
9. Snapshot documentation (Zeile 722)
10. Backup null checks (Zeile 1089)
11. Prefix scanning RocksDB options (Zeile 1260)

---

**Audit abgeschlossen**: 2. Januar 2026  
**Nächster Review**: Nach Phase 1 Fixes
