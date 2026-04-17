[docs](../../README.md) > [de](../README.md) > [reports](./README.md) > [HARDWARE_ANFORDERUNGEN](./HARDWARE_ANFORDERUNGEN.md)
**Datum:** 2026-04-17
**Status:** stable
**Primary (Quelle der Wahrheit):**
- `src/server/`
- `include/server/`
- `src/storage/`
- `include/storage/`
- `src/llm/`
- `include/llm/`
- `src/sharding/`
- `include/sharding/`
- `src/index/`
- `include/index/`
- `src/cache/`
- `include/cache/`
- `src/acceleration/`
- `src/rag/`

**Bezug / Reference:**
- `benchmarks/docs/HARDWARE_CONSTRAINTS_README.md`
- `docs/de/reports/VARIANT_STRATEGY_v1.1.0.md`
- `docs/de/reports/ENTERPRISE_FEATURES_STRATEGY.md`
- `include/server/ROADMAP.md`
- `include/llm/ROADMAP.md`
- `include/sharding/ROADMAP.md`
- `include/storage/ROADMAP.md`
- `include/index/ROADMAP.md`
- `include/cache/ROADMAP.md`

---

# ThemisDB – Hardwareanforderungsanalyse

## Theoretische CPU / Cores / Threads / GPU / VRAM / RAM / Cache / NVMe / HDD-Belastung im Serverbetrieb mit vielen anfragenden Instanzen

**Version:** 1.0
**Erstellt:** 2026-04-17
**Scope:** ThemisDB v1.8+ im Serverbetrieb – Einzelknoten bis horizontales Cluster
**Skalierungsszenarien:** 100 → 1.000 → 10.000 → 100.000 → 10.000.000 gleichzeitig anfragende Instanzen

---

## Inhaltsverzeichnis

1. [Executive Summary](#1-executive-summary)
2. [Modulares Ressourcenprofil](#2-modulares-ressourcenprofil)
3. [Detailanalyse je Ressourcentyp](#3-detailanalyse-je-ressourcentyp)
   - 3.1 [CPU / Cores / Threads](#31-cpu--cores--threads)
   - 3.2 [GPU / CUDA / Compute](#32-gpu--cuda--compute)
   - 3.3 [VRAM](#33-vram)
   - 3.4 [RAM (Systemspeicher)](#34-ram-systemspeicher)
   - 3.5 [CPU-Cache (L1 / L2 / L3)](#35-cpu-cache-l1--l2--l3)
   - 3.6 [NVMe-SSD](#36-nvme-ssd)
   - 3.7 [HDD](#37-hdd)
4. [Skalierungsszenarien nach Instanzanzahl](#4-skalierungsszenarien-nach-instanzanzahl)
   - 4.1 [Szenario S-1: 100 Instanzen (Dev / Embedded)](#41-szenario-s-1-100-instanzen-dev--embedded)
   - 4.2 [Szenario S-2: 1.000 Instanzen (SME / Edge)](#42-szenario-s-2-1000-instanzen-sme--edge)
   - 4.3 [Szenario S-3: 10.000 Instanzen (Mid-Scale Production)](#43-szenario-s-3-10000-instanzen-mid-scale-production)
   - 4.4 [Szenario S-4: 100.000 Instanzen (High-Scale Enterprise)](#44-szenario-s-4-100000-instanzen-high-scale-enterprise)
   - 4.5 [Szenario S-5: 10.000.000+ Instanzen (Hyperscale)](#45-szenario-s-5-10000000-instanzen-hyperscale)
5. [Hardwareanforderungsmatrix (Min / Opt / Max)](#5-hardwareanforderungsmatrix-min--opt--max)
6. [Konfigurationsempfehlungen je Tier](#6-konfigurationsempfehlungen-je-tier)
7. [Cluster & Horizontal Scaling](#7-cluster--horizontal-scaling)
8. [Engpass-Identifikation & Faustregeln](#8-engpass-identifikation--faustregeln)
9. [Fazit](#9-fazit)

---

## 1. Executive Summary

ThemisDB ist eine modulare Hybrid-Datenbank mit polyglotter Persistenz (Dokument, Key-Value,
Graph, Vector, Timeseries, Geo) und optionaler eingebetteter LLM-Inference (llama.cpp, LoRA,
RAG). Der Ressourcenbedarf skaliert entlang zweier orthogonaler Dimensionen:

- **Datenmenge** – bestimmt I/O, RAM-Cache-Größe, Compaction-Last.
- **Anfragelast (Concurrent Clients)** – bestimmt CPU-Thread-Bedarf, Connection-Stack-RAM,
  Lock-Contention, GPU-KV-Cache-Druck (bei LLM-Betrieb).

Die wichtigsten Erkenntnisse vorab:

| Ressource | Dominanter Verbraucher | Kritischer Engpass ab |
|-----------|------------------------|----------------------|
| CPU Cores | Query-Engine (TBB), Compaction (RocksDB), Sharding-Gossip | 10.000 concurrent requests ohne Skalierung |
| RAM | RocksDB Block-Cache, MVCC-Versionsketten, HNSW-Index | > 1 TB Datensatz auf Einzelknoten |
| VRAM | LLM-Inference (llama.cpp), GPU-Vector-Search (CUDA HNSW) | Jede Modellinstanz > 2 GB VRAM |
| NVMe | RocksDB LSM-Compaction, WAL, Blob-Storage | Write-Heavy > 100.000 ops/s |
| L3-Cache | RocksDB Block-Cache Hot-Path, HNSW-Traversal | Hot-Dataset > L3-Kapazität |
| Threads | HTTP-Accept-Pool, gRPC, TBB-Worker, Compaction-BG | OS-Thread-Limit (default 32k) ab 50.000+ |
| HDD | Archiv, Cold-Tier Blob-Storage | Nur für Cold-Data geeignet |

**Grundregel:** ThemisDB ist primär **CPU- und RAM-gebunden**. GPU und NVMe sind
sekundäre Engpässe, die je nach aktivierten Modulen (LLM, GPU-Acceleration, Streaming-Ingest)
dominant werden können.

---

## 2. Modulares Ressourcenprofil

ThemisDB besteht aus ~46 Modulen. Die folgende Tabelle ordnet jedem Kernmodul sein
Ressourcenprofil nach Hauptlast zu.

| Modul | CPU | RAM | GPU/VRAM | NVMe I/O | Threads |
|-------|-----|-----|----------|----------|---------|
| **Core / Storage (RocksDB MVCC)** | ★★★★ | ★★★★★ | – | ★★★★★ | ★★★ |
| **AQL / Query-Engine (TBB)** | ★★★★★ | ★★★ | – | ★★ | ★★★★★ |
| **Vector Index (HNSW / FAISS)** | ★★★ | ★★★★★ | ★★★★ | ★★ | ★★★ |
| **Graph Index** | ★★★ | ★★★★ | – | ★ | ★★★ |
| **Full-Text Search (BM25)** | ★★★★ | ★★★ | – | ★★★ | ★★★ |
| **LLM Inference (llama.cpp)** | ★★★★ | ★★★ | ★★★★★ | ★ | ★★★ |
| **LoRA / RAG Orchestration** | ★★★ | ★★★ | ★★★★ | ★★ | ★★★ |
| **Sharding / Gossip / Raft** | ★★★ | ★★ | – | ★★ | ★★★★ |
| **Replication / WAL Streaming** | ★★ | ★★ | – | ★★★★ | ★★★ |
| **Transaction (MVCC / SAGA)** | ★★★★ | ★★★★ | – | ★★★ | ★★★★ |
| **Cache (L1-L3 AdaptiveCache)** | ★★ | ★★★★★ | – | – | ★★ |
| **Server (HTTP/2, gRPC, WS, MQTT)** | ★★★ | ★★★ | – | – | ★★★★★ |
| **Security (RBAC, HSM, Encryption)** | ★★★ | ★★ | – | ★ | ★★ |
| **Observability (OTEL, Prometheus)** | ★★ | ★★ | – | ★★ | ★★ |
| **CDC / Changefeed** | ★★ | ★★ | – | ★★★ | ★★★ |
| **Stable Diffusion / Image AI** | ★★ | ★★ | ★★★★★ | ★ | ★★ |
| **Timeseries (Gorilla Codec)** | ★★★ | ★★★ | – | ★★★ | ★★ |
| **Geo / Spatial** | ★★★ | ★★★ | ★★ (cuSpatial) | ★★ | ★★ |

★ = gering · ★★ = moderat · ★★★ = deutlich · ★★★★ = hoch · ★★★★★ = dominanter Engpass

---

## 3. Detailanalyse je Ressourcentyp

### 3.1 CPU / Cores / Threads

#### Nutzungsmuster

ThemisDB nutzt Intel TBB für parallele Query-Ausführung und hat einen dedizierten
HTTP-Accept-Thread-Pool (Konfiguration: `server.worker_threads`). Die Thread-Allokation
gliedert sich wie folgt:

```
┌──────────────────────────────────────────────────────────┐
│ CPU-Thread-Verteilung (Beispiel: 64 Cores)               │
├──────────────────────────────────────────────────────────┤
│ HTTP Accept / Dispatch:       2 – 4 Threads (dediziert)  │
│ gRPC-Worker:                  8 – 16 Threads (TBB-Pool)  │
│ WebSocket / MQTT:             4 – 8 Threads              │
│ TBB Query-Worker:             20 – 32 Threads            │
│ RocksDB Background:           6 – 12 Threads (Compaction)│
│ WAL-Flush / Replication:      2 – 4 Threads              │
│ Gossip / Raft / Paxos:        2 – 4 Threads              │
│ LLM-Inference (CPU-only):     4 – 8 Threads (optional)   │
│ OTEL / Logging / CDC:         2 – 4 Threads              │
│ OS Reserve:                   2 – 4 Threads              │
└──────────────────────────────────────────────────────────┘
```

#### Skalierungsverhalten

- **Linear bis ~8 Cores** für einfache Point-Lookups (Amdahl-Gesetz, p ≈ 0.97).
- **Sub-linear ab 32 Cores** wegen NUMA-Boundary-Crossing, Lock-Contention in MVCC und
  RocksDB Write-Path.
- **Hyperthreading (SMT):** Bringt 15–30 % Mehrwert bei Mixed Read/Write-Workloads. Bei
  reinen Compaction-Heavy-Szenarien kaum Gewinn (compute-bound Kernel-Paths).
- **NUMA:** Auf Dual-Socket-Systemen NUMA-aware Thread-Binding empfohlen
  (`numactl --cpunodebind=0 --membind=0 themisdb`). Andernfalls +20–40 % Latenz durch
  Remote-Memory-Zugriffe.

#### Faustregel

```
Cores_benötigt ≈ (concurrent_requests / throughput_per_core) + compaction_overhead
```

Wobei `throughput_per_core` (typisch):
- Simple Key-Value Read: ~200.000 ops/s/Core
- AQL Filter-Scan: ~20.000 ops/s/Core
- HNSW Vector Search (k=10): ~5.000 qps/Core
- LLM-Token-Generierung (CPU): ~20 tokens/s/Core (7B-Modell, Q4-Quantisierung)

---

### 3.2 GPU / CUDA / Compute

#### Nutzungsmuster

Die GPU ist in ThemisDB **optional**, aber für folgende Module hochrelevant:

| Modul | GPU-Auslastung | CUDA-Kernel | Fallback |
|-------|---------------|-------------|---------|
| LLM Inference (llama.cpp, CUDA) | 85–99 % (Inference-Zeit) | Flash-Attention, Kernel Fusion | CPU (10–50× langsamer) |
| Vector Search GPU (CUDA HNSW) | 60–90 % (Batch) | HNSW CUDA, cuBLAS | CPU-HNSW |
| GPU Erasure Coding | 30–60 % | CUDA Reed-Solomon | CPU |
| Stable Diffusion | 95–99 % | Diffusion Steps | CPU (nicht praktikabel) |
| GPU Compression | 20–40 % | zstd/CUDA | CPU compressor |
| cuSpatial (Geo) | 50–80 % | Geo-Kernels | GEOS/PROJ |

#### Parallelitätsfaktor

Die GPU serialisiert bei Multi-Tenant-LLM-Inference durch den KV-Cache. ThemisDB
implementiert Paged-Attention (vLLM-style): Jede Anfrage bekommt logische Seiten,
physikalisch zusammengelegt. Trotzdem gilt:

```
GPU_Engpass = ceil(batch_active_tokens / tokens_per_ms_per_GPU)
```

Beispiel RTX 4090 (24 GB, ~82 TFLOPS BF16):
- Mistral-7B Q4: ~180 tokens/s → 5,6 ms/Token
- Bei 1.000 concurrent Streams: Warteschlange wird dominant → Latenz steigt exponentiell
- Lösung: Continuous Batching (bereits implementiert), Horizontal GPU Scaling

---

### 3.3 VRAM

VRAM ist der **härteste Engpass** bei LLM-Betrieb. Die Aufteilung:

```
VRAM_total = VRAM_model_weights + VRAM_KV_cache + VRAM_activations + VRAM_ThemisDB_overhead
```

#### Modellgewichte (nicht skalierend mit Anfragen)

| Modell | Präzision | VRAM (Gewichte) |
|--------|-----------|-----------------|
| Phi-3-Mini 3.8B | Q4_K_M | ~2,5 GB |
| Llama-3-8B | Q4_K_M | ~4,5 GB |
| Mistral-7B | Q4_K_M | ~4,0 GB |
| Llama-3-70B | Q4_K_M | ~38 GB |
| Llama-3-70B | FP16 | ~140 GB |

#### KV-Cache (skaliert linear mit concurrent Requests)

```
KV_cache_pro_request = 2 * num_layers * num_heads * head_dim * max_seq_len * bytes_per_element
```

Beispiel Mistral-7B (FP16, seq_len=4096):
```
= 2 × 32 × 32 × 128 × 4096 × 2 bytes ≈ 2,1 GB pro Anfrage (full KV)
```

Mit Paged-Attention und durchschnittlich 50 % Auslastung:
```
KV_cache_effektiv ≈ 1,05 GB pro concurrent Anfrage
```

#### VRAM-Bedarf nach concurrent LLM-Anfragen (Mistral-7B)

| Concurrent Requests | VRAM Gewichte | VRAM KV-Cache | Gesamt |
|--------------------|---------------|---------------|--------|
| 1 | 4 GB | ~1 GB | ~5 GB |
| 4 | 4 GB | ~4 GB | ~8 GB |
| 8 | 4 GB | ~8 GB | ~12 GB |
| 16 | 4 GB | ~16 GB | ~20 GB |
| 24 (RTX 4090 max) | 4 GB | ~20 GB | ~24 GB |
| 32+ → Warteschlange | 4 GB | – | 24 GB (Limit) |

**Für > 24 concurrent LLM-Sessions:** Multi-GPU (Tensor Parallel) oder
horizontales Scaling (mehrere ThemisDB-Instanzen) erforderlich.

---

### 3.4 RAM (Systemspeicher)

#### Hauptverbraucher

```
RAM_total ≈ RocksDB_BlockCache + MVCC_Working_Set + HNSW_Index_RAM
           + Connection_Stack_RAM + OS_Page_Cache + LLM_CPU_Buffer
           + TBB_Task_Arenas + Shard_Metadata
```

##### RocksDB Block-Cache

Empfehlung: **60–70 % des verfügbaren RAM** für den Block-Cache (LRU).

```
block_cache_bytes = total_ram * 0.65

Beispiel: 64 GB RAM → 41,6 GB Block-Cache
          256 GB RAM → 166 GB Block-Cache
```

Ein wachsender Block-Cache reduziert I/O-Reads linear bis zum Cache-Sättigungspunkt.
Empirisch: Bei 256 GB RAM und < 400 GB Hot-Dataset → > 95 % Cache-Hit-Rate erreichbar.

##### MVCC-Versionsketten (Transaktionslast)

Jede offene Transaktion hält eine MVCC-Snapshot-ID. Bei langer Laufzeit akkumulieren
sich veraltete Versionen:

```
MVCC_overhead ≈ concurrent_txns * avg_modified_keys * version_entry_size
```

Faustregel: ~512 Byte pro Key pro Version. Bei 10.000 concurrent Transaktionen mit
durchschnittlich 100 Keys: **~512 MB** MVCC-Overhead (vertretbar).

##### HNSW Vector Index

```
HNSW_RAM ≈ num_vectors * (dim * 4 bytes + 2 * M * 4 bytes * avg_level_factor)
```

Beispiel: 100 Mio. Vektoren, dim=1024, M=16:
```
≈ 100M * (4096 + 128) Bytes ≈ 420 GB RAM
```

→ Große Vector-Indizes **erzwingen** horizontales Sharding.

##### Connection-Stack-RAM

Jede TCP-Verbindung reserviert Kernel-Puffer + Anwendungs-Read/Write-Buffer:

```
connection_stack ≈ concurrent_connections * (recv_buf + send_buf + TLS_context + app_state)
                ≈ concurrent_connections * 128 KB (default)
```

| Concurrent Connections | Stack-RAM |
|-----------------------|-----------|
| 1.000 | ~128 MB |
| 10.000 | ~1,3 GB |
| 100.000 | ~12,8 GB |
| 1.000.000 | ~128 GB |

→ Ab 100.000 Connections: `SO_RCVBUF`/`SO_SNDBUF` auf 32–64 KB reduzieren,
`net.core.somaxconn` und `net.ipv4.tcp_max_syn_backlog` erhöhen.

---

### 3.5 CPU-Cache (L1 / L2 / L3)

Der CPU-Cache ist der unsichtbare Performance-Multiplikator. ThemisDB nutzt
TBB und RocksDB, beide cache-aware:

| Cache-Ebene | Typische Größe | Latenz | ThemisDB-Nutzung |
|-------------|---------------|--------|-----------------|
| L1 | 32–64 KB/Core | ~4 Zyklen | Loop-Invarianten, TBB Task-Metadaten |
| L2 | 256 KB–1 MB/Core | ~12 Zyklen | RocksDB Block-Index, Query-Cursor |
| L3 | 16–192 MB (geteilt) | ~40 Zyklen | RocksDB Hot-Blocks, HNSW Neighbor-Lists |
| RAM | 32–512 GB | ~200 Zyklen | Block-Cache, Index-Seiten |

#### L3-Druck-Analyse

RocksDB BlockBasedTable liest 4–16 KB Blöcke. Der L3-Cache fasst bei 32 MB:
```
L3_capacity_blocks = 32 MB / 4 KB = 8.192 Blöcke ≈ ~33 MB Hot-Dataset in L3
```

Bei intensiver Concurrent-Query-Last mit verschiedenen Schlüsselbereichen steigt
die L3-Miss-Rate:
- **< 1.000 concurrent Requests:** L3 ausreichend für hot dataset (< 30 MB)
- **1.000–10.000 Requests:** L3 teilweise gesättigt; Block-Cache im RAM übernimmt
- **> 10.000 Requests:** L3-Miss-Rate > 30 %; RAM-Bandwidth wird kritisch
  (~50–100 GB/s DDR5 Ceiling)

**Empfehlung:** Für hochfrequente AQL-Scans: RocksDB `prefix_extractor` + Bloom-Filter
aktivieren → reduziert unnötige L3-Zugriffe drastisch.

---

### 3.6 NVMe-SSD

#### Hauptlast

```
NVMe_I/O = WAL_writes + Compaction_reads_writes + Blob_reads + Index_reads
```

##### WAL (Write-Ahead Log)

- Jede Transaktion schreibt synchron in den WAL (fsync oder group-commit).
- Typisch: 512 Byte – 16 KB pro Transaktion.
- Bei 100.000 Writes/s: **~1,6 GB/s Sequenzial-Write** (ohne Kompression).

##### RocksDB Compaction

Compaction ist der **dominanteste NVMe-Verbraucher**:
```
Write Amplification ≈ 10–30× bei Standard-Leveled-Compaction
```

D.h.: 1 GB/s Anwendungs-Writes → 10–30 GB/s Compaction-I/O.

| Write-Rate | Compaction I/O (Leveled) | Erforderlich |
|------------|--------------------------|-------------|
| 10 MB/s | 100–300 MB/s | SATA SSD (550 MB/s) |
| 100 MB/s | 1–3 GB/s | NVMe Gen3 (3,5 GB/s) |
| 1 GB/s | 10–30 GB/s | NVMe Gen4 RAID-0 oder Gen5 |
| 10 GB/s | > 100 GB/s | NVMe Array + Storage-Offload |

**Empfehlung:** `compaction_style = kUniversal` bei write-heavy Workloads →
Write Amplification reduziert auf 4–8×.

##### NVMe Manager (io_uring)

ThemisDB implementiert `nvme_manager.h` mit io_uring für asynchrones I/O:
- Reduziert Context-Switch-Overhead bei High-IOPS-Szenarien um 15–30 %.
- Aktiviert durch Build-Flag `-DTHEMIS_ENABLE_IO_URING=ON`.

#### IOPS-Anforderungen

| Szenario | Reads IOPS | Writes IOPS | Empfohlenes Medium |
|----------|-----------|------------|-------------------|
| 100 Requests/s | 5K–10K | 1K–5K | SATA SSD |
| 10.000 Requests/s | 200K–500K | 50K–200K | NVMe Gen3/4 |
| 100.000 Requests/s | 1M–5M | 500K–2M | NVMe Gen4 Array / optane |
| 1.000.000 Requests/s | > 10M | > 5M | Distributed Storage / Horizontal Scale |

---

### 3.7 HDD

HDDs sind für ThemisDB-Produktivbetrieb **nicht geeignet** als primärer Storage:

| Eigenschaft | HDD (7.200 RPM) | NVMe Gen4 | Faktor |
|-------------|-----------------|-----------|--------|
| Sequenziell Read | 150 MB/s | 7.000 MB/s | 46× |
| Random Read IOPS | 150 | 1.000.000 | 6.700× |
| Latenz | 5–10 ms | 0,02–0,1 ms | 50–500× |
| Compaction Eignung | ❌ Blocker | ✅ Optimal | – |

**Sinnvoller HDD-Einsatz in ThemisDB:**

- **Cold-Tier Blob-Storage:** Große Binärdaten (Videos, Backups, Archive) die
  seltener als 1×/Tag gelesen werden.
- **Offline-Backups:** Inkrementelle RocksDB-Backups auf HDD-Array (kostengünstig).
- **WAL-Archivierung:** Ältere WAL-Segmente auf HDD verschoben.

Konfiguration `tiered_storage`:
```
hot_tier:  NVMe  (aktuell, < 90 Tage)
warm_tier: SATA SSD (selten, 90–365 Tage)
cold_tier: HDD (Archiv, > 365 Tage)
```

---

## 4. Skalierungsszenarien nach Instanzanzahl

> **Hinweis zur Begrifflichkeit:** „anfragende Instanz" = eine TCP-Verbindung /
> HTTP-Session / gRPC-Stream, die aktiv Anfragen stellt. Nicht zu verwechseln mit
> Datenbankknoten.

---

### 4.1 Szenario S-1: 100 Instanzen (Dev / Embedded)

**Profil:** Entwickler-System, kleine Anwendung, IoT-Gateway, QNAP NAS-Deployment.

```
Annahmen:
- 100 concurrent connections
- Mix: 70 % Reads, 25 % Writes, 5 % AQL-Scans
- Datensatz: < 10 GB
- LLM: nein (oder CPU-only, kleines Modell)
```

| Ressource | Last | Engpass? |
|-----------|------|----------|
| CPU Cores | 2–4 Cores @ 30–50 % | Nein |
| RAM | 2–4 GB (Block-Cache + Working Set) | Nein |
| GPU | Nicht erforderlich | – |
| NVMe | 1K–5K IOPS, 10–50 MB/s | Nein (SATA SSD ausreichend) |
| Threads | 50–100 (HTTP + TBB) | Nein |
| L3-Cache | Hot-Dataset passt vollständig in L3 | Kein Engpass |
| Netzwerk | < 100 Mbit/s | Nein |

**Charakteristik:** Alle Anfragen unter 5 ms p99. Keine Warteschlangen.
Compaction läuft idle-time ohne Impact auf Query-Latenz.

---

### 4.2 Szenario S-2: 1.000 Instanzen (SME / Edge)

**Profil:** Mittelgroße Applikation, Edge-Server, API-Backend.

```
Annahmen:
- 1.000 concurrent connections
- Mix: 60 % Reads, 30 % Writes, 10 % AQL/Vector
- Datensatz: 10–100 GB
- LLM: optional (GPU empfohlen)
```

| Ressource | Last | Engpass? |
|-----------|------|----------|
| CPU Cores | 8–16 Cores @ 40–70 % | Grenzwertig bei AQL-heavy |
| RAM | 8–32 GB | Nur wenn Block-Cache zu klein |
| GPU | 1× RTX 4060 Ti (16 GB) empfohlen für Vector/LLM | Optional |
| NVMe | 20K–100K IOPS, 100–500 MB/s | Achtung: Compaction-Spitzen |
| Threads | 200–600 (HTTP + TBB + BG) | OS-Limit unkritisch |
| L3-Cache | Hot-Dataset beginnt L3 zu übersteigen | Mäßiger Druck |
| Netzwerk | 100 Mbit/s – 1 Gbit/s | Nein |

**Charakteristik:** p99-Latenz 5–20 ms für Reads, 10–50 ms für Writes mit Compaction.
Compaction-Bursts können zu kurzzeitigem Write-Stall führen → `max_write_buffer_size`
erhöhen oder `rate_limiter` für Compaction aktivieren.

---

### 4.3 Szenario S-3: 10.000 Instanzen (Mid-Scale Production)

**Profil:** SaaS-Anwendung, internes Enterprise-System mit hoher Nutzerzahl.

```
Annahmen:
- 10.000 concurrent connections
- Mix: 50 % Reads, 35 % Writes, 15 % komplexe AQL / Vector / Graph
- Datensatz: 100 GB – 5 TB
- LLM: aktiv, mehrere concurrent Inference-Requests
- Sharding: 3–5 Knoten empfohlen
```

| Ressource | Last | Engpass? |
|-----------|------|----------|
| CPU Cores | 32–64 Cores @ 60–80 % | ⚠️ Ja, bei einfachem Knoten |
| RAM | 64–256 GB | ⚠️ Kritisch bei großem Block-Cache-Bedarf |
| GPU | 1–2× RTX 4090 (24 GB) oder 1× A100 (40 GB) | Für LLM erforderlich |
| VRAM | 24–48 GB für 8–24 concurrent LLM-Sessions | ⚠️ Engpass ohne Multi-GPU |
| NVMe | 200K–2M IOPS, 1–7 GB/s | ⚠️ Gen4 NVMe zwingend |
| Threads | 2.000–8.000 | ⚠️ Thread-Pool-Management wichtig |
| Connection RAM | ~1,3 GB Stack | Vertretbar |
| Netzwerk | 1–10 Gbit/s | Nein (10 GbE empfohlen) |

**Charakteristik:** Ohne Sharding werden CPU und NVMe zum Engpass.
Compaction schlägt direkt durch auf Write-Latenz (p99 > 100 ms möglich).
**Horizontales Sharding über Gossip/Raft wird hier erforderlich.**

**Kritische Tuning-Parameter:**
```
rocksdb.max_background_jobs = 16
rocksdb.rate_limiter_bytes_per_sec = 500MB
server.worker_threads = 48
tbb.max_allowed_parallelism = 32
cache.block_cache_size = 120GB (60% von 200GB alloc.)
```

---

### 4.4 Szenario S-4: 100.000 Instanzen (High-Scale Enterprise)

**Profil:** Große SaaS-Plattform, Telekommunikation, Financial Services.

```
Annahmen:
- 100.000 concurrent connections
- Mix: 45 % Reads, 40 % Writes, 15 % komplexe Queries
- Datensatz: 5–100 TB
- LLM: mehrere Modelle, dedizierte GPU-Knoten
- Architektur: Cluster mit 10–50 ThemisDB-Knoten + Load Balancer
```

**Dieser Lastbereich kann NICHT von einem Einzelknoten bedient werden.**

#### Cluster-Architektur (Referenz)

```
┌─────────────────────────────────────────────────────────────────┐
│ Load Balancer (L7, z.B. Nginx / Envoy)                          │
├─────────┬──────────────────────────────────┬───────────────────┤
│ Shard 1 │         Shard 2 … N              │  LLM-GPU-Cluster  │
│ ThemisDB│  ThemisDB (Raft Consensus)        │  (dediziert)      │
│ Leader  │  Follower × 2 (Quorum Read)       │  A100 × 4 pro     │
│ 64C/256G│  32C/128G jeder                  │  Inference-Node    │
│ 2× NVMe │  2× NVMe Gen4 (4 TB)             │                   │
└─────────┴──────────────────────────────────┴───────────────────┘
```

#### Ressourcenbedarf pro Knoten (bei 20 Knoten, 5.000 connections/Knoten)

| Ressource | Pro Knoten | Gesamt (20 Knoten) |
|-----------|------------|-------------------|
| CPU Cores | 64 | 1.280 |
| RAM | 256 GB | 5 TB |
| NVMe | 2× 4 TB Gen4 (RAID-1) | 80 TB raw |
| Netzwerk | 25 GbE | 500 Gbit/s total |
| GPU (LLM-Cluster) | 4× A100 (80 GB) | 80× A100 (6,4 TB VRAM) |

#### OS-Parameter für 100.000 Connections (pro Knoten)

```bash
fs.file-max = 2000000
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535
net.core.netdev_max_backlog = 65535
net.ipv4.tcp_fin_timeout = 15
net.ipv4.tcp_tw_reuse = 1
net.core.rmem_max = 67108864
net.core.wmem_max = 67108864
```

---

### 4.5 Szenario S-5: 10.000.000+ Instanzen (Hyperscale)

**Profil:** Globale Plattform (Social Media, IoT-Flotten, Realtime Analytics).

```
Annahmen:
- 10.000.000 concurrent connections (z.B. über WebSocket / MQTT)
- Stark heterogener Workload: IoT-Sensor-Streams, Push-Notifications,
  Realtime-Dashboards
- Datensatz: > 1 PB
- Architektur: Multi-Region, Multi-Cluster, CDC-Replikation
```

**10 Mio. Connections sind eine infrastrukturelle Herausforderung, die ThemisDB
als Datenbank nicht allein löst.** Die Architektur erfordert:

#### Mehrstufige Entkopplung

```
┌────────────────────────────────────────────────────────────────────┐
│ Tier 1: Connection Layer (stateless)                               │
│  MQTT Broker / WebSocket-Gateway / Nginx Cluster                   │
│  100–500 Gateway-Knoten × 100K Connections = 10M Connections total │
│  RAM pro Gateway: 1–4 GB Stack-RAM bei reduziertem Buffer          │
├────────────────────────────────────────────────────────────────────┤
│ Tier 2: Message Queue / Event Bus                                  │
│  Kafka / Pulsar für Write-Entkopplung (1M–10M events/s)            │
│  ThemisDB Streaming Ingest Manager → WAL (≥ 1M events/s/Knoten)   │
├────────────────────────────────────────────────────────────────────┤
│ Tier 3: ThemisDB Cluster (50–500 Knoten)                           │
│  Sharding über VCC-URN-Consistent-Hash                             │
│  Jeder Knoten: 64C/256G/2×NVMe Gen5                               │
│  Gossip-Protokoll für Topology-Awareness                           │
├────────────────────────────────────────────────────────────────────┤
│ Tier 4: LLM / AI Cluster (dediziert)                               │
│  H100 × N (nach Modellgröße und Anfragelast)                       │
│  Federated Inference (ThemisDB v1.17.0 planned)                    │
└────────────────────────────────────────────────────────────────────┘
```

#### Gesamtressourcenbedarf Hyperscale (grobe Schätzung)

| Ressource | Bedarf |
|-----------|--------|
| CPU Cores (Tier 3) | 500 Knoten × 64 Cores = 32.000 Cores |
| RAM (Tier 3) | 500 × 256 GB = 128 TB |
| NVMe (Tier 3) | 500 × 8 TB = 4 PB nutzbar |
| GPU (Tier 4, LLM) | 200–2.000 H100 (je nach LLM-Bedarf) |
| Netzwerk Backbone | 400 GbE pro Rack, RoCE für GPU-Cluster |
| Gateway-Nodes (Tier 1) | 200–500 × 16C/32G |

---

## 5. Hardwareanforderungsmatrix (Min / Opt / Max)

### 5.1 Einzelknoten-Matrix (ohne LLM-Modul)

| Komponente | **Minimal** | **Optimal** | **Maximal (Einzelknoten)** |
|------------|------------|------------|--------------------------|
| CPU Cores | 4 Cores (x86-64, AVX2) | 32 Cores (AMD EPYC / Intel Xeon) | 128 Cores (EPYC Genoa) |
| CPU Frequenz | 2,4 GHz | 3,2 GHz (Boost 4,5+) | 3,8+ GHz |
| Hyperthreading | Egal (< 8 Cores) | Empfohlen | Empfohlen |
| RAM | 8 GB DDR4 | 128 GB DDR5 ECC | 4 TB DDR5 ECC (8-Channel) |
| RAM-Geschwindigkeit | DDR4-3200 | DDR5-4800 | DDR5-6400 |
| GPU | Nicht erforderlich | – | – |
| NVMe | 500 GB SATA SSD | 2× 4 TB NVMe Gen4 | 4× 7,8 TB NVMe Gen5 |
| NVMe IOPS | 50K random | 1M random (Gen4) | 5M+ random (Gen5) |
| NVMe Bandbreite | 500 MB/s | 7 GB/s | 14 GB/s |
| HDD | Optional (Cold-Tier) | Optional | Optional |
| Netzwerk | 1 GbE | 25 GbE | 100 GbE |
| **Max. sinnvolle Concurrent Connections** | **~500** | **~20.000** | **~80.000** |

### 5.2 Einzelknoten-Matrix (mit LLM-Modul aktiv)

| Komponente | **Minimal** | **Optimal** | **Maximal (Einzelknoten)** |
|------------|------------|------------|--------------------------|
| CPU Cores | 8 Cores | 64 Cores | 128 Cores |
| RAM | 32 GB | 256 GB DDR5 | 512 GB DDR5 |
| GPU | RTX 4060 Ti (16 GB) | RTX 4090 (24 GB) | 4× A100 (80 GB = 320 GB VRAM) |
| VRAM | 16 GB | 24 GB | 320 GB (Multi-GPU) |
| NVMe | 1 TB NVMe Gen3 | 4 TB NVMe Gen4 | 2× 8 TB NVMe Gen5 |
| **Max. concurrent LLM-Sessions (Mistral-7B)** | **2–3** | **8–16** | **60–120** |
| **Max. concurrent DB-Connections** | **~1.000** | **~10.000** | **~50.000** |

### 5.3 Cluster-Matrix nach Anfragelast

| Anfragelast | Knoten | CPU (gesamt) | RAM (gesamt) | NVMe (gesamt) | GPU (gesamt) |
|-------------|--------|--------------|-------------|---------------|-------------|
| 100 | 1 | 4–8 Cores | 8–16 GB | 500 GB SSD | Optional |
| 1.000 | 1 | 8–16 Cores | 32–64 GB | 1–2 TB NVMe | Optional |
| 10.000 | 1–3 | 32–96 Cores | 128–384 GB | 4–12 TB NVMe | 1–2× GPU |
| 100.000 | 5–20 | 320–1.280 Cores | 1,6–5 TB | 20–80 TB NVMe | 8–40× GPU |
| 1.000.000 | 50–100 | 3.200–6.400 Cores | 12–25 TB | 200–400 TB NVMe | 80–200× GPU |
| 10.000.000 | 500+ | 32.000+ Cores | 128+ TB | 2+ PB NVMe | 500–2.000× GPU |

---

## 6. Konfigurationsempfehlungen je Tier

### Tier 1: Dev / Embedded (< 500 Connections)

```ini
[storage]
block_cache_size_mb = 4096        # 4 GB
max_background_jobs = 4
write_buffer_size_mb = 64

[server]
worker_threads = 8
max_connections = 1000
http2_enabled = true
grpc_enabled = false              # optional

[llm]
enabled = false                   # oder cpu_only = true

[sharding]
enabled = false                   # Einzelknoten
```

### Tier 2: SME Production (1.000–5.000 Connections)

```ini
[storage]
block_cache_size_mb = 32768       # 32 GB
max_background_jobs = 12
write_buffer_size_mb = 256
compaction_style = kLevel
rate_limiter_bytes_per_sec = 200MB

[server]
worker_threads = 16
max_connections = 10000
tls_enabled = true
http2_enabled = true
grpc_enabled = true

[index]
hnsw_ef_construction = 200
hnsw_m = 16

[llm]
enabled = true
cuda_enabled = true
gpu_layers = -1                   # alle Layer auf GPU
kv_cache_size_mb = 8192

[cache]
l1_max_entries = 100000
l2_max_size_mb = 4096
```

### Tier 3: Enterprise (10.000–100.000 Connections, Cluster)

```ini
[storage]
block_cache_size_mb = 131072      # 128 GB
max_background_jobs = 24
compaction_style = kUniversal     # niedrigere Write-Amplification
target_file_size_base_mb = 256
io_uring_enabled = true           # async I/O

[server]
worker_threads = 48
max_connections = 100000
rate_limit_rps = 50000            # pro Knoten

[sharding]
enabled = true
replication_factor = 3
gossip_interval_ms = 100
raft_election_timeout_ms = 300

[transaction]
mvcc_snapshot_gc_interval_ms = 1000
max_concurrent_transactions = 50000

[llm]
enabled = true
tensor_parallel_size = 4          # Multi-GPU
continuous_batching = true
max_num_seqs = 256
gpu_memory_utilization = 0.90

[observability]
prometheus_enabled = true
otel_tracing_sample_rate = 0.01   # 1% sampling bei hoher Last
```

### Tier 4: Hyperscale (> 1 Mio. Connections, Multi-Cluster)

- Dedizierte Connection-Gateways (MQTT/WebSocket) vor ThemisDB.
- Kafka/Pulsar als Write-Buffer zwischen Gateway und ThemisDB-Ingest.
- ThemisDB Streaming Ingest Manager aktiviert (`streaming_ingest_manager.h`).
- Geo-Distribution mit TrueTime/HLC für globale Kausalordnung.
- Federated LLM Inference (geplant v1.17.0).
- Adaptive Rate Limiter + Cost-Based Rate Limiter aktiv.

---

## 7. Cluster & Horizontal Scaling

### 7.1 Wann horizontal skalieren?

| Symptom | Ursache | Lösung |
|---------|---------|--------|
| CPU > 80 % sustained | Query-Last, Compaction | +Shards oder read replicas |
| Write-Stall (> 5s) | Compaction kann nicht mithalten | +NVMe Disks oder Write-Rate begrenzen |
| MVCC GC < 1 version/s | Zu viele open transactions | Txn-Timeout senken, read replicas |
| RAM < 20 % frei | Block-Cache zu voll | +RAM oder +Shards |
| VRAM OOM (LLM) | KV-Cache voll | +GPU oder Batch-Limit senken |
| p99 Latenz > 200 ms | Alle oben kombinierten | Sharding obligatorisch |

### 7.2 Sharding-Strategie (VCC-URN)

ThemisDB nutzt VCC-URN Consistent-Hash-Sharding:
- Automatisches Rebalancing über Gossip-Protokoll.
- Raft-Consensus für Leader-Election pro Shard.
- Empfehlung: **Shard-Größe 100–500 GB** (RocksDB optimal für ≤ 300 GB SST-Files).

### 7.3 Read Replicas

Für Read-Heavy-Workloads (> 80 % Reads):
- Jeder Shard: 1 Leader + 2 Follower (Quorum Reads möglich).
- Follower können Leselasten übernehmen (strong consistency optional via Raft-Read-Index).
- Reduziert Leader-CPU-Last um 40–60 % bei typischer Read-Last.

---

## 8. Engpass-Identifikation & Faustregeln

```
┌──────────────────────────────────────────────────────────────┐
│ SCHNELL-DIAGNOSE: Engpass-Baum                               │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│  Latenz hoch?                                                │
│  ├── p50 hoch → CPU überlastet (Query-Engine / Compaction)   │
│  ├── p99 >> p50 → Lock-Contention / MVCC-GC / NVMe-Spitzen   │
│  └── p999 extrem → Compaction Write-Stall oder GC-Pause       │
│                                                              │
│  Throughput sinkt?                                           │
│  ├── CPU sat. → Mehr Cores / Horizontal Scale                │
│  ├── RAM sat. → Block-Cache erhöhen oder Sharding            │
│  ├── NVMe sat. → io_uring + Universal Compaction + RAID      │
│  └── Network sat. → 25/100 GbE, Connection Multiplexing      │
│                                                              │
│  LLM-Latenz hoch?                                            │
│  ├── VRAM voll → KV-Cache-Limit senken oder +GPU             │
│  ├── GPU-Auslastung > 95 % → Continuous Batching prüfen      │
│  └── Modell zu groß → Kleineres Modell / Quantisierung        │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

### Faustregeln Zusammenfassung

| Faustregel | Erklärung |
|-----------|-----------|
| 1 Core ≈ 2.000 einfache AQL-Anfragen/s | Bei 10 ms avg-Latenz, TBB-parallel |
| 1 GB RAM Block-Cache ≈ 10.000 heiße Keys (1 KB avg) | 100 % Cache-Hit Rate |
| 1 NVMe Gen4 ≈ 10× RocksDB-Write-Durchsatz vs. SATA SSD | ~3,5 GB/s vs. 550 MB/s seq. |
| 1 A100 (80 GB) ≈ 40–60 concurrent Mistral-7B-Sessions | Bei Paged-Attention + Continuous Batching |
| 1 ThemisDB-Knoten ≈ max. 50.000–80.000 stable Connections | Jenseits davon horizontal skalieren |
| MVCC-Overhead ≈ 512 Byte × offene Txns × geänderte Keys | Txn-Timeouts < 60 s halten |
| Write-Amplification Leveled: 10–30×, Universal: 4–8× | Compaction-Style nach Write-Profil wählen |
| L3-Cache-Druckpunkt ≈ 32 MB Hot-Dataset | Darüber Block-Cache im RAM erforderlich |

---

## 9. Fazit

ThemisDB skaliert durch seinen modularen Aufbau von Embedded-Szenarien (100 Connections,
4 Cores, 8 GB RAM, SATA SSD) bis zu Hyperscale-Clustern (10 Mio. Connections, 500+ Knoten,
Petabyte-Storage). Die kritischen Skalierungsgrenzen auf einem Einzelknoten:

| Grenze | Ressource | Symptom |
|--------|-----------|---------|
| **~500 Connections** | Thread-Scheduler-Overhead | Latenzanstieg bei Burst |
| **~5.000 Connections** | CPU & TBB-Contention | p99-Degradation |
| **~20.000 Connections** | NVMe Write-Amplification | Write-Stall-Risiko |
| **~50.000 Connections** | RAM/Connection-Stack | OOM bei Default-Buffern |
| **~80.000 Connections** | OS-FD-Limits + TCP-Stack | Verbindungsabbrüche |
| **> 80.000 Connections** | Alles obige | Horizontales Sharding zwingend |

Die LLM-Ebene ist unabhängig davon ausschließlich durch **VRAM** begrenzt und erfordert
ab ~16 parallelen Inference-Sessions auf Mistral-7B mindestens 24 GB VRAM (RTX 4090) oder
Multi-GPU-Konfiguration. Für produktiven LLM-Betrieb auf Enterprise-Niveau ist die Trennung
von ThemisDB (CPU/RAM) und Inference-Cluster (GPU) — wie in der vLLM-Co-Location-Strategie
beschrieben — die empfohlene Architektur.

---

*Erstellt: 2026-04-17 | Version: 1.0 | Maintainer: ThemisDB Engineering*
