---
name: Product Quantization Research
about: Research on Product Quantization improvements for vector indexing
title: '[PQ RESEARCH] '
labels: ['type:discussion', 'area:llm', 'area:performance', 'priority:P2', 'effort:medium']
assignees: ''
---

## Product Quantization Research / Product-Quantization-Forschung

### Research Topic / Forschungsthema
<!-- Specific aspect of Product Quantization to investigate -->

## Background / Hintergrund

### Current PQ Implementation in ThemisDB
<!-- Describe current PQ usage, if any -->
- **Current Method:** [ ] Not implemented [ ] Basic PQ [ ] Optimized PQ [ ] Other: ______
- **Compression Ratio:** <!-- e.g., 32:1 (1024D → 32 bytes) -->
- **Recall@10:** <!-- e.g., 90% -->
- **Query Overhead:** <!-- e.g., +5ms vs uncompressed -->

### Problem Statement / Problemstellung
<!-- Why investigate PQ improvements? -->
- 
- 

## Research Focus / Forschungsschwerpunkt

### PQ Variants to Investigate / Zu untersuchende PQ-Varianten
- [ ] **Optimized Product Quantization (OPQ)**
  - Rotation matrix learning for better subspace alignment
  - Papers: Ge et al. (CVPR 2014), Matsui et al. (2015)
  - Expected improvement: +5-10% recall, -10% distortion

- [ ] **Additive Quantization (AQ)**
  - Sum of M codewords instead of product
  - Papers: Babenko & Lempitsky (ICCV 2014)
  - Expected improvement: Better reconstruction, higher memory

- [ ] **Residual Quantization (RQ)**
  - Iterative quantization of residuals
  - Papers: Chen et al. (CVPR 2010)
  - Expected improvement: +3-5% recall, multi-stage refinement

- [ ] **Polysemous Codes**
  - Dual interpretation of codes for fast filtering
  - Papers: Douze et al. (ECCV 2016)
  - Expected improvement: 2-5x faster filtering, same recall

- [ ] **Locally-Adaptive Product Quantization**
  - Adapt quantizers to local data distribution
  - Papers: Kalantidis & Avrithis (CVPR 2014)
  - Expected improvement: +5-8% recall, +20% build time

- [ ] **Cartesian k-means**
  - Jointly optimize all codebooks
  - Papers: Norouzi & Fleet (CVPR 2013)
  - Expected improvement: +10-15% recall, 2-3x build time

### Key Research Questions / Wichtige Forschungsfragen
1. **Compression-Accuracy Trade-off:** How much recall is lost at different compression ratios?
2. **Build Time:** What is the offline training cost for different PQ variants?
3. **Query Performance:** How do asymmetric distance computations (ADC) compare?
4. **Hardware Utilization:** Can we leverage SIMD/GPU for PQ distance calculations?
5. **Scalability:** How do methods scale to billions of vectors and high dimensions?

## Technical Details / Technische Details

### Product Quantization Fundamentals / PQ-Grundlagen

**Standard PQ:**
```
1. Split D-dimensional vector into M subspaces (D/M dimensions each)
2. Train M independent codebooks (k centroids each)
3. Encode: Map each subspace to nearest centroid ID
4. Result: M × log₂(k) bits per vector (e.g., M=8, k=256 → 64 bits)
```

**Asymmetric Distance Computation (ADC):**
```cpp
// Query: full precision (D dimensions)
// Database: PQ codes (M subquantizers)
float asymmetric_distance(const float* query, const uint8_t* codes) {
    float dist = 0.0f;
    for (int m = 0; m < M; m++) {
        // Precompute distances from query subvector to all centroids
        dist += lookup_table[m][codes[m]];
    }
    return dist;
}
```

### Performance Characteristics / Performance-Eigenschaften

| Method | Compression | Recall@10 | Build Time | Query Time | SIMD-friendly |
|--------|-------------|-----------|------------|------------|---------------|
| No compression | 1:1 | 100% | 0 | Baseline | ✓ |
| Standard PQ | 32:1 | 85-90% | 1x | 0.5x | ✓ |
| OPQ | 32:1 | 90-95% | 2x | 0.5x | ✓ |
| AQ | 16:1 | 92-96% | 3x | 0.6x | ✓ |
| RQ | 32:1 | 88-93% | 1.5x | 0.7x | ✓ |
| Polysemous | 32:1 | 85-90% | 1.2x | 0.2x (filter) | ✓✓ |

## State-of-the-Art Research / Stand der Forschung

### Key Papers / Wichtige Papiere

#### 1. Optimized Product Quantization (OPQ)
- **Authors:** Tiezheng Ge, Kaiming He, Qifa Ke, Jian Sun
- **Venue:** CVPR 2014
- **Key Innovation:** Learn rotation matrix R to align data with quantization axes
- **Performance:** +5-10% recall over standard PQ
- **Complexity:** O(D³) for rotation learning
- **Code Available:** Yes (FAISS, PQTable)

#### 2. Additive Quantization (AQ)
- **Authors:** Artem Babenko, Victor Lempitsky
- **Venue:** ICCV 2014
- **Key Innovation:** Sum of M codewords instead of concatenation
- **Performance:** Better reconstruction, higher recall
- **Complexity:** O(M × k × D) per iteration
- **Code Available:** Yes (AQCpp)

#### 3. Polysemous Codes
- **Authors:** Matthijs Douze, Hervé Jégou, Florent Perronnin
- **Venue:** ECCV 2016
- **Key Innovation:** Codes interpretable as both PQ and binary hashing
- **Performance:** 2-5x faster filtering at same recall
- **Complexity:** Similar to standard PQ
- **Code Available:** Yes (FAISS)

#### 4. Cartesian k-means
- **Authors:** Mohammad Norouzi, David J. Fleet
- **Venue:** CVPR 2013
- **Key Innovation:** Joint optimization of all codebooks
- **Performance:** +10-15% recall, but 2-3x slower training
- **Complexity:** O(iterations × n × M × k × D)
- **Code Available:** Partial (research code)

### Recent Advances (2020-2026) / Neueste Fortschritte
<!-- List recent papers beyond the classics -->
1. **[ScaNN]** Guo et al., "Accelerating Large-Scale Inference with Anisotropic Vector Quantization" (ICML 2020)
   - Anisotropic quantization aware training
   - 2-3x better compression vs OPQ

2. **[Deep PQ]** Klein & Wolf, "End-to-End Supervised Product Quantization" (ICCV 2019)
   - Learn PQ parameters end-to-end with neural networks
   - +5-10% recall, requires training data

3. **Other:**
   - 
   - 

## Benchmark Plan / Benchmark-Plan

### Datasets / Datensätze
- [ ] **SIFT1M** (1M vectors, 128D) - Standard benchmark
- [ ] **GIST1M** (1M vectors, 960D) - High-dimensional
- [ ] **Deep1B** (1B vectors, 96D) - Large-scale
- [ ] **ThemisDB Production Data** (Real workload)

### Evaluation Metrics / Bewertungsmetriken
1. **Recall@k:** Recall at k=1, 10, 100
2. **Compression Ratio:** Original size / Compressed size
3. **Build Time:** Time to train codebooks and encode database
4. **Query Latency:** p50, p95, p99 query times
5. **Memory Footprint:** Total RAM usage
6. **Distance Computation Cost:** CPU cycles per distance computation

### Baseline / Referenz
- **Method:** HNSW + Standard PQ (M=8, k=256)
- **Recall@10:** <!-- Current performance -->
- **Memory:** <!-- Current memory usage -->
- **Query Time:** <!-- Current latency -->

## Implementation Plan / Implementierungsplan

### Phase 1: Prototype (2-3 weeks)
- [ ] Implement 2-3 most promising PQ variants
- [ ] Integrate with existing vector index infrastructure
- [ ] Create unit tests and validation suite

### Phase 2: Benchmark (1-2 weeks)
- [ ] Run benchmarks on standard datasets
- [ ] Compare against baseline (HNSW + standard PQ)
- [ ] Profile CPU/memory usage

### Phase 3: Optimization (1-2 weeks)
- [ ] SIMD optimization (AVX2/AVX-512)
- [ ] GPU implementation (CUDA/HIP)
- [ ] Multi-threading

### Phase 4: Integration (2-3 weeks)
- [ ] API design for PQ configuration
- [ ] Migration path for existing indexes
- [ ] Documentation and examples

## Dependencies / Abhängigkeiten

### Libraries / Bibliotheken
- **FAISS** (Meta AI): Comprehensive PQ implementations
- **PQTable** (Matsui): Standalone OPQ/PQ library
- **Eigen/BLAS**: Linear algebra for rotation learning
- **OpenMP/TBB**: Multi-threading

### Hardware / Hardware
- **CPU:** AVX2 support (for SIMD)
- **GPU:** CUDA 11.8+ or HIP (for GPU variants)
- **Memory:** Sufficient RAM for codebook training

## Expected Outcomes / Erwartete Ergebnisse

### Success Criteria / Erfolgskriterien
1. **Compression:** Achieve 16:1 to 32:1 compression ratio
2. **Recall:** Maintain 90%+ recall@10 (vs 100% uncompressed)
3. **Speed:** <5% query latency overhead
4. **Memory:** Reduce index size by 10-30x
5. **Scalability:** Support 100M+ vectors

### Deliverables / Liefergegenstände
- [ ] Research report with comparative analysis
- [ ] Benchmark results on standard datasets
- [ ] Prototype implementation(s)
- [ ] Integration roadmap and effort estimate
- [ ] Recommendation: Which PQ variant for ThemisDB?

## Integration Considerations / Integrationsüberlegungen

### API Design / API-Design
```cpp
// Example configuration
VectorIndexConfig config;
config.index_type = IndexType::HNSW;
config.compression = CompressionType::OPTIMIZED_PQ;
config.pq_config = {
    .num_subquantizers = 8,
    .codebook_size = 256,
    .training_size = 10000,
    .use_opq_rotation = true
};
```

### Backward Compatibility / Rückwärtskompatibilität
- [ ] Support legacy uncompressed indexes
- [ ] Provide migration tool for existing indexes
- [ ] Allow per-collection compression configuration

### Testing / Testen
- [ ] Unit tests for PQ encoding/decoding
- [ ] Integration tests with vector search
- [ ] Regression tests for recall accuracy
- [ ] Performance benchmarks

## Additional Context / Zusätzlicher Kontext

### Related Issues / Verwandte Issues
<!-- Link to related vector indexing issues -->
- 
- 

### External Resources / Externe Ressourcen
- **FAISS Documentation:** https://github.com/facebookresearch/faiss/wiki
- **PQ Tutorial:** http://mccormickml.com/2017/10/13/product-quantizer-tutorial-part-1/
- **Benchmark Results:** http://ann-benchmarks.com/

---

**Checklist:**
- [ ] I have identified specific PQ variants to investigate
- [ ] I have listed key research papers
- [ ] I have defined benchmark datasets and metrics
- [ ] I have outlined an implementation plan
- [ ] I have considered integration and testing requirements
