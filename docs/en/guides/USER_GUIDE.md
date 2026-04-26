---
category: "📋 Guides"
version: "v1.3.0"
status: "✅"
date: "December 22, 2025"
audience: "Application developers, data engineers, AI/ML engineers"
---

# 📋 ThemisDB User Guide

Comprehensive guide for application developers and data engineers.

## 📋 Table of Contents

- [📋 Overview](#-overview)
- [✨ Features & Highlights](#-features--highlights)
- [🚀 Quick Start](#-quick-start)
- [📖 Multi-Model Data](#-multi-model-data)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 See Also](#-see-also)
- [📝 Changelog](#-changelog)

---

## 📋 Overview

ThemisDB is a multi-model database for modern applications with:
- 6 integrated data models
- Enterprise features like transactions & security
- RAG (Retrieval-Augmented Generation) for AI/ML
- GPU acceleration for vector search

**Target Audience:** Application developers, data engineers, AI/ML engineers  
**Version:** 1.3.0  
**Last Updated:** April 2026

---

## ✨ Features & Highlights

- 🚀 **Multi-Model** - Relational, Document, Graph, Vector, Time-Series, Spatial
- 🔐 **Enterprise** - ACID transactions, RBAC, TLS/mTLS
- 🎯 **RAG-Ready** - Hybrid Search (BM25 + Vector), Embedding Cache
- ⚡ **High-Performance** - SIMD-optimized, GPU-supported
- 📊 **Monitoring** - OpenTelemetry, Prometheus metrics

---

## 🚀 Quick Start (5 Minutes)

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

---

## Best Practices

### Performance Optimization

**1. Use Appropriate Indexes:**
```python
# Range queries
client.create_index('users', 'age', index_type='range')

# Text search
client.create_index('documents', 'content', index_type='fulltext')

# Vector search
client.create_vector_index('embeddings', dimension=1536)
```

**2. Batch Operations:**
```python
# Better: batch insert
client.insert_batch('users', [
    {'id': 1, 'name': 'Alice'},
    {'id': 2, 'name': 'Bob'},
    # ... 1000s of records
])

# Avoid: individual inserts in loop
for user in users:
    client.insert('users', user)  # Slow!
```

**3. Use Transactions for Consistency:**
```python
with client.transaction() as txn:
    txn.insert('accounts', {'id': 1, 'balance': 1000})
    txn.update('accounts', {'id': 2, 'balance': 2000})
    txn.commit()
```

### Security Best Practices

**1. Enable Authentication:**
```python
client = ThemisClient('localhost:9042', 
    username='app_user',
    password='secure_password',
    use_tls=True
)
```

**2. Use Role-Based Access Control:**
```python
# Grant minimal permissions
client.grant_permission('app_user', 'READ', 'public_data')
client.grant_permission('app_user', 'WRITE', 'user_content')
```

**3. Encrypt Sensitive Data:**
```python
from themisdb.encryption import FieldEncryption

# Encrypt sensitive fields
encryption = FieldEncryption(key=your_key)
client.insert('users', {
    'id': 1,
    'name': 'Alice',
    'ssn': encryption.encrypt('123-45-6789')
})
```

---

## Troubleshooting

### Connection Issues

**Problem:** Cannot connect to database
```python
# Check connectivity
import socket
sock = socket.socket()
try:
    sock.connect(('localhost', 9042))
    print("Port is open")
except:
    print("Cannot connect - check if server is running")
```

**Solution:**
- Verify server is running: `docker ps` or `systemctl status themisdb`
- Check firewall rules
- Verify connection string and credentials

### Performance Issues

**Problem:** Slow queries
```python
# Enable query profiling
result = client.query("FOR doc IN documents RETURN doc", 
                     profile=True)
print(result.profile)
```

**Solutions:**
- Add appropriate indexes
- Use pagination for large result sets
- Check cache configuration
- Monitor resource usage

### Memory Issues

**Problem:** Out of memory errors
```python
# Check memory usage
stats = client.get_stats()
print(f"Memory used: {stats['memory_used_gb']} GB")
print(f"Cache hit rate: {stats['cache_hit_rate']}")
```

**Solutions:**
- Increase available memory
- Tune cache size in configuration
- Use streaming for large datasets
- Implement pagination

---

## See Also

### Documentation
- [Quick Start Guide](QUICK_START.md) - Get started in 5 minutes
- [Administrator Guide](ADMINISTRATOR_GUIDE.md) - Operations and deployment
- [AQL Reference](../aql/README.md) - Query language documentation
- [API Reference](../apis/README.md) - Complete API documentation

### Advanced Topics
- [LLM Integration](../../de/llm/README.md) - AI/ML capabilities
- [Security](../security/README.md) - Security features
- [Performance Tuning](../../de/guides/guides_performance.md) - Optimization guide

### Resources
- [GitHub Repository](https://github.com/makr-code/ThemisDB)
- [Examples](https://github.com/makr-code/ThemisDB/tree/main/examples)
- [Community Forum](https://github.com/makr-code/ThemisDB/discussions)

---

## Changelog

### v1.3.0 - December 22, 2025
- ✅ Updated to new documentation template
- ✅ Added RAG application patterns
- ✅ Added vLLM co-location examples
- ✅ Enhanced vector search section
- ✅ Added best practices section

### v1.0.0 - December 5, 2025
- 🚀 Initial user guide release
- 📖 Multi-model data examples
- 💡 Basic best practices

---

> **Note:** For the most detailed and up-to-date information, please refer to the [German user guide](../../de/guides/USER_GUIDE.md).

**Version:** 1.3.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
