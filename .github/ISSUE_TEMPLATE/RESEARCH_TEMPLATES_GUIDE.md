# Research Issue Templates - Quick Reference Guide

## Which Template Should I Use?

### 🔍 General Research / Paper Investigation
**Template:** `research_paper_investigation.md`

**Use when:**
- Investigating a broad research area
- Doing literature review without specific focus
- Evaluating multiple approaches across different domains
- Planning research without clear direction yet

**Example Issues:**
- "Research: State-of-the-art Query Optimization Techniques"
- "Paper Investigation: Modern Consensus Algorithms"
- "Research: Real-time Analytics in Database Systems"

---

### 🎯 Vector Indexing Research (Comprehensive)
**Template:** `vector_indexing_research.md`

**Use when:**
- Researching vector indexing methods broadly
- Comparing multiple approaches (PQ, learned, GPU)
- Evaluating ANN algorithms
- Planning comprehensive vector search improvements

**Example Issues:**
- "Paper-Recherche: Advanced Vector Indexing (über HNSW hinaus)" ✅ **This issue!**
- "Evaluate Vector Indexing Methods for ThemisDB v2.0"
- "ANN Algorithm Comparison: HNSW vs IVF vs Graph-based"

**Covers:**
- Product Quantization basics
- Learned index overview
- GPU optimization overview
- All ANN methods

---

### 📦 Product Quantization (Specialized)
**Template:** `product_quantization_research.md`

**Use when:**
- Specifically researching PQ compression methods
- Deep-dive into quantization techniques
- Optimizing memory footprint for vector indexes
- Comparing PQ variants (OPQ, AQ, RQ, etc.)

**Example Issues:**
- "OPQ vs Standard PQ: Performance Evaluation"
- "Implement Polysemous Codes for Faster Filtering"
- "Product Quantization Memory Optimization"

**Covers:**
- Standard PQ
- Optimized PQ (OPQ)
- Additive Quantization (AQ)
- Residual Quantization (RQ)
- Polysemous Codes
- Cartesian k-means

---

### 🧠 Learned Index Structures (Specialized)
**Template:** `learned_index_research.md`

**Use when:**
- Researching neural/learned indexes
- Evaluating machine learning for indexing
- Deep learning-based search methods
- End-to-end learned systems

**Example Issues:**
- "Neural Approximate Nearest Neighbor Evaluation"
- "Deep Hashing for ThemisDB Vector Search"
- "GNN-based Graph Navigation for HNSW"

**Covers:**
- Neural ANN (NANN)
- Learning to Hash
- Learned Space Partitioning
- End-to-End Learned Indexes
- Hybrid Learned/Traditional
- Graph Neural Networks

---

### 🎮 GPU-Optimized Indexing (Specialized)
**Template:** `gpu_indexing_research.md`

**Use when:**
- Researching GPU acceleration for vector search
- Evaluating CUDA/HIP implementations
- Multi-GPU scaling strategies
- GPU vs CPU cost-benefit analysis

**Example Issues:**
- "FAISS-GPU vs CAGRA Performance Comparison"
- "Multi-GPU Scaling for 100M+ Vectors"
- "Tensor Core Utilization for Distance Computation"

**Covers:**
- GPU brute-force search
- GPU-accelerated IVF
- GPU-accelerated HNSW
- GPU Product Quantization
- Multi-GPU scaling
- Tensor Core optimization

---

## Decision Tree

```
Start: Vector indexing research?
│
├─ Yes ─┐
│       │
│       ├─ Comprehensive (multiple approaches)
│       │  → Use: vector_indexing_research.md
│       │
│       ├─ Focus: Memory compression / quantization
│       │  → Use: product_quantization_research.md
│       │
│       ├─ Focus: Machine learning / neural methods
│       │  → Use: learned_index_research.md
│       │
│       └─ Focus: GPU acceleration / hardware
│          → Use: gpu_indexing_research.md
│
└─ No: General research / other topic
   → Use: research_paper_investigation.md
```

---

## Template Comparison Matrix

| Feature | General | Vector (Comprehensive) | PQ (Specialized) | Learned (Specialized) | GPU (Specialized) |
|---------|---------|------------------------|------------------|----------------------|-------------------|
| **Effort** | Low-Medium | Medium-High | Medium | High | High |
| **Timeline** | 2-4 weeks | 4-6 weeks | 6-8 weeks | 8-12 weeks | 8-12 weeks |
| **Technical Depth** | ⭐⭐ | ⭐⭐⭐ | ⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ |
| **Prerequisites** | None | Vector search basics | Compression theory | ML/DL knowledge | CUDA/GPU programming |
| **Implementation** | N/A | Varies | C++ | Python+C++ | CUDA/HIP |

---

## Labels Applied Automatically

### All Research Templates
- `type:discussion` - Marks as research/discussion issue
- `priority:P2` - Medium priority (default for research)

### Vector Indexing Templates
- `area:llm` - Related to AI/vector features
- `area:performance` - Performance optimization focus

### Specialized Templates
- `effort:medium` - Product Quantization (6-8 weeks)
- `effort:large` - Learned Index, GPU (8-12 weeks)

---

## Common Sections in All Templates

1. **Background / Hintergrund** - Current state and problem statement
2. **Research Focus** - Specific topics to investigate
3. **Key Papers** - Literature references
4. **Benchmark Plan** - Evaluation methodology
5. **Implementation Plan** - Phased approach (typically 3-5 phases)
6. **Expected Outcomes** - Success criteria and deliverables
7. **Dependencies** - Libraries, hardware, data requirements

---

## For the Current Issue

**Issue:** "Paper-Recherche: Advanced Vector Indexing (über HNSW hinaus)"

**Recommended Template:** `vector_indexing_research.md` ✅

**Why?**
- Covers all three focus areas (PQ, Learned, GPU)
- Comprehensive evaluation across methods
- Comparative analysis built-in
- Can spawn specialized sub-issues later if needed

**Alternative Approach:**
Create 3 separate issues using specialized templates:
1. "PQ Research: Product Quantization Improvements" → `product_quantization_research.md`
2. "Learned Index Research: Neural ANN Methods" → `learned_index_research.md`
3. "GPU Research: GPU-Optimized Vector Indexing" → `gpu_indexing_research.md`

---

## Next Steps

1. **Choose Template** based on research focus
2. **Create Issue** from template
3. **Fill Sections** with specific details
4. **Phase 1:** Literature review (1-2 weeks)
5. **Phase 2:** Proof-of-concept (2-4 weeks)
6. **Phase 3:** Evaluation & recommendation (1 week)
7. **Phase 4:** Implementation planning (if promising)

---

**Created:** 2026-01-27  
**For:** ThemisDB Advanced Vector Indexing Research
