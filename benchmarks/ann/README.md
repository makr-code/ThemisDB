> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# ANN-Benchmarks for ThemisDB

## Overview

**ANN-Benchmarks** (Approximate Nearest Neighbor Benchmarks) is the standard benchmark for vector similarity search systems, focusing on high-dimensional vector search performance and accuracy trade-offs.

**Official Website:** http://ann-benchmarks.com/

## What is ANN-Benchmarks?

ANN-Benchmarks evaluates approximate nearest neighbor (ANN) algorithms on standardized datasets:
- **SIFT1M:** 1M 128-dim SIFT image descriptors
- **GIST1M:** 1M 960-dim GIST image descriptors
- **GloVe:** 1.2M 100/200-dim word embeddings
- **Deep1B:** 1B 96-dim deep neural network descriptors
- **Random:** Various synthetic datasets

### ANN vs Exact Search

| Feature | Exact KNN | Approximate NN (ANN) |
|---------|-----------|----------------------|
| **Accuracy** | 100% recall | 90-99% recall |
| **Speed** | O(n) linear scan | O(log n) with index |
| **Memory** | Low (raw vectors) | High (index structure) |
| **Use Case** | Small datasets (<100K) | Large datasets (>1M) |
| **Scalability** | Poor | Excellent |

## Key Metrics

### 1. Recall@k

**Definition:** Fraction of true k-nearest neighbors found

```
Recall@10 = |true_neighbors ∩ found_neighbors| / k
```

**Target:** ≥ 90% for production use (95%+ ideal)

### 2. Queries Per Second (QPS)

**Definition:** Throughput at given recall level

**Target:** 
- SIFT1M @ 90% recall: 10,000+ QPS
- SIFT1M @ 95% recall: 5,000+ QPS

### 3. Index Build Time

**Definition:** Time to construct search index

**Target:** < 10 minutes for 1M vectors (128-dim)

### 4. Index Size

**Definition:** Memory footprint of index

**Target:** < 2× raw vector size

### 5. Search Latency

**Definition:** P50, P95, P99 latency for single query

**Target:** 
- P50: < 1ms
- P95: < 5ms  
- P99: < 10ms

## Standard Datasets

### SIFT1M (Most Popular)

- **Vectors:** 1,000,000 train + 10,000 query
- **Dimensions:** 128
- **Data Type:** uint8 (0-255)
- **Source:** Scale-Invariant Feature Transform image descriptors
- **Use Case:** Image similarity search
- **Size:** ~128 MB (raw vectors)

### GIST1M

- **Vectors:** 1,000,000 train + 1,000 query
- **Dimensions:** 960
- **Data Type:** float32
- **Source:** GIST global image descriptors
- **Use Case:** Image retrieval
- **Size:** ~3.6 GB (raw vectors)

### GloVe

- **Vectors:** 1,183,514 train + 10,000 query
- **Dimensions:** 100 or 200
- **Data Type:** float32
- **Source:** Word embeddings from web text
- **Use Case:** NLP, semantic search
- **Size:** ~450 MB (100-dim) or ~900 MB (200-dim)

### Deep1B (Large Scale)

- **Vectors:** 1,000,000,000 train + 10,000 query
- **Dimensions:** 96
- **Data Type:** float32
- **Source:** Deep neural network outputs
- **Use Case:** Billion-scale vector search
- **Size:** ~360 GB (raw vectors)

## Distance Metrics

### Euclidean (L2)

```
distance = sqrt(Σ(a_i - b_i)²)
```

**Use Case:** Most common, SIFT, image embeddings

### Cosine Similarity

```
similarity = (a · b) / (||a|| × ||b||)
```

**Use Case:** Text embeddings (GloVe, BERT), normalized vectors

### Inner Product (Dot Product)

```
similarity = a · b
```

**Use Case:** Recommendation systems, collaborative filtering

### Angular Distance

```
distance = arccos(cosine_similarity)
```

**Use Case:** Spherical data

## ANN Algorithms

### HNSW (Hierarchical Navigable Small World)

- **Best For:** High recall, medium-large datasets
- **Pros:** Excellent recall/speed trade-off, scalable
- **Cons:** High memory usage (2-5× vectors)
- **Parameters:** M (connections), efConstruction, efSearch

### IVF (Inverted File Index)

- **Best For:** Large datasets, lower memory
- **Pros:** Lower memory than HNSW, fast
- **Cons:** Recall drops faster than HNSW
- **Parameters:** nlist (clusters), nprobe (search clusters)

### ANNOY (Approximate Nearest Neighbors Oh Yeah)

- **Best For:** Read-only workloads, static indices
- **Pros:** Low memory, immutable index
- **Cons:** Lower recall than HNSW
- **Parameters:** n_trees, search_k

### Flat (Brute Force)

- **Best For:** Small datasets, 100% recall required
- **Pros:** Perfect accuracy, simple
- **Cons:** O(n) complexity, slow for large n

## Implementation Plan

### Phase 5.4: ANN Foundation (Week 4)
- [ ] SIFT1M dataset integration
- [ ] HNSW index implementation (basic)
- [ ] Flat (brute force) baseline
- [ ] Recall@k measurement
- [ ] QPS measurement
- [ ] Build system integration
- [ ] Documentation

### Phase 5.4 Extended: Full ANN Suite (Future)
- [ ] Advanced HNSW (optimized)
- [ ] IVF index
- [ ] GloVe dataset
- [ ] GIST1M dataset
- [ ] Multiple distance metrics (L2, cosine, dot product)
- [ ] Batch query optimization
- [ ] Performance comparison with FAISS

## Performance Targets

### Industry Baselines (SIFT1M, 8-core, 32GB, NVMe)

| System | QPS @ 90% Recall | QPS @ 95% Recall | Index Size | Notes |
|--------|------------------|------------------|------------|-------|
| **FAISS (HNSW)** | 15,000 | 8,000 | ~400 MB | CPU, M=16 |
| **FAISS (IVF)** | 12,000 | 6,000 | ~200 MB | CPU, nlist=4096 |
| **Annoy** | 8,000 | 4,000 | ~256 MB | n_trees=100 |
| **Hnswlib** | 18,000 | 10,000 | ~450 MB | C++, M=16 |
| **Elasticsearch** | 2,000 | 1,000 | ~500 MB | Lucene-based |

### ThemisDB Targets (SIFT1M, 8-core, 32GB, NVMe)

| Metric | Target | Baseline (FAISS) |
|--------|--------|------------------|
| **QPS @ 90% recall** | 8,000 - 12,000 | ~15,000 |
| **QPS @ 95% recall** | 5,000 - 8,000 | ~8,000 |
| **Index build time** | < 10 minutes | ~5 minutes |
| **Index size** | < 500 MB | ~400 MB |
| **P50 latency** | < 1ms | ~0.5ms |
| **P95 latency** | < 5ms | ~2ms |

## Usage (Planned)

```bash
# Build
cd build
cmake .. -DTHEMIS_BUILD_BENCHMARKS=ON
make bench_ann

# Run SIFT1M benchmark
./bench_ann --dataset=sift1m --index=hnsw --recall_target=0.90

# Output:
# Building HNSW index...
# Index built in 8.3 minutes
# Index size: 423 MB
# 
# Benchmark Results (k=10):
# Recall@10: 90.5%
# QPS: 9,234
# P50: 0.89ms
# P95: 3.2ms
# P99: 7.1ms

# Run with different parameters
./bench_ann --dataset=sift1m --index=hnsw --M=32 --efConstruction=200 --efSearch=100

# Compare algorithms
./bench_ann --dataset=sift1m --index=flat    # Baseline (100% recall)
./bench_ann --dataset=sift1m --index=hnsw    # HNSW
./bench_ann --dataset=sift1m --index=ivf     # IVF

# Export results
./bench_ann --benchmark_out=ann_results.json --benchmark_out_format=json
```

## Optimization Tips

### For ThemisDB

1. **SIMD:** Use AVX2/AVX-512 for distance computations
2. **Batch Processing:** Process multiple queries together
3. **Memory Layout:** Optimize vector storage for cache efficiency
4. **Prefetching:** Prefetch vectors during graph traversal
5. **Quantization:** Use PQ (Product Quantization) for compression

### HNSW Parameters

**M (connections per node):**
- Higher M → Better recall, higher memory
- Typical: M=16 for SIFT, M=32 for high-dim

**efConstruction (build time search depth):**
- Higher ef → Better index quality, slower build
- Typical: 200-400

**efSearch (query time search depth):**
- Higher ef → Better recall, slower search
- Typical: 100-200 for 90% recall, 200-400 for 95%

## Datasets Download

```bash
# SIFT1M (~135 MB)
wget ftp://ftp.irisa.fr/local/texmex/corpus/sift.tar.gz
tar -xzf sift.tar.gz

# GloVe-100 (~450 MB)
wget http://ann-benchmarks.com/glove-100-angular.hdf5

# Deep1B (~360 GB, optional)
wget http://sites.skoltech.ru/compvision/noimi/deep1b_groundtruth.tar.gz
```

## Status

**Phase 5.4:** Planned for Week 4 of Phase 5  
**Priority:** High (vector search is critical for AI applications)  
**Complexity:** Medium (HNSW implementation, dataset integration)

## References

- **ANN-Benchmarks Website:** http://ann-benchmarks.com/
- **Academic Paper:** Aumüller et al., "ANN-Benchmarks: A Benchmarking Tool for Approximate Nearest Neighbor Algorithms" (SISAP 2017)
- **HNSW Paper:** Malkov & Yashunin, "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs" (TPAMI 2018)
- **FAISS:** https://github.com/facebookresearch/faiss
- **Hnswlib:** https://github.com/nmslib/hnswlib

## License

ANN-Benchmarks datasets are publicly available for research purposes. SIFT1M and GIST1M are from INRIA TEXMEX project. GloVe is from Stanford NLP.
