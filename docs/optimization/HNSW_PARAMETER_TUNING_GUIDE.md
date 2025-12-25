# HNSW Parameter Tuning Guide for ThemisDB

**Date:** December 25, 2024  
**Version:** 1.0  
**Target:** ThemisDB v1.4.0+  
**Source:** THEMISDB_OPTIMIZATION_IMPACT_ANALYSIS_SUMMARY.md

---

## 📋 Executive Summary

This guide provides comprehensive recommendations for tuning HNSW (Hierarchical Navigable Small World) parameters in ThemisDB to optimize vector search performance based on your specific use case.

**Key Takeaway:** Properly tuned HNSW parameters can provide **+15-40% search speed** or **+10-20% recall** improvement depending on your workload requirements.

---

## 🎯 HNSW Parameters Explained

### Core Parameters

#### 1. **M (Maximum Connections per Node)**

**What it controls:** Number of bi-directional links created for each node in the graph.

**Impact:**
- **Lower M (8-12):**
  - ✅ Faster index building
  - ✅ Lower memory usage (~30% less than M=16)
  - ⚠️ Slightly lower recall (90-93%)
  - **Use case:** Edge devices, memory-constrained environments

- **Medium M (16-24):**
  - ✅ Balanced performance
  - ✅ Good recall (95-97%)
  - ✅ Moderate memory usage
  - **Use case:** Production deployments (recommended)

- **Higher M (32-48):**
  - ✅ Best recall (98-99%)
  - ⚠️ Slower index building (+2-3×)
  - ⚠️ Higher memory usage (+60% vs M=16)
  - **Use case:** Scientific/medical applications

**Formula:** Memory per vector ≈ M × 4 bytes × 2 (bidirectional)

#### 2. **ef_construction (Build-time Search Width)**

**What it controls:** Size of dynamic candidate list during index construction.

**Impact:**
- **Lower (100-200):**
  - ✅ Faster index building
  - ⚠️ Lower quality graph structure
  - **Use case:** Rapid prototyping, development

- **Medium (200-400):**
  - ✅ Good index quality
  - ✅ Reasonable build time
  - **Use case:** Production (recommended)

- **Higher (400-800):**
  - ✅ Best index quality
  - ⚠️ Significantly slower builds
  - **Use case:** Offline batch processing

**Rule of Thumb:** ef_construction should be ≥ M × 8 for good quality

#### 3. **ef_search (Query-time Search Width)**

**What it controls:** Size of dynamic candidate list during search.

**Impact:**
- **Lower (16-32):**
  - ✅ Fastest queries (+2× vs ef=64)
  - ⚠️ Lower recall (90-93%)
  - **Use case:** Real-time, high-QPS applications

- **Medium (64-128):**
  - ✅ Balanced speed and quality
  - ✅ Good recall (95-97%)
  - **Use case:** Standard production workloads

- **Higher (128-512):**
  - ✅ Best recall (98-99%)
  - ⚠️ Slower queries
  - **Use case:** Accuracy-critical applications

**Key Insight:** ef_search can be adjusted **at runtime** without rebuilding the index!

---

## 📊 Preset Configurations

### Quick Reference Card

| Use Case | Preset | M | ef_construction | ef_search | Recall@10 | QPS |
|----------|--------|---|-----------------|-----------|-----------|-----|
| Real-time Search | Speed | 12 | 100 | 32 | 93% | 15k |
| Production (Recommended) | Production | 24 | 300 | 96 | 97% | 7.5k |
| General Purpose | Balanced | 16 | 200 | 64 | 96% | 10k |
| Critical Accuracy | Quality | 32 | 400 | 128 | 99% | 5k |
| Edge/IoT | Memory | 8 | 100 | 32 | 91% | 12k |

**Full preset documentation:** See `config/hnsw_presets.yaml`

---

## 🤖 Multi-Agent LLM Specific Recommendations

### Agent Role Matching

**Use Case:** Fast agent role selection based on task requirements

```json
{
  "hnsw_m": 12,
  "hnsw_ef_construction": 100,
  "ef_search": 32
}
```

**Rationale:** Role matching is approximate; speed matters more than perfect recall.

### Agent Document Retrieval

**Use Case:** Retrieving context documents for agent reasoning

```json
{
  "hnsw_m": 24,
  "hnsw_ef_construction": 300,
  "ef_search": 96
}
```

**Rationale:** Agents need high-quality document context for accurate reasoning.

---

## 🚀 Quick Start Migration

### Apply Production Preset (Recommended)

**Step 1:** Update `config/config.json`:

```json
{
  "vector_index": {
    "hnsw_m": 24,
    "hnsw_ef_construction": 300,
    "ef_search": 96
  }
}
```

**Step 2:** Rebuild indexes (existing indexes keep old parameters):

```bash
themisdb-cli rebuild-index --collection documents --preset production
```

**Step 3:** Monitor:

```bash
curl http://localhost:8765/api/vector/stats
```

**Expected:** +1% recall, +50% latency, +29% memory vs balanced preset

---

## 🔧 Runtime Tuning

### Dynamic ef_search Adjustment

**Key Feature:** Change ef_search without rebuilding!

```bash
# Fast approximate search
curl -X POST /api/vector/config -d '{"ef_search": 32}'

# High-quality search
curl -X POST /api/vector/config -d '{"ef_search": 128}'

# Per-query tuning
curl -X POST /api/vector/search \
  -d '{"vector": [...], "k": 10, "ef_search": 256}'
```

---

## 📈 Performance Benchmarks

Based on 1M vectors, 384 dimensions:

| Preset | P99 Latency | Memory | Build Time |
|--------|-------------|--------|------------|
| Speed | 2.0ms | 2.8GB | 12min |
| Balanced | 3.5ms | 3.5GB | 20min |
| Production | 5.0ms | 4.5GB | 36min |
| Quality | 8.0ms | 5.6GB | 50min |
| Memory | 3.0ms | 2.0GB | 10min |

---

## 🔍 Troubleshooting

### Low Recall (<90%)

1. Increase ef_search (runtime, no rebuild)
2. If still low, increase M and ef_construction (rebuild required)

### High Query Latency

1. Decrease ef_search
2. Use lower M (12-16) for new indexes
3. Consider query result caching

### Out of Memory

1. Use Memory preset (M=8)
2. Reduce ef_construction
3. Shard data across multiple indexes

---

## 📚 References

1. **THEMISDB_OPTIMIZATION_IMPACT_ANALYSIS_SUMMARY.md**
2. **ANN-Benchmarks:** https://ann-benchmarks.com/
3. **HNSW Paper:** Malkov & Yashunin, TPAMI 2018
4. **Config File:** `config/hnsw_presets.yaml`

---

**Remember:** ef_search is the only parameter you can tune at runtime without rebuilding!
