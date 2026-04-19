> **Aktueller Build-Flow:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`

# ThemisDB LLM/NLP Integration - Quick Start Guide

**Status:** Ready to Use  
**Version:** 1.0  
**Updated:** December 9, 2025

---

## Overview

This guide shows how to test ThemisDB with real-world LLM/NLP workloads:
- RAG (Retrieval-Augmented Generation)
- Semantic Search
- Multi-Modal Search
- Batch Embedding Processing
- Real-Time Streaming
- Large-Scale Deployments

---

## Prerequisites

### System Requirements

**Minimum (Consumer):**
- 8 CPU cores
- 16GB RAM
- 100GB free disk
- 8GB VRAM (optional, for local LLM)

**Recommended (Professional):**
- 16 CPU cores
- 32GB RAM
- 500GB free disk
- 16GB VRAM (NVIDIA/AMD GPU)

**Enterprise:**
- 32+ CPU cores
- 128GB+ RAM
- 2TB+ fast storage
- 24GB+ VRAM (for multiple LLMs)

### Software

```bash
# Python 3.10+
python3 --version

# Docker & Docker Compose
docker --version        # >= 25.0
docker compose version  # >= 2.20

# Git
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/benchmarks
```

### Optional LLM Providers

**Local (Free):**
```bash
# Install Ollama (https://ollama.ai)
# Automatically downloads models on first run
```

**API-Based (Requires API Key):**
```bash
# OpenAI
export OPENAI_API_KEY="sk-..."

# Anthropic Claude
export ANTHROPIC_API_KEY="sk-ant-..."

# Hugging Face
export HF_API_KEY="hf_..."
```

---

## Quick Start (5 minutes)

### Option 1: Minimal Test (No Docker)

```bash
# 1. Install Python dependencies
pip install -r benchmarks/requirements-llm-integration.txt

# 2. Run basic test (no external services needed)
cd benchmarks
python3 llm_nlp_integration_test_suite.py --scenario basic

# 3. View results
cat llm_nlp_results/*.json | jq .
```

**Output:**
```
[✓] Basic RAG Pipeline: 1234.56ms total
  - Embedding latency: 87ms
  - Vector search latency: 12ms
  - LLM generation: 1135ms
  
Test Results:
  Total Tests: 1
  Successful: 1
  Failed: 0
  Success Rate: 100.0%
```

---

### Option 2: Docker Stack (Recommended)

```bash
# 1. Start Docker services
docker compose -f docker-compose.llm-integration.yml up -d

# Wait for all services to be healthy
docker compose -f docker-compose.llm-integration.yml logs -f

# 2. Run intermediate tests (with vector DB + caching)
python3 llm_nlp_integration_test_suite.py \
    --scenario intermediate \
    --hardware-profile professional

# 3. Monitor live dashboard
# Open http://localhost:3000 (Grafana)
# - Username: admin
# - Password: admin

# 4. View Prometheus metrics
# Open http://localhost:9090

# 5. Check logs
docker compose -f docker-compose.llm-integration.yml logs themis_vector
```

---

### Option 3: Full Enterprise Test

```bash
# 1. Start complete stack
docker compose -f docker-compose.llm-integration.yml up -d

# 2. Initialize models (Ollama/vLLM auto-downloads on first use)
curl http://localhost:11434/api/pull -d '{"name":"mistral:7b"}' &
curl http://localhost:8001/v1/models  # Check vLLM readiness

# 3. Run all test scenarios
for scenario in basic intermediate advanced enterprise scale; do
    echo "===== Testing $scenario ====="
    python3 llm_nlp_integration_test_suite.py \
        --scenario $scenario \
        --hardware-profile professional \
        --output-dir results_$(date +%Y%m%d)
    echo ""
done

# 4. Generate comparison report
python3 scripts/compare_llm_integration_results.py \
    results_* \
    --output-html comparison_report.html

# 5. Stop stack
docker compose -f docker-compose.llm-integration.yml down
```

---

## Detailed Scenarios

### Scenario 1: Basic RAG (5 min)

```bash
# Single query → embedding → search → LLM response
python3 llm_nlp_integration_test_suite.py --scenario basic

# Expected:
# - 5 sample documents
# - 1 user query
# - <3000ms end-to-end latency
# - Output: llm_nlp_results/llm_nlp_*.json
```

**Use When:**
- Testing embedding API integration
- Validating vector search works
- Checking LLM connectivity
- Debugging issues

---

### Scenario 2: Intermediate (10 min)

```bash
# RAG pipeline + multi-modal search
python3 llm_nlp_integration_test_suite.py --scenario intermediate

# Tests:
# 1. Basic RAG pipeline
# 2. Cross-modal (text + image) search
#
# Expected:
# - 1000 product images + descriptions
# - <800ms cross-modal search latency
```

**Use When:**
- Testing multi-modal applications
- E-commerce visual search
- Content discovery platforms
- Design systems

---

### Scenario 3: Advanced (20 min)

```bash
# RAG + multi-modal + semantic caching
python3 llm_nlp_integration_test_suite.py --scenario advanced

# Tests:
# 1. Basic RAG pipeline
# 2. Multi-modal search
# 3. Semantic caching (cache hits)
#
# Expected:
# - 35% cache hit rate
# - 95% latency reduction on cache hits
# - 35% cost savings
```

**Use When:**
- Optimizing LLM costs
- High-volume QA systems
- FAQ search
- Cost-sensitive applications

---

### Scenario 4: Enterprise (30-45 min)

```bash
# All features: batch processing, streaming, etc.
python3 llm_nlp_integration_test_suite.py \
    --scenario enterprise \
    --hardware-profile professional

# Tests:
# 1. Basic RAG pipeline
# 2. Multi-modal search
# 3. Semantic caching
# 4. Batch embedding (10K docs)
# 5. Real-time streaming (100 docs/sec)
#
# Expected:
# - 1200+ docs/sec batch throughput
# - 100+ sustained docs/sec streaming
# - <100ms P50 query latency during streaming
```

**Use When:**
- Production deployments
- Document ingestion pipelines
- Real-time indexing systems
- News aggregation services

---

### Scenario 5: Scale Test (30-60 min)

```bash
# Simulated 10M embeddings across 8 shards
python3 llm_nlp_integration_test_suite.py \
    --scenario scale \
    --hardware-profile enterprise

# Tests:
# 1. Batch embedding (simulated 10M docs)
# 2. Real-time streaming at scale
# 3. Cross-shard search (10M doc database)
#
# Expected:
# - <500ms P99 query latency
# - 1500+ queries/sec throughput
# - Linear scaling with shard count
```

**Use When:**
- Enterprise knowledge bases
- Web-scale semantic search
- Global content distribution
- Large-scale recommendation systems

---

## Real Integration with APIs

### Using OpenAI Embeddings

```python
# Requires: export OPENAI_API_KEY="sk-..."

from openai import OpenAI

client = OpenAI()

# Generate embeddings
response = client.embeddings.create(
    model="text-embedding-3-small",
    input="What is machine learning?",
    encoding_format="float"
)

embedding = response.data[0].embedding  # 1536-dim
```

### Using Ollama (Local)

```bash
# 1. Install Ollama: https://ollama.ai
# 2. Pull model:
ollama pull mistral:7b

# 3. Test API:
curl http://localhost:11434/api/generate \
    -d '{
        "model": "mistral:7b",
        "prompt": "Explain machine learning briefly",
        "stream": false
    }'

# 4. Use in tests:
export LLM_PROVIDER=ollama
export LLM_BASE_URL=http://localhost:11434
python3 llm_nlp_integration_test_suite.py --scenario enterprise
```

### Using Hugging Face

```python
from sentence_transformers import SentenceTransformer

# Load model (auto-downloads)
model = SentenceTransformer('all-MiniLM-L6-v2')

# Generate embeddings
embedding = model.encode("What is machine learning?")  # 384-dim
```

### Using vLLM (Fast Inference)

```bash
# vLLM already running in docker-compose
# Uses OpenAI-compatible API

curl http://localhost:8001/v1/chat/completions \
    -H "Content-Type: application/json" \
    -d '{
        "model": "mistralai/Mistral-7B-Instruct-v0.2",
        "messages": [
            {"role": "user", "content": "Explain RAG"}
        ],
        "max_tokens": 100
    }'
```

---

## Monitoring & Results

### View Live Metrics

```bash
# Grafana Dashboards
open http://localhost:3000

# Panels:
# - Latency Heatmap
# - Throughput Chart
# - Cache Hit Rate
# - Resource Utilization
# - Cross-Shard Performance
```

### Prometheus Queries

```bash
# Open http://localhost:9090

# Average query latency
avg(llm_integration_query_latency_ms)

# P95 latency
histogram_quantile(0.95, llm_integration_query_latency_ms)

# Throughput (docs/sec)
rate(llm_integration_embeddings_processed_total[5m])

# Cache hit rate
rate(llm_integration_cache_hits_total[1m]) / 
rate(llm_integration_cache_lookups_total[1m])
```

### Analyze Results

```bash
# View all results
ls -la llm_nlp_results/

# Pretty-print JSON
cat llm_nlp_results/llm_nlp_*.json | jq .

# Extract specific metrics
jq '.results[] | {test_name, duration_ms, success}' *.json

# Compare scenarios
python3 scripts/compare_llm_integration_results.py llm_nlp_results/
```

---

## Docker Service URLs

| Service | URL | Credentials |
|---------|-----|-------------|
| ThemisDB API | http://localhost:8000 | - |
| Grafana | http://localhost:3000 | admin/admin |
| Prometheus | http://localhost:9090 | - |
| Ollama API | http://localhost:11434 | - |
| vLLM API | http://localhost:8001 | - |
| Redis | localhost:6379 | - |
| PostgreSQL | localhost:5432 | themis/themis_secure_password |
| Milvus | localhost:19530 | - |
| MinIO | http://localhost:9001 | minioadmin/minioadmin |

---

## Common Issues & Solutions

### Issue: Docker service won't start

```bash
# Check logs
docker compose -f docker-compose.llm-integration.yml logs <service>

# Remove stuck containers
docker compose -f docker-compose.llm-integration.yml down -v
docker system prune -f

# Restart
docker compose -f docker-compose.llm-integration.yml up -d
```

### Issue: Out of memory

```bash
# Check memory usage
docker stats

# Reduce memory limits in docker-compose.yml
# Or upgrade hardware
```

### Issue: Slow embedding generation

```bash
# Check CPU usage
docker stats

# Reduce batch size in python script
batch_size = 16  # was 32

# Or use faster model
embedding_model = "bge-small-en"  # smaller
```

### Issue: LLM API errors

```bash
# Check API keys
echo $OPENAI_API_KEY
echo $ANTHROPIC_API_KEY

# Test API connectivity
curl https://api.openai.com/v1/models \
    -H "Authorization: Bearer $OPENAI_API_KEY"

# Use local model instead
export LLM_PROVIDER=ollama
```

---

## Performance Tuning

### For Speed (Latency)

```python
# Use smaller models
EmbeddingModel.COHERE  # 384-dim, fastest
# or
LLMProvider.VLLM      # Local, sub-100ms

# Reduce batch size
batch_size = 8

# Use GPU if available
# (automatically detected in vLLM)
```

### For Throughput (Batch Processing)

```python
# Use larger batches
batch_size = 128

# Use CPU-optimized models
EmbeddingModel.BGE  # 1024-dim, fast

# Parallelize across shards
shard_count = 8
parallel_threads = 8
```

### For Cost (API Usage)

```python
# Enable semantic caching
use_semantic_cache = True

# Use cheaper embedding model
EmbeddingModel.COHERE_LIGHT  # Cheaper

# Use local LLM instead of API
LLMProvider.OLLAMA  # Free

# Batch API calls
batch_size = 100  # Higher = cheaper per-token
```

---

## Next Steps

### After Basic Tests Pass:

1. **Real Embedding API**
   ```bash
   export OPENAI_API_KEY="sk-..."
   python3 llm_nlp_integration_test_suite.py --scenario intermediate
   ```

2. **Real LLM Provider**
   ```bash
   export ANTHROPIC_API_KEY="sk-ant-..."
   # Modify test suite to use Anthropic instead of mock
   ```

3. **Real Data**
   ```bash
   # Load Wikipedia or customer data
   python3 scripts/load_real_embeddings.py \
       --source wikipedia \
       --themis-host localhost:8000
   ```

4. **Production Deployment**
   ```bash
   # Scale to enterprise hardware
   docker-compose.prod.yml
   # with 32+ shards, 128GB RAM, GPU clusters
   ```

---

## Advanced Topics

### Multi-GPU Setup

```yaml
# In docker-compose.yml
vllm_server:
  deploy:
    resources:
      reservations:
        devices:
          - driver: nvidia
            device_ids: ['0', '1']
            capabilities: [gpu]
```

### Kubernetes Deployment

```bash
# Convert to K8s manifests
kompose convert -f docker-compose.llm-integration.yml

# Deploy to cluster
kubectl apply -f .
```

### Hybrid Scenarios

```bash
# Mix local + API providers
python3 llm_nlp_integration_test_suite.py \
    --embedding-provider openai \
    --llm-provider ollama \
    --scenario enterprise
```

### Cost Optimization

```bash
# Analyze cost per query
python3 scripts/analyze_costs.py \
    --results llm_nlp_results/ \
    --api-prices openai_prices.json
```

---

## Support & Resources

**Documentation:**
- [LLM/NLP Test Plan](./LLM_NLP_INTEGRATION_TEST_PLAN.md)
- [Scientific Protocol](./SCIENTIFIC_BENCHMARK_PROTOCOL_TEMPLATE.md)
- [Docker Documentation](./DOCKER_QUICKSTART.md)

**External Resources:**
- [Ollama Models](https://ollama.ai)
- [OpenAI Embeddings](https://platform.openai.com/docs/guides/embeddings)
- [Hugging Face](https://huggingface.co)
- [vLLM](https://github.com/lm-sys/vllm)

**Issues & Feedback:**
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Discussions: https://github.com/makr-code/ThemisDB/discussions

---

## Example: Complete Production Setup

```bash
#!/bin/bash
set -e

echo "=== ThemisDB LLM Integration Setup ==="

# 1. Clone and prepare
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB/benchmarks

# 2. Set up API keys (if using)
read -p "OpenAI API Key (or skip): " OPENAI_KEY
[ -n "$OPENAI_KEY" ] && export OPENAI_API_KEY=$OPENAI_KEY

# 3. Start stack
docker compose -f docker-compose.llm-integration.yml up -d
echo "Waiting for services to be healthy..."
sleep 30

# 4. Verify all services
docker compose -f docker-compose.llm-integration.yml ps

# 5. Run tests
python3 llm_nlp_integration_test_suite.py --scenario enterprise

# 6. Generate report
echo "Results saved to llm_nlp_results/"
ls -la llm_nlp_results/

# 7. Open dashboards
echo ""
echo "Grafana:     http://localhost:3000 (admin/admin)"
echo "Prometheus:  http://localhost:9090"
echo "ThemisDB:    http://localhost:8000"

echo ""
echo "✓ Setup complete! Visit the dashboards above to monitor tests."
```

---

**Ready to test? Start with:**
```bash
python3 llm_nlp_integration_test_suite.py --scenario basic
```
