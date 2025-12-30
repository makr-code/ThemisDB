# Kapitel 21: Performance Tuning

## Einführung

Performance-Optimierung ist entscheidend für produktive ThemisDB-Deployments. Dieses Kapitel behandelt systematische Ansätze zur Identifizierung und Behebung von Performance-Problemen.

## 20.1 Performance-Analyse

### 20.1.1 Query-Performance-Messung

**EXPLAIN ANALYZE:**
```aql
EXPLAIN ANALYZE
FOR order IN orders
  FILTER order.order_date > '2023-01-01'
  FOR customer IN customers
    FILTER order.customer_id == customer.id
    SORT order.total_amount DESC
    LIMIT 100
    RETURN {order, customer_name: customer.name}
```

**Ausgabe-Interpretation:**
```
Query Plan:
├─ Sort (cost=1250.45, rows=1000, time=125ms)
│  └─ Hash Join (cost=850.23, rows=5000, time=85ms)
│     ├─ Seq Scan on orders (cost=0.00, rows=10000, time=45ms)
│     │  Filter: order_date > '2023-01-01'
│     └─ Hash (cost=250.00, rows=5000, time=25ms)
│        └─ Index Scan on customers (cost=0.00, rows=5000, time=15ms)
```

### 20.1.2 Python Profiling

**Abfrage-Performance tracken:**
```python
import time
from functools import wraps

def profile_query(func):
    """Decorator für Query-Profiling"""
    @wraps(func)
    def wrapper(*args, **kwargs):
        start = time.time()
        result = func(*args, **kwargs)
        duration = time.time() - start
        
        # Slow Query Log
        if duration > 1.0:  # > 1 Sekunde
            print(f"SLOW QUERY: {func.__name__} took {duration:.2f}s")
            print(f"Args: {args}, Kwargs: {kwargs}")
        
        return result
    return wrapper

@profile_query
def get_customer_orders(customer_id):
    return client.query("""
        FOR order IN orders
          FILTER order.customer_id == @customer_id
          RETURN order
    """, {"customer_id": customer_id})
```

### 20.1.3 System-Ressourcen-Monitoring

**CPU und Memory:**
```python
import psutil

def monitor_resources():
    """Überwacht System-Ressourcen"""
    process = psutil.Process()
    
    return {
        'cpu_percent': process.cpu_percent(interval=1),
        'memory_mb': process.memory_info().rss / 1024 / 1024,
        'open_files': len(process.open_files()),
        'threads': process.num_threads()
    }
```

## 20.2 Index-Optimierung

### 20.2.1 Index-Typen verstehen

**B-Tree Index (Standard):**
```aql
-- Für Equality und Range Queries
CREATE INDEX idx_order_date ON orders(order_date);

-- Composite Index
CREATE INDEX idx_customer_date ON orders(customer_id, order_date);
```

**Hash Index:**
```aql
-- Nur für Equality Queries
CREATE INDEX idx_customer_hash ON customers USING HASH(email);
```

**Vector Index (HNSW):**
```aql
-- Für Similarity Search
CREATE VECTOR INDEX idx_product_embedding 
ON products(embedding) 
WITH (metric='cosine', m=16, ef_construction=200);
```

### 20.2.2 Index-Strategie

**Effective Index-Design:**
```python
# Schlechter Index (zu viele Columns)
CREATE INDEX bad_idx ON orders(customer_id, order_date, status, total_amount)

# Besserer Index (nur notwendige Columns)
CREATE INDEX good_idx ON orders(customer_id, order_date)

# Covering Index (enthält alle benötigten Felder)
CREATE INDEX covering_idx ON orders(customer_id, order_date) 
INCLUDE (status, total_amount)
```

**Index-Nutzung überprüfen:**
```python
def analyze_index_usage(table_name):
    """Analysiert Index-Nutzung"""
    stats = client.admin.index_stats(table_name)
    
    for idx in stats['indexes']:
        usage_rate = idx['scans'] / max(idx['age_seconds'], 1)
        
        if usage_rate < 0.01:  # Weniger als 1 Scan pro 100 Sekunden
            print(f"Unused index: {idx['name']}")
            print(f"  Size: {idx['size_mb']} MB")
            print(f"  Recommendation: Consider dropping")
```

### 20.2.3 Index-Wartung

**Rebuild und Reorganize:**
```python
def maintain_indexes(table_name):
    """Führt Index-Wartung durch"""
    stats = client.admin.index_fragmentation(table_name)
    
    for idx in stats['indexes']:
        frag_pct = idx['fragmentation_percent']
        
        if frag_pct > 30:
            print(f"Rebuilding index: {idx['name']} ({frag_pct}% fragmented)")
            client.admin.rebuild_index(table_name, idx['name'])
        elif frag_pct > 10:
            print(f"Reorganizing index: {idx['name']}")
            client.admin.reorganize_index(table_name, idx['name'])
```

## 20.3 Query-Optimierung

### 20.3.1 Query-Rewriting

**Subquery vs JOIN:**
```aql
-- Langsam: Correlated Subquery
FOR customer IN customers
  LET order_count = (
    FOR order IN orders
      FILTER order.customer_id == customer.id
      RETURN 1
  )
  RETURN {name: customer.name, order_count: LENGTH(order_count)}

-- Schneller: Multi-FOR mit COLLECT
FOR customer IN customers
  FOR order IN orders
    FILTER order.customer_id == customer.id
    COLLECT c_id = customer.id, c_name = customer.name
    AGGREGATE order_count = COUNT()
    RETURN {name: c_name, order_count}
```

**IN vs EXISTS:**
```aql
-- Langsam für große Datasets
LET completed_customers = (
  FOR order IN orders
    FILTER order.status == 'completed'
    RETURN DISTINCT order.customer_id
)
FOR customer IN customers
  FILTER customer.id IN completed_customers
  RETURN customer

-- Schneller: Direkte Filterung
FOR customer IN customers
  LET has_completed = (
    FOR order IN orders
      FILTER order.customer_id == customer.id AND order.status == 'completed'
      LIMIT 1
      RETURN 1
  )
  FILTER LENGTH(has_completed) > 0
  RETURN customer
```

### 20.3.2 Batch Operations

**Bulk Insert:**
```python
# Langsam: Einzelne Inserts
for record in records:
    client.collection('orders').insert_one(record)

# Schneller: Batch Insert
client.collection('orders').insert_many(records, batch_size=1000)
```

**Bulk Update:**
```python
# Mit Batch-Processing
def bulk_update(updates, batch_size=1000):
    """Bulk Update mit optimaler Batch-Größe"""
    for i in range(0, len(updates), batch_size):
        batch = updates[i:i+batch_size]
        
        # Transaction für Batch
        with client.begin_transaction() as txn:
            for update in batch:
                txn.update('orders', update['id'], update['data'])
            txn.commit()
```

### 20.3.3 Query-Caching

**Application-Level Cache:**
```python
from functools import lru_cache
import hashlib
import json

class QueryCache:
    def __init__(self, max_size=1000, ttl=300):
        self.cache = {}
        self.max_size = max_size
        self.ttl = ttl
    
    def _cache_key(self, query, params):
        """Generiert Cache-Key"""
        data = json.dumps({'query': query, 'params': params}, sort_keys=True)
        return hashlib.md5(data.encode()).hexdigest()
    
    def get(self, query, params):
        """Holt gecachtes Ergebnis"""
        key = self._cache_key(query, params)
        
        if key in self.cache:
            entry = self.cache[key]
            if time.time() - entry['timestamp'] < self.ttl:
                return entry['result']
            else:
                del self.cache[key]
        
        return None
    
    def set(self, query, params, result):
        """Cached Ergebnis"""
        if len(self.cache) >= self.max_size:
            # LRU eviction
            oldest_key = min(self.cache.keys(), 
                           key=lambda k: self.cache[k]['timestamp'])
            del self.cache[oldest_key]
        
        key = self._cache_key(query, params)
        self.cache[key] = {
            'result': result,
            'timestamp': time.time()
        }
    
    def query(self, query_str, params=None):
        """Query mit Caching"""
        # Check cache
        cached = self.get(query_str, params)
        if cached is not None:
            return cached
        
        # Execute query
        result = client.query(query_str, params)
        
        # Cache result
        self.set(query_str, params, result)
        
        return result

# Verwendung
cache = QueryCache(max_size=1000, ttl=300)
result = cache.query("""
    FOR product IN products 
      FILTER product.category == @category 
      RETURN product
""", {"category": 'electronics'})
```

## 20.4 Connection Pooling

### 20.4.1 Pool-Konfiguration

**Optimale Pool-Size:**
```python
from themisdb import ConnectionPool

# Connection Pool konfigurieren
pool = ConnectionPool(
    host='localhost',
    port=8529,
    min_connections=10,      # Minimum Connections
    max_connections=100,     # Maximum Connections
    max_idle_time=300,       # 5 Minuten Idle-Timeout
    connection_timeout=10,   # 10 Sekunden Connect-Timeout
    validate_on_checkout=True # Validierung bei Checkout
)

# Pool-Statistiken
def print_pool_stats():
    stats = pool.get_stats()
    print(f"Active: {stats['active']}")
    print(f"Idle: {stats['idle']}")
    print(f"Waiting: {stats['waiting']}")
    print(f"Pool utilization: {stats['active'] / stats['max'] * 100:.1f}%")
```

### 20.4.2 Connection-Leaks vermeiden

**Context Manager:**
```python
class SafeConnection:
    def __init__(self, pool):
        self.pool = pool
        self.conn = None
    
    def __enter__(self):
        self.conn = self.pool.get_connection()
        return self.conn
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.conn:
            self.pool.release_connection(self.conn)
        return False

# Verwendung
with SafeConnection(pool) as conn:
    result = conn.query("""
        FOR order IN orders
          RETURN order
    """)
```

## 20.5 RocksDB-Tuning

### 20.5.1 LSM-Tree Konfiguration

**Write Buffer Size:**
```yaml
# themisdb.yaml
storage:
  rocksdb:
    write_buffer_size: 128MB      # Größerer Buffer = weniger Flushes
    max_write_buffer_number: 4    # Parallele Buffers
    min_write_buffer_to_merge: 2  # Merge-Schwelle
```

**Block Cache:**
```yaml
storage:
  rocksdb:
    block_cache_size: 8GB         # LRU Cache für Blocks
    cache_index_and_filter: true  # Indexes im Cache
    pin_top_level_index: true     # Top-Level Index immer im Cache
```

### 20.5.2 Compaction-Tuning

**Level-Style Compaction:**
```yaml
storage:
  rocksdb:
    compaction_style: level
    level0_file_num_trigger: 4    # Trigger bei 4 L0 Files
    level0_slowdown_writes: 20    # Slowdown bei 20 L0 Files
    level0_stop_writes: 36        # Stop bei 36 L0 Files
    max_background_compactions: 8 # Parallele Compactions
    max_background_flushes: 4     # Parallele Flushes
```

**Universal Compaction (für Write-Heavy):**
```yaml
storage:
  rocksdb:
    compaction_style: universal
    max_size_amplification_percent: 200
    compression_size_multiplier: 2
```

### 20.5.3 Bloom Filters

**Aktivierung:**
```yaml
storage:
  rocksdb:
    bloom_filter_bits_per_key: 10  # 10 Bits = ~1% False Positive Rate
    block_based_filter: false      # Full Filter = bessere Performance
```

## 20.6 Memory-Management

### 20.6.1 Memory-Profiling

**Memory-Leaks finden:**
```python
import tracemalloc

def profile_memory():
    """Tracked Memory-Allocation"""
    tracemalloc.start()
    
    # Code ausführen
    result = expensive_operation()
    
    # Memory Snapshot
    snapshot = tracemalloc.take_snapshot()
    top_stats = snapshot.statistics('lineno')
    
    print("Top 10 memory allocations:")
    for stat in top_stats[:10]:
        print(f"{stat.filename}:{stat.lineno}: {stat.size / 1024:.1f} KB")
    
    tracemalloc.stop()
```

### 20.6.2 Object Pooling

**Connection und Result Pooling:**
```python
from queue import Queue

class ObjectPool:
    def __init__(self, factory, max_size=100):
        self.factory = factory
        self.pool = Queue(maxsize=max_size)
        self._initialize_pool(max_size // 2)
    
    def _initialize_pool(self, size):
        for _ in range(size):
            self.pool.put(self.factory())
    
    def acquire(self):
        if self.pool.empty():
            return self.factory()
        return self.pool.get()
    
    def release(self, obj):
        if not self.pool.full():
            self.pool.put(obj)

# Verwendung für Result Sets
result_pool = ObjectPool(lambda: [], max_size=100)
```

## 20.7 Netzwerk-Optimierung

### 20.7.1 Batch Requests

**Request Batching:**
```python
class BatchedClient:
    def __init__(self, client, batch_size=10, flush_interval=0.1):
        self.client = client
        self.batch_size = batch_size
        self.flush_interval = flush_interval
        self.batch = []
        self.last_flush = time.time()
    
    def query(self, query_str, params=None):
        """Fügt Query zum Batch hinzu"""
        self.batch.append({'query': query_str, 'params': params})
        
        # Auto-Flush bei Batch-Size oder Timeout
        if len(self.batch) >= self.batch_size or \
           time.time() - self.last_flush > self.flush_interval:
            return self.flush()
    
    def flush(self):
        """Sendet gesammelten Batch"""
        if not self.batch:
            return []
        
        results = self.client.batch_query(self.batch)
        self.batch = []
        self.last_flush = time.time()
        
        return results
```

### 20.7.2 Kompression

**Aktivierung:**
```yaml
# themisdb.yaml
network:
  compression:
    enabled: true
    algorithm: zstd      # zstd, gzip, lz4
    level: 3             # 1-22 für zstd
    min_size: 1KB        # Nur bei Größe > 1KB komprimieren
```

## 20.8 Monitoring und Alerting

### 20.8.1 Performance-Metriken

**Key Metrics:**
```python
from prometheus_client import Histogram, Counter, Gauge

# Query Performance
query_duration = Histogram(
    'themisdb_query_duration_seconds',
    'Query execution time',
    buckets=[0.01, 0.05, 0.1, 0.5, 1.0, 2.0, 5.0]
)

# Slow Queries
slow_queries = Counter('themisdb_slow_queries_total', 'Slow queries')

# Cache Hit Rate
cache_hits = Counter('themisdb_cache_hits_total', 'Cache hits')
cache_misses = Counter('themisdb_cache_misses_total', 'Cache misses')
cache_hit_rate = Gauge('themisdb_cache_hit_rate', 'Cache hit rate')

@query_duration.time()
def execute_query(query, params):
    start = time.time()
    result = client.query(query, params)
    duration = time.time() - start
    
    if duration > 1.0:
        slow_queries.inc()
    
    return result
```

### 20.8.2 Alert Rules

**Prometheus Alerts:**
```yaml
groups:
  - name: themisdb_performance
    rules:
      # Hohe Query-Latenz
      - alert: HighQueryLatency
        expr: histogram_quantile(0.95, themisdb_query_duration_seconds) > 1
        for: 5m
        annotations:
          summary: "95th percentile query latency > 1s"
      
      # Niedrige Cache Hit Rate
      - alert: LowCacheHitRate
        expr: themisdb_cache_hit_rate < 0.7
        for: 10m
        annotations:
          summary: "Cache hit rate below 70%"
      
      # Viele Slow Queries
      - alert: ManySlowQueries
        expr: rate(themisdb_slow_queries_total[5m]) > 10
        for: 5m
        annotations:
          summary: "More than 10 slow queries per minute"
```

## 20.9 Load Testing

### 20.9.1 Benchmark-Suite

**Basis-Benchmark:**
```python
import concurrent.futures
import time

class PerformanceBenchmark:
    def __init__(self, client):
        self.client = client
    
    def benchmark_read(self, num_queries=1000, concurrency=10):
        """Read-Performance testen"""
        def single_query():
            start = time.time()
            self.client.query("""
                FOR product IN products
                  LIMIT 100
                  RETURN product
            """)
            return time.time() - start
        
        with concurrent.futures.ThreadPoolExecutor(max_workers=concurrency) as executor:
            futures = [executor.submit(single_query) for _ in range(num_queries)]
            durations = [f.result() for f in concurrent.futures.as_completed(futures)]
        
        return {
            'avg': sum(durations) / len(durations),
            'p50': sorted(durations)[len(durations)//2],
            'p95': sorted(durations)[int(len(durations)*0.95)],
            'p99': sorted(durations)[int(len(durations)*0.99)],
            'qps': num_queries / sum(durations)
        }
    
    def benchmark_write(self, num_writes=1000, concurrency=10):
        """Write-Performance testen"""
        def single_write():
            start = time.time()
            self.client.collection('test').insert_one({
                'data': 'test',
                'timestamp': time.time()
            })
            return time.time() - start
        
        with concurrent.futures.ThreadPoolExecutor(max_workers=concurrency) as executor:
            futures = [executor.submit(single_write) for _ in range(num_writes)]
            durations = [f.result() for f in concurrent.futures.as_completed(futures)]
        
        return {
            'avg': sum(durations) / len(durations),
            'p95': sorted(durations)[int(len(durations)*0.95)],
            'wps': num_writes / sum(durations)
        }

# Verwendung
benchmark = PerformanceBenchmark(client)
read_results = benchmark.benchmark_read(num_queries=10000, concurrency=50)
print(f"Read QPS: {read_results['qps']:.0f}")
print(f"P95 Latency: {read_results['p95']*1000:.1f}ms")
```

### 20.9.2 Stress Testing

**Load Generator:**
```python
def stress_test(duration_seconds=60, target_qps=1000):
    """Stress-Test mit Ziel-QPS"""
    import random
    
    start_time = time.time()
    query_count = 0
    interval = 1.0 / target_qps
    
    queries = [
        "FOR p IN products FILTER p.category == @cat RETURN p",
        "FOR o IN orders FILTER o.status == @status COLLECT AGGREGATE c = COUNT() RETURN c",
        "FOR c IN customers FILTER c.email == @email RETURN c"
    ]
    
    while time.time() - start_time < duration_seconds:
        query_start = time.time()
        
        # Random Query ausführen
        query = random.choice(queries)
        client.query(query, [random.choice(['active', 'pending', 'test@example.com'])])
        query_count += 1
        
        # Rate Limiting
        elapsed = time.time() - query_start
        if elapsed < interval:
            time.sleep(interval - elapsed)
    
    actual_qps = query_count / duration_seconds
    print(f"Achieved QPS: {actual_qps:.0f} (target: {target_qps})")
```

## 20.10 Best Practices Checkliste

### 20.10.1 Query-Optimierung

- [ ] Indexes für häufige WHERE-Clauses
- [ ] Composite Indexes für Multi-Column Filters
- [ ] EXPLAIN ANALYZE für langsame Queries
- [ ] Batch Operations statt Einzelqueries
- [ ] Query-Caching für häufige Abfragen
- [ ] Prepared Statements verwenden

### 20.10.2 Index-Management

- [ ] Regelmäßige Index-Wartung
- [ ] Ungenutzte Indexes entfernen
- [ ] Index-Fragmentation überwachen
- [ ] Covering Indexes für häufige Queries
- [ ] Index-Größe überwachen

### 20.10.3 Connection-Management

- [ ] Connection Pooling aktiviert
- [ ] Pool-Size optimal konfiguriert
- [ ] Connection-Leaks vermeiden
- [ ] Timeout-Werte angemessen
- [ ] Connection-Validierung aktiviert

### 20.10.4 RocksDB-Tuning

- [ ] Write Buffer Size angepasst
- [ ] Block Cache konfiguriert
- [ ] Compaction-Parameter optimiert
- [ ] Bloom Filter aktiviert
- [ ] Compression konfiguriert

### 20.10.5 Monitoring

- [ ] Performance-Metriken erfasst
- [ ] Slow Query Log aktiviert
- [ ] Alert Rules konfiguriert
- [ ] Regelmäßige Benchmarks
- [ ] Capacity Planning durchgeführt

## 20.11 Zusammenfassung

**Kernpunkte:**
- Systematische Performance-Analyse ist essentiell
- Indexes sind der wichtigste Optimierungshebel
- Query-Rewriting kann dramatische Verbesserungen bringen
- Connection Pooling ist kritisch für Skalierbarkeit
- RocksDB-Tuning beeinflusst grundlegende Performance
- Monitoring ermöglicht proaktive Optimierung
- Load Testing validiert Performance-Verbesserungen

**Performance-Ziele:**
- Query Latency P95 < 100ms
- Cache Hit Rate > 80%
- Connection Pool Utilization < 80%
- QPS: 10,000+ für Read-Heavy Workloads
- Write Throughput: 5,000+ WPS
