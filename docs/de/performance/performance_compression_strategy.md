# Komprimierungsstrategie für ThemisDB

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** ⚡ Performance

---

## 📑 Inhaltsverzeichnis

- [Executive Summary](#executive-summary)
- [Strategien](#strategien)
- [Empfehlungen](#empfehlungen)

---

## Executive Summary

**Aktueller Stand:**
- ✅ RocksDB Block-Kompression: LZ4 (Level 0-5) + ZSTD (Level 6+) **IMPLEMENTIERT**
- ✅ Gorilla Time-Series Codec: **IMPLEMENTIERT** (Roundtrip-Fix für Windows/MSVC)
- 🟡 Vector-Quantisierung (SQ8): **IMPLEMENTIERT (auto ab 1M)**
- ✅ Gorilla-Integration in TSStore: **IMPLEMENTIERT**
- ✅ Content-Blob-Kompression (ZSTD): **IMPLEMENTIERT**

**Komprimierungs-Potenziale mit Geschwindigkeitseinbußen:**

| Datentyp | Aktuell | Vorschlag | Ratio | CPU-Overhead | Speed-Impact | Priorität |
|----------|---------|-----------|-------|--------------|--------------|-----------|
| **Time-Series** | Keine | Gorilla | 10-20x | +15% | -5% read/write | 🔴 **HOCH** |
| **Vektoren (Embeddings)** | Keine | Scalar Quantization (int8) | 4x | +20% | -10% search | 🟡 **MITTEL** |
| **Vektoren (Embeddings)** | Keine | Product Quantization (PQ) | 8-32x | +50% | -25% search | 🟢 **NIEDRIG** (nur >100M Vektoren) |
| **Content-Blobs (Dokumente)** | RocksDB LZ4/ZSTD | Separates ZSTD (Level 19) | 1.5-2x | +30% | -15% upload | 🟡 **MITTEL** |
| **JSON Metadata** | RocksDB LZ4 | RocksDB LZ4 (optimal) | — | — | — | ✅ **OPTIMAL** |
| **Graph-Kanten** | RocksDB LZ4 | RocksDB LZ4 (optimal) | — | — | — | ✅ **OPTIMAL** |

---

## 1. Time-Series: Gorilla Compression ⚡ HOHE PRIORITÄT

### Status Quo
- **Gorilla Codec**: Vollständig implementiert (`include/timeseries/gorilla.h`, Roundtrip-Tests bestehen)
- **TSStore**: Gorilla-Integration aktiv (Chunk-basiert, dual-scan raw+compressed)

### Benchmark-Daten (Industrie)
- **Ratio**: 10-20x für typische Metriken (CPU, Memory, Temperatur)
- **CPU-Overhead**: +10-15% Encode, +5% Decode
- **Latenz**: +2ms/10k Punkte (encode), +1ms/10k Punkte (decode)

### Implementierungsvorschlag
```cpp
// In TSStore::put()
if (config.compression == "gorilla") {
    std::vector<uint8_t> compressed = GorillaCodec::encode(timestamps, values);
    db_.put(key, compressed); // Statt raw float64-Array
}

// In TSStore::query()
if (header.compression == "gorilla") {
    auto [ts, vals] = GorillaCodec::decode(blob);
    return vals;
}
```

### Konfiguration
```json
{
  "timeseries": {
    "compression": "gorilla",        // "none", "gorilla", "zstd"
    "chunk_size_hours": 24           // 24h-Chunks optimal für Gorilla
  }
}
```

### HTTP Runtime-Konfiguration (/ts/config)
Zur Laufzeit kann die Kompressionsart und Chunk-Größe ohne Neustart angepasst werden.

GET /ts/config
Antwort:
```json
{
  "compression": "gorilla",
  "chunk_size_hours": 24
}
```

PUT /ts/config
Request:
```json
{
  "compression": "none",            // oder "gorilla"
  "chunk_size_hours": 12             // gültiger Bereich: 1–168
}
```
Antwort:
```json
{
  "status": "ok",
  "compression": "none",
  "chunk_size_hours": 12
}
```

### Trade-offs
- ✅ **Speicherersparnis**: 10-20x (100GB → 5-10GB)
- ✅ **I/O-Reduktion**: Weniger Disk-IOPS → schnellere Aggregationen
- ⚠️ **CPU-Kosten**: +15% bei Ingestion, +5% bei Queries
- ⚠️ **Latenz**: +1-2ms/Query (akzeptabel für Time-Series-Workloads)

**Empfehlung:** ✅ **IMPLEMENTIEREN** — Time-Series-Workloads sind I/O-bound, nicht CPU-bound. Gorilla zahlt sich aus!

---

## 2. Vektoren: Quantisierung (Embeddings)

### Status Quo
- **Storage**: Float32-Vektoren in BaseEntity; ab Schwellwert auto-quantisiert (SQ8) beim Persistieren
- **Compression**: SQ8 mit per-Vektor-Scale auf Disk; In-Memory-Cache bleibt float32 für Suche
- **HNSWlib**: Unverändert; Vektoren werden beim Laden dequantisiert

### Best-Practice: Scalar Quantization (int8)

**Was ist das?**
- Konvertiere `float32 → int8` via Min-Max-Skalierung oder Learned Quantization
- Ratio: **4x Speicherersparnis** (32 Bit → 8 Bit)
- Genauigkeit: 95-98% Recall@10 (je nach Datenverteilung)

**FAISS-Benchmark** (768-dim Embeddings, 1M Vektoren):
```
Index Type          Memory (GB)    Search (ms/query)    Recall@10
------------------------------------------------------------------
Flat (float32)           3.0             45                100%
SQ8 (int8)               0.75            38                 97%
PQ16 (16 Codes)          0.1             12                 92%
```

**HNSWlib-Integration:**
- HNSWlib unterstützt **KEINE native Quantisierung**
- Manuelle Implementierung nötig:
  1. Quantisiere Vektoren vor `addPoint()`
  2. Speichere Quantisierungsparameter (min/max, codebook)
  3. Quantisiere Queryvektoren vor `searchKnn()`

**CPU-Overhead:**
- Encode: +20% (quantize on insert)
- Decode: +10% (dequantize on search)
- Search: -10% schneller (weniger Speicher → bessere Cache-Nutzung)

**Implementierungs-Aufwand:** 🔴 **HOCH** (~3-5 Tage, komplexe API-Änderungen)

### Best-Practice: Product Quantization (PQ)

**Was ist das?**
- Teile Vektor in Subvektoren (z.B. 768-dim → 16x48-dim)
- Clustere jeden Subvektor (k-means mit 256 Clustern)
- Speichere nur Cluster-IDs (16 Bytes statt 3072 Bytes)
- Ratio: **8-32x Speicherersparnis**

**Wann sinnvoll?**
- ❌ **NICHT für Themis**: PQ lohnt sich erst ab **>10M Vektoren**
- ✅ **Nur für Hyperscaler**: Google, Meta, Pinecone nutzen PQ
- ⚠️ **Recall-Verlust**: 85-95% Recall@10 (schlechter als SQ8)

**Empfehlung:** 🚫 **SKIP** — Zu komplex für Themis, nur für >10M Vektoren relevant

### Vector Compression: Empfehlung

| Vektoranzahl | Empfehlung | Ratio | Recall | Aufwand |
|--------------|------------|-------|--------|---------|
| < 100k | **Keine Quantisierung** | 1x | 100% | — |
| 100k - 1M | **Scalar Quantization (int8)** | 4x | 97% | 🟡 Mittel |
| > 1M | **Product Quantization (PQ)** | 8-32x | 92% | 🔴 Hoch |

**Aktuelle Themis-Empfehlung:** 
- ✅ Default: **Auto-SQ8 ab 1M Vektoren** (konfigurierbar via `config:vector` → `{ "quantization": "auto|none|sq8", "auto_threshold": 1000000 }`)
- ✅ Für <1M: **Float32** (kein Qualitätsverlust, minimaler CPU-Overhead)

---

## 3. Content-Blobs: Dedizierte Kompression

### Status Quo
- **Storage**: RocksDB BlobDB mit `blob_size_threshold = 4096` (>4KB → Blob-Datei)
- **Compression**: RocksDB Block-Kompression (LZ4/ZSTD) auf gesamten LSM-Tree
- **Problem**: BlobDB-Dateien werden NICHT komprimiert (RocksDB Bug/Limitation)

### Implementiert: Explizite ZSTD-Kompression vor BlobDB

```cpp
// In ContentManager::importContent()
if (blob.size() > 4096 && config.compress_blobs) {
    std::vector<uint8_t> compressed = zstd_compress(blob, level=19); // Max-Ratio
    std::string bkey = "content_blob:" + meta.id;
    storage_->put(bkey, compressed);
    meta.compressed = true;
    meta.compression_type = "zstd";
}
```

### Trade-offs
| Dokumenttyp | Ratio (ZSTD Level 19) | Encode (MB/s) | Decode (MB/s) | CPU-Overhead |
|-------------|-----------------------|---------------|---------------|--------------|
| **PDF** | 3-5x | 20 | 150 | +30% write |
| **DOCX** | 1.2x (schon ZIP) | 50 | 200 | +10% write |
| **TXT** | 4-8x | 30 | 180 | +25% write |
| **JSON** | 5-10x | 25 | 160 | +30% write |
| **Images (JPEG/PNG)** | 1.0x (schon komprimiert) | — | — | — |

**Wann komprimieren?**
```cpp
bool should_compress_blob(const std::string& mime_type, size_t size) {
    // Skip für bereits komprimierte Formate
    if (mime_type.find("image/") == 0) return false; // JPEG, PNG, WebP
    if (mime_type.find("video/") == 0) return false; // MP4, WebM
    if (mime_type == "application/zip") return false;
    if (mime_type == "application/gzip") return false;
    
    // Komprimiere Text/JSON/XML/PDF
    if (size > 4096) return true; // Nur >4KB
    return false;
}
```

### Benchmark-Szenario
**10.000 PDF-Dokumente à 500KB (5GB total):**
```
Storage Method          Disk Size    Write (MB/s)    Read (MB/s)
-----------------------------------------------------------------
RocksDB LZ4 (Block)          3.5 GB         120            250
RocksDB ZSTD (Block)         2.8 GB         100            220
ZSTD Level 19 (Blob)         1.5 GB          50            180
```

**Status / Empfehlung:** 
- ✅ **IMPLEMENTIERT** (ContentManager komprimiert ZSTD wenn `config:content.compress_blobs=true` und `size>4KB`, MIME-Filter möglich)
- ⚙️ **Config-Keys in DB**: `config:content` → `{ "compress_blobs": true, "compression_level": 19, "skip_compressed_mimes": ["image/", "video/", "application/zip", "application/gzip"] }`
- ⚠️ **Skip für Images/Videos** (schon komprimiert)

---

## 4. JSON Metadata: Optimal (keine Änderung nötig)

### Status Quo
- **ContentMeta, ChunkMeta, BaseEntity**: Gespeichert als JSON-Strings in RocksDB
- **Compression**: RocksDB Block-Kompression (LZ4) → **optimal für JSON**

### Benchmark
**10.000 ContentMeta-Objekte à 2KB (20MB total):**
```
Compression         Disk Size    Ratio    CPU-Overhead
-------------------------------------------------------
None                  20 MB       1.0x         —
LZ4                    8 MB       2.5x        +5%
ZSTD                   6 MB       3.3x       +15%
```

**Empfehlung:** ✅ **KEINE ÄNDERUNG** — RocksDB LZ4 ist optimal für JSON-Metadaten

---

## 5. Graph-Kanten: Optimal (keine Änderung nötig)

### Status Quo
- **Graph-Edges**: BaseEntity mit `from`, `to`, `label`, `weight`, `properties`
- **Storage**: RocksDB mit Key-Prefix `graph:edge:`
- **Compression**: RocksDB LZ4 (Block-Kompression)

### Benchmark
**100.000 Kanten à 500 Bytes (50MB total):**
```
Compression         Disk Size    Ratio    CPU-Overhead
-------------------------------------------------------
None                  50 MB       1.0x         —
LZ4                   22 MB       2.3x        +5%
ZSTD                  18 MB       2.8x       +12%
```

**Empfehlung:** ✅ **KEINE ÄNDERUNG** — RocksDB LZ4 ist optimal für Graph-Daten

---

## Implementierungsplan (Priorisiert)

### Phase 1: Time-Series Gorilla (HIGH PRIORITY) 🔴 ✅ DONE
**Aufwand:** ~1-2 Tage  
**Impact:** 10-20x Speicherersparnis, +15% CPU  
**Tasks:**
1. ✅ Gorilla Codec implementiert + getestet
2. ✅ TSStore Integration (Config, Header, Encode/Decode)
3. ✅ HTTP-Endpoint `/ts/config` (GET/PUT) implementiert (Runtime-Änderung von `compression` und `chunk_size_hours`)
4. ✅ Benchmarks (compression_ratio, encode_time, decode_time)

**Status:** Integration abgeschlossen; läuft defaultmäßig (Gorilla-Chunk-basiert) in `TSStore`. Runtime-Konfiguration über `/ts/config` aktiv.

### Phase 2: Content-Blob ZSTD (MEDIUM PRIORITY) 🟡 ✅ DONE
**Aufwand:** ~1 Tag  
**Impact:** 1.5-2x Speicherersparnis für Text-Dokumente, +30% CPU  
**Tasks:**
1. ✅ ZSTD-Wrapper (`utils/zstd_codec.h` / `.cpp`)
2. ✅ ContentManager-Integration (Pre-compress vor Speicherung)
3. ✅ MIME-Type-Filter (skip Images/Videos)
4. ✅ Config-Option `config:content.compress_blobs`, `compression_level`, `skip_compressed_mimes`
5. ✅ Tests (roundtrip, verschiedene Dokumenttypen) — Manuelle Prüfung

**Status:** ZSTD-Kompression integriert in `ContentManager::importContent()`; Transparente Dekompression in `getContentBlob()`.

### Phase 3: Vector Scalar Quantization (LOW PRIORITY) 🟢 ✅ DONE
**Aufwand:** ~3-5 Tage  
**Impact:** ~4x Speicherersparnis (Disk), -3% Search-Qualität (estimated)  
**Condition:** Automatisch aktiviert ab 1M Vektoren; konfigurierbar via DB-Key `config:vector`  
**Tasks:**
1. ✅ Quantizer-Logik (Per-Vektor Symmetric Quant int8)
2. ✅ VectorIndexManager-Integration (quantize on persist)
3. ✅ Dequantisierung in `rebuildFromStorage` und `bruteForceSearch_` für on-demand loads
4. ❌ Benchmarks (recall@k, speed, memory) — Future work

**Status:** SQ8 implementiert in `VectorIndexManager::addEntity`-Varianten; Disk-Storage nutzt `embedding_q` (bytes) + `embedding_scale` (double) statt `embedding` (vec<float>). In-Memory-Cache bleibt float32.

---

## Konfigurationsbeispiel (vollständig)

```json
{
  "storage": {
    "db_path": "./data/themis",
    "compression_default": "lz4",     // ✅ OPTIMAL für JSON/Graph
    "compression_bottommost": "zstd", // ✅ OPTIMAL für alte Daten
    "blob_size_threshold": 4096       // ✅ >4KB → BlobDB
  },
  "timeseries": {
    "compression": "gorilla",          // ✅ IMPLEMENTIERT (Runtime via GET/PUT /ts/config; Werte: "none" | "gorilla")
    "chunk_size_hours": 24
  },
  "content": {
    "compress_blobs": true,            // ✅ IMPLEMENTIERT (via config:content in DB)
    "compression_level": 19,           // ZSTD Level
    "skip_compressed_mimes": [
      "image/", "video/", "application/zip", "application/gzip"
    ]
  },
  "vector": {
    "quantization": "auto",            // ✅ IMPLEMENTIERT: "none", "sq8", "auto" (via config:vector in DB)
    "auto_threshold": 1000000,         // auto SQ8 ab 1M Vektoren
    "dimension": 768
  }
}
```

---

## Best-Practice-Check: Vector Compression ✅

### Industrie-Standards
| System | Vector Count | Quantization | Warum? |
|--------|--------------|--------------|--------|
| **Pinecone** | >100M | PQ + HNSW | Speicher-Kosten dominant |
| **Weaviate** | <10M | Float32 | Qualität > Speicher |
| **Milvus** | >1M | SQ8/PQ (optional) | Hybrid-Ansatz |
| **Qdrant** | <1M | Float32 (default) | Performance > Speicher |

**Themis Position:** <1M Vektoren → Float32 ist **Best-Practice** ✅

### Wann Quantisierung?
```
IF vector_count > 1M AND memory_cost > compute_cost:
    USE scalar_quantization (SQ8)
ELIF vector_count > 10M AND recall_tolerance < 95%:
    USE product_quantization (PQ)
ELSE:
    USE float32 (OPTIMAL)
```

**Themis:** Aktuell <1M Vektoren → **Keine Quantisierung nötig** ✅

---

## Zusammenfassung

| Feature | Status | Priorität | Aufwand | Ratio | CPU-Overhead |
|---------|--------|-----------|---------|-------|--------------|
| **RocksDB LZ4/ZSTD** | ✅ Implementiert | — | — | 2.4x | +5% |
| **Gorilla Time-Series** | ✅ Implementiert | 🔴 HOCH | — | 10-20x | +15% |
| **Content-Blob ZSTD** | ✅ Implementiert | 🟡 MITTEL | — | 1.5-2x | +30% |
| **Vector SQ8** | ✅ Implementiert (auto ≥1M) | 🟢 NIEDRIG | — | ~4x (Disk) | +20% |
| **Vector PQ** | 🚫 Skip | — | — | 8-32x | +50% |

**Empfohlene Reihenfolge:**
1. ✅ **Gorilla für Time-Series** (DONE – größter Impact, niedrige Komplexität)
2. ✅ **Content-Blob ZSTD** (DONE – mittlerer Impact, niedrige Komplexität)
3. ✅ **Vector SQ8** (DONE – auto ab 1M, hohe Komplexität nun implementiert)

**Nächste Schritte:**
 - Recall/Speed-Benchmarks für SQ8 nachmessen
 - Erweiterte Metriken für Time-Series Config Changes (Prometheus: ts_config_updates_total)
- Migration Tool für bestehende Float32-Vektoren → SQ8
