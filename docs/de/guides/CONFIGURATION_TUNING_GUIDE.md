# ThemisDB Konfigurations- und Tuning-Guide

**Version:** 1.3.0  
**Stand:** 6. April 2026  
**Zielgruppe:** Database Administrators, DevOps Engineers, System Architects

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Konfigurationsdateien](#konfigurationsdateien)
- [Storage-Tuning (RocksDB)](#storage-tuning-rocksdb)
- [Server-Tuning](#server-tuning)
- [Vector-Index-Tuning](#vector-index-tuning)
- [Memory-Management](#memory-management)
- [I/O-Optimierung](#io-optimierung)
- [Feature-Konfiguration](#feature-konfiguration)
- [Use-Case-spezifisches Tuning](#use-case-spezifisches-tuning)
- [Hardware-spezifische Konfigurationen](#hardware-spezifische-konfigurationen)
- [Monitoring und Troubleshooting](#monitoring-und-troubleshooting)
- [Checkliste für Production](#checkliste-für-production)

---

## Übersicht

ThemisDB bietet umfangreiche Konfigurationsmöglichkeiten zur Optimierung für verschiedene Workloads, Hardware-Konfigurationen und Anwendungsfälle. Dieser Guide erklärt systematisch alle Schalter und Parameter und gibt Best Practices für das Tuning.

### Wichtigste Prinzipien

1. **Keine universelle Konfiguration**: Optimale Settings hängen von Workload, Hardware und Zielen ab
2. **Messen vor Optimieren**: Baseline-Metriken erfassen, dann iterativ tunen
3. **Trade-offs verstehen**: Jede Optimierung hat Vor- und Nachteile
4. **Schrittweise vorgehen**: Eine Änderung nach der anderen, dann messen

---

## Konfigurationsdateien

### Dateiformat und Speicherorte

ThemisDB unterstützt **YAML** und **JSON** für Konfigurationsdateien.

**Standard-Suchpfade** (in dieser Reihenfolge):
```
1. ./config.yaml
2. ./config.yml
3. ./config.json
4. ./config/config.yaml
5. ./config/config.yml
6. ./config/config.json
7. /etc/vccdb/config.yaml
8. /etc/vccdb/config.yml
9. /etc/vccdb/config.json
```

**Eigenen Pfad angeben:**
```bash
./themis_server --config /pfad/zur/config.yaml
```

### Vollständige Referenz

Die komplette Konfigurationsreferenz mit allen verfügbaren Parametern finden Sie in:
```
config/config.yaml
```

### Vordefinierte Konfigurationen

ThemisDB enthält optimierte Konfigurationen für gängige Hardware:

| Datei | Hardware | RAM | Zweck |
|-------|----------|-----|-------|
| `config.rpi3.json` | Raspberry Pi 3 | 2 GB | Edge, Testing |
| `config.rpi4.json` | Raspberry Pi 4 | 4 GB | Development |
| `config.rpi5.json` | Raspberry Pi 5 | 8 GB | Production |
| `config.qnap.json` | QNAP NAS | 4+ GB | NAS-Deployment |

**Verwendung:**
```bash
cp config/config.rpi5.json config/config.json
```

---

## Storage-Tuning (RocksDB)

RocksDB ist die Storage-Engine von ThemisDB. Die meisten Performance-Optimierungen betreffen RocksDB-Parameter.

### Memory-Budget

**Grundregel:** `memtable_size_mb + block_cache_size_mb ≈ 60-70% des verfügbaren RAM`

```yaml
storage:
  # Memtable: Write-Buffer im RAM
  memtable_size_mb: 256
  
  # Block Cache: Read-Cache
  block_cache_size_mb: 1024
```

#### Empfehlungen nach RAM-Größe

| RAM | memtable_size_mb | block_cache_size_mb | Use Case |
|-----|------------------|---------------------|----------|
| 2 GB | 64 | 256 | Low-memory devices |
| 4 GB | 128 | 512 | Development, light prod |
| 8 GB | 256 | 1024 | Standard production |
| 16 GB | 512 | 2048 | High-performance |
| 32+ GB | 1024 | 4096+ | Enterprise workloads |

**Trade-offs:**
- **Größerer Memtable**: Weniger Disk-Flushes, bessere Write-Performance, mehr RAM
- **Größerer Block Cache**: Bessere Read-Performance, mehr RAM-Verbrauch

### Write-Performance

#### Concurrent Memtable Writes (v1.3.0)

```yaml
storage:
  # Parallele Writes zu verschiedenen Memtables
  allow_concurrent_memtable_write: true
  
  # Pipelined Writes über mehrere Threads
  enable_pipelined_write: true
```

**Wann aktivieren:**
- ✅ Multi-Core-System (4+ Cores)
- ✅ Write-intensive Workloads
- ✅ Mehrere gleichzeitige Clients

**Performance-Gewinn:** +30-50% Write-Throughput auf 8+ Cores

#### Write Buffer Configuration

```yaml
storage:
  # Anzahl der Memtables pro Column Family
  max_write_buffer_number: 3
  
  # Minimum zum Merge vor Flush
  min_write_buffer_number_to_merge: 1
  
  # Total memtable limit über alle CFs (MB, 0 = unlimited)
  db_write_buffer_size_mb: 0
```

**Tuning-Guidelines:**
- **Write-Heavy**: `max_write_buffer_number: 4-6`
- **Read-Heavy**: `max_write_buffer_number: 2-3`
- **Gemischt**: `max_write_buffer_number: 3` (default)

#### Write Stall Prevention

```yaml
storage:
  # L0 compaction triggers
  level0_file_num_compaction_trigger: 4   # Start compaction
  level0_slowdown_writes_trigger: 20      # Slow writes
  level0_stop_writes_trigger: 36          # Stop writes
```

**Problem:** L0-Files akkumulieren schneller als Compaction mitkommt
**Lösung:** 
- Erhöhe `max_background_jobs`
- Aktiviere `enable_high_parallel_tuning`
- Reduziere Write-Last oder erhöhe `memtable_size_mb`

### Compaction-Tuning

#### Background Jobs

```yaml
storage:
  # Gesamt-Background-Jobs
  max_background_jobs: 4
  
  # Granular control (optional, -1 = auto)
  max_background_compactions: -1
  max_background_flushes: -1
  
  # Sub-compactions (parallele Compaction-Splits)
  max_subcompactions: 1
  
  # Thread pools
  background_threads_high: 2  # Flushes
  background_threads_low: 2   # Compactions
```

**Empfehlungen:**

| CPU Cores | max_background_jobs | max_subcompactions |
|-----------|---------------------|---------------------|
| 2-4 | 2-4 | 1 |
| 4-8 | 4-8 | 2 |
| 8-16 | 8-12 | 2-4 |
| 16+ | 12-16 | 4 |

**Trade-off:** Mehr Jobs = schnellere Compaction, höherer CPU/IO-Verbrauch

#### High-Parallelism Auto-Tuning

```yaml
storage:
  # Automatische Optimierung für High-Concurrency
  enable_high_parallel_tuning: true
  
  # Ab dieser Thread-Zahl aktiviert
  high_parallel_thread_threshold: 16
```

**Was wird automatisch gesetzt:**
- `max_background_jobs = 16`
- `max_subcompactions = 4`
- `two_write_queues = true`
- Optimierte L0-Trigger

**Wann nutzen:**
- ✅ Server mit 16+ Cores
- ✅ High-throughput Workloads
- ❌ Low-memory Devices

#### Compaction Strategy

```yaml
storage:
  # Leveled (default) vs Universal
  use_universal_compaction: false
  
  # Dynamic level sizing
  dynamic_level_bytes: true
  
  # SSTable file sizes
  target_file_size_base_mb: 64
  max_bytes_for_level_base_mb: 256
```

**Leveled Compaction (default):**
- ✅ Bessere Read-Performance
- ✅ Predictable Space Amplification
- ❌ Höherer Write Amplification

**Universal Compaction:**
- ✅ Bessere Write-Performance
- ✅ Einfacher
- ❌ Höherer Space Amplification
- **Use Case:** Write-dominated, append-only Workloads

### Read-Performance

#### Block Cache Optimization

```yaml
storage:
  block_cache_size_mb: 2048
  
  # Cache-Sharding für bessere Concurrency
  block_cache_shard_bits: 6  # 64 shards
  
  # Index/Filter blocks im Cache
  cache_index_and_filter_blocks: true
  
  # L0 Index/Filter pinnen (nicht evicten)
  pin_l0_filter_and_index_blocks_in_cache: true
  
  # Partitionierte Filter (weniger Memory)
  partition_filters: true
  
  # High-priority pool für Index/Filter
  high_pri_pool_ratio: 0.5
```

**Best Practices:**
- Großer Cache (>2 GB): `block_cache_shard_bits: 6` (64 shards)
- Mittlerer Cache (512 MB - 2 GB): `block_cache_shard_bits: 4` (16 shards)
- Kleiner Cache (<512 MB): `block_cache_shard_bits: -1` (auto)

#### Bloom Filters

```yaml
storage:
  # Bits pro Key
  bloom_bits_per_key: 10
```

**False-Positive Rate:**

| bloom_bits_per_key | False Positive Rate | Memory Impact |
|-------------------|---------------------|---------------|
| 8 | ~2% | Low |
| 10 | ~1% | Medium (recommended) |
| 15 | ~0.1% | High |
| 20 | ~0.01% | Very high |

**Tuning:**
- **Point Lookups häufig**: 15-20 bits
- **Range Scans dominieren**: 8-10 bits (default)

#### I/O-Optimierungen

```yaml
storage:
  # Direct I/O (bypasses OS page cache)
  use_direct_reads: false
  use_direct_io_for_flush_and_compaction: false
  
  # Async I/O mit Prefetching (v1.3.0, experimental)
  enable_async_io: false
  async_io_readahead_size_mb: 64
  async_io_multiget_batch_size: 100
  async_io_num_threads: 4
```

**Direct I/O verwenden wenn:**
- ✅ Großer Block Cache (>4 GB)
- ✅ Vermeidung doppelten Cachings
- ❌ Kleiner Block Cache (<1 GB)

### Compression

```yaml
storage:
  compression:
    # Standard für L0-L(n-1)
    default: "lz4"
    
    # Bottommost level (selten gelesen)
    bottommost: "zstd"
```

**Compression-Optionen:**

| Algorithm | Speed | Ratio | CPU | Use Case |
|-----------|-------|-------|-----|----------|
| none | Fastest | 1.0x | Lowest | Hot data, SSD |
| lz4 | Very fast | 2-3x | Low | **Recommended default** |
| snappy | Fast | 2x | Low | Alternative to lz4 |
| zstd | Medium | 3-5x | Medium | **Recommended bottommost** |
| zlib | Slow | 3-4x | High | Legacy |
| bzip2 | Very slow | 4-5x | Very high | Rarely used |

**Best Practice:**
```yaml
compression:
  default: "lz4"      # Schnell, gute Balance
  bottommost: "zstd"  # Bessere Ratio für kalte Daten
```

### BlobDB für große Values

```yaml
storage:
  # BlobDB aktivieren
  enable_blobdb: true
  
  # Threshold (Bytes) - darüber geht zu BlobDB
  blob_size_threshold: 4096  # 4 KB
```

**Wann nutzen:**
- ✅ Viele große Values (>4 KB): JSON documents, Binärdaten
- ✅ Reduziert LSM-Tree Write Amplification
- ❌ Kleine Values (<1 KB): Overhead nicht sinnvoll

**Tuning:**
- **Sehr große Values (>100 KB)**: `blob_size_threshold: 16384` (16 KB)
- **Gemischte Sizes**: `blob_size_threshold: 4096` (default)

### WAL (Write-Ahead Log)

```yaml
storage:
  # WAL aktivieren (Durability)
  enable_wal: true
  
  # Separates WAL-Verzeichnis (optional)
  wal_dir: "/fast-disk/wal"
```

**WAL auf separate SSD legen:**
- ✅ Verbessert Write-Latenz
- ✅ Separiert sequential writes (WAL) von random I/O (Compaction)

**WAL deaktivieren (DANGER!):**
```yaml
storage:
  disable_wal_for_benchmark: true  # NUR für Benchmarks!
```
⚠️ **Datenverlust bei Crash!** Nur für Benchmarks, nie in Production.

---

## Server-Tuning

### Thread-Konfiguration

```yaml
server:
  # Worker threads (0 = auto-detect CPU cores)
  worker_threads: 0
  
  # I/O threads (für async I/O)
  io_threads: 2
  
  # Max concurrent connections
  max_connections: 100
```

**Worker Threads Guidelines:**

| Workload | worker_threads | Begründung |
|----------|----------------|------------|
| Low concurrency | CPU cores | Standard |
| High concurrency | CPU cores × 2 | Mehr Context-Switching OK |
| CPU-bound | CPU cores | Verhindert Over-Subscription |
| I/O-bound | CPU cores × 1.5-2 | Threads blockieren auf I/O |

**Auto-detect (`worker_threads: 0`)** funktioniert gut in den meisten Fällen.

### Connection Limits

```yaml
server:
  max_connections: 100
  
  # Request size limit
  max_request_size_mb: 10
  
  # Request timeout
  request_timeout_ms: 30000
```

**Tuning nach Expected Load:**

| Expected Clients | max_connections | Memory per Connection | Total RAM |
|------------------|-----------------|----------------------|-----------|
| Low (1-10) | 50 | ~1 MB | ~50 MB |
| Medium (10-50) | 100 | ~1 MB | ~100 MB |
| High (50-200) | 200 | ~1 MB | ~200 MB |
| Very High (200+) | 500+ | ~1 MB | ~500 MB+ |

**Trade-off:** Mehr Connections = mehr Memory, mehr File Descriptors

### TLS/SSL Configuration

```yaml
server:
  enable_tls: true
  tls_cert_path: "/etc/themis/certs/server.crt"
  tls_key_path: "/etc/themis/certs/server.key"
  
  # Mutual TLS (optional)
  tls_ca_cert_path: "/etc/themis/certs/ca.crt"
  tls_require_client_cert: true
  
  # TLS version
  tls_min_version: "TLSv1.3"
```

**Security Best Practices:**
- ✅ Verwende TLSv1.3 (schneller, sicherer)
- ✅ Nutze mTLS für kritische Systeme
- ✅ Zertifikate regelmäßig rotieren

**Performance-Impact:** TLS adds ~1-2ms latency per request

### HTTP/2 und WebSocket

```yaml
server:
  # HTTP/2 (requires TLS)
  enable_http2: true
  http2_max_concurrent_streams: 100
  
  # WebSocket
  enable_websocket: true
  websocket_max_message_size: 1048576  # 1 MB
```

**HTTP/2 Vorteile:**
- Multiplexing (mehrere Requests über eine Connection)
- Header compression
- Server push

**Wann nutzen:**
- ✅ Viele kleine Requests
- ✅ Modern clients (Browser, mobile apps)

---

## Vector-Index-Tuning

Vector-Search ist eine der ressourcenintensivsten Features von ThemisDB.

### HNSW Parameter

```yaml
vector_index:
  # Index engine
  engine: "hnsw"
  
  # M: Connections per node
  hnsw_m: 16
  
  # ef_construction: Build-time quality
  hnsw_ef_construction: 200
  
  # ef_search: Search-time quality
  ef_search: 64
  
  # Max vectors
  max_elements: 100000
```

#### M Parameter (Build-Time)

**Definition:** Anzahl der Verbindungen pro Node im HNSW-Graph

| M | Index Size | Build Time | Query Speed | Recall | Use Case |
|---|-----------|-----------|------------|--------|----------|
| 8 | 1x | 1x | Slower | 93-95% | Low memory |
| 16 | 1.5x | 1.5x | Baseline | 95-97% | **Default** |
| 32 | 2.2x | 2.5x | Faster | 97-98% | High quality |
| 64 | 3.5x | 4x | Fastest | 98-99% | Ultra-high quality |

**Empfehlungen:**
- **<100k vectors**: M=16
- **100k-1M vectors**: M=32
- **>1M vectors**: M=48-64 + Quantization

**⚠️ M kann nur zur Build-Time gesetzt werden!** Index-Rebuild nötig bei Änderung.

#### ef_construction Parameter (Build-Time)

**Definition:** Größe der Kandidatenliste beim Index-Aufbau

| ef_construction | Build Time | Index Quality | Typical Use |
|-----------------|-----------|---------------|-------------|
| 100 | 1x | Good | Fast builds |
| 200 | 2x | Very good | **Default** |
| 400 | 4x | Excellent | High-quality |
| 800 | 8x | Near-perfect | Research |

**Empfehlung:** `ef_construction: 200` (default) für die meisten Fälle

#### ef_search Parameter (Runtime)

**Definition:** Größe der Kandidatenliste bei Suche

| ef_search | Latency | Recall@10 | Use Case |
|-----------|---------|-----------|----------|
| 20 | 1-2 ms | ~85% | Real-time, speed-critical |
| 50 | 3-5 ms | ~95% | **Default, balanced** |
| 100 | 8-12 ms | ~98% | High-precision |
| 200 | 20-30 ms | ~99.5% | Offline batch processing |

**✅ ef_search kann zur Laufzeit per API geändert werden!**

```bash
# Via API
POST /vector/index/config
{
  "ef_search": 100
}
```

**Best Practice:**
- Development/Testing: `ef_search: 50`
- Production (latency-critical): `ef_search: 30-40`
- Production (quality-critical): `ef_search: 80-120`
- Offline Analytics: `ef_search: 150-200`

### GPU-Acceleration

```yaml
vector_index:
  use_gpu: true
```

**Requirements:**
- NVIDIA GPU mit CUDA
- FAISS mit GPU-Support kompiliert

**Performance-Gewinn:**
- **Kleine Batches (1-10 queries)**: 2-3x Speedup
- **Große Batches (100+ queries)**: 10-20x Speedup

**Memory:**
- GPU VRAM ≥ Index Size in RAM
- Für 1M vectors (768-dim, M=32): ~4-6 GB VRAM

### Persistence

```yaml
vector_index:
  auto_save: true
  save_path: "./data/vector_indexes"
  save_on_shutdown: true
  load_on_startup: true
```

**Best Practices:**
- ✅ `auto_save: true` in Production
- ✅ `save_path` auf schneller SSD
- ⚠️ Große Indizes (>1M vectors) können 30-60 Sekunden zum Laden brauchen

---

## Memory-Management

### Memory Budget Planung

**Gesamt-RAM-Verbrauch:**
```
Total RAM = RocksDB Memory + Vector Index Memory + Server Overhead + OS
```

**Komponenten:**

1. **RocksDB Memory:**
   ```
   RocksDB = memtable_size_mb × CFs + block_cache_size_mb
   ```

2. **Vector Index Memory:**
   ```
   HNSW ≈ vectors × dimension × 4 bytes × (1 + M/10)
   Beispiel: 1M vectors, 768-dim, M=32
   = 1M × 768 × 4 × (1 + 3.2) ≈ 12.9 GB
   ```

3. **Server Overhead:**
   ```
   Server ≈ worker_threads × 8 MB + connections × 1 MB
   Beispiel: 8 threads, 100 connections = 64 + 100 = 164 MB
   ```

4. **OS Reserve:**
   ```
   OS ≈ 10-20% of total RAM
   ```

### Beispiel-Rechnung (8 GB RAM)

```
Verfügbar für ThemisDB: 8 GB × 0.8 = 6.4 GB

Allokation:
- RocksDB memtable: 256 MB
- RocksDB block cache: 1024 MB (1 GB)
- Vector Index: 100k vectors, 768-dim, M=16 ≈ 500 MB
- Server: 8 threads, 100 conns ≈ 164 MB
- Buffer: 500 MB

Total: 256 + 1024 + 500 + 164 + 500 = 2.44 GB ✅
```

### Low-Memory Optimizations

**Für Geräte mit <4 GB RAM:**

```yaml
storage:
  memtable_size_mb: 64
  block_cache_size_mb: 256
  max_write_buffer_number: 2
  max_background_jobs: 2

server:
  worker_threads: 4
  max_connections: 50

vector_index:
  hnsw_m: 12
  max_elements: 50000
```

**Features deaktivieren:**
```yaml
features:
  semantic_cache: false
  llm_store: false
  cdc: false
  timeseries: false

tracing:
  enabled: false
```

---

## I/O-Optimierung

### SSD vs. HDD

**Empfohlen:** SSD für RocksDB data + WAL

**Performance-Vergleich:**

| Storage | Random Read IOPS | Random Write IOPS | Latency |
|---------|------------------|-------------------|---------|
| HDD | 100-200 | 100-200 | 10-20 ms |
| SATA SSD | 10k-80k | 10k-80k | 0.1-0.5 ms |
| NVMe SSD | 100k-1M | 50k-500k | 0.01-0.1 ms |

**Optimierung:**
1. **Data auf SSD/NVMe**
2. **WAL auf separate SSD** (für sehr hohe Write-Last)
3. **Logs/Temp auf HDD**

### Filesystem-Tuning

**Für ext4 auf SSD:**
```bash
# Mount options
sudo mount -o remount,noatime,nodiratime,commit=60 /data
```

**In /etc/fstab:**
```
/dev/sdb1 /data ext4 defaults,noatime,nodiratime,commit=60 0 2
```

**I/O Scheduler:**
```bash
# Für SSD: none oder mq-deadline
echo none | sudo tee /sys/block/sdb/queue/scheduler

# Für NVMe: none (default)
```

---

## Feature-Konfiguration

### Semantic Cache

```yaml
features:
  semantic_cache: true
```

**Was es tut:** Cached query results basierend auf semantischer Ähnlichkeit

**Wann aktivieren:**
- ✅ Wiederholte ähnliche Queries
- ✅ Teuer zu berechnende Ergebnisse
- ❌ Sehr dynamische Daten

**Memory-Impact:** ~100-500 MB (abhängig von Cache-Größe)

### LLM Store

```yaml
features:
  llm_store: true
```

**Was es tut:** Speichert LLM-Embeddings und Model-Metadaten

**Requirements:**
- LLM-Integration aktiviert (siehe `config/llm_config.example.yaml`)
- Zusätzliche ~1-2 GB RAM für Models

### CDC (Change Data Capture)

```yaml
features:
  cdc: true

sse:
  max_events_per_second: 0  # unlimited
```

**Was es tut:** Real-time change streams über SSE

**Wann aktivieren:**
- ✅ Real-time Replikation
- ✅ Event-driven Architekturen
- ❌ High-frequency Updates (kann zu Overhead führen)

### Retention Policies

```yaml
features:
  retention:
    enabled: true
    interval_hours: 24
    policies_path: "./config/retention_policies.yaml"
```

**Was es tut:** Automatisches Löschen alter Daten basierend auf Policies

**Example Policy (`retention_policies.yaml`):**
```yaml
policies:
  - name: "delete_old_sessions"
    collection: "sessions"
    retention_days: 30
    
  - name: "delete_old_logs"
    collection: "audit_logs"
    retention_days: 90
```

---

## Use-Case-spezifisches Tuning

### Use Case 1: Write-Heavy Workload

**Beispiel:** Logging, IoT sensor data, event streaming

**Optimierungen:**
```yaml
storage:
  memtable_size_mb: 512
  max_write_buffer_number: 4
  max_background_jobs: 12
  enable_pipelined_write: true
  allow_concurrent_memtable_write: true
  use_universal_compaction: true
  enable_blobdb: true
  compression:
    default: "lz4"  # Schnell
    bottommost: "zstd"

server:
  worker_threads: 0  # auto

vector_index:
  # Wenn nicht benötigt, nicht initialisieren
  object_name: ""
```

**Key Metrics:**
- Write throughput: 50k-100k writes/sec (depends on hardware)
- Latency p99: <10 ms

### Use Case 2: Read-Heavy Workload

**Beispiel:** Analytics, reporting, serving

**Optimierungen:**
```yaml
storage:
  memtable_size_mb: 256
  block_cache_size_mb: 4096
  cache_index_and_filter_blocks: true
  pin_l0_filter_and_index_blocks_in_cache: true
  bloom_bits_per_key: 15
  max_background_jobs: 4
  compression:
    default: "lz4"
    bottommost: "zstd"

server:
  worker_threads: 0
  max_connections: 200

vector_index:
  hnsw_m: 32
  ef_search: 100
```

**Key Metrics:**
- Read throughput: 100k-500k reads/sec
- Latency p99: <5 ms

### Use Case 3: Vector Search (AI/ML)

**Beispiel:** Semantic search, recommendation systems, RAG

**Optimierungen:**
```yaml
storage:
  memtable_size_mb: 256
  block_cache_size_mb: 2048

server:
  worker_threads: 0
  max_connections: 100

vector_index:
  dimension: 768
  hnsw_m: 32
  hnsw_ef_construction: 400
  ef_search: 100
  use_gpu: true
  max_elements: 1000000

features:
  semantic_cache: true
  llm_store: true
```

**Key Metrics:**
- Vector search latency: 5-20 ms (depends on ef_search)
- Recall@10: >97%

### Use Case 4: Mixed Workload (Balanced)

**Beispiel:** General-purpose database, web applications

**Optimierungen:**
```yaml
storage:
  memtable_size_mb: 256
  block_cache_size_mb: 1024
  max_background_jobs: 8
  enable_pipelined_write: true
  compression:
    default: "lz4"
    bottommost: "zstd"

server:
  worker_threads: 0
  max_connections: 100

vector_index:
  hnsw_m: 16
  ef_search: 64
```

**Key Metrics:**
- Balanced throughput: 30k writes/sec, 100k reads/sec
- Latency p99: <10 ms

---

## Hardware-spezifische Konfigurationen

### Raspberry Pi 3 (2 GB RAM)

```yaml
storage:
  memtable_size_mb: 64
  block_cache_size_mb: 256
  max_background_jobs: 2

server:
  worker_threads: 4
  max_connections: 50

vector_index:
  hnsw_m: 12
  max_elements: 50000
```

**Siehe:** `config/config.rpi3.json`

### Raspberry Pi 4 (4 GB RAM)

```yaml
storage:
  memtable_size_mb: 128
  block_cache_size_mb: 512
  max_background_jobs: 4

server:
  worker_threads: 4
  max_connections: 100

vector_index:
  hnsw_m: 16
  max_elements: 100000
```

**Siehe:** `config/config.rpi4.json`

### Raspberry Pi 5 (8 GB RAM)

```yaml
storage:
  memtable_size_mb: 256
  block_cache_size_mb: 1024
  max_background_jobs: 4

server:
  worker_threads: 4
  max_connections: 200

vector_index:
  hnsw_m: 20
  max_elements: 200000
```

**Siehe:** `config/config.rpi5.json`

### High-End Server (32+ GB RAM)

```yaml
storage:
  memtable_size_mb: 1024
  block_cache_size_mb: 8192
  max_background_jobs: 16
  enable_high_parallel_tuning: true
  max_subcompactions: 4

server:
  worker_threads: 0  # auto (all cores)
  max_connections: 500

vector_index:
  hnsw_m: 64
  hnsw_ef_construction: 800
  ef_search: 200
  use_gpu: true
  max_elements: 10000000
```

---

## Monitoring und Troubleshooting

### Key Metrics

**RocksDB Stats:**
```bash
curl http://localhost:8765/stats
```

**Wichtige Metriken:**
- `rocksdb.compaction.pending` - Pending compactions
- `rocksdb.memtable.size` - Current memtable size
- `rocksdb.block.cache.hit` - Cache hit rate
- `rocksdb.level0.num.files` - L0 file count

**Vector Index Stats:**
```bash
curl http://localhost:8765/vector/stats
```

**Prometheus Metrics:**
```yaml
tracing:
  enabled: true
  otlp_endpoint: "http://prometheus:4318"
```

### Performance Issues

#### Problem: Hohe Write-Latenz

**Symptoms:**
- Write p99 > 50 ms
- `rocksdb.level0.num.files` > 20

**Lösungen:**
1. Erhöhe `max_background_jobs`
2. Aktiviere `enable_pipelined_write`
3. Vergrößere `memtable_size_mb`
4. WAL auf separate SSD

#### Problem: Hohe Read-Latenz

**Symptoms:**
- Read p99 > 20 ms
- Low `rocksdb.block.cache.hit` rate (<80%)

**Lösungen:**
1. Vergrößere `block_cache_size_mb`
2. Erhöhe `bloom_bits_per_key` auf 15
3. Aktiviere `cache_index_and_filter_blocks`
4. Verwende SSD statt HDD

#### Problem: Hoher Memory-Verbrauch

**Symptoms:**
- OOM kills
- Swap usage > 0

**Lösungen:**
1. Reduziere `memtable_size_mb`
2. Reduziere `block_cache_size_mb`
3. Reduziere `max_connections`
4. Limitiere `vector_index.max_elements`

#### Problem: Write Stalls

**Symptoms:**
- Writes blockieren
- Logs: "Stopping writes"

**Lösungen:**
1. Erhöhe `level0_stop_writes_trigger`
2. Erhöhe `max_background_jobs`
3. Verwende schnellere Storage
4. Reduziere Write-Rate

---

## Checkliste für Production

### Essential (Must-Have)

- [ ] Config-File erstellt und getestet
- [ ] Memory-Budget geplant (RAM nicht überschritten)
- [ ] Storage auf SSD (nicht HDD)
- [ ] WAL aktiviert (`enable_wal: true`)
- [ ] TLS aktiviert für externe Zugriffe
- [ ] Backup-Strategie definiert
- [ ] Monitoring aktiviert (Prometheus/Grafana)
- [ ] Logging konfiguriert und rotiert

### Recommended

- [ ] `enable_pipelined_write: true` (8+ cores)
- [ ] `enable_blobdb: true` (large values)
- [ ] Bloom filters optimiert (`bloom_bits_per_key: 10-15`)
- [ ] Block cache optimiert (60-70% RAM)
- [ ] Background jobs tuned für CPU cores
- [ ] Retention policies definiert
- [ ] Health checks implementiert
- [ ] Alerts konfiguriert

### Advanced

- [ ] High-parallel tuning aktiviert (16+ cores)
- [ ] WAL auf separate SSD
- [ ] Direct I/O getestet (large cache)
- [ ] Vector index GPU-accelerated
- [ ] HTTP/2 aktiviert
- [ ] mTLS für kritische Systeme
- [ ] Multi-path storage (mehrere NVMe)

---

## Referenzen

- **RocksDB Tuning Guide:** https://github.com/facebook/rocksdb/wiki/RocksDB-Tuning-Guide
- **HNSW Parameter Guide:** Malkov & Yashunin (2018)
- **ThemisDB Docs:** `docs/de/guides/`
- **ARM/Raspberry Pi Tuning:** `docs/de/deployment/deployment_raspberry_tuning.md`
- **Vector Search Tuning:** `docs/de/search/performance_tuning.md`

---

## Support

Bei Fragen oder Performance-Problemen:

1. Collect metrics: `curl http://localhost:8765/stats`
2. Check logs: `tail -f logs/themis_server.log`
3. Run benchmarks: `./scripts/run-arm-benchmarks.sh` (ARM) oder `./build/bench_*`
4. Öffnen Sie ein Issue: https://github.com/makr-code/ThemisDB/issues

**Happy Tuning! 🚀**
