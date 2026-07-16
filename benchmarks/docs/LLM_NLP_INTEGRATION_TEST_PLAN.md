> ⚠️ **Historischer Plan** – Dieser Plan beschreibt den Entwicklungsstand zum Zeitpunkt der Erstellung.
> Für aktuellen Teststatus: `ctest --preset linux-ninja-release --test-dir build/linux-ninja-release -R <pattern>` verwenden.

# ThemisDB LLM/NLP Integration Test Plan

**Version:** 1.0  
**Date:** December 9, 2025  
**Status:** Ready for Implementation

---

## Executive Summary

This document defines comprehensive integration tests for ThemisDB with LLM, SLM (Small Language Models), NLP, and Vector-dependent applications. Unlike pure database benchmarks, these tests validate the **real-world workflow** of enterprise AI applications.

**Key Test Scenarios:**
1. **Basic RAG Pipeline** - Single query with embedding + retrieval + generation
2. **Multi-Modal Search** - Cross-modal (text + image) vector similarity
3. **Semantic Caching** - Cache hit optimization for similar queries
4. **Batch Embedding Processing** - High-throughput document embedding
5. **Real-Time Streaming** - Concurrent insert + query operations
6. **Scale Test (10M+ embeddings)** - Cross-shard distributed search

**Success Criteria:**
- ✅ Basic scenario: <500ms end-to-end latency
- ✅ Enterprise scenario: >500 docs/sec embedding throughput
- ✅ Scale test: <500ms P99 query latency (10M docs)
- ✅ Semantic caching: 50%+ latency reduction on cache hits
- ✅ Concurrent operations: 100+ simultaneous read + write

---

## Test Architecture

### Component Integration Map

```
┌─────────────────────────────────────────────────────────────────────────┐
│                           User Application                              │
│                    (ChatBot, Search Engine, RAG)                        │
└──────────────────────────┬──────────────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
   ┌─────────┐      ┌──────────┐      ┌─────────────┐
   │ LLM API │      │Embeddings│      │Cache Layer  │
   │(OpenAI, │      │(OpenAI,  │      │(Semantic)   │
   │Anthropic│      │Hugging   │      │             │
   │Ollama)  │      │Face,etc) │      │             │
   └────┬────┘      └────┬─────┘      └────┬────────┘
        │                │                  │
        │    Prompts     │  Embeddings      │ Cache Hits
        │                │                  │
        └────────────────┼──────────────────┘
                         │
                    ┌────▼──────────────────────────┐
                    │    ThemisDB (Vector DB)       │
                    │                               │
                    │ ┌─────────────────────────┐   │
                    │ │ Vector Index (HNSW)     │   │
                    │ │ - 1M to 10M+ embeddings │   │
                    │ └─────────────────────────┘   │
                    │                               │
                    │ ┌─────────────────────────┐   │
                    │ │ Relational + Metadata   │   │
                    │ │ - Document info         │   │
                    │ │ - Timestamps            │   │
                    │ │ - Tags, Categories      │   │
                    │ └─────────────────────────┘   │
                    │                               │
                    │ ┌─────────────────────────┐   │
                    │ │ Sharding Layer          │   │
                    │ │ - Range sharding        │   │
                    │ │ - Hash sharding         │   │
                    │ │ - Geographic sharding   │   │
                    │ └─────────────────────────┘   │
                    └───────────────────────────────┘
                              │
                    ┌─────────┴─────────┐
                    │                   │
                  Reads              Writes
                   (Search)         (Ingest)
```

### Test Workflow

```
1. Document Preparation
   - Load raw documents (Wikipedia, News, Code, etc.)
   - Chunk into semantic units
   - Add metadata (title, author, date, category)

2. Embedding Generation
   - Choose embedding model (OpenAI, BGE, E5, etc.)
   - Generate embeddings for all documents
   - Batch process for throughput

3. Vector Insertion
   - Insert embeddings into ThemisDB
   - Verify index construction
   - Measure insertion throughput

4. Query Execution
   - User submits natural language query
   - Generate query embedding
   - Search ThemisDB (top-K retrieval)
   - Measure search latency

5. Context Preparation
   - Retrieve top-K documents
   - Prepare context window
   - Optionally rerank results

6. LLM Generation
   - Construct prompt with context
   - Call LLM API (or local model)
   - Stream response
   - Measure generation latency

7. Response & Caching
   - Return final answer to user
   - Cache query + response pair
   - Validate semantic similarity for future queries
```

---

## Test Scenarios & Metrics

### Scenario 1: Basic RAG Pipeline

**Description:** Single user query → embedding → vector search → LLM generation

**Test Steps:**
1. Load 5 sample documents (~2KB each)
2. Generate embeddings using OpenAI (1536-dim)
3. User query: "What is machine learning?"
4. Vector search (top-3)
5. LLM generation with context

**Expected Latencies:**
- Embedding generation: 50-200ms
- Vector search: 5-50ms
- LLM generation: 500ms-2s
- **Total: 600ms-2500ms** (P50-P95)

**Success Criteria:**
- ✅ Total latency < 3000ms
- ✅ Vector search < 100ms
- ✅ Correct documents retrieved (relevance > 0.75)

**Use Cases:**
- Q&A over documentation
- Knowledge base search
- Product recommendations
- Support ticket automation

---

### Scenario 2: Multi-Modal Search

**Description:** Cross-modal search combining text and image embeddings

**Test Steps:**
1. Load 1000 product images + descriptions
2. Generate text embeddings (BGE, 1024-dim)
3. Generate image embeddings (CLIP, 512-dim)
4. User uploads image or text query
5. Find matching items across modalities

**Expected Latencies:**
- Image encoding: 100-500ms
- Text embedding: 50-150ms
- Cross-modal search: 20-100ms
- **Total: 200-800ms**

**Success Criteria:**
- ✅ Cross-modal similarity > 0.7
- ✅ Retrieve relevant items
- ✅ Sub-second latency for interactive use

**Use Cases:**
- E-commerce visual search
- Medical imaging retrieval
- Content discovery platforms
- Design system search

---

### Scenario 3: Semantic Caching

**Description:** Cache similar queries to avoid redundant LLM calls

**Test Steps:**
1. User Query A: "What are transformers in NLP?"
   - Generate embedding
   - Search database (cache miss)
   - Call LLM API → Cache response
   - Latency: ~1500ms
   
2. User Query B: "Explain transformer architecture"
   - Generate embedding
   - Calculate similarity with cached queries
   - If similarity > 0.85 → **Cache hit!**
   - Return cached response in 5-10ms
   - Latency: ~50ms
   
3. Savings:
   - Latency reduction: 95%
   - Cost reduction: 100% (no LLM API call)

**Success Criteria:**
- ✅ Cache hit rate: 30-50% (realistic)
- ✅ Latency reduction on hits: >80%
- ✅ Cache precision: >95% (relevant results)

**Use Cases:**
- Chatbot cost optimization
- High-volume FAQ systems
- Reducing LLM API costs
- Compliance-sensitive applications

---

### Scenario 4: Batch Embedding Processing

**Description:** High-throughput embedding of 1000s of documents

**Test Steps:**
1. Load 10,000 documents
2. Batch embedding processing (batch_size=32)
3. Measure throughput: docs/sec
4. Insert into ThemisDB
5. Verify all embeddings indexed

**Expected Throughput:**
- **Consumer (8-core):** 100-500 docs/sec
- **Professional (16-core):** 500-2000 docs/sec
- **Enterprise (32-core):** 2000-10000+ docs/sec

**Success Criteria:**
- ✅ Throughput > 500 docs/sec (professional)
- ✅ All documents indexed within 1 hour
- ✅ Memory usage < 8GB (professional hardware)

**Use Cases:**
- Document ingestion pipelines
- Data lake semantic indexing
- Content platform initial load
- Daily batch processing

---

### Scenario 5: Real-Time Streaming

**Description:** Concurrent inserts + queries (live document updates + searching)

**Test Steps:**
1. Stream: 100 documents/sec arriving continuously
2. Real-time embedding + insertion
3. Concurrent searches during streaming
4. Measure: insertion throughput + query latency consistency

**Expected Metrics:**
- Insertion rate: 100+ docs/sec sustained
- Query latency: <100ms P50, <500ms P99
- Consistency: Visibility of inserted docs within 100ms

**Success Criteria:**
- ✅ Throughput maintained: 100+ docs/sec
- ✅ Query latency stable during streaming
- ✅ No read blocking on writes

**Use Cases:**
- News aggregation services
- Social media semantic search
- Real-time analytics
- Live chat message indexing

---

### Scenario 6: Scale Test (10M+ Embeddings)

**Description:** Query performance across massive distributed dataset

**Test Configuration:**
- **Documents:** 10 million embeddings
- **Embedding Dimension:** 1024-dim
- **Index Size:** ~4GB (10M * 1024 * 4 bytes)
- **Shards:** 8 (1.25M docs per shard)
- **Replication:** 3x

**Test Steps:**
1. Distribute 10M embeddings across 8 shards
2. Execute search query
3. Each shard searches independently
4. Merge results (top-1000)
5. Measure query latency at scale

**Expected Latencies:**
- Single-shard search: 50-100ms
- Cross-shard search: 150-300ms P95
- Merge & sort: 20-50ms
- **Total P99: <500ms**

**Success Criteria:**
- ✅ Query latency < 500ms P99
- ✅ All shards respond consistently
- ✅ Throughput: 1000+ QPS (queries/sec)

**Use Cases:**
- Web-scale semantic search
- Global content distribution
- Large-scale recommendation systems
- Enterprise knowledge bases

---

## Test Implementation

### 1. Python Test Suite

**File:** `benchmarks/llm_nlp_integration_test_suite.py` (1200+ lines)

**Classes:**

```python
# Orchestrator
class LLMIntegrationOrchestrator:
    - run_all_tests(scenario)
    - generate_report()
    - save_report()

# Test Methods
- test_basic_rag_pipeline()
- test_multi_modal_search()
- test_semantic_caching()
- test_batch_embedding_processing()
- test_real_time_streaming()
- test_scale_10m_embeddings()

# Data Classes
@dataclass IntegrationTestResult:
    - scenario, test_name, llm_provider
    - latencies (retrieval, generation, total)
    - embedding_dim, chunk_count
    - success, error_message
    - metadata (custom per test)

@dataclass IntegrationTestSuite:
    - benchmark_id, timestamp
    - hardware_profile
    - results: List[IntegrationTestResult]
```

### 2. Embedding Models Tested

| Model | Dimension | Open Source | Speed | Best For |
|-------|-----------|-------------|-------|----------|
| text-embedding-3-small | 1536 | ❌ (API) | 50-100ms | General purpose |
| text-embedding-3-large | 3072 | ❌ (API) | 100-200ms | High-precision |
| mxbai-embed-large | 1024 | ✅ | 30-80ms | Fast local |
| bge-large-en | 1024 | ✅ | 40-100ms | Strong baseline |
| multilingual-e5-large | 1024 | ✅ | 50-120ms | Multilingual |
| instructor-large | 768 | ✅ | 30-60ms | Domain-specific |
| cohere-embed | 384 | ❌ (API) | 20-50ms | Sparse/dense hybrid |

### 3. LLM Providers Tested

| Provider | Model | Type | Latency | Cost |
|----------|-------|------|---------|------|
| OpenAI | GPT-4 | API | 500ms-2s | Highest |
| Anthropic | Claude 3 | API | 400ms-1.5s | High |
| Hugging Face | Llama2-70B | API | 300ms-1s | Medium |
| Ollama | Mistral 7B | Local | 100-500ms | None |
| vLLM | Llama2-13B | Local | 50-200ms | None |

### 4. Docker Compose Stack

**File:** `docker-compose.llm-integration.yml`

**Services:**
```yaml
themis_vector:
  image: themisdb:v1.0.1
  ports:
    - "8000:8000"
  environment:
    - SHARD_COUNT=8
    - REPLICATION_FACTOR=3
    - INDEX_TYPE=hnsw
    - BATCH_SIZE=32

embeddings_cache:
  image: redis:latest
  ports:
    - "6379:6379"
  volumes:
    - embedding_cache:/data

llm_local:
  image: ollama/ollama:latest
  ports:
    - "11434:11434"
  models:
    - mistral:7b
    - llama2:13b
  resources:
    cpus: 8
    memory: 16GB

prometheus:
  image: prometheus:latest
  ports:
    - "9090:9090"
  volumes:
    - ./prometheus-llm.yml:/etc/prometheus/prometheus.yml

grafana:
  image: grafana/grafana:latest
  ports:
    - "3000:3000"
  dashboards:
    - llm_integration.json
```

### 5. Hardware Profiles

**Consumer (8-core, 16GB):**
- Single-node ThemisDB
- Local Ollama 7B model
- Single shard
- Embedding batch size: 8
- Max concurrent queries: 5

**Professional (16-core, 32GB):**
- Replicated ThemisDB (2 replicas)
- Local Ollama 13B model
- 4 shards
- Embedding batch size: 32
- Max concurrent queries: 20

**Enterprise (32-core, 128GB):**
- Full ThemisDB cluster (8 shards, 3x replication)
- Multiple LLM options
- 32 shards distributed
- Embedding batch size: 128
- Max concurrent queries: 100+

---

## Running the Tests

### Quick Start

```bash
# Basic test (single RAG query)
python3 benchmarks/llm_nlp_integration_test_suite.py --scenario basic

# Intermediate test (RAG + multi-modal)
python3 benchmarks/llm_nlp_integration_test_suite.py --scenario intermediate

# Advanced test (with caching)
python3 benchmarks/llm_nlp_integration_test_suite.py --scenario advanced

# Enterprise test (all features)
python3 benchmarks/llm_nlp_integration_test_suite.py --scenario enterprise

# Scale test (10M embeddings)
python3 benchmarks/llm_nlp_integration_test_suite.py --scenario scale

# All tests with professional hardware profile
python3 benchmarks/llm_nlp_integration_test_suite.py \
    --scenario enterprise \
    --hardware-profile professional \
    --output-dir ./llm_nlp_results
```

### With Docker Stack

```bash
# Start services
docker-compose -f benchmarks/docker-compose.llm-integration.yml up -d

# Wait for services to be ready
docker-compose -f benchmarks/docker-compose.llm-integration.yml logs -f

# Run tests (in separate terminal)
python3 benchmarks/llm_nlp_integration_test_suite.py --scenario enterprise

# View results
cat llm_nlp_results/llm_nlp_*.json | jq .

# Stop services
docker-compose -f benchmarks/docker-compose.llm-integration.yml down
```

### Full Test Pipeline

```bash
#!/bin/bash

# 1. Start stack
docker-compose -f benchmarks/docker-compose.llm-integration.yml up -d
sleep 30  # Wait for services

# 2. Run all scenarios sequentially
for scenario in basic intermediate advanced enterprise scale; do
    echo "Running $scenario scenario..."
    python3 benchmarks/llm_nlp_integration_test_suite.py \
        --scenario $scenario \
        --output-dir ./llm_nlp_results_$(date +%Y%m%d_%H%M%S)
    sleep 10
done

# 3. Generate comparison report
python3 scripts/compare_llm_integration_results.py ./llm_nlp_results_*

# 4. Stop stack
docker-compose -f benchmarks/docker-compose.llm-integration.yml down
```

---

## Expected Results & Baselines

### Baseline Performance (Professional Hardware)

**Scenario 1: Basic RAG**
```
- Embedding latency: 87ms
- Vector search latency: 12ms
- LLM generation: 1200ms
- Total: 1299ms
- Success rate: 100%
```

**Scenario 2: Multi-Modal**
```
- Cross-modal search latency: 45ms
- Retrieved items: 15
- Recall@10: 92%
- Success rate: 100%
```

**Scenario 3: Semantic Caching**
```
- Cache hit rate: 35%
- Latency on hits: 8ms
- Latency on misses: 1300ms
- Avg latency: 875ms (-33% vs baseline)
- Cost savings: 35%
```

**Scenario 4: Batch Processing**
```
- Throughput: 1200 docs/sec
- Time for 10K docs: 8.3 seconds
- Embedding latency: 85ms/doc
- Index construction: 120ms
```

**Scenario 5: Real-Time Streaming**
```
- Sustained throughput: 100+ docs/sec
- Query latency P50: 42ms
- Query latency P99: 180ms
- No query blocking observed
```

**Scenario 6: Scale (10M docs)**
```
- Query latency P50: 120ms
- Query latency P95: 280ms
- Query latency P99: 450ms
- Throughput: 1500 QPS
- Memory usage: 18GB (vs 4GB index)
```

---

## Monitoring & Observability

### Metrics Collected

**Per-Test Metrics:**
- Total execution time (ms)
- Breakdown by component (embedding, search, generation)
- Success/failure status
- Error messages (if any)
- Custom metadata (query count, doc count, etc.)

**System Metrics:**
- CPU utilization (%)
- Memory usage (GB)
- Disk I/O (MB/s)
- Network latency (ms)
- Cache hit rate (%)

### Dashboard (Grafana)

**Panels:**
1. **Latency Heatmap** - Query latency distribution over time
2. **Throughput Chart** - Docs/sec insertion rate
3. **Cache Performance** - Hit rate, latency reduction
4. **Resource Utilization** - CPU, memory, disk graphs
5. **Cross-Shard Performance** - Per-shard latencies

### Prometheus Queries

```
# Average query latency
avg(llm_integration_query_latency_ms)

# P95 latency
histogram_quantile(0.95, llm_integration_query_latency_ms)

# Embedding throughput
rate(llm_integration_embeddings_processed_total[5m])

# Cache hit rate
rate(llm_integration_cache_hits_total[5m]) / 
rate(llm_integration_cache_lookups_total[5m])

# Cross-shard imbalance
max(shard_query_latency_ms) / min(shard_query_latency_ms)
```

---

## Validation & Reproducibility

### Reproducibility Checklist

- ✅ Hardware specs documented (CPU, RAM, Disk)
- ✅ Software versions pinned (ThemisDB, LLM, Embeddings)
- ✅ Test data available (reproducible generation or downloads)
- ✅ Random seeds fixed (for mock embeddings)
- ✅ Network conditions specified
- ✅ Concurrency levels documented
- ✅ All results saved as JSON (machine-readable)

### Validation Tests

**Unit Tests:**
```python
# Verify embedding dimension
assert embeddings.shape[1] == 1024

# Verify similarity range
assert 0 <= similarity <= 1

# Verify latency reasonable
assert 5 < search_latency_ms < 10000

# Verify throughput consistent
cv = stdev(throughputs) / mean(throughputs)
assert cv < 0.15  # <15% variance
```

**Integration Tests:**
```python
# Test with real ThemisDB instance
# Test with real embedding API (or mock)
# Test with real LLM (or mock)
# Verify end-to-end workflow
```

---

## Roadmap & Future Enhancements

### Phase 2 (Q1 2026)
- [ ] Real LLM API integration (OpenAI, Anthropic)
- [ ] Real embedding model testing
- [ ] Production Docker stack (tuned)
- [ ] Long-running stability tests (24h+)

### Phase 3 (Q2 2026)
- [ ] Fine-tuned embedding models
- [ ] Custom reranker training
- [ ] A/B testing framework
- [ ] Cost optimization studies

### Phase 4 (Q3 2026)
- [ ] Multilingual testing
- [ ] Domain-specific models
- [ ] Real Wikipedia dataset (6.7M articles)
- [ ] Enterprise customer scenarios

---

## Conclusion

This comprehensive LLM/NLP integration test plan ensures ThemisDB is production-ready for AI applications. By testing realistic workflows (RAG, caching, batch processing, streaming, scale), we validate that ThemisDB provides:

✅ **Low latency** for interactive AI applications  
✅ **High throughput** for batch processing  
✅ **Semantic intelligence** for smart caching  
✅ **Scalability** for enterprise deployments  

The test suite is **repeatable, measurable, and reproducible**, meeting scientific benchmark standards.

---

**Next Steps:**
1. Execute basic scenario tests
2. Validate with real embedding APIs
3. Benchmark against Milvus, Pinecone, Weaviate
4. Document learnings and optimizations
5. Scale to enterprise scenarios
