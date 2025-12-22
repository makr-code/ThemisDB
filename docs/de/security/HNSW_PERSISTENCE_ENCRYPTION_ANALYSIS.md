# HNSW-Persistenz und Verschlüsselung - Sicherheitsanalyse

**Datum:** 15. Dezember 2025  
**Version:** 1.0  
**Kontext:** Analyse der Sicherheitsimplikationen beim Speichern von HNSW-Indizes auf Disk  
**Anforderung:** "Wir müssen darauf achten, dass der HNSW auch auf der Festplatte für 'warm-start' gespeichert wird."


## 📑 Inhaltsverzeichnis

- [Executive Summary](#executive-summary)
- [1. Bestehende Implementierung](#1-bestehende-hnsw-persistenz-implementierung)
- [2. Angriffsvektoren](#2-angriffsvektoren)
- [3. Lösungsansätze](#3-lösungsansätze)
- [4. Empfehlung & Roadmap](#4-empfehlung--roadmap)

## Executive Summary

**Problem:** HNSW-Index muss für schnelle Warm-Starts auf Disk gespeichert werden, aber die Vektoren im Index sind Plaintext.

**Sicherheitsrisiko:** ⚠️ Der persistierte HNSW-Index enthält **PLAINTEXT-Vektoren** auf Disk!

**Empfehlung:** **HNSW-Index MUSS verschlüsselt** werden, oder alternativ: Index nicht persistieren und nur verschlüsselte Vektoren in RocksDB speichern.

---

## 1. Bestehende HNSW-Persistenz-Implementierung

### 1.1 Aktueller Stand

ThemisDB hat bereits HNSW-Persistenz implementiert (siehe `docs/features/features_hnsw_persistence.md`):

```cpp
// Speichern
vix.saveIndex("./data/hnsw_chunks");

// Laden (Warm-Start)
vix.loadIndex("./data/hnsw_chunks");
```

**Verzeichnisstruktur:**
```
data/hnsw_chunks/
  ├─ index.bin      # HNSW Graph-Struktur (PLAINTEXT Vektoren!)
  ├─ meta.txt       # Metadaten
  └─ mapping.txt    # PK-Mapping
```

### 1.2 Problem: Plaintext Vektoren auf Disk

**index.bin enthält:**
- HNSW-Graph (Nachbarschaftsbeziehungen)
- **ALLE Vektoren im Klartext** (für schnelle Distanzberechnungen)
- Node-IDs und Layer-Informationen

**Sicherheitsrisiko:**
```
Verschlüsselte Vektoren in RocksDB:
  data/rocksdb/vec:123 → AES-256-GCM(embedding)  ✅ Sicher

ABER:

Persistierter HNSW-Index:
  data/hnsw_chunks/index.bin → embedding (PLAINTEXT!)  ❌ RISIKO!
```

**Resultat:** Die gesamte Verschlüsselung wird **umgangen** durch den persistierten Index!

---

## 2. Angriffsvektoren

### 2.1 Szenario 1: Disk-Compromise

```
Angreifer erhält Zugriff auf Disk (Backup, gestohlener Server, etc.)
    ↓
Liest ./data/hnsw_chunks/index.bin
    ↓
Extrahiert ALLE Vektoren im Plaintext (mit hnswlib API)
    ↓
Rekonstruiert PII aus Embeddings (siehe EMBEDDING_REVERSIBILITY_ANALYSIS.md)
    ↓
DSGVO/HIPAA-Verletzung!
```

### 2.2 Szenario 2: Backup-Exposure

```
Unverschlüsselte Backups:
  /backups/2025-12-15/hnsw_chunks/index.bin  (PLAINTEXT)
  
Cloud-Backups:
  S3://backups/themisdb/hnsw_chunks/index.bin  (wenn nicht S3-verschlüsselt)
  
Resultat: Langfristige Exposition von Plaintext-Vektoren
```

### 2.3 Szenario 3: Insider-Threat

```
Mitarbeiter mit Filesystem-Zugriff:
    ↓
cp ./data/hnsw_chunks/index.bin /tmp/stolen.bin
    ↓
Kein Audit-Log (Filesystem-Operation)
    ↓
Daten exfiltriert, keine Detektion
```

---

## 3. Lösungsansätze

### 3.1 Option A: Verschlüsselter HNSW-Index (Empfohlen)

**Konzept:** HNSW-Index mit AES-256-GCM verschlüsseln vor dem Speichern

```cpp
class EncryptedHNSWPersistence {
public:
    // Speichern: Index verschlüsseln
    Status saveEncryptedIndex(const std::string& directory) {
        // 1. Speichere HNSW in temporäres Memory-File
        hnswlib::L2Space space(dim_);
        std::string temp_path = "/tmp/hnsw_temp.bin";
        hnsw_index_->saveIndex(temp_path);
        
        // 2. Lade in Memory
        std::ifstream file(temp_path, std::ios::binary);
        std::vector<uint8_t> plaintext_index(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );
        file.close();
        
        // 3. Verschlüssele Index-Daten
        EncryptedField<std::vector<uint8_t>> enc_index;
        enc_index.encrypt(plaintext_index, "hnsw_index");
        
        // 4. Speichere verschlüsselt
        std::string enc_path = directory + "/index.bin.encrypted";
        std::ofstream enc_file(enc_path, std::ios::binary);
        auto enc_data = enc_index.toBytes();  // Binary serialization
        enc_file.write(
            reinterpret_cast<const char*>(enc_data.data()), 
            enc_data.size()
        );
        enc_file.close();
        
        // 5. Lösche Temp-File
        std::filesystem::remove(temp_path);
        
        // 6. Speichere Metadaten (auch verschlüsselt)
        saveEncryptedMetadata(directory);
        
        Logger::info("HNSW index encrypted and saved to {}", directory);
        return Status::OK();
    }
    
    // Laden: Index entschlüsseln
    Status loadEncryptedIndex(const std::string& directory) {
        std::string enc_path = directory + "/index.bin.encrypted";
        
        // 1. Prüfe ob verschlüsselter Index existiert
        if (!std::filesystem::exists(enc_path)) {
            return Status::Error("Encrypted index not found");
        }
        
        // 2. Lade verschlüsselte Daten
        std::ifstream enc_file(enc_path, std::ios::binary);
        std::vector<uint8_t> enc_data(
            (std::istreambuf_iterator<char>(enc_file)),
            std::istreambuf_iterator<char>()
        );
        enc_file.close();
        
        // 3. Entschlüssele
        EncryptedField<std::vector<uint8_t>> enc_index;
        enc_index.fromBytes(enc_data);
        auto plaintext_index = enc_index.decrypt();
        
        // 4. Schreibe in temporäres File für hnswlib
        std::string temp_path = "/tmp/hnsw_temp_load.bin";
        std::ofstream temp_file(temp_path, std::ios::binary);
        temp_file.write(
            reinterpret_cast<const char*>(plaintext_index.data()),
            plaintext_index.size()
        );
        temp_file.close();
        
        // 5. Lade Index mit hnswlib
        hnswlib::L2Space space(dim_);
        hnsw_index_ = std::make_unique<hnswlib::HierarchicalNSW<float>>(
            &space, temp_path
        );
        
        // 6. Lösche Temp-File
        std::filesystem::remove(temp_path);
        
        // 7. Lade Metadaten
        loadEncryptedMetadata(directory);
        
        Logger::info("HNSW index decrypted and loaded from {}", directory);
        return Status::OK();
    }
    
private:
    std::unique_ptr<hnswlib::HierarchicalNSW<float>> hnsw_index_;
    int dim_;
};
```

**Vorteile:**
- ✅ **At-Rest geschützt:** HNSW-Index verschlüsselt auf Disk
- ✅ **BSI C5 konform:** CRY-03 erfüllt
- ✅ **Warm-Start:** Weiterhin schnelles Laden möglich
- ✅ **Transparent:** Keine API-Änderungen für Anwendung

**Nachteile:**
- ⚠️ **Overhead:** Entschlüsselung beim Laden (~2-5 Sekunden für 1M Vektoren)
- ⚠️ **Temp-File:** Benötigt temporären Speicher (2x Index-Größe während Load)

**Performance:**
```
1M Vektoren (768-dim):
  Index-Größe: ~3 GB
  
  Speichern:
    - HNSW saveIndex():    2 sec
    - AES-256-GCM Encrypt: 3 sec  (1 GB/s throughput)
    - Gesamt:              5 sec
  
  Laden:
    - AES-256-GCM Decrypt: 3 sec
    - HNSW loadIndex():    2 sec
    - Gesamt:              5 sec
  
  Overhead: 5 sec (akzeptabel für Startup)
```

---

### 3.2 Option B: Filesystem-Level Encryption

**Konzept:** Verschlüssele gesamtes Verzeichnis auf OS-Ebene

**Linux (dm-crypt/LUKS):**
```bash
# Erstelle verschlüsseltes Volume
sudo cryptsetup luksFormat /dev/sdb1
sudo cryptsetup open /dev/sdb1 themisdb_encrypted

# Mount
sudo mkfs.ext4 /dev/mapper/themisdb_encrypted
sudo mount /dev/mapper/themisdb_encrypted /var/lib/themisdb/data

# ThemisDB speichert HNSW-Index normal
# Filesystem verschlüsselt transparent
```

**Windows (BitLocker):**
```powershell
Enable-BitLocker -MountPoint "D:" -EncryptionMethod Aes256
```

**macOS (FileVault):**
```bash
sudo fdesetup enable
```

**Vorteile:**
- ✅ **Transparent:** Keine Code-Änderungen erforderlich
- ✅ **OS-Level:** Bewährte Kryptographie
- ✅ **Performance:** Hardware-Beschleunigung (AES-NI)
- ✅ **Alle Daten:** Nicht nur HNSW, sondern auch RocksDB verschlüsselt

**Nachteile:**
- ❌ **Key Management:** OS muss Schlüssel verwalten
- ❌ **Nicht granular:** Alle oder nichts
- ❌ **Cloud-unfähig:** Funktioniert nicht bei S3-Backups (separate Encryption nötig)

**BSI C5 Compliance:**
- ✅ **CRY-03:** Akzeptiert als "At-Rest Encryption"
- ⚠️ **CRY-02:** Key Management nicht ideal (OS-managed statt Application-managed)

---

### 3.3 Option C: Kein Persistent HNSW (Rebuild on Startup)

**Konzept:** HNSW-Index NICHT persistieren, nur verschlüsselte Vektoren in RocksDB

```cpp
class NonPersistentHNSW {
public:
    Status init() {
        Logger::info("Building HNSW index from encrypted vectors...");
        
        // Lade ALLE verschlüsselten Vektoren aus RocksDB
        auto encrypted_vectors = db_->scan("vec:");
        
        // Entschlüssele und baue Index
        for (auto& enc_doc : encrypted_vectors) {
            auto embedding = decryptEmbedding(enc_doc);
            hnsw_index_->addVector(enc_doc.getPrimaryKey(), embedding);
        }
        
        Logger::info("HNSW index built: {} vectors", encrypted_vectors.size());
        return Status::OK();
    }
    
    // Kein saveIndex() - Index wird nie auf Disk gespeichert
    // Bei Shutdown: Index geht verloren (kein Risiko)
    // Bei Startup: Rebuild (langsamer, aber sicher)
};
```

**Vorteile:**
- ✅ **Maximale Sicherheit:** Kein Plaintext-Index auf Disk
- ✅ **BSI C5 konform:** 100% At-Rest verschlüsselt
- ✅ **Einfach:** Keine zusätzliche Encryption-Logik

**Nachteile:**
- ❌ **Langsamer Startup:** Rebuild bei jedem Neustart
  - 100k Vektoren: ~30 Sekunden
  - 1M Vektoren: ~5 Minuten
  - 10M Vektoren: ~50 Minuten
- ❌ **Nicht praktikabel:** Für große Datenbanken unakzeptabel

---

### 3.4 Option D: Hybrid (Verschlüsselt + Filesystem)

**Konzept:** Kombination aus Option A + B

```
Application-Layer:
  ├─ Verschlüssele HNSW-Index (Option A)
  └─ Speichere in ./data/hnsw_encrypted/

Filesystem-Layer:
  └─ ./data/ auf encrypted Volume (Option B)
```

**Vorteile:**
- ✅ **Defense-in-Depth:** Doppelte Verschlüsselung
- ✅ **BSI C5+:** Übererfüllt Anforderungen

**Nachteile:**
- ⚠️ **Overhead:** 2x Encryption/Decryption
- ⚠️ **Komplexität:** Mehr Konfiguration

---

## 4. Vergleich der Optionen

| Kriterium | Option A<br>App Encryption | Option B<br>Filesystem | Option C<br>No Persist | Option D<br>Hybrid |
|-----------|------------|------------|------------|---------|
| **At-Rest Sicherheit** | ✅ Hoch | ✅ Hoch | ✅ Maximal | ✅ Sehr Hoch |
| **Warm-Start Performance** | ✅ Gut (+5s) | ✅ Sehr Gut | ❌ Schlecht (5min) | ✅ Gut (+5s) |
| **BSI C5 CRY-03** | ✅ Konform | ✅ Konform | ✅ Konform | ✅ Konform |
| **Implementierung** | ⚠️ Moderat | ✅ Einfach | ✅ Einfach | ❌ Komplex |
| **Key Management** | ✅ Application | ⚠️ OS | ✅ Application | ✅ Application |
| **Cloud-Backups** | ✅ Verschlüsselt | ⚠️ Separate Encryption | ✅ Verschlüsselt | ✅ Verschlüsselt |
| **Granularität** | ✅ Pro-Index | ❌ Alles | ✅ N/A | ✅ Pro-Index |
| **Overhead** | ⚠️ 5 sec | ✅ Minimal | ❌ 5 min | ⚠️ 10 sec |

---

## 5. Empfehlung

### 5.1 Empfohlene Lösung: **Option A** (Verschlüsselter HNSW-Index)

**Begründung:**
1. ✅ **BSI C5 konform:** Application-managed Encryption
2. ✅ **Warm-Start:** Weiterhin schnell (nur +5s Overhead)
3. ✅ **Granular:** Per-Index Verschlüsselung
4. ✅ **Cloud-ready:** Funktioniert mit S3-Backups
5. ✅ **Key Rotation:** Einfach möglich

### 5.2 Alternative: **Option B** (Filesystem-Level) als Zusatz

**Wenn bereits vorhanden:**
- ✅ Nutze dm-crypt/BitLocker/FileVault zusätzlich zu Option A
- ✅ Defense-in-Depth Strategie

**Wenn noch nicht vorhanden:**
- ⚠️ Implementiere erst Option A
- Optional: Filesystem-Encryption später hinzufügen

### 5.3 NICHT empfohlen: **Option C** (No Persist)

**Nur für:**
- Kleine Datenbanken (<100k Vektoren)
- Development/Testing
- Höchste Security-Anforderungen (z.B. Militär)

---

## 6. Implementierungs-Roadmap

### Phase 1 (Woche 1): Verschlüsseltes Speichern

```cpp
Status VectorIndexManager::saveEncryptedIndex(const std::string& directory) {
    // 1. Speichere HNSW in Memory-Buffer
    std::string temp_path = getTempPath();
    hnsw_->saveIndex(temp_path);
    
    // 2. Lade und verschlüssele
    auto plaintext = readFile(temp_path);
    EncryptedField<std::vector<uint8_t>> enc_index;
    enc_index.encrypt(plaintext, "hnsw_index");
    
    // 3. Speichere verschlüsselt
    writeFile(directory + "/index.bin.encrypted", enc_index.toBytes());
    
    // 4. Cleanup
    std::filesystem::remove(temp_path);
    
    return Status::OK();
}
```

### Phase 2 (Woche 2): Verschlüsseltes Laden

```cpp
Status VectorIndexManager::loadEncryptedIndex(const std::string& directory) {
    // 1. Lade verschlüsselte Daten
    auto enc_data = readFile(directory + "/index.bin.encrypted");
    
    // 2. Entschlüssele
    EncryptedField<std::vector<uint8_t>> enc_index;
    enc_index.fromBytes(enc_data);
    auto plaintext = enc_index.decrypt();
    
    // 3. Schreibe temp file und lade
    std::string temp_path = getTempPath();
    writeFile(temp_path, plaintext);
    hnsw_->loadIndex(temp_path);
    
    // 4. Cleanup
    std::filesystem::remove(temp_path);
    
    return Status::OK();
}
```

### Phase 3 (Woche 3): Integration

- [ ] Modifiziere `VectorIndexManager::saveIndex()`
- [ ] Modifiziere `VectorIndexManager::loadIndex()`
- [ ] Backward-Compatibility (alte unverschlüsselte Indizes)
- [ ] Migration Tool

### Phase 4 (Woche 4): Testing

- [ ] Unit-Tests
- [ ] Performance-Benchmarks
- [ ] Security-Tests (verify no plaintext on disk)
- [ ] Migration-Tests

**Gesamt-Zeitaufwand: 4 Wochen**

---

## 7. Sicherheits-Checkliste

### 7.1 Vor Implementierung

- [ ] **RocksDB-Vektoren:** Bereits verschlüsselt (EncryptedField)
- [ ] **HNSW-Index:** Noch NICHT verschlüsselt ⚠️
- [ ] **Metadaten:** Noch NICHT verschlüsselt ⚠️
- [ ] **Mapping:** PK-IDs im Klartext (OK, keine PII)

### 7.2 Nach Implementierung

- [ ] **RocksDB-Vektoren:** Verschlüsselt ✅
- [ ] **HNSW-Index:** Verschlüsselt ✅
- [ ] **Metadaten:** Verschlüsselt ✅
- [ ] **Mapping:** Weiterhin Plaintext (OK)
- [ ] **Temp-Files:** Werden gelöscht nach Gebrauch ✅
- [ ] **Backups:** Verschlüsselte Indizes ✅

### 7.3 Audit-Punkte

- [ ] Keine `.bin` Files mit Plaintext-Vektoren auf Disk
- [ ] Nur `.bin.encrypted` Files
- [ ] Temp-Files in `/tmp` werden nach Verwendung gelöscht
- [ ] Key-Access wird geloggt
- [ ] Filesystem-Permissions korrekt (600 für encrypted files)

---

## 8. Fazit

### 8.1 Zusammenfassung

**Frage:** "Wir müssen darauf achten, dass der HNSW auch auf der Festplatte für 'warm-start' gespeichert wird."

**Antwort:** ✅ **JA, aber NUR verschlüsselt!**

**Problem identifiziert:**
- Aktuell: HNSW-Index wird im **Plaintext** gespeichert
- Risiko: Alle Verschlüsselungsmaßnahmen werden umgangen

**Empfohlene Lösung:**
- Option A: Verschlüsselter HNSW-Index (Application-Layer)
- Overhead: +5 Sekunden beim Laden (akzeptabel)
- BSI C5: Voll konform

### 8.2 Nächste Schritte

1. **Implementierung genehmigen:** Verschlüsselter HNSW-Index
2. **Prototyp:** 2 Wochen
3. **Testing:** 1 Woche
4. **Rollout:** 1 Woche
5. **Migration:** Bestehende Indizes re-encrypten

**Gesamt: 4 Wochen bis Production-Ready**

---

**Erstellt:** 15. Dezember 2025  
**Status:** Analysiert, Empfehlung gegeben  
**Priority:** P1 (High - Sicherheitslücke schließen)  
**Owner:** Security Team + Development Team
