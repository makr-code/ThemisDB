# Enterprise Features Strategie: vLLM AI, Geo-Spatial, IoT/Timescale

**Version:** 1.0  
**Datum:** Dezember 2025  
**Ziel:** Enterprise-Features für v1.2.0+ mit Fokus auf AI, Geo, IoT

## Executive Summary

Analyse der Enterprise-Feature-Bereiche und Library-Optimierungen für:
1. **vLLM AI Support** - LoRA, Vector/HNSW, Embeddings
2. **Geo-Spatial** - PostGIS-Level Features
3. **IoT/Timescale** - Time-Series Analytics

**Strategie:** Bestehende Features ausbauen + gezielte neue Libraries für Enterprise-Differentierung

---

## 1. vLLM AI Support - Enterprise Features

### 1.1 Aktueller Stand (ThemisDB v1.0.x)

**Vorhanden:**
```cpp
// src/llm/llm_interaction_store.cpp
- LLM Interaction Logging
- Prompt Management

// src/index/vector_index.cpp
- HNSW Vector Index (hnswlib)
- Basic Vector Search

// src/index/gnn_embeddings.cpp
- Graph Neural Network Embeddings
- Node2Vec, GraphSAGE

// src/acceleration/faiss_gpu_backend.cpp
- FAISS GPU Backend (basic)
```

**Bereits genutzte Libraries:**
- ✅ **hnswlib** - HNSW Vector Index
- ✅ **FAISS** - GPU-accelerated Vector Search (basic)
- ⚠️ **CUDA** - Minimal genutzt für Vector Ops

**NICHT genutzt:**
- ❌ **LoRA** (Low-Rank Adaptation)
- ❌ **Quantization** (INT8, INT4)
- ❌ **Multi-LoRA Serving**
- ❌ **Embedding Cache**
- ❌ **vLLM Integration** (nur Co-Location, keine direkte Integration)

---

### 1.2 Enterprise Feature: LoRA Support

**Use Case:** Multi-Tenant LoRA Serving mit vLLM
- Base Model (Llama 3 70B) + 100+ LoRA Adapter pro Tenant
- Dynamic LoRA Loading/Unloading
- LoRA Weight Caching in ThemisDB

**Library-Empfehlung: HuggingFace PEFT (Parameter-Efficient Fine-Tuning)**

**Status:** ❌ Nicht integriert  
**Priorität:** 🔥 Hoch (Enterprise-Differenzierung)  
**Engineering Effort:** 6-8 Wochen

**Integration Strategy:**
```cpp
// src/llm/lora_manager.cpp (NEU)
#include <pybind11/pybind11.h> // Python Bridge für PEFT
#include <rocksdb/db.h>

class LoRAManager {
private:
    std::unique_ptr<rocksdb::DB> lora_store_; // LoRA Weights Storage
    std::shared_ptr<Cache> lora_cache_;       // Hot LoRA Adapter Cache
    
public:
    // LoRA Adapter Management
    void registerLoRA(const std::string& tenant_id, 
                      const std::string& lora_id,
                      const std::vector<float>& weights) {
        // Store LoRA weights in RocksDB (komprimiert)
        std::string key = tenant_id + ":" + lora_id;
        std::string value = compressLoRA(weights); // ZSTD Compression
        lora_store_->Put(rocksdb::WriteOptions(), key, value);
    }
    
    // Hot LoRA Loading für vLLM
    std::vector<float> loadLoRA(const std::string& tenant_id,
                                 const std::string& lora_id) {
        std::string key = tenant_id + ":" + lora_id;
        
        // Check Cache first
        if (lora_cache_->has(key)) {
            return lora_cache_->get(key);
        }
        
        // Load from RocksDB
        std::string value;
        lora_store_->Get(rocksdb::ReadOptions(), key, &value);
        auto weights = decompressLoRA(value);
        
        // Warm Cache
        lora_cache_->put(key, weights);
        return weights;
    }
    
    // vLLM Integration via gRPC
    void serveLoRAToVLLM(const std::string& lora_id) {
        auto weights = loadLoRA(tenant_id, lora_id);
        // Send to vLLM via gRPC
        vllm_client_->LoadLoRA(lora_id, weights);
    }
};
```

**Wechselwirkung mit bestehenden Modulen:**
- **RocksDB:** LoRA Weight Storage (komprimiert mit ZSTD)
- **TBB:** Paralleles LoRA Loading für Multi-Tenant
- **vLLM:** gRPC Integration für LoRA Serving
- **OpenTelemetry:** LoRA Cache Hit Rate Metrics

**Zusätzliche Libraries:**

| Library | Zweck | Status | Effort |
|---------|-------|--------|--------|
| **HuggingFace PEFT** | LoRA Adaptation | ❌ Neu | 4 Wochen |
| **gRPC** | vLLM Communication | ⚠️ Optional | 2 Wochen |
| **Protobuf** | LoRA Serialization | ⚠️ Optional | 1 Woche |

**Alternative (weniger Dependencies):**
- Nur LoRA Weight Storage in RocksDB
- vLLM lädt LoRA direkt von File-System (ThemisDB exportiert zu `/tmp/lora/`)
- **Effort:** 2 Wochen (kein gRPC, kein PEFT)

---

### 1.3 Enterprise Feature: Advanced Vector Search

**Use Case:** RAG mit Multi-Vector-Suche
- Hybrid Search (BM25 + Vector)
- Multi-Modal Embeddings (Text, Image, Audio)
- Query Reranking

**Aktuell genutzt:**
- ✅ **hnswlib** - HNSW Index
- ✅ **FAISS** - GPU Backend (basic)

**Ungenutztes Potenzial:**

#### 1.3.1 FAISS Advanced Features

**Status:** ⚠️ Nur basic IndexFlatL2 genutzt  
**Priorität:** 🔥 Hoch

**Ungenutztes Potenzial:**
```cpp
// src/acceleration/faiss_gpu_backend.cpp - Aktuell: Basic Flat Index
// ❌ NICHT genutzt: IVF (Inverted File), PQ (Product Quantization), HNSW-Flat

// Optimierte FAISS Integration
#include <faiss/IndexIVFPQ.h>
#include <faiss/gpu/GpuIndexIVFPQ.h>

class AdvancedVectorIndex {
public:
    void buildIVFPQIndex(const std::vector<float>& vectors, size_t d) {
        // IVF + Product Quantization (10-100x weniger RAM)
        size_t nlist = 1024;  // Anzahl Clusters
        size_t m = 8;         // Subquantizer
        size_t nbits = 8;     // Bits pro Subquantizer
        
        faiss::IndexIVFPQ index(d, nlist, m, nbits);
        
        // Training auf GPU
        faiss::gpu::GpuIndexIVFPQ gpu_index(&res_, &index);
        gpu_index.train(vectors.size() / d, vectors.data());
        gpu_index.add(vectors.size() / d, vectors.data());
        
        // 90% weniger RAM vs. IndexFlatL2!
    }
};
```

**Nutzen:**
- **Memory:** 10-100x Reduktion (wichtig für Milliarden Vektoren)
- **Speed:** 2-10x bei großen Datasets
- **GPU:** Bessere GPU-Nutzung (Batch-Processing)

**Engineering Effort:** 3-4 Wochen (FAISS bereits integriert!)

---

#### 1.3.2 ScaNN (Google Scalable Nearest Neighbors)

**Status:** ❌ Nicht integriert  
**Priorität:** 🟡 Mittel (nur wenn FAISS nicht ausreicht)

**Warum ScaNN?**
- State-of-the-Art Performance (Google Research)
- 2-10x schneller als FAISS bei großen Datasets
- Optimiert für High-Recall Searches

**Wechselwirkung:**
- **Konflikt:** Ähnliche Funktionalität wie FAISS
- **Empfehlung:** Nur wenn > 100M Vektoren (für ThemisDB v1.3.0+)

---

#### 1.3.3 Hybrid Search (BM25 + Vector)

**Status:** ⚠️ Separat implementiert (BM25 in `search/`, Vector in `index/`)  
**Priorität:** 🔥 Hoch

**Fehlende Integration:**
```cpp
// src/search/hybrid_search.cpp (NEU)
class HybridSearch {
public:
    std::vector<Result> search(const std::string& query,
                                const std::vector<float>& query_vector,
                                size_t k) {
        // 1. BM25 Search (Text Relevance)
        auto bm25_results = bm25_index_->search(query, k * 2);
        
        // 2. Vector Search (Semantic Similarity)
        auto vector_results = vector_index_->search(query_vector.data(), k * 2);
        
        // 3. Reciprocal Rank Fusion (RRF)
        return fuseResults(bm25_results, vector_results, k);
    }
    
private:
    std::vector<Result> fuseResults(const std::vector<Result>& bm25,
                                     const std::vector<Result>& vector,
                                     size_t k) {
        // RRF Score = sum(1 / (rank_i + 60))
        std::map<std::string, double> scores;
        
        for (size_t i = 0; i < bm25.size(); ++i) {
            scores[bm25[i].id] += 1.0 / (i + 60);
        }
        for (size_t i = 0; i < vector.size(); ++i) {
            scores[vector[i].id] += 1.0 / (i + 60);
        }
        
        // Sort by RRF score
        // ...
    }
};
```

**Engineering Effort:** 2-3 Wochen (keine neue Library!)

---

### 1.4 Enterprise Feature: Embedding Cache & Reranking

**Use Case:** Kostenreduktion durch Embedding-Caching
- vLLM Embedding-Calls reduzieren (OpenAI API kosten $$$)
- Semantic Caching für Queries

**Aktuell:**
- ✅ `src/cache/semantic_cache.cpp` - Basic Semantic Cache

**Ungenutztes Potenzial:**

#### 1.4.1 Advanced Semantic Caching

**Library-Empfehlung: GPTCache (Open-Source)**

**Status:** ❌ Nicht integriert  
**Priorität:** 🟡 Mittel  
**Engineering Effort:** 3-4 Wochen

**Alternative (keine neue Library):**
```cpp
// src/cache/embedding_cache.cpp - Erweiterte Semantic Cache
class EmbeddingCache {
private:
    std::unique_ptr<VectorIndex> cache_index_; // HNSW für Similarity
    std::unique_ptr<rocksdb::DB> cache_store_;  // Embedding Storage
    
public:
    // Fuzzy Match für ähnliche Queries
    std::optional<std::vector<float>> getEmbedding(const std::string& text) {
        // 1. Text → Vector (Quick Hash)
        auto query_hash = hashText(text);
        
        // 2. Check Exact Match
        std::string value;
        if (cache_store_->Get(rocksdb::ReadOptions(), query_hash, &value).ok()) {
            return deserializeEmbedding(value);
        }
        
        // 3. Fuzzy Match (Semantic Similarity)
        auto temp_embedding = quickEmbedding(text); // Fast but imprecise
        auto similar = cache_index_->search(temp_embedding.data(), 1);
        
        if (similar[0].score > 0.95) { // 95% Similarity Threshold
            return getEmbeddingByID(similar[0].id);
        }
        
        return std::nullopt; // Cache Miss → Call vLLM
    }
    
    void putEmbedding(const std::string& text,
                      const std::vector<float>& embedding) {
        auto hash = hashText(text);
        cache_store_->Put(rocksdb::WriteOptions(), hash, 
                         serializeEmbedding(embedding));
        
        // Add to Similarity Index
        cache_index_->add(hash, embedding.data());
    }
};
```

**Nutzen:**
- **Cost Reduction:** 70-90% weniger vLLM API Calls
- **Latency:** Cache Hit = 1ms vs. vLLM Call = 50-100ms

**Engineering Effort:** 2-3 Wochen (nur Code, keine Library!)

---

## 2. Geo-Spatial - Enterprise Features

### 2.1 Aktueller Stand

**Vorhanden:**
```cpp
// src/geo/boost_cpu_exact_backend.cpp
- Boost.Geometry für grundlegende Geo-Ops
- CPU-basierte Berechnungen

// src/geo/gpu_backend_stub.cpp
- GPU Backend Stub (nicht implementiert)

// src/index/spatial_index.cpp
- R-Tree für Spatial Indexing
```

**Bereits genutzte Libraries:**
- ✅ **Boost.Geometry** - Geo Operations (2D)
- ⚠️ **CUDA** - Stub vorhanden, nicht implementiert

**Fehlende PostGIS-Level Features:**
- ❌ 3D Geometries (Z-Koordinate)
- ❌ Geography (Spherical Coordinates)
- ❌ Topology (ST_TopologyPreservingSimplify)
- ❌ Raster Operations
- ❌ Routing (Shortest Path on Road Networks)

---

### 2.2 Enterprise Feature: PostGIS-Kompatibilität

**Use Case:** Drop-in Replacement für PostGIS
- Migration von PostgreSQL+PostGIS → ThemisDB
- Spatial SQL Queries (ST_* Functions)

**Library-Empfehlung: GEOS + PROJ + GDAL**

#### 2.2.1 GEOS (Geometry Engine Open Source)

**Status:** ❌ Nicht genutzt (Boost.Geometry statt GEOS)  
**Priorität:** 🔥 Hoch (PostGIS Compatibility)  
**Engineering Effort:** 4-6 Wochen

**Warum GEOS?**
- PostGIS nutzt GEOS → 100% Kompatibilität
- Topology Operations (Buffer, Union, Intersection)
- 3D Geometries

**Integration:**
```cpp
// src/geo/geos_backend.cpp (NEU)
#include <geos/geom/GeometryFactory.h>
#include <geos/operation/buffer/BufferOp.h>

class GEOSBackend {
public:
    Geometry buffer(const Geometry& geom, double distance) {
        auto geos_geom = toGEOS(geom);
        auto buffered = geos::operation::buffer::BufferOp::bufferOp(
            geos_geom.get(), distance
        );
        return fromGEOS(buffered.get());
    }
    
    // PostGIS ST_* Functions
    bool st_intersects(const Geometry& a, const Geometry& b) {
        return toGEOS(a)->intersects(toGEOS(b).get());
    }
    
    Geometry st_union(const std::vector<Geometry>& geoms) {
        // GEOS UnaryUnion (optimiert)
        auto geos_geoms = toGEOSVector(geoms);
        auto unioned = geos::operation::geounion::CascadedUnion::Union(
            geos_geoms
        );
        return fromGEOS(unioned.get());
    }
};
```

**Wechselwirkung:**
- **Boost.Geometry:** Bleibt für 2D Basic Ops (leichtgewichtig)
- **GEOS:** Für PostGIS-Kompatibilität + Topology
- **Strategie:** Hybrid (Boost für Performance, GEOS für Compatibility)

---

#### 2.2.2 PROJ (Coordinate Transformations)

**Status:** ❌ Nicht genutzt  
**Priorität:** 🔥 Hoch (Geography Support)  
**Engineering Effort:** 2-3 Wochen

**Warum PROJ?**
- Coordinate Transformations (WGS84 ↔ UTM ↔ Web Mercator)
- Geography Support (Spherical Distances)

**Integration:**
```cpp
// src/geo/proj_backend.cpp (NEU)
#include <proj.h>

class PROJBackend {
private:
    PJ_CONTEXT* ctx_;
    PJ* transformer_;
    
public:
    Point transform(const Point& p, const std::string& from_crs,
                    const std::string& to_crs) {
        PJ* transformer = proj_create_crs_to_crs(
            ctx_, from_crs.c_str(), to_crs.c_str(), nullptr
        );
        
        PJ_COORD coord = proj_coord(p.x, p.y, 0, 0);
        PJ_COORD result = proj_trans(transformer, PJ_FWD, coord);
        
        return Point{result.xy.x, result.xy.y};
    }
    
    // Geography Distance (Spherical)
    double st_distance_sphere(const Point& a, const Point& b) {
        // WGS84 Ellipsoid
        PJ* geod = proj_create(ctx_, "+proj=geod +ellps=WGS84");
        double distance;
        proj_geod_direct(geod, a.y, a.x, /* azimuth */ 0, 
                         b.y, b.x, &distance, nullptr);
        return distance;
    }
};
```

---

#### 2.2.3 GDAL (Raster Operations)

**Status:** ❌ Nicht genutzt  
**Priorität:** 🟡 Mittel (nur für Raster-Workloads)  
**Engineering Effort:** 6-8 Wochen

**Warum GDAL?**
- Raster Data (GeoTIFF, HDF5, etc.)
- Raster Algebra (NDVI, Slope, etc.)
- Used by PostGIS Raster Extension

**Empfehlung:** Nur wenn Raster-Support explizit gefordert (v1.3.0+)

---

### 2.3 Enterprise Feature: GPU-Accelerated Geo Operations

**Use Case:** Real-Time Geo Processing
- Millions of Polygons
- Real-Time Intersection Queries

**Aktuell:**
- ❌ `src/geo/gpu_backend_stub.cpp` - Stub, nicht implementiert

**Library-Empfehlung: cuSpatial (NVIDIA RAPIDS)**

**Status:** ❌ Nicht integriert  
**Priorität:** 🟡 Mittel (nur für massive Geo Workloads)  
**Engineering Effort:** 6-8 Wochen

**Warum cuSpatial?**
- 10-100x Speedup für Spatial Joins
- GPU-accelerated Point-in-Polygon
- Arrow Integration (Zero-Copy)

**Integration:**
```cpp
// src/geo/cuspatial_backend.cpp (NEU)
#include <cuspatial/point_in_polygon.hpp>
#include <cudf/table/table.hpp>

class cuSpatialBackend {
public:
    std::vector<bool> pointInPolygon(const std::vector<Point>& points,
                                      const Polygon& poly) {
        // Points → cuDF Table (Arrow-based)
        auto points_table = pointsToArrowTable(points);
        
        // Polygon → cuDF Table
        auto poly_table = polygonToArrowTable(poly);
        
        // GPU Point-in-Polygon (100x faster than CPU)
        auto result = cuspatial::point_in_polygon(
            points_table, poly_table
        );
        
        return arrowToBoolVector(result);
    }
};
```

**Wechselwirkung:**
- **Arrow:** cuSpatial nutzt Arrow → Zero-Copy Integration
- **CUDA:** cuSpatial basiert auf CUDA (Kernbestand!)
- **RocksDB:** Geo-Index bleibt in RocksDB, GPU nur für Compute

---

## 3. IoT / Timescale - Enterprise Features

### 3.1 Aktueller Stand

**Vorhanden:**
```cpp
// src/timeseries/timeseries.cpp
- Time-Series Storage
- Gorilla Compression

// src/timeseries/aggregate_scheduler.cpp
- Continuous Aggregates
- Downsampling

// src/timeseries/tsstore.cpp
- Specialized Time-Series Store
```

**Bereits genutzte Libraries:**
- ✅ **RocksDB** - Time-Series Storage
- ✅ **Custom Gorilla Codec** - Compression
- ⚠️ **Arrow** - Nicht für Time-Series genutzt

**Fehlende TimescaleDB-Level Features:**
- ❌ Hypertables (automatische Partitionierung)
- ❌ Compression Policies (automatische Kompression)
- ❌ Retention Policies (TTL bereits in v1.1.0!)
- ❌ Continuous Aggregates (teilweise vorhanden)

---

### 3.2 Enterprise Feature: TimescaleDB-Kompatibilität

**Use Case:** Drop-in Replacement für TimescaleDB
- IoT Sensor Data (Millionen Devices)
- Real-Time Analytics

**Library-Empfehlung: Keine (RocksDB + Arrow ausreichend!)**

#### 3.2.1 Hypertables mit RocksDB Column Families

**Status:** ⚠️ Teilweise implementiert (Column Families vorhanden)  
**Priorität:** 🔥 Hoch  
**Engineering Effort:** 3-4 Wochen (keine neue Library!)

**Optimierung:**
```cpp
// src/timeseries/hypertable.cpp (NEU - nur Code, keine Library!)
class Hypertable {
private:
    std::unique_ptr<rocksdb::DB> db_;
    std::map<int64_t, rocksdb::ColumnFamilyHandle*> chunks_;
    
public:
    void insert(int64_t timestamp, const std::string& data) {
        // Auto-Partitioning: 1 Chunk pro Tag
        int64_t chunk_id = timestamp / (24 * 3600 * 1000000); // Mikrosekunden
        
        auto cf = getOrCreateChunk(chunk_id);
        
        std::string key = std::to_string(timestamp);
        db_->Put(rocksdb::WriteOptions(), cf, key, data);
    }
    
    rocksdb::ColumnFamilyHandle* getOrCreateChunk(int64_t chunk_id) {
        if (chunks_.count(chunk_id) == 0) {
            rocksdb::ColumnFamilyOptions cf_opts;
            
            // Chunk-specific TTL (RocksDB v1.1.0!)
            cf_opts.ttl = 30 * 24 * 3600; // 30 Tage Retention
            
            rocksdb::ColumnFamilyHandle* cf;
            db_->CreateColumnFamily(cf_opts, "chunk_" + std::to_string(chunk_id), &cf);
            chunks_[chunk_id] = cf;
        }
        return chunks_[chunk_id];
    }
};
```

**Nutzen:**
- **Automatic Partitioning:** Wie TimescaleDB Hypertables
- **Retention Policies:** Via RocksDB TTL (v1.1.0!)
- **Compression:** Via RocksDB Compression (bereits vorhanden)

**Engineering Effort:** 3-4 Wochen (nur Code!)

---

#### 3.2.2 Arrow für Time-Series Aggregations

**Status:** ⚠️ Arrow vorhanden, nicht für Time-Series genutzt  
**Priorität:** 🔥 Hoch  
**Engineering Effort:** 2-3 Wochen

**Optimierung:**
```cpp
// src/timeseries/arrow_aggregator.cpp (NEU)
#include <arrow/compute/api.h>

class ArrowTimeSeriesAggregator {
public:
    arrow::Table aggregateTimeSeriesHour(const std::string& metric,
                                          int64_t start, int64_t end) {
        // 1. Load Time-Series from RocksDB
        auto data = loadTimeSeriesRange(metric, start, end);
        
        // 2. Convert to Arrow Table
        auto table = timeSeriesDataToArrowTable(data);
        
        // 3. Arrow Compute: GROUP BY time_bucket('1 hour', timestamp)
        arrow::compute::ExecContext ctx;
        auto grouped = arrow::compute::CallFunction(
            "hash_aggregate",
            {
                arrow::Datum(table),
                arrow::compute::HashAggregateOptions(
                    /* group_by */ {"time_bucket_1h"},
                    /* aggregates */ {
                        {"value", "mean"},
                        {"value", "min"},
                        {"value", "max"}
                    }
                )
            },
            nullptr, &ctx
        );
        
        return grouped.table();
    }
};
```

**Nutzen:**
- **SIMD Performance:** Arrow Compute ist 5-10x schneller als Custom Code
- **Memory Efficiency:** Columnar Format (weniger RAM)

---

#### 3.2.3 Apache Parquet für Cold Storage

**Status:** ⚠️ Arrow Parquet in v1.1.0 geplant!  
**Priorität:** 🔥 Hoch  
**Engineering Effort:** 2 Wochen (bereits in v1.1.0!)

**Time-Series Use Case:**
```cpp
// src/timeseries/parquet_archiver.cpp (v1.1.0)
void archiveOldTimeSeriesData() {
    // 1. Old Time-Series (> 30 Tage) → Parquet
    auto old_data = loadTimeSeriesOlderThan(30 * 24 * 3600);
    
    // 2. Convert to Arrow Table
    auto table = timeSeriesDataToArrowTable(old_data);
    
    // 3. Write Parquet (ZSTD Compression)
    parquet::WriterProperties::Builder builder;
    builder.compression(parquet::Compression::ZSTD);
    builder.compression_level(9); // Max Compression
    
    parquet::arrow::WriteTable(*table, arrow::default_memory_pool(),
                               output_stream, chunk_size, builder.build());
    
    // 4. Delete from RocksDB
    deleteTimeSeriesOlderThan(30 * 24 * 3600);
}
```

**Nutzen:**
- **Storage Cost:** 90% Reduktion (Parquet vs. RocksDB)
- **Query Performance:** DuckDB kann direkt Parquet querien (v1.2.0!)

---

## 4. Zusammenfassung: Enterprise Features Roadmap

### 4.1 v1.2.0 - AI & Geo Enhancements (Q2 2026)

**AI Features:**
| Feature | Library | Neu? | Effort | Priorität |
|---------|---------|------|--------|-----------|
| LoRA Manager | HuggingFace PEFT | ✅ | 6-8 W | 🔥 Hoch |
| FAISS IVF+PQ | FAISS (erweitert) | ❌ | 3-4 W | 🔥 Hoch |
| Hybrid Search | - (nur Code) | ❌ | 2-3 W | 🔥 Hoch |
| Embedding Cache | - (nur Code) | ❌ | 2-3 W | 🟡 Mittel |

**Geo Features:**
| Feature | Library | Neu? | Effort | Priorität |
|---------|---------|------|--------|-----------|
| GEOS Integration | GEOS | ✅ | 4-6 W | 🔥 Hoch |
| PROJ Transforms | PROJ | ✅ | 2-3 W | 🔥 Hoch |
| cuSpatial GPU Ops | cuSpatial | ✅ | 6-8 W | 🟡 Mittel |

**Total:** 25-35 Wochen (parallelisierbar auf 12-16 Wochen mit Team)

---

### 4.2 v1.3.0 - IoT & Advanced Features (Q3-Q4 2026)

**IoT Features:**
| Feature | Library | Neu? | Effort | Priorität |
|---------|---------|------|--------|-----------|
| Hypertables | - (RocksDB CF) | ❌ | 3-4 W | 🔥 Hoch |
| Arrow Aggregates | Arrow Compute | ❌ | 2-3 W | 🔥 Hoch |
| Parquet Archive | Arrow Parquet | ❌ | 2 W | ✅ v1.1.0! |

**Advanced AI:**
| Feature | Library | Neu? | Effort | Priorität |
|---------|---------|------|--------|-----------|
| ScaNN Vector Search | ScaNN | ✅ | 6-8 W | 🟢 Niedrig |
| Multi-LoRA Serving | - (erweitert) | ❌ | 4-6 W | 🟡 Mittel |

**Advanced Geo:**
| Feature | Library | Neu? | Effort | Priorität |
|---------|---------|------|--------|-----------|
| GDAL Raster | GDAL | ✅ | 6-8 W | 🟢 Niedrig |
| Routing Engine | OSRM (optional) | ✅ | 8-10 W | 🟢 Niedrig |

---

### 4.3 Neue Dependencies: Kosten-Nutzen-Analyse

**TIER 1 - Essenziell für v1.2.0:**
| Library | Nutzen | Verwaltung | Empfehlung |
|---------|--------|------------|------------|
| **GEOS** | PostGIS Compatibility | +6% | ✅ JA |
| **PROJ** | Geography Support | +6% | ✅ JA |
| **HuggingFace PEFT** | LoRA Support | +6% | ✅ JA (via Python) |

**TIER 2 - Optional für v1.2.0:**
| Library | Nutzen | Verwaltung | Empfehlung |
|---------|--------|------------|------------|
| **cuSpatial** | GPU Geo (10-100x) | +6% | 🟡 Wenn Geo-Heavy |
| **gRPC/Protobuf** | vLLM Integration | +12% | 🟡 Optional (File-based alternative) |

**TIER 3 - Nur v1.3.0+:**
| Library | Nutzen | Verwaltung | Empfehlung |
|---------|--------|------------|------------|
| **ScaNN** | Vector Search (> 100M) | +6% | 🟢 Nur bei Bedarf |
| **GDAL** | Raster Operations | +6% | 🟢 Niche Use Case |
| **OSRM** | Routing | +6% | 🟢 Specialized |

---

### 4.4 Wechselwirkungen: Optimierte Integration

**AI Stack:**
```
vLLM (extern) ←─────→ ThemisDB LoRA Manager (PEFT)
                          ↓
                      RocksDB (LoRA Storage, ZSTD)
                          ↓
                      TBB (Parallel Loading)
                          ↓
                      FAISS (Vector Index, IVF+PQ)
                          ↓
                      CUDA (GPU Acceleration, Kernbestand!)
```

**Geo Stack:**
```
PostGIS-Queries ←─────→ ThemisDB GEOS Backend
                            ↓
                        PROJ (Coordinate Transform)
                            ↓
                        Boost.Geometry (2D Basic Ops)
                            ↓
                        cuSpatial (GPU Geo Ops, optional)
                            ↓
                        RocksDB R-Tree Index
```

**IoT/Time-Series Stack:**
```
TimescaleDB-Queries ←──→ ThemisDB Hypertables (RocksDB CF)
                            ↓
                        RocksDB TTL (v1.1.0, Retention)
                            ↓
                        Arrow Compute (Aggregations)
                            ↓
                        Parquet (Cold Storage, v1.1.0)
                            ↓
                        DuckDB (Parquet Queries, v1.2.0)
```

---

## 5. Empfehlungen

### v1.2.0 Focus (Q2 2026):
1. **GEOS + PROJ** (6-9 Wochen) - PostGIS Compatibility
2. **LoRA Manager** (6-8 Wochen) - vLLM AI Support
3. **FAISS Advanced** (3-4 Wochen) - Vector Search Optimization
4. **Hybrid Search** (2-3 Wochen) - RAG Performance

**Total:** 17-24 Wochen (parallelisierbar auf ~12 Wochen)

### v1.3.0 Focus (Q3-Q4 2026):
1. **Hypertables** (3-4 Wochen) - TimescaleDB Compatibility
2. **cuSpatial** (6-8 Wochen) - GPU Geo (optional)
3. **Multi-LoRA Serving** (4-6 Wochen) - Advanced AI

---

## 6. Strategische Überlegungen

**Minimale Dependencies:**
- **v1.2.0:** +3 neue Libs (GEOS, PROJ, PEFT)
- **Verwaltungsaufwand:** +18% (3/16)

**Fokus auf bestehende Libraries:**
- ✅ RocksDB: Hypertables, LoRA Storage, Geo Index
- ✅ Arrow: Time-Series Aggregations, Parquet Archive (v1.1.0!)
- ✅ FAISS: Advanced Vector Search (keine neue Lib!)
- ✅ CUDA: cuSpatial, FAISS GPU (Kernbestand!)

**Philosophie:**
"Enterprise Features durch Smart Combination bestehender Libs + gezielte neue Libs für Compatibility"

---

**Anhänge:**
- A: LoRA Manager Implementation Guide
- B: GEOS/PostGIS Compatibility Matrix
- C: FAISS IVF+PQ Benchmark Results
- D: Hypertable vs. TimescaleDB Performance Comparison
