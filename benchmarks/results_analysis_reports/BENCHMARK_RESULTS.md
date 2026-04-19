> ⚠️ **Historische Messdaten** – Die in diesem Dokument enthaltenen Zahlen entstammen einem bestimmten Messzeipunkt und sind nicht mehr reproduzierbar ohne die ursprüngliche Testumgebung.
> Für reproduzierbare Ergebnisse: Benchmark-Kommandos und aktuelle CMake-Presets unter [`benchmarks/README.md`](../README.md) verwenden.

# Comprehensive Performance Benchmark Suite - Results Summary

**Executed:** 2025-12-18 21:13:51 UTC+1
**Platform:** Windows x64 (20 CPU cores @ 3696 MHz)
**Build:** Release Mode (MSVC 14.44)

## Performance Results Overview

### 🚀 Fastest Operations (Throughput)

| Operation | Throughput | Time/Op |
|-----------|-----------|---------|
| **RAG Search Top-50** | 7.17M ops/s | 140 ns |
| **Graph BFS Traversal** | 9.56M ops/s | 105 ns |
| **AQL Simple WHERE** | 3.43M ops/s | 291 ns |
| **AQL Complex Conditions** | 3.35M ops/s | 298 ns |
| **Binary Blob Retrieval** | 49.0M ops/s | 20 ns |

### 🐌 Slowest Operations (Optimization Candidates)

| Operation | Throughput | Time/Op |
|-----------|-----------|---------|
| **1MB Blob Storage** | 741 ops/s | 1.39 ms |
| **10K Batch Insert (Metadata)** | 128k ops/s | 78.1 μs |
| **1536D Batch LLM Insert** | 116.4k ops/s | 8.59 μs |
| **Complex Vector 4096D Search** | 5.19M ops/s | 192 ns |

---

## Detailed Results by Category

### Simple Vector Operations
- **RGB Vector Insert**: 1.83M vectors/s (548 ns each)
- **RGB KNN Search (Top-10)**: 59.7M queries/s (16.7 ns each)
- **384D Embedding Insert**: 411k vectors/s (244 ns each)

### Complex Vector Operations
- **1536D LLM Batch Insert**: 124.7k vectors/s (8.0 μs each)
- **4096D Top-100 Search Batch**: 5.19M queries/s (192 ns each)

### LLM Inferencing Simulations
- **Embedding Generation + Store**: 113.8k embeddings/s
- **RAG Search (Top-50)**: 7.17M queries/s
- **Multi-Query Expansion (5 queries)**: 2.80M combined results/s

### AQL Query Operations
- **Simple SELECT WHERE**: 3.43M queries/s
- **Complex WHERE (Multiple Conditions)**: 3.35M queries/s
- **JOIN (Users-Posts)**: 10.2M operations/s

### Binary Operations & Blob Storage
- **10KB Thumbnail Storage**: 388.5k blobs/s
- **1MB Document Storage**: 741 documents/s ⚠️ (Performance bottleneck)
- **100KB Blob Retrieval**: 49.0M lookups/s

### Graph Operations
- **Sparse Graph Edge Addition**: 1.26M edges/s
- **Dense Graph Neighbor Query**: 8.96M queries/s
- **Graph BFS Traversal (Depth-3)**: 9.56M traversals/s

### Secondary Index Operations
- **Small Index Insert (1K)**: 1.75M entities/s
- **Medium Index Insert (100K)**: 1.06M entities/s
- **Large Index Lookup (1M)**: 3.12M lookups/s
- **Composite Index Lookup**: 2.40M queries/s

### Batch Operations
- **Batch Insert (10K with Metadata)**: 128k items/s
- **Batch Update (Multi-Field 5K)**: 626k updates/s

### Stress Tests
- **Mixed Read/Write (80/20)**: 1.90M ops/s
- **Hotspot Access (99% Contention)**: 2.90M ops/s

---

## Performance Characteristics

### Scaling Analysis

**Vector Dimension Impact:**
- 3D vectors: 1.83M inserts/s
- 384D vectors: 411k inserts/s (4.5x slower)
- 1536D vectors: 116k inserts/s (16x slower than 3D)
- 4096D vectors: Batch search at 5.19M ops/s

**Dataset Size Impact:**
- Small (1K): 1.75M ops/s
- Medium (100K): 1.06M ops/s (40% slower)
- Large (1M): 3.12M ops/s (optimized with batch)

**Blob Size Impact:**
- 10KB: 388k ops/s
- 100KB: 49.0M ops/s (127x faster!)
- 1MB: 741 ops/s (scale-dependent bottleneck)

### Concurrency & Contention

- **Mixed Read/Write**: 1.90M ops/s (balanced)
- **Hotspot Contention (99%)**: 2.90M ops/s (better than expected!)
  - Cache locality effects dominates
  - Lock contention minimal at database level

---

## Key Insights

### ✅ Strengths
1. **LLM Integration Ready**: RAG search at 7.17M queries/s
2. **Query Performance**: AQL queries sustain 3.3M+ ops/s
3. **Efficient Graph Traversal**: BFS at 9.56M ops/s
4. **Excellent Blob Retrieval**: 49M lookups/s with batch access
5. **Stress Resilient**: Hotspot access maintains 2.9M ops/s

### ⚠️ Optimization Opportunities
1. **Large Blob Storage**: 1MB documents at only 741 ops/s
   - Consider streaming/chunking for blobs > 100KB
2. **High-Dimension Vector Inserts**: 1536D at 116k ops/s
   - Potential for batch optimization
   - Encryption (if enabled) will further reduce this
3. **Batch Metadata Overhead**: 78 μs per 10K items
   - Metadata indexing cost is significant

### 📊 Throughput Hierarchy
```
Fastest:     Binary Retrieval > Binary Access > AQL WHERE
             ↓
Medium:      Graph Operations > Secondary Index > Vector Search
             ↓
Slowest:     Large Blob Storage > High-D Insert > Batch Metadata
```

---

## Recommendations

### For Production Deployment

1. **Vector Storage**: 
   - Use batch inserts for 384D+ vectors
   - Expect 116-411k vectors/s depending on dimension

2. **LLM/RAG Workloads**:
   - Ready for production at 7M+ queries/s
   - Multi-query expansion works efficiently

3. **Large Document Storage**:
   - Implement chunking for blobs > 500KB
   - Consider separate blob store for > 1MB documents

4. **Graph Workloads**:
   - Excellent performance for traversal-heavy workloads
   - 9.56M ops/s BFS supports real-time graph analytics

5. **Query Workloads**:
   - AQL performs consistently at 3.3M+ queries/s
   - Joins and complex queries scale well

---

## Benchmark Files

- **Comprehensive Suite**: `benchmarks/bench_comprehensive.cpp`
  - 25 benchmark functions
  - Covers simple/complex operations
  - LLM, AQL, binary, graph, stress tests

- **Raw Results**: `/tmp/bench_results.json`
  - Full JSON export
  - Suitable for CI/CD integration
  - Historical tracking

---

## Next Steps

1. **Encrypted Vector Performance**: Profile vector operations with encryption enabled
2. **Distributed Benchmarks**: Add network latency simulations
3. **Memory Profiling**: Track memory usage patterns during benchmarks
4. **Comparative Analysis**: Windows vs. Linux performance comparison

