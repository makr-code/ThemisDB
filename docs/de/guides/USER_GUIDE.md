---
category: "📋 Guides"
version: "v1.9.0-beta"
status: "✅"
date: "2026-05"
audience: "Application developers, data engineers, AI/ML engineers"
---

# 📋 ThemisDB User Guide

Comprehensive guide for application developers and data engineers.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features & Highlights](#-features--highlights)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Multi-Model Daten](#-multi-model-daten)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

ThemisDB ist eine Multi-Model Datenbank für moderne Anwendungen mit:
- 6 integrierten Datenmodellen
- Unternehmens-Features wie Transaktionen & Sicherheit
- RAG (Retrieval-Augmented Generation) für AI/ML
- GPU-Beschleunigung für Vector Search

**Target Audience:** Application developers, data engineers, AI/ML engineers  
**Version:** 1.9.0-beta  
**Last Updated:** May 2026

---

## ✨ Features & Highlights

- 🚀 **Multi-Model** - Relational, Document, Graph, Vector, Time-Series, Spatial
- 🔐 **Enterprise** - ACID-Transaktionen, RBAC, TLS/mTLS
- 🎯 **RAG-Ready** - Hybrid Search (BM25 + Vector), Embedding Cache
- ⚡ **High-Performance** - SIMD-optimiert, GPU-unterstützt
- 📊 **Monitoring** - OpenTelemetry, Prometheus metrics

---

## 🚀 Schnellstart (5 Minuten)

```bash
# 1. Pull Docker image
docker pull themisdb/themisdb:latest

# 2. Start ThemisDB
docker run -d \
  -p 9042:9042 \
  -v /data/themis:/data \
  --name themisdb \
    themisdb/themisdb:latest

# 3. Connect with Python client
pip install themisdb-client
```

```python
from themisdb import ThemisClient

# Connect to database
client = ThemisClient('localhost:9042')

# Create first entity
client.create_entity('users', {
    'id': 1,
    'name': 'Alice',
    'email': 'alice@example.com'
})

# Query data
users = client.query('FOR user IN users RETURN user')
print(users)
```

---

## Multi-Model Data Operations

ThemisDB supports **6 data models** in a unified architecture:

### 1. Relational (Tabular)

```python
# Create table-like structure
client.create_collection('employees', schema={
    'id': 'int',
    'name': 'string',
    'department': 'string',
    'salary': 'float'
})

# Insert rows
client.insert('employees', [
    {'id': 1, 'name': 'Alice', 'department': 'Engineering', 'salary': 120000},
    {'id': 2, 'name': 'Bob', 'department': 'Sales', 'salary': 95000}
])

# SQL-like queries (AQL)
result = client.query("""
    FOR emp IN employees
    FILTER emp.department == 'Engineering'
    SORT emp.salary DESC
    RETURN emp
""")
```

### 2. Document (JSON)

```python
# Insert flexible JSON documents
client.insert('products', {
    'id': 'p123',
    'name': 'Laptop',
    'specs': {
        'cpu': 'Intel i7',
        'ram': '16GB',
        'storage': {'type': 'SSD', 'size': '512GB'}
    },
    'tags': ['electronics', 'computers']
})

# Query nested fields
laptops = client.query("""
    FOR p IN products
    FILTER p.specs.storage.type == 'SSD'
    RETURN p
""")
```

### 3. Graph (Relationships)

```python
# Create vertices
client.create_vertex('users', {'id': 'u1', 'name': 'Alice'})
client.create_vertex('users', {'id': 'u2', 'name': 'Bob'})

# Create edges
client.create_edge('follows', 'u1', 'u2', {'since': '2024-01-15'})

# Traverse graph
followers = client.query("""
    FOR v, e, p IN 1..3 OUTBOUND 'users/u1' follows
    RETURN v
""")
```

### 4. Vector (Embeddings)

```python
# Store embeddings
client.insert('embeddings', {
    'id': 'doc1',
    'text': 'Machine learning tutorial',
    'embedding': [0.1, 0.2, 0.3, ...]  # 1536 dimensions
})

# Similarity search
similar = client.vector_search(
    collection='embeddings',
    query_vector=[0.15, 0.18, 0.25, ...],
    top_k=10
)
```

### 5. Time-Series (Hypertables)

```python
# Create hypertable
from themisdb.timeseries import Hypertable

hyper = Hypertable(client, 'metrics', 
                   chunk_interval_seconds=86400)  # 1 day

# Insert time-series data
hyper.insert(timestamp=1703001600, data={
    'sensor_id': 's1',
    'temperature': 23.5,
    'humidity': 65.2
})

# Query time range
data = hyper.query(
    start_time=1703001600,
    end_time=1703088000
)
```

### 6. Spatial (Geo)

```python
# Store location data
client.insert('locations', {
    'id': 'loc1',
    'name': 'Office',
    'coordinates': {'lat': 37.7749, 'lon': -122.4194}
})

# Geo queries
nearby = client.query("""
    FOR loc IN locations
    FILTER DISTANCE(loc.coordinates.lat, loc.coordinates.lon, 
                    37.7750, -122.4195) < 1000
    RETURN loc
""")
```

---

## RAG Application Patterns

### Basic RAG Pipeline

```python
from themisdb import ThemisClient
from themisdb.rag import HybridSearch, EmbeddingCache
import openai

# 1. Initialize components
client = ThemisClient('localhost:9042')
cache = EmbeddingCache(client, max_entries=100000, ttl_seconds=3600)
search = HybridSearch(client, bm25_weight=0.5, vector_weight=0.5)

# 2. Index documents
documents = [
    "ThemisDB is a multi-model database",
    "It supports vectors, graphs, and time-series",
    # ... more docs
]

for doc in documents:
    # Check embedding cache first
    embedding = cache.get(doc)
    if embedding is None:
        embedding = openai.Embedding.create(input=doc, model="text-embedding-3-small")
        cache.set(doc, embedding)
    
    client.insert('knowledge_base', {
        'text': doc,
        'embedding': embedding
    })

# 3. Query with hybrid search
query = "What is ThemisDB?"
query_embedding = cache.get(query) or openai.Embedding.create(input=query)

results = search.search(
    text_query=query,
    embedding=query_embedding,
    top_k=5
)

# 4. Generate response with LLM
context = "\n".join([r['text'] for r in results])
response = openai.ChatCompletion.create(
    model="gpt-4",
    messages=[
        {"role": "system", "content": f"Context: {context}"},
        {"role": "user", "content": query}
    ]
)
```

### vLLM Co-Location Pattern

```python
# Use ThemisDB + vLLM on same hardware
from themisdb.acceleration import VLLMResourceManager

# Initialize resource manager
resource_mgr = VLLMResourceManager({
    'gpu_memory_fraction': 0.7,  # 70% for vLLM, 30% for ThemisDB
    'adaptive_scaling': True
})

# Query vector DB
if resource_mgr.can_use_gpu():
    # GPU available - fast vector search
    results = client.vector_search_gpu(query_vector, top_k=10)
else:
    # CPU fallback
    results = client.vector_search(query_vector, top_k=10)
```

---

## Vector Search

### Basic Vector Search

```python
# 1. Create vector index
client.create_vector_index('documents', 
    dimension=1536,
    metric='cosine'
)

# 2. Insert vectors
client.insert('documents', {
    'id': 'doc1',
    'text': 'Sample document',
    'embedding': [0.1, 0.2, ...]  # 1536-dim vector
})

# 3. Search by similarity
results = client.vector_search(
    collection='documents',
    query_vector=[0.15, 0.18, ...],
    top_k=10,
    metric='cosine'
)

for result in results:
    print(f"{result['id']}: {result['score']}")
```

### Advanced Vector Search (FAISS IVF+PQ)

```python
from themisdb.index import AdvancedVectorIndex

# Create advanced index for billion-scale
index = AdvancedVectorIndex(
    dimension=1536,
    index_type='IVF_PQ',
    nlist=1024,      # Number of clusters
    nprobe=64,       # Search clusters
    pq_m=8,          # Sub-quantizers
    pq_nbits=8       # Bits per sub-quantizer
)

# Train index (required for IVF+PQ)
training_vectors = get_sample_vectors(100000)
index.train(training_vectors)

# Add vectors (10M+)
index.add(all_vectors)

# Search (10-100x memory reduction)
results = index.search(query_vector, top_k=10)
```

### Hybrid Search (BM25 + Vector)

```python
from themisdb.search import HybridSearch

# Initialize hybrid search
hybrid = HybridSearch(
    client,
    use_rrf=True,           # Reciprocal Rank Fusion
    bm25_weight=0.5,        # 50% keyword matching
    vector_weight=0.5,      # 50% semantic similarity
    k=10
)

# Search with both text and embedding
results = hybrid.search(
    text_query="machine learning tutorial",
    embedding=query_embedding,
    top_k=10
)

# 70-90% better recall than single-method search
```

---

## Time-Series Data

### Hypertables (TimescaleDB-compatible)

```python
from themisdb.timeseries import Hypertable

# Create hypertable
config = {
    'table_name': 'sensor_data',
    'chunk_interval_seconds': 86400,  # 1 day chunks
    'retention_days': 30
}
hypertable = Hypertable(client, **config)

# Insert time-series data
hypertable.insert(
    timestamp=1703001600,
    data={
        'sensor_id': 'temp_sensor_1',
        'value': 23.5,
        'location': 'warehouse_a'
    }
)

# Batch insert
hypertable.insert_batch([
    {'timestamp': t, 'data': {...}} 
    for t in range(start, end, 60)
])

# Query time range
results = hypertable.query(
    start_time=1703001600,
    end_time=1703088000,
    filters={'sensor_id': 'temp_sensor_1'}
)

# Compress old chunks
hypertable.compress_old_chunks()  # Chunks > 7 days

# Drop expired data
hypertable.drop_expired_chunks()  # Based on retention_days
```

### Time-Series Aggregates (SIMD-accelerated)

```python
from themisdb.timeseries import TimeSeriesAggregates

agg = TimeSeriesAggregates(client)

# Resample: 1-second data → 1-minute averages
result = agg.resample(
    collection='sensor_data',
    interval_seconds=60,
    aggregate_function='AVG',
    field='value'
)

# Rolling window: 5-minute moving average
rolling = agg.rolling_window(
    collection='sensor_data',
    window_seconds=300,
    aggregate_function='AVG',
    field='value'
)

# Time bucketing with multiple aggregates
buckets = agg.aggregate(
    collection='sensor_data',
    bucket_size=3600,  # 1 hour
    aggregates=['MIN', 'MAX', 'AVG', 'STDDEV', 'P95']
)

# 5-10x faster than regular aggregation (SIMD optimization)
```

---

## Authentication & Authorization

### Basic Authentication

```python
# Connect with username/password
client = ThemisClient(
    'localhost:9042',
    username='alice',
    password='secret123'
)

# Or use token
client = ThemisClient(
    'localhost:9042',
    token='eyJhbGc...'
)
```

### Role-Based Access Control (RBAC)

```python
# Check permissions before operations
if client.has_permission('documents', 'read'):
    docs = client.query('FOR d IN documents RETURN d')

# Different clients for different roles
admin_client = ThemisClient('localhost:9042', role='admin')
reader_client = ThemisClient('localhost:9042', role='reader')
```

---

## Best Practices

### 1. Connection Management

```python
# Use connection pooling
from themisdb import ConnectionPool

pool = ConnectionPool(
    host='localhost:9042',
    max_connections=10,
    timeout=30
)

with pool.get_connection() as client:
    # Operations
    client.query('...')
# Connection automatically returned to pool
```

### 2. Batch Operations

```python
# Bad: Insert one-by-one
for item in items:
    client.insert('collection', item)  # Slow

# Good: Batch insert
client.insert_batch('collection', items)  # 10-100x faster
```

### 3. Embedding Cache Usage

```python
# Always check cache before API calls
embedding = cache.get(text)
if embedding is None:
    embedding = expensive_api_call(text)
    cache.set(text, embedding, ttl=3600)

# 70-90% cost reduction on embedding APIs
```

### 4. Index Selection

```python
# Small datasets (< 1M vectors): Use HNSW
index = client.create_vector_index('docs', index_type='HNSW')

# Large datasets (1M - 1B vectors): Use IVF+PQ
index = AdvancedVectorIndex(index_type='IVF_PQ', nlist=1024)

# Choose based on memory vs. accuracy trade-off
```

### 5. Query Optimization

```python
# Bad: Full scan
result = client.query('FOR d IN documents RETURN d')

# Good: Use filters and indexes
result = client.query("""
    FOR d IN documents
    FILTER d.category == 'tech'
    LIMIT 100
    RETURN d
""")
```

---

## Troubleshooting

### Common Issues

#### 1. Connection Timeout

```python
# Increase timeout
client = ThemisClient('localhost:9042', timeout=60)

# Check network connectivity
client.ping()  # Returns True if connected
```

#### 2. Out of Memory (Vector Search)

```python
# Solution 1: Use IVF+PQ compression
index = AdvancedVectorIndex(index_type='IVF_PQ')  # 10-100x memory reduction

# Solution 2: Reduce batch size
client.insert_batch('docs', items, batch_size=1000)
```

#### 3. Slow Queries

```python
# Enable query profiling
client.enable_profiling()
result = client.query('...')
stats = client.get_query_stats()
print(f"Query time: {stats['duration_ms']}ms")

# Check if indexes are being used
explain = client.explain_query('FOR d IN documents FILTER d.id == 1 RETURN d')
```

#### 4. Embedding Cache Misses

```python
# Check cache statistics
stats = cache.get_stats()
print(f"Hit rate: {stats['hit_rate']}%")
print(f"Cost savings: ${stats['cost_savings_usd']}")

# Adjust similarity threshold
cache = EmbeddingCache(similarity_threshold=0.90)  # Lower for more hits
```

### Getting Help

- **Documentation:** https://docs.themisdb.com
- **GitHub Issues:** https://github.com/makr-code/ThemisDB/issues
- **Community Discord:** https://discord.gg/themisdb
- **Stack Overflow:** Tag `themisdb`

---

## Next Steps

- **Power User Guide:** Advanced optimization and performance tuning
- **Administrator Guide:** Deployment, backup, and operations
- **System Architect Guide:** Sharding, distributed systems, and migration

**Version:** 1.2.0 | **License:** MIT | **Support:** service@themisdb.org
