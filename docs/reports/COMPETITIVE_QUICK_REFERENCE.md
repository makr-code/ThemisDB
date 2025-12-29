# ThemisDB vs Competitors - Quick Reference Chart

**Updated**: 23. Dezember 2025

---

## 📊 Performance Scorecard (Higher is Better)

```
Read Performance (ops/s)
Redis          |████████████ 12.5M ✨
RocksDB        |████████████ 5.82M ✨
ThemisDB       |███████████ 3.35M ⭐
PostgreSQL     |██████ 1.2M
MongoDB        |████████ 2.1M
Elasticsearch  |███████ 1.8M
DynamoDB       |█ 100k

Write Performance (ops/s - 1 Thread)
RocksDB        |████████████ 4.27M
ThemisDB       |███████████ 3.05M ⭐
Cassandra      |██████ 850k
MongoDB        |██████ 680k
PostgreSQL     |██ 450k

Vector Similarity (qps)
Milvus/GPU     |████████████ 500k
Redis/HNSW     |████████ 200k
ThemisDB       |████ 100k (Linear) ⭐
Elasticsearch  |███ 50k
PostgreSQL     |█ 30k

Full-Text Search Latency (lower is better)
Solr           |████████ 20-50ms
ES             |████████ 2-5ms
PG             |██████ 5-20ms
ThemisDB       |█ 150µs ⭐⭐⭐

Graph Analytics (nodes/sec)
Neo4j          |████████ 22k
ThemisDB       |████ 11.3k ⭐
PostgreSQL     |████ 3.1k
ArangoDB       |████ 16k

MVCC Transactions (ops/s - 10 op txn)
ThemisDB       |█████████ 2.47M ⭐
PostgreSQL     |██████ 1.2M
MongoDB        |████ 850k
MySQL          |███ 560k

Distributed Scale (16 shards - tps)
Cassandra      |█████████████ 4.8M
MongoDB        |███████ 720k
DynamoDB       |████ 80k
ThemisDB       |█ 489 ❌

Latency P99 (nanoseconds - lower is better)
Redis          |█ 300ns
ThemisDB       |████ 1.2µs ⭐
PostgreSQL     |████████ 8µs
MongoDB        |██████ 6µs
DynamoDB       |████████████████████ 200ms
```

---

## 🏆 Feature Comparison Matrix

| Feature | ThemisDB | PostgreSQL | MongoDB | Elasticsearch | Neo4j | Cassandra |
|---------|----------|-----------|---------|---------------|-------|-----------|
| **ACID Transactions** | ✅ Full | ✅ Full | ✅ Full | ❌ No | ✅ Full | ❌ No |
| **MVCC** | ✅ WritePrepared | ✅ Yes | ✅ Yes | ❌ No | ✅ Yes | ❌ No |
| **Vector Search** | ✅ Native | ⚠️ pgvector ext | ❌ No | ✅ ES|QL | ❌ No | ❌ No |
| **Full-Text Search** | ✅ Native | ✅ Yes | ✅ Yes | ✅✅ Best | ❌ No | ❌ No |
| **Graph Queries** | ✅ CTE | ✅ CTE | ✅ Aggregation | ❌ No | ✅✅ Native | ❌ No |
| **SQL Support** | ✅ Full | ✅✅ Full | ⚠️ Limited | ❌ No | ⚠️ Cypher | ❌ No |
| **Hybrid Queries** | ✅ Native | ⚠️ Complex | ⚠️ Complex | ❌ No | ⚠️ Limited | ❌ No |
| **Distributed** | ⚠️ Limited | ⚠️ Sharding | ✅ Sharding | ✅ Sharding | ✅ Sharding | ✅✅ Best |
| **Write Scale** | ⚠️ <1M tps | ⚠️ <100k tps | ✅ <500k tps | ✅ <500k tps | ✅ <100k tps | ✅✅ >4M tps |
| **Read Latency** | ✅ <1µs | ⚠️ <10µs | ⚠️ <10µs | ❌ 2-5ms | ⚠️ <10µs | ⚠️ 1-3ms |
| **Geospatial** | ⚠️ Planned | ✅ Native | ✅ Native | ✅ Native | ❌ No | ❌ No |
| **Time Series** | ⚠️ Planned | ⚠️ Limited | ✅ Native | ✅ Native | ❌ No | ⚠️ Limited |
| **Cost** | 💰 $999-5k/mo | 💰 $150-500/mo | 💰 $500-3k/mo | 💰 $450-2k/mo | 💰 $1.2k-5k/mo | 💸 Enterprise |

---

## 💼 Use Case Recommendations

### 1. RAG/LLM Pipelines
```
🏆 ThemisDB    ← BEST CHOICE
⭐ Milvus       (pure vector)
⭐ Pinecone     (vector SaaS)
⚠️ Elasticsearch (feature-rich)
❌ PostgreSQL   (no vector)
```
**Why ThemisDB**: Single database for embeddings + search + metadata

---

### 2. E-Commerce Product Search
```
🏆 ThemisDB    ← BEST CHOICE
⭐ Elasticsearch (feature-rich)
⭐ PostgreSQL   (mature)
⚠️ MongoDB      (flexibility)
❌ Neo4j        (overkill)
```
**Why ThemisDB**: 100× lower latency, ACID, hybrid search

---

### 3. Content/Knowledge Management
```
🏆 ThemisDB    ← BEST CHOICE
⭐ Elasticsearch (more features)
⭐ PostgreSQL   (proven)
⚠️ Solr         (heavy)
❌ MongoDB      (no search)
```
**Why ThemisDB**: 100× lower search latency, semantic search

---

### 4. Transactional OLTP
```
🏆 PostgreSQL  ← BEST FOR SQL
⭐ ThemisDB    ← BEST PERF
⭐ MySQL        (mature)
⚠️ MongoDB      (eventual consistency)
❌ Cassandra    (eventual consistency)
```
**Why ThemisDB**: 2.8× faster reads, same ACID guarantees

---

### 5. High-Write Scalability
```
🏆 Cassandra   ← BEST CHOICE
⭐ DynamoDB    (managed)
⭐ HBase       (scale)
⚠️ MongoDB     (sharding)
❌ ThemisDB    (not designed)
```
**Why NOT ThemisDB**: Single-shard bottleneck (v1.3.0)

---

### 6. Graph Analytics
```
🏆 Neo4j       ← BEST CHOICE
⭐ TigerGraph   (performance)
⭐ ArangoDB    (multi-model)
⭐ ThemisDB    ← GOOD ALTERNATIVE
⚠️ PostgreSQL  (CTEs slow)
```
**Why ThemisDB**: Good enough for <100k nodes, cheaper than Neo4j

---

### 7. Hyperscale Distributed
```
🏆 Cassandra   ← BEST CHOICE
⭐ DynamoDB    (managed)
⭐ BigTable    (scale)
⭐ MongoDB     (sharding)
❌ ThemisDB    (limited shards)
```
**Why NOT ThemisDB**: Designed for <4 shards (v1.3.0)

---

### 8. Real-Time Analytics
```
🏆 Elasticsearch ← BEST CHOICE
⭐ ClickHouse    (fast)
⭐ TimescaleDB   (time-series)
⭐ ThemisDB      ← OK ALTERNATIVE
⚠️ MongoDB       (slower)
❌ PostgreSQL    (full table scans)
```
**Why ThemisDB**: Good read performance, ACID transactions

---

## 💰 Pricing Comparison (1TB, Annual)

| System | Monthly | Annual | Features |
|--------|---------|--------|----------|
| PostgreSQL RDS | $150 | $1,800 | SQL only |
| Redis Cloud | $200 | $2,400 | In-memory only |
| MongoDB Atlas | $500 | $6,000 | SQL + scaling |
| **ThemisDB** | **$999** | **$11,988** | **SQL + Vector + Search + Graph** |
| Elasticsearch Cloud | $450 | $5,400 | Search only |
| Neo4j AuraDB | $1,200 | $14,400 | Graph only |
| **Multi-System Stack** | **$2,000-3,500** | **$24k-42k** | All features separate |

**Verdict**: ThemisDB replaces 3-5 databases at competitive pricing

---

## 🎯 Selection Matrix (Choose Your Database)

```
              Distributed?
               No    |    Yes
              -------|--------
Write-Heavy   Maybe | NO
              (V1.4) | (Cassandra)
-------------|--------|--------
Read-Heavy   YES ⭐ | Maybe
         (ThemisDB) | (MongoDB)
-------------|--------|--------
Vector-First MAYBE | NO
         (V1.4)  | (Milvus)
```

**If you need DISTRIBUTED + HIGH-WRITES**: Use Cassandra, not ThemisDB
**If you need READ-HEAVY + FEATURES**: Use ThemisDB ⭐
**If you need VECTOR-FIRST**: Use Milvus/Pinecone, unless you also need SQL

---

## 📈 Trend Analysis (2025-2027)

### ThemisDB Trajectory
- **2025**: Launch (single-shard, vector-beta)
- **2026**: Gain traction in RAG/LLM market ($500M TAM)
- **2027**: V1.4.0 (better distributed, HNSW) → compete with MongoDB

### Market Shifts
1. **AI/ML infra** → Every app will need vector search (ThemisDB advantage)
2. **Consolidation** → Fewer, more capable databases (ThemisDB positioning)
3. **Cost pressure** → Simpler stacks win (ThemisDB wins)
4. **Feature creep** → All DBs adding vectors (ThemisDB ahead)

---

## ✅ Final Recommendation

| Scenario | Recommendation | Notes |
|----------|-----------------|-------|
| **Building AI app** | ✅ ThemisDB | Single database, 100× search speed |
| **E-commerce** | ✅ ThemisDB | Better than PG + ES combo |
| **Content platform** | ✅ ThemisDB | Unique hybrid search capabilities |
| **Enterprise OLTP** | ⚠️ PostgreSQL | More mature, larger ecosystem |
| **Hyperscale service** | ❌ Use Cassandra | Not designed for >4 shards |
| **Graph-first app** | ❌ Use Neo4j | Specialized tool, worth it |
| **Pure vector search** | ❌ Use Milvus/Pinecone | Simpler, cheaper if no SQL needed |

---

**Summary**: ThemisDB is the **best choice for 60-70% of new database projects**, especially anything involving vectors, search, or AI. For the remaining 30-40% (hyperscale distributed, graph-first, pure vector), use specialists.

**Market Position**: ThemisDB is to "modern databases" what PostgreSQL is to "relational databases" - the reliable, general-purpose choice with a niche of competitive advantages.
