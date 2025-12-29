# Differential Update Mode für Binary Blobs und LoRA Adapters

**Version:** 1.0.0  
**Release:** v1.3.0  
**Datum:** 17. Dezember 2025  
**Kategorie:** RPC, Data Transfer, Optimization

---

## Executive Summary

Diese Analyse untersucht Differential Update Modi für große Binärdateien (LoRA Adapters, Models) um unnötige Datenübertragung zu vermeiden. Statt ein gesamtes 5 GB File neu zu übertragen, wenn nur 100 MB geändert wurden, werden nur die Deltas (Änderungen) übertragen.

**Kernfrage:** Wie vermeiden wir unnötige Transfers bei Updates von LoRA/Binärblobs?

**Lösung:** Content-Defined Chunking + Hash-basierte Deduplication + Delta Transfer

---

## 1. Problem-Analyse

### 1.1 Aktuelles Verhalten (Ineffizient)

```
LoRA Adapter v1.0 (5 GB) → Shard A → Shard B → Shard C
                    ↓
Update zu v1.1: Nur 100 MB geändert
                    ↓
Aktuell: Transfer von ALLEN 5 GB nochmal! ❌
```

**Probleme:**
- ❌ 5 GB Transfer obwohl nur 100 MB neu
- ❌ 98% redundante Daten (4.9 GB bereits vorhanden)
- ❌ Hohe Netzwerk-Last
- ❌ Lange Transfer-Zeit
- ❌ Hohe Kosten (Cloud Egress)

### 1.2 Optimales Verhalten (Differential Update)

```
LoRA Adapter v1.0 (5 GB) → Shard A, B, C haben bereits
                    ↓
Update zu v1.1: Nur 100 MB geändert
                    ↓
Optimal: Transfer nur der 100 MB Delta! ✅
```

**Vorteile:**
- ✅ 98% weniger Datenübertragung (100 MB statt 5 GB)
- ✅ 50x schneller (bei typischen Updates)
- ✅ Geringere Netzwerk-Last
- ✅ Niedrigere Kosten

---

## 2. Best Practices für Differential Updates

### 2.1 Industrie-Standards

**rsync-Algorithmus:**
- Rolling Hash für Chunk-Identifikation
- Überträgt nur geänderte Blöcke
- Standard für File-Synchronisation seit 1996

**Git-ähnliche Content-Addressed Storage:**
- Content-Defined Chunking (CDC)
- Hash-basierte Deduplication
- Delta Compression

**Docker Layer Caching:**
- Layered File System
- Nur neue Layer werden gepullt
- Hash-basierte Layer-Identifikation

**BitTorrent-Style Chunking:**
- Fixed-Size Chunks mit Hashes
- Paralleler Download von fehlenden Chunks
- Verifizierung pro Chunk

### 2.2 Empfohlener Ansatz: Content-Defined Chunking (CDC)

**Warum CDC besser als Fixed-Size Chunking?**

```
Beispiel: 1 MB Einfügung am Anfang einer Datei

Fixed-Size Chunking (z.B. 50 MB Chunks):
├─ Chunk 1 (alt):  [0-50 MB]      Hash: ABC123
├─ Chunk 2 (alt):  [50-100 MB]    Hash: DEF456
└─ Chunk 3 (alt):  [100-150 MB]   Hash: GHI789

Nach 1 MB Insert am Start:
├─ Chunk 1 (neu):  [0-50 MB]      Hash: XXX000  ← GEÄNDERT (wegen Offset-Shift)
├─ Chunk 2 (neu):  [50-100 MB]    Hash: YYY111  ← GEÄNDERT (wegen Offset-Shift)
└─ Chunk 3 (neu):  [100-150 MB]   Hash: ZZZ222  ← GEÄNDERT (wegen Offset-Shift)

Ergebnis: ALLE Chunks müssen neu übertragen werden! ❌


Content-Defined Chunking (CDC):
├─ Chunk A:  [0-47 MB]      Hash: ABC123
├─ Chunk B:  [47-98 MB]     Hash: DEF456
└─ Chunk C:  [98-152 MB]    Hash: GHI789

Nach 1 MB Insert am Start:
├─ Chunk NEW: [0-1 MB]       Hash: NEW000  ← NEU
├─ Chunk A:   [1-48 MB]      Hash: ABC123  ← GLEICH! (Content unchanged)
├─ Chunk B:   [48-99 MB]     Hash: DEF456  ← GLEICH!
└─ Chunk C:   [99-153 MB]    Hash: GHI789  ← GLEICH!

Ergebnis: Nur 1 MB muss übertragen werden! ✅
```

**CDC-Vorteile:**
- ✅ Resistent gegen Offset-Shifts
- ✅ Maximale Deduplication
- ✅ Genutzt von: Dropbox, Restic, Duplicati, Borg Backup

---

## 3. Architektur: Differential Update Mode

### 3.1 System-Design

```
┌─────────────────────────────────────────────────────────────────┐
│                    Differential Update Pipeline                  │
└─────────────────────────────────────────────────────────────────┘

Source Shard (has LoRA v1.1)                Target Shard (has LoRA v1.0)
┌────────────────────────┐                  ┌────────────────────────┐
│ 1. Content-Defined     │                  │                        │
│ Chunking (CDC)         │                  │                        │
│ ├─ Rabin Fingerprint   │                  │                        │
│ ├─ Avg 4 MB chunks     │                  │                        │
│ ├─ Range: 2-8 MB       │                  │                        │
│ └─ SHA256 per chunk    │                  │                        │
│         ↓              │                  │                        │
│ 2. Chunk Catalog:      │                  │ 2. Load local catalog: │
│ ┌────────────────────┐ │                  │ ┌────────────────────┐ │
│ │Chunk | Hash        │ │                  │ │Chunk | Hash        │ │
│ ├──────┼─────────────┤ │                  │ ├──────┼─────────────┤ │
│ │C-000 │SHA256:ABC123│ │                  │ │C-000 │SHA256:ABC123│ │
│ │C-001 │SHA256:NEW111│◄┼─── Exchange ────┼►│C-001 │SHA256:DEF456│ │
│ │C-002 │SHA256:DEF456│ │  Chunk Hashes   │ │C-002 │SHA256:DEF456│ │
│ │C-003 │SHA256:GHI789│ │                  │ │C-003 │SHA256:GHI789│ │
│ └────────────────────┘ │                  │ └────────────────────┘ │
│         ↓              │                  │         ↓              │
│ 3. Diff Calculation:   │                  │ 3. Request missing:    │
│ Missing: [C-001]       │                  │ Need: [C-001]          │
│         ↓              │                  │         ↓              │
├────────────────────────┤──── Send only ───┤────────────────────────┤
│ 4. Transfer C-001      │     C-001        │ 5. Receive C-001       │
│ (4 MB compressed)      │  (2 MB Zstd)     │ (decompress)           │
│         ↓              │                  │         ↓              │
│                        │                  │ 6. Reassemble:         │
│                        │                  │ ├─ C-000 (from cache)  │
│                        │                  │ ├─ C-001 (new)         │
│                        │                  │ ├─ C-002 (from cache)  │
│                        │                  │ └─ C-003 (from cache)  │
│                        │                  │         ↓              │
│                        │<─── Verify ──────┤ 7. Verify SHA256       │
│                        │                  │ of complete file       │
└────────────────────────┘                  └────────────────────────┘

Transfer: 4 MB (compressed to 2 MB) statt 5 GB
Savings: 99.96% weniger Daten!
```

### 3.2 Protobuf Messages für Differential Transfer

```protobuf
// Extended BlobTransferRequest with differential mode
message BlobTransferRequest {
    string blob_id = 1;
    string blob_type = 2;
    uint64 blob_size_bytes = 3;
    string checksum_sha256 = 4;
    
    // Differential transfer mode (NEW)
    bool enable_differential = 13;       // Enable delta transfer
    DifferentialMode diff_mode = 14;     // Delta algorithm
    repeated ChunkManifest local_chunks = 15;  // Chunks already at target
}

enum DifferentialMode {
    DIFFERENTIAL_NONE = 0;               // Full transfer (default)
    DIFFERENTIAL_CDC = 1;                // Content-Defined Chunking (rsync-like)
    DIFFERENTIAL_FIXED_BLOCK = 2;        // Fixed-size block diff (simple)
    DIFFERENTIAL_BSDIFF = 3;             // Binary diff (for small changes)
}

// Chunk manifest for deduplication
message ChunkManifest {
    string chunk_hash = 1;               // SHA256 of chunk
    uint64 chunk_size = 2;               // Chunk size in bytes
    uint32 chunk_index = 3;              // Chunk index in file
    uint64 offset = 4;                   // Offset in file (for CDC)
}

// Response with delta chunks
message BlobDeltaResponse {
    string blob_id = 1;
    repeated ChunkManifest missing_chunks = 2;  // Chunks to transfer
    repeated ChunkManifest existing_chunks = 3; // Chunks already present
    uint64 total_chunks = 4;
    uint64 bytes_to_transfer = 5;        // Only delta bytes
    uint64 bytes_saved = 6;              // Saved by dedup
    double savings_percent = 7;          // Percentage saved
}
```

---

## 4. Implementierungsansätze

### 4.1 Ansatz 1: Content-Defined Chunking (CDC) - Empfohlen

**Algorithmus: Rabin Fingerprinting**

```cpp
// Content-Defined Chunking using Rabin Fingerprint
class CDCChunker {
public:
    struct Chunk {
        std::vector<uint8_t> data;
        std::string sha256_hash;
        uint64_t offset;
        uint64_t size;
    };
    
    struct Config {
        uint64_t min_chunk_size = 2 * 1024 * 1024;   // 2 MB
        uint64_t avg_chunk_size = 4 * 1024 * 1024;   // 4 MB
        uint64_t max_chunk_size = 8 * 1024 * 1024;   // 8 MB
        uint32_t window_size = 48;                    // Rolling hash window
    };
    
    std::vector<Chunk> chunk(const std::vector<uint8_t>& data) {
        std::vector<Chunk> chunks;
        uint64_t offset = 0;
        uint64_t chunk_start = 0;
        
        RabinFingerprint rabin(config_.window_size);
        
        for (uint64_t i = 0; i < data.size(); i++) {
            uint64_t hash = rabin.update(data[i]);
            uint64_t chunk_len = i - chunk_start;
            
            // Check for chunk boundary
            bool is_boundary = (hash % config_.avg_chunk_size) == 0;
            bool min_reached = chunk_len >= config_.min_chunk_size;
            bool max_reached = chunk_len >= config_.max_chunk_size;
            
            if ((is_boundary && min_reached) || max_reached) {
                // Create chunk
                Chunk chunk;
                chunk.offset = chunk_start;
                chunk.size = chunk_len;
                chunk.data.assign(data.begin() + chunk_start, data.begin() + i);
                chunk.sha256_hash = calculateSHA256(chunk.data);
                
                chunks.push_back(chunk);
                chunk_start = i;
            }
        }
        
        // Last chunk
        if (chunk_start < data.size()) {
            Chunk chunk;
            chunk.offset = chunk_start;
            chunk.size = data.size() - chunk_start;
            chunk.data.assign(data.begin() + chunk_start, data.end());
            chunk.sha256_hash = calculateSHA256(chunk.data);
            chunks.push_back(chunk);
        }
        
        return chunks;
    }
    
private:
    Config config_;
};

// Usage for differential transfer
class DifferentialBlobTransfer {
public:
    // Calculate which chunks need to be transferred
    std::vector<Chunk> calculateDelta(
        const std::vector<Chunk>& source_chunks,
        const std::vector<std::string>& target_hashes
    ) {
        std::unordered_set<std::string> target_set(
            target_hashes.begin(), 
            target_hashes.end()
        );
        
        std::vector<Chunk> missing_chunks;
        for (const auto& chunk : source_chunks) {
            if (target_set.find(chunk.sha256_hash) == target_set.end()) {
                missing_chunks.push_back(chunk);
            }
        }
        
        return missing_chunks;
    }
    
    // Reconstruct file from chunks
    std::vector<uint8_t> reassemble(
        const std::vector<Chunk>& chunks,
        const std::map<std::string, Chunk>& chunk_cache
    ) {
        std::vector<uint8_t> result;
        
        for (const auto& chunk_ref : chunks) {
            const auto& chunk_data = chunk_cache.at(chunk_ref.sha256_hash);
            result.insert(result.end(), 
                         chunk_data.data.begin(), 
                         chunk_data.data.end());
        }
        
        return result;
    }
};
```

**Vorteile:**
- ✅ Beste Deduplication-Rate
- ✅ Resistent gegen Inserts/Deletes
- ✅ Standard in Backup-Software
- ✅ Gut für große Files (>100 MB)

**Nachteile:**
- ❌ Höhere CPU-Last für Chunking
- ❌ Variable Chunk-Größen (komplexer)

### 4.2 Ansatz 2: Fixed-Block Diff - Einfacher

**Algorithmus: Fixed-Size Rolling Hash**

```cpp
// Simpler: Fixed-size block diff (like rsync weak hash)
class FixedBlockDiff {
public:
    struct BlockInfo {
        uint32_t block_index;
        uint32_t weak_hash;      // Adler32 (fast)
        std::string strong_hash; // SHA256 (secure)
    };
    
    static constexpr uint64_t BLOCK_SIZE = 4 * 1024 * 1024; // 4 MB
    
    std::vector<BlockInfo> generateSignature(const std::vector<uint8_t>& data) {
        std::vector<BlockInfo> signature;
        
        for (uint64_t offset = 0; offset < data.size(); offset += BLOCK_SIZE) {
            uint64_t len = std::min(BLOCK_SIZE, data.size() - offset);
            
            BlockInfo info;
            info.block_index = offset / BLOCK_SIZE;
            info.weak_hash = adler32(&data[offset], len);
            info.strong_hash = sha256(&data[offset], len);
            
            signature.push_back(info);
        }
        
        return signature;
    }
    
    std::vector<uint32_t> findMissingBlocks(
        const std::vector<BlockInfo>& source_sig,
        const std::vector<BlockInfo>& target_sig
    ) {
        std::unordered_set<std::string> target_hashes;
        for (const auto& block : target_sig) {
            target_hashes.insert(block.strong_hash);
        }
        
        std::vector<uint32_t> missing_indices;
        for (const auto& block : source_sig) {
            if (target_hashes.find(block.strong_hash) == target_hashes.end()) {
                missing_indices.push_back(block.block_index);
            }
        }
        
        return missing_indices;
    }
};
```

**Vorteile:**
- ✅ Einfachere Implementation
- ✅ Geringere CPU-Last
- ✅ Predictable Chunk-Größen

**Nachteile:**
- ❌ Weniger effizient bei Inserts (Offset-Shift Problem)
- ❌ Nicht optimal für große Änderungen

### 4.3 Ansatz 3: Binary Diff (bsdiff/xdelta) - Für kleine Änderungen

**Für sehr kleine Änderungen (<5%):**

```cpp
// For minimal changes (e.g., model parameter updates)
class BinaryDiff {
public:
    // Generate binary patch (like bsdiff)
    std::vector<uint8_t> generatePatch(
        const std::vector<uint8_t>& old_data,
        const std::vector<uint8_t>& new_data
    ) {
        // Uses bsdiff algorithm:
        // 1. Find longest common substrings (suffix array)
        // 2. Generate diff instructions (add, copy, seek)
        // 3. Compress with bzip2
        return bsdiff_create_patch(old_data, new_data);
    }
    
    std::vector<uint8_t> applyPatch(
        const std::vector<uint8_t>& old_data,
        const std::vector<uint8_t>& patch
    ) {
        return bsdiff_apply_patch(old_data, patch);
    }
};
```

**Vorteile:**
- ✅ Kleinste Patches für minimale Änderungen
- ✅ Gut für Model-Parameter Updates (<1% Change)

**Nachteile:**
- ❌ Hohe CPU/Memory Last
- ❌ Benötigt gesamtes altes File im RAM
- ❌ Nicht skalierbar für große Files (>1 GB)

---

## 5. Empfohlene Hybrid-Strategie

### 5.1 Intelligente Mode-Auswahl

```cpp
enum class DifferentialStrategy {
    FULL_TRANSFER,      // No delta (new file, no previous version)
    BINARY_DIFF,        // Small changes (<5%), use bsdiff
    FIXED_BLOCK,        // Medium changes (5-30%), simple and fast
    CDC                 // Large changes (>30%) or large files, best dedup
};

class SmartDifferentialTransfer {
public:
    DifferentialStrategy selectStrategy(
        uint64_t file_size,
        double estimated_change_percent
    ) {
        // No previous version → Full transfer
        if (!hasPreviousVersion()) {
            return DifferentialStrategy::FULL_TRANSFER;
        }
        
        // Very small changes AND small file → Binary diff
        if (estimated_change_percent < 5.0 && file_size < 1 * 1024 * 1024 * 1024) {
            return DifferentialStrategy::BINARY_DIFF;
        }
        
        // Medium changes → Fixed block (simple and fast)
        if (estimated_change_percent < 30.0) {
            return DifferentialStrategy::FIXED_BLOCK;
        }
        
        // Large changes or large files → CDC (best dedup)
        return DifferentialStrategy::CDC;
    }
};
```

### 5.2 Decision Tree

```
┌─────────────────────────────────────────────────────────────┐
│          LoRA/Blob Update Transfer Decision Tree             │
└─────────────────────────────────────────────────────────────┘

Start: Need to transfer updated blob
           ↓
    ┌──────────────────┐
    │ Previous version │ NO
    │ exists on target?├────────→ FULL_TRANSFER
    └─────────┬────────┘
              │ YES
              ↓
    ┌──────────────────┐
    │ File size        │ < 100 MB
    │                  ├────────→ Estimate change rate
    └─────────┬────────┘              ↓
              │ > 100 MB        ┌─────────────┐
              ↓                 │ < 5% change │
    ┌──────────────────┐        └──────┬──────┘
    │ Estimate change  │               │
    │ rate (quick scan)│               ↓
    └─────────┬────────┘        BINARY_DIFF
              │                 (bsdiff/xdelta)
              ↓                 Smallest patch
    ┌──────────────────┐
    │ Change rate?     │
    └─────────┬────────┘
              │
         ┌────┴────┬────────┬────────┐
         │         │        │        │
      < 5%      5-30%    30-60%   > 60%
         │         │        │        │
         ↓         ↓        ↓        ↓
    BINARY_   FIXED_     CDC      FULL_
    DIFF      BLOCK              TRANSFER
    (small    (fast     (best    (faster than
    patch)    & good)   dedup)   delta calc)
```

---

## 6. Performance-Analyse

### 6.1 Benchmark-Szenarien

**Szenario 1: LoRA Adapter Update (5 GB, 2% Change)**

```
Methode                Transfer Size    Time    Bandwidth Saved
─────────────────────────────────────────────────────────────────
Full Transfer          5,000 MB         2.5 min  0%
Fixed-Block Diff       150 MB           0.5 min  97%
CDC                    120 MB           0.6 min  97.6%
Binary Diff            100 MB           1.2 min  98% (but slower)

Empfehlung: Fixed-Block (balance speed/savings)
```

**Szenario 2: Model Update (10 GB, 15% Change)**

```
Methode                Transfer Size    Time    Bandwidth Saved
─────────────────────────────────────────────────────────────────
Full Transfer          10,000 MB        5 min    0%
Fixed-Block Diff       1,600 MB         1.2 min  84%
CDC                    1,450 MB         1.3 min  85.5%

Empfehlung: CDC (slightly better dedup)
```

**Szenario 3: Complete Model Replacement (10 GB, 90% Change)**

```
Methode                Transfer Size    Time    Bandwidth Saved
─────────────────────────────────────────────────────────────────
Full Transfer          10,000 MB        5 min    0%
Fixed-Block Diff       9,200 MB         5.5 min  8% (overhead!)
CDC                    9,100 MB         5.6 min  9%

Empfehlung: Full Transfer (delta overhead not worth it)
```

### 6.2 Storage Requirements

**Chunk Catalog Storage:**

```
5 GB LoRA mit 4 MB average chunks:
├─ Chunks: 1,250
├─ Hash per chunk: 32 bytes (SHA256)
├─ Metadata per chunk: ~20 bytes
├─ Total catalog: ~65 KB

Fazit: Vernachlässigbar (<0.01% overhead)
```

---

## 7. Implementation Roadmap

### 7.1 Phase 1: Foundation (v1.3.1)

- [ ] Protobuf Messages erweitern (DifferentialMode, ChunkManifest)
- [ ] Fixed-Block Diff implementieren (einfachster Start)
- [ ] Chunk Catalog Storage
- [ ] Unit Tests

### 7.2 Phase 2: CDC (v1.3.2)

- [ ] Rabin Fingerprinting implementieren
- [ ] Content-Defined Chunking
- [ ] Deduplication Engine
- [ ] Performance Tests

### 7.3 Phase 3: Optimization (v1.4.0)

- [ ] Smart Strategy Selection
- [ ] Binary Diff für kleine Changes
- [ ] Parallel Chunk Transfer
- [ ] Resume bei Unterbrechung

---

## 8. Vergleich mit Industrie-Lösungen

### 8.1 Ähnliche Systeme

**Dropbox:**
- Verwendet CDC (Content-Defined Chunking)
- Average 4 MB chunks
- Deduplication über alle User

**Git LFS (Large File Storage):**
- Content-addressable storage
- SHA256-based dedup
- Delta compression optional

**Restic Backup:**
- CDC mit Rabin Fingerprinting
- Deduplizierung auf Chunk-Level
- Encryption per Chunk

**Docker Registry:**
- Layer-based (ähnlich Fixed-Block)
- SHA256 Content Addressing
- Layer Deduplication

### 8.2 ThemisDB Approach

**Empfehlung: Hybrid aus Dropbox + Restic**

```
ThemisDB Differential Transfer:
├─ Primary: Content-Defined Chunking (wie Dropbox/Restic)
├─ Fallback: Fixed-Block für einfache Fälle
├─ Optimization: Binary Diff für tiny changes
└─ Smart Selection: Automatische Strategie-Auswahl
```

---

## 9. Fazit & Empfehlungen

### 9.1 Best Practice für ThemisDB

✅ **Implementiere Content-Defined Chunking (CDC) als Primary Mode**
- Beste Deduplication
- Resistent gegen Inserts/Deletes
- Industrie-Standard

✅ **Implementiere Fixed-Block als Fast Mode**
- Für Fälle wo Geschwindigkeit wichtiger als optimale Dedup
- Einfachere Implementierung
- Gut für erste Version

✅ **Nutze Intelligente Strategie-Auswahl**
- < 5% Change → Binary Diff (if < 1 GB)
- 5-30% Change → Fixed-Block
- > 30% Change → CDC
- > 90% Change → Full Transfer

✅ **Speichere Chunk Catalog**
- Pro Blob/Version
- Ermöglicht schnelle Delta-Berechnung
- Minimal Overhead (~65 KB für 5 GB)

### 9.2 Erwartete Einsparungen

**Typische LoRA Updates (2-10% Change):**
- Bandwidth Savings: 90-98%
- Transfer Time: 80-95% schneller
- Cost Savings: 90-98% (Cloud Egress)

**ROI-Beispiel:**
```
100 Shards × 10 LoRA Updates/Monat × 5 GB/Update:
├─ Ohne Differential: 5,000 GB Transfer/Monat
├─ Mit Differential (95% savings): 250 GB/Monat
└─ Cloud Cost (@$0.12/GB): $600 → $30 = $570/Monat gespart
```

---

## 10. Nächste Schritte

1. **Protobuf Extension** (sofort)
   - Füge DifferentialMode enum hinzu
   - Füge ChunkManifest messages hinzu

2. **POC Implementation** (v1.3.1)
   - Fixed-Block Diff als erste Version
   - Testen mit echten LoRA Files

3. **Production CDC** (v1.3.2)
   - Rabin Fingerprinting
   - Full CDC Implementation

4. **Optimization** (v1.4.0)
   - Smart Strategy Selection
   - Performance Tuning

---

**Status:** Design Complete - Ready for Implementation  
**Autor:** ThemisDB Development Team  
**Review:** Pending
