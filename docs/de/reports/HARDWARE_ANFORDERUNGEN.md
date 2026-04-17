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

**Version:** 1.1 — granuliert: GPU/VRAM getrennt · PCIe/I/O · CPU-Cache+C-States · GPU-Cache · AI-Cards · Sweet Spots
**Erstellt:** 2026-04-17
**Scope:** ThemisDB v1.8+ im Serverbetrieb – Einzelknoten bis horizontales Cluster
**Skalierungsszenarien:** 100 → 1.000 → 10.000 → 100.000 → 10.000.000 gleichzeitig anfragende Instanzen

---

## Inhaltsverzeichnis

1. [Executive Summary](#1-executive-summary)
2. [Modulares Ressourcenprofil](#2-modulares-ressourcenprofil)
3. [Detailanalyse je Ressourcentyp](#3-detailanalyse-je-ressourcentyp)
   - 3.1 [CPU / Cores / Threads](#31-cpu--cores--threads)
   - 3.2 [GPU (Compute / CUDA)](#32-gpu-compute--cuda)
   - 3.3 [VRAM (Kapazität)](#33-vram-kapazität)
   - 3.4 [RAM (Systemspeicher)](#34-ram-systemspeicher)
   - 3.5 [CPU-Cache (L1 / L2 / L3)](#35-cpu-cache-l1--l2--l3)
   - 3.6 [NVMe-SSD](#36-nvme-ssd)
   - 3.7 [HDD](#37-hdd)
   - 3.8 [PCIe & I/O-Bus — Bandbreite und Flaschenhälse](#38-pcie--io-bus--bandbreite-und-flaschenhälse)
   - 3.9 [CPU-interne Cache-Hierarchie & C-States](#39-cpu-interne-cache-hierarchie--c-states)
   - 3.10 [GPU-interne Cache-Architektur](#310-gpu-interne-cache-architektur)
   - 3.11 [AI Accelerator Cards](#311-ai-accelerator-cards)
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
9. [Sweet-Spot-Analyse je Ressourcentyp](#9-sweet-spot-analyse-je-ressourcentyp)
   - 9.1 [CPU / Cores — ROI-Kurve](#91-cpu--cores--roi-kurve)
   - 9.2 [GPU (Compute) — Durchsatz-Sweet-Spots](#92-gpu-compute--durchsatz-sweet-spots)
   - 9.3 [VRAM (Kapazität) — Session-Sweet-Spots](#93-vram-kapazität--session-sweet-spots)
   - 9.4 [RAM — Block-Cache-Hit-Rate-Kurve](#94-ram--block-cache-hit-rate-kurve)
   - 9.5 [NVMe-SSD — Generationen-Sweet-Spots](#95-nvme-ssd--generationen-sweet-spots)
   - 9.6 [Threads & Connections](#96-threads--connections)
   - 9.7 [L3-CPU-Cache — Plattformvergleich](#97-l3-cpu-cache--plattformvergleich)
10. [Workload-spezifische Profilmatrix](#10-workload-spezifische-profilmatrix)
11. [Granulierte Skalierungsstufen](#11-granulierte-skalierungsstufen)
12. [Upgrade-Pfad-Empfehlungen](#12-upgrade-pfad-empfehlungen)
13. [Fazit](#13-fazit)

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
| GPU (Compute) | LLM-Tokens/s, GPU-HNSW-Batch, CUDA-Kernel-Throughput | GPU-Auslastung > 90 % bei VRAM < 80 % |
| VRAM (Kapazität) | LLM KV-Cache, Modellgewichte, GPU-Vector-Index | Jede LLM-Session > 1 GB; harter Grenzwert (kein Swap) |
| PCIe-Bus | GPU↔CPU-Transfer, NVMe↔CPU, RDMA-NIC | Multi-GPU ohne NVLink, NVMe Gen5 auf ×4 Gen4 |
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

| Modul | CPU | RAM | GPU (Compute) | VRAM | NVMe I/O | Threads |
|-------|-----|-----|--------------|------|----------|---------|
| **Core / Storage (RocksDB MVCC)** | ★★★★ | ★★★★★ | – | – | ★★★★★ | ★★★ |
| **AQL / Query-Engine (TBB)** | ★★★★★ | ★★★ | – | – | ★★ | ★★★★★ |
| **Vector Index (HNSW / FAISS)** | ★★★ | ★★★★★ | ★★★★ | ★★★ | ★★ | ★★★ |
| **Graph Index** | ★★★ | ★★★★ | – | – | ★ | ★★★ |
| **Full-Text Search (BM25)** | ★★★★ | ★★★ | – | – | ★★★ | ★★★ |
| **LLM Inference (llama.cpp)** | ★★★★ | ★★★ | ★★★★★ | ★★★★★ | ★ | ★★★ |
| **LoRA / RAG Orchestration** | ★★★ | ★★★ | ★★★★ | ★★★★ | ★★ | ★★★ |
| **Sharding / Gossip / Raft** | ★★★ | ★★ | – | – | ★★ | ★★★★ |
| **Replication / WAL Streaming** | ★★ | ★★ | – | – | ★★★★ | ★★★ |
| **Transaction (MVCC / SAGA)** | ★★★★ | ★★★★ | – | – | ★★★ | ★★★★ |
| **Cache (L1-L3 AdaptiveCache)** | ★★ | ★★★★★ | – | – | – | ★★ |
| **Server (HTTP/2, gRPC, WS, MQTT)** | ★★★ | ★★★ | – | – | – | ★★★★★ |
| **Security (RBAC, HSM, Encryption)** | ★★★ | ★★ | – | – | ★ | ★★ |
| **Observability (OTEL, Prometheus)** | ★★ | ★★ | – | – | ★★ | ★★ |
| **CDC / Changefeed** | ★★ | ★★ | – | – | ★★★ | ★★★ |
| **Stable Diffusion / Image AI** | ★★ | ★★ | ★★★★★ | ★★★★★ | ★ | ★★ |
| **Timeseries (Gorilla Codec)** | ★★★ | ★★★ | – | – | ★★★ | ★★ |
| **Geo / Spatial** | ★★★ | ★★★ | ★★ (cuSpatial) | ★★ | ★★ | ★★ |

★ = gering · ★★ = moderat · ★★★ = deutlich · ★★★★ = hoch · ★★★★★ = dominanter Engpass
GPU (Compute) und VRAM werden als unabhängige Dimensionen bewertet (siehe §3.2 + §3.3).

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

### 3.2 GPU (Compute / CUDA)

> **Begriffsabgrenzung:** GPU (Compute) = Rechenleistung in TFLOPS, bestimmt
> Inferenz-Geschwindigkeit, Batch-Throughput und CUDA-Kernel-Latenz.
> VRAM (Kapazität) = Speichergröße in GB, bestimmt max. concurrent Sessions und
> Modellgröße — wird separat in §3.3 behandelt. Beide Dimensionen sind unabhängig.

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

### 3.3 VRAM (Kapazität)

> **Begriffsabgrenzung:** VRAM ist ein **harter Grenzwert** — kein transparentes
> VRAM-Swap möglich. Überschreitung führt zu OOM-Kill oder Request-Rejection.
> VRAM bestimmt: max. concurrent LLM-Sessions, max. Modellgröße, GPU-Vector-Index-Größe.
> Engpasszeichen: VRAM > 90 % belegt bei GPU-Auslastung < 70 % → Speicher-gebunden,
> nicht Compute-gebunden → mehr VRAM nötig, nicht schnellere GPU.

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

### 3.8 PCIe & I/O-Bus — Bandbreite und Flaschenhälse

Der PCIe-Bus ist die **Verbindungsebene zwischen CPU, GPU, NVMe und Netzwerkkarte**.
Seine Bandbreite begrenzt, wie schnell Daten zwischen den Subsystemen fließen können.

#### PCIe-Generationen im Überblick

| Generation | GB/s/Lane | ×4 Lanes | ×8 Lanes | ×16 Lanes | ThemisDB-Einsatz |
|-----------|----------|---------|---------|---------|----------------|
| PCIe Gen3 | 1,0 GB/s | 4 GB/s | 8 GB/s | 16 GB/s | Legacy NVMe, ältere GPUs |
| PCIe Gen4 | 2,0 GB/s | 8 GB/s | 16 GB/s | 32 GB/s | NVMe Gen4, RTX 4090, A100 PCIe |
| PCIe Gen5 | 4,0 GB/s | 16 GB/s | 32 GB/s | 64 GB/s | NVMe Gen5, H100, MI300X |
| PCIe Gen6 | 8,0 GB/s | 32 GB/s | 64 GB/s | 128 GB/s | Emerging (2025+) |

Hinweis: Werte sind unidirektional. PCIe ist Full-Duplex: effektiv 2× in beide Richtungen.

#### PCIe-Topologie in ThemisDB-Deployments

```
┌─────────────────────────────────────────────────────────────┐
│ CPU (AMD EPYC / Intel Xeon)                                 │
│  ├── PCIe ×16 Gen5 → GPU 1 (H100 / A100)   [64 GB/s]      │
│  ├── PCIe ×16 Gen5 → GPU 2 (Tensor-Parallel)[64 GB/s]      │
│  ├── PCIe ×4  Gen5 → NVMe 1                [16 GB/s]       │
│  ├── PCIe ×4  Gen5 → NVMe 2                [16 GB/s]       │
│  ├── PCIe ×8  Gen5 → NIC (100 GbE RDMA)    [32 GB/s]       │
│  └── PCIe ×4  Gen4 → NVMe 3 (PLX Switch)   [ 8 GB/s geteilt]│
│                                                             │
│ GPU-GPU ohne NVLink: über PCIe Switch / P2P                 │
│ GPU-GPU mit NVLink v4 (H100): 900 GB/s → PCIe bypass        │
└─────────────────────────────────────────────────────────────┘
```

#### PCIe-Engpass-Analyse

**Szenario 1: GPU-Inferenz (LLM)**
```
Modell-Loading (einmalig):
  Mistral-7B Q4 (4 GB)  bei PCIe Gen4 ×16 (32 GB/s) = 125 ms
  Llama-70B Q4 (38 GB)  bei PCIe Gen4 ×16 (32 GB/s) = 1,2 s
  → Kein Problem; geschieht nur beim Start

Runtime-Inference:
  Alle Operationen verbleiben im GPU-VRAM → PCIe nicht kritisch
  PCIe wird erst Engpass bei > 100.000 Token-Streaming/s (CPU↔GPU)
```

**Szenario 2: NVMe-Intensiv (RocksDB Cache-Miss)**
```
NVMe Gen4 (7 GB/s) auf PCIe ×4 Gen4 (8 GB/s) → kein Engpass
NVMe Gen5 (14 GB/s) auf PCIe ×4 Gen5 (16 GB/s) → kein Engpass
4× NVMe Gen5 auf PCIe ×16 Gen5 (geteilt) → 16 GB/s Deckel → Engpass!
→ Lösung: Jede NVMe auf eigenen PCIe ×4-Slot; kein PLX Switch
```

**Szenario 3: RDMA-Netzwerk (Sharding-Cluster)**
```
100 GbE RDMA: 12,5 GB/s → PCIe ×8 Gen4 (16 GB/s)  → kein Engpass
400 GbE InfiniBand: 50 GB/s → PCIe ×16 Gen5 (64 GB/s) zwingend
```

**Szenario 4: Multi-GPU Tensor-Parallel ohne NVLink**
```
GPU-GPU über PCIe Switch: effektiv ~16 GB/s bidirektional
NVLink v4 (H100): 900 GB/s → 56× schneller als PCIe P2P

→ Tensor-Parallel über PCIe: nur für 7B-Modelle tolerierbar
→ Ab Llama-70B: NVLink zwingend; sonst GPU-GPU = dominanter Engpass
```

#### Takt-Analyse: CPU-Frequenz, Memory-Bus, I/O-Timing

| Parameter | Einfluss auf ThemisDB | Empfehlung |
|-----------|----------------------|-----------|
| CPU Base Clock | Latenz synchroner I/O-Ops | ≥ 2,8 GHz |
| CPU Boost Clock | Single-Thread (AQL-Planner, MVCC) | ≥ 4,5 GHz |
| Memory Bus Breite | Parallele Block-Cache-Transfers | DDR5: 64 Bit × N Channels |
| Memory Bandwidth | RocksDB Block-Cache Throughput | ≥ 150 GB/s (4-Channel DDR5) |
| Memory Latency | L3-Cache-Miss-Penalty | DDR5: ~75 ns; DDR4: ~65 ns |
| NVMe Queue Depth | Concurrent I/O-Requests | QD ≥ 32 (io_uring empfohlen) |

**Memory-Bus-Bandbreite nach Plattform:**

| Plattform | Channels | Typ | MT/s | Bandwidth |
|-----------|---------|-----|------|-----------|
| Intel Core i9-14900K | 2 | DDR5 | 5.600 | ~90 GB/s |
| AMD Ryzen 9 7950X | 2 | DDR5 | 5.200 | ~83 GB/s |
| Intel Xeon Platinum 8490H | 8 | DDR5 | 4.800 | ~307 GB/s |
| AMD EPYC Genoa 9654 | 12 | DDR5 | 4.800 | ~460 GB/s |
| AMD EPYC Bergamo 9754 | 12 | DDR5 | 4.800 | ~460 GB/s |

Empfehlung: AMD EPYC mit 12-Channel-DDR5 (460 GB/s) verhindert Memory-Bus-Sättigung
bei massiven Block-Cache-Reads — entscheidend wenn Hot-Dataset > L3-Cache-Kapazität.

---

### 3.9 CPU-interne Cache-Hierarchie & C-States

Die CPU-Cache-Hierarchie beeinflusst ThemisDB auf drei kritischen Pfaden:
(1) RocksDB Block-Index-Lookup, (2) TBB Task-Queue-Ops, (3) HNSW Neighbor-Traversal.

#### L1 / L2 / L3 — Detail

| Cache-Ebene | Inhalt | Latenz (Zyklen) | Latenz (ns @ 4 GHz) | ThemisDB-Relevanz |
|-------------|--------|----------------|--------------------|--------------------|
| L1d (Daten) | 32–64 KB/Core | 4–5 | ~1,2 ns | TBB Task-Metadaten, Loop-Counter |
| L1i (Instr.) | 32–64 KB/Core | 4 | ~1,0 ns | Hot-Code-Paths (Query-Planner) |
| L2 (unified) | 256 KB–1 MB/Core | 12–14 | ~3,5 ns | RocksDB Skip-List, Filter-Blocks |
| L3 (shared) | 16–384 MB | 35–50 | ~10 ns | HNSW Neighbor-Lists, Hot SST-Blocks |
| RAM | 32 GB–4 TB | 200–300 | ~75 ns | Block-Cache Cold-Data |
| NVMe | TB-Bereich | 200.000–500.000 | 50–125 µs | Compaction, Cold-Data |

Cache-Line-Größe: **64 Byte** auf allen modernen x86-64-CPUs.
RocksDB-Block (4 KB) = 64 Cache-Lines → L3-Hit kostet 64 sequenzielle Zugriffe.

**False Sharing:** TBB-Worker-Threads mit gemeinsamen Countern auf derselben Cache-Line
verursachen Cache-Invalidierungen. ThemisDB padded kritische Counters auf 64 Bytes.

#### Prefetcher & TLB

| Mechanismus | Wirkung | ThemisDB-Relevanz |
|-------------|---------|-------------------|
| HW Prefetcher (L1) | Erkennt sequenzielle Zugriffe | Positiv für RocksDB Sequential Scan |
| Stride Prefetcher (L2) | Erkennt Schrittmuster | Positiv für HNSW Layer-Iteration |
| LLC Prefetcher (L3) | Prefetcht L3-nahe Daten | Variabel bei Random-Key-Lookups |
| TLB L1 (DTLB) | 64–128 Einträge, 4 KB Pages | TLB-Thrashing bei > 4 GB Working-Set |
| TLB L2 (STLB) | 1.024–4.096 Einträge, 2 MB+4 KB | Huge-Pages reduzieren TLB-Druck |

```bash
echo always > /sys/kernel/mm/transparent_hugepage/enabled
echo defer+madvise > /sys/kernel/mm/transparent_hugepage/defrag
```

Mit Transparent Huge Pages (THP): 2 MB Pages für Block-Cache → 16× weniger TLB-Einträge nötig.

#### CPU C-States (Power Management States)

C-States sind für latenz-sensitive ThemisDB-Deployments kritisch: Deep-Sleep-Zustände
erhöhen die Wake-up-Latenz bei Request-Bursts erheblich.

| C-State | Name | Stromersparnis | Wake-up-Latenz | ThemisDB-Empfehlung |
|---------|------|---------------|----------------|---------------------|
| C0 | Active | 0 % | 0 µs | Dauerbetrieb bei hoher Last |
| C1 | Halt | ~30 % | < 1 µs | ✅ Akzeptabel |
| C1E | Enhanced Halt | ~35 % | < 10 µs | ✅ Akzeptabel |
| C3 | Sleep | ~50 % | ~50 µs | ⚠️ Erzeugt p99-Spitzen bei Burst |
| C6 | Deep Power Down | ~70 % | ~200 µs | ❌ Nicht für Production |
| C7/C8 | Enhanced Deep Sleep | ~80 % | ~500 µs | ❌ Nur Dev/Test |

**Konfiguration für ThemisDB-Production:**

```bash
cpupower idle-set -D 2
```

Oder permanent per Kernel-Parameter:

```
GRUB_CMDLINE_LINUX="processor.max_cstate=1 intel_idle.max_cstate=1"
```

Auswirkung: C6/C8 aktiviert → p99 steigt um +200–500 µs bei Idle→Burst-Transition.
C1/C1E: < 10 µs Penalty (tolerierbar). `idle=poll`: 0 µs, aber ~15 % mehr Leistungsaufnahme.

---

### 3.10 GPU-interne Cache-Architektur

GPU-interne Caches sind relevant, wenn CUDA-Kernel aktiv sind (HNSW-GPU-Traversal,
Flash-Attention, cuSpatial). Die GPU-Cache-Hierarchie unterscheidet sich grundlegend
von CPU-Caches:

```
┌─────────────────────────────────────────────────────────────┐
│ GPU-Cache-Hierarchie (Beispiel: H100 SXM5, 132 SMs)         │
│                                                             │
│ Pro SM (Streaming Multiprocessor):                          │
│  ├── Register File:   256 KB (schnellster Speicher)         │
│  ├── Shared Memory:   0–228 KB (konfigurierbar per Kernel)  │
│  └── L1-Cache:        Rest von 256 KB (Shared + L1 = 256 KB)│
│                                                             │
│ L2-Cache: 50 MB (geteilt, alle 132 SMs)                     │
│                                                             │
│ HBM3: 80 GB @ 3.350 GB/s                                    │
└─────────────────────────────────────────────────────────────┘
```

#### GPU L2-Cache und Shared Memory nach Modell

| GPU | L2-Cache | Shared Mem/SM | HBM Bandwidth | HNSW-Eignung | LLM-Eignung |
|-----|---------|--------------|--------------|-------------|------------|
| RTX 3090 | 6 MB | 100 KB | 936 GB/s | ⚠️ L2-Thrashing | Moderat |
| RTX 4090 | **72 MB** | 100 KB | 1.008 GB/s | ✅ Neighbor-Lists im L2 | ✅ Gut |
| A100 40/80G | 40 MB | 192 KB | 2.000 GB/s | ✅ Gut | ✅ Optimal |
| H100 80G SXM | 50 MB | 228 KB | 3.350 GB/s | ✅ Gut | ✅ Optimal |
| AMD MI300X | 32 MB | variabel | 5.300 GB/s | Moderat | ✅ Bandwidth-Champion |

**Bedeutung für ThemisDB:**

1. **HNSW-GPU-Traversal:** Nachbarschaftslisten (M=16, ~64 Byte/Eintrag) werden bei
   wiederholten Queries im GPU-L2 gecached. RTX 4090 mit 72 MB L2 hält ~1,1 Mio.
   Einträge im L2 — erheblicher Vorteil gegenüber RTX 3090 (6 MB L2).

2. **Flash-Attention (LLM):** Q·Kᵀ-Zwischenergebnisse bleiben in Shared Memory (L1).
   Größeres Shared Memory → größere Attention-Block-Größe → weniger HBM-Zugriffe
   (FlashAttention-2 Algorithmus).

3. **KV-Cache:** Liegt im HBM/VRAM. GPU-L2 puffert häufig gelesene KV-Einträge
   (System-Prompt-Tokens, die alle Sessions teilen → natürlicher L2-Hit).

**Shared-Memory-Konfiguration für HNSW-CUDA-Kernel:**

```cpp
cudaFuncSetAttribute(
    hnsw_search_kernel,
    cudaFuncAttributeMaxDynamicSharedMemorySize,
    228 * 1024  // 228 KB Maximum (H100)
);
```

---

### 3.11 AI Accelerator Cards

Neben NVIDIA-GPUs gibt es spezialisierte AI-Karten, die für LLM-Inference in
ThemisDB-Deployments relevant sind:

| Karte | Compute (BF16) | VRAM | Bandwidth | PCIe-Anschluss | ThemisDB-Kompatibilität |
|-------|--------------|------|-----------|---------------|------------------------|
| NVIDIA H100 80G SXM5 | 989 TFLOPS | 80 GB HBM3 | 3.350 GB/s | SXM (NVLink v4) | ✅ Vollständig (CUDA) |
| NVIDIA H200 141G SXM | 989 TFLOPS | 141 GB HBM3e | 4.800 GB/s | SXM (NVLink v4) | ✅ Vollständig |
| NVIDIA A100 80G SXM | 312 TFLOPS | 80 GB HBM2e | 2.000 GB/s | SXM / PCIe Gen4 | ✅ Vollständig |
| **AMD MI300X** | **1.307 TFLOPS** | **192 GB HBM3** | **5.300 GB/s** | PCIe Gen5 ×16 | ⚠️ ROCm (llama.cpp ROCm-Build) |
| Intel Gaudi 3 | 1.835 TOPS | 128 GB HBM2e | 3.700 GB/s | PCIe Gen5 ×16 | ⚠️ Gaudi SDK (kein llama.cpp nativ) |
| Intel Gaudi 2 | 865 TOPS | 96 GB HBM2e | 2.450 GB/s | PCIe Gen4 ×16 | ⚠️ Gaudi SDK only |
| AWS Inferentia 2 | 190 TOPS | 32 GB | 820 GB/s | Cloud only | ❌ AWS-proprietär |
| Google TPU v5p | 918 TFLOPS | 95 GB HBM | 4.800 GB/s | Cloud only | ❌ Google Cloud only |
| Groq LPU | ~750 TOPS* | 230 MB SRAM | — | PCIe / Cloud | ❌ Proprietäres Modellformat |
| Apple M4 Ultra ANE | 38 TOPS | 192 GB unified | 546 GB/s | Integriert | ⚠️ llama.cpp Metal, macOS only |

*Groq LPU: ultra-niedrige Latenz (~1 ms TTFT für 7B), aber proprietäres Modellformat.

#### AMD MI300X — VRAM-Sweet-Spot für große Modelle

```
192 GB VRAM → Llama-3-70B FP16 (140 GB) auf einer Karte
Concurrent Sessions: ~30 bei Llama-70B FP16 (kein Multi-GPU nötig)
Bandbreite: 5.300 GB/s → 1,6× schneller als H100 (Memory-Bandwidth-bound Decode)
GPU-L2: 32 MB (kleiner als RTX 4090) → HNSW-intensive Workloads bevorzugen NVIDIA

llama.cpp Build: cmake -DGGML_HIPBLAS=ON
Reife: ROCm rapide verbessernd, aber CUDA weiterhin ausgereifter
```

#### Intel Gaudi 3 — kosteneffiziente Alternative

```
~40 % günstiger als H100 bei ähnlicher LLM-Performance
Vorteil: 8× 200G RDMA-NIC integriert → kein separates InfiniBand nötig
Nachteil: kein natives llama.cpp; Intel Optimum Gaudi + PyTorch-Wrapper nötig
Geeignet: Nur wenn Inference-Stack über Intel Optimum läuft
```

#### Apple M4 Ultra — Unified Memory Vorteil

```
192 GB unified Memory = VRAM + RAM geteilt (kein PCIe-Bottleneck!)
→ Llama-3-70B Q4 (38 GB) + RocksDB Block-Cache (120 GB) auf einer Apple Silicon CPU
Vorteil: CPU und GPU teilen denselben Speicher; keine PCIe-Bandbreitenbeschränkung
Nachteil: 38 TOPS ANE; keine ECC; macOS only; nicht für Enterprise-Production
```

#### Entscheidungsbaum AI-Karte

```
Budget < 2.000 €?
  Ja → RTX 4090 (CUDA, bester Consumer-Sweet-Spot)
  Nein →
    Budget < 20.000 €?
      Ja → A100 80G SXM (beste Enterprise-Balance)
      Nein →
        Modell > 70B FP16 oder > 100 LLM-Sessions?
          Ja → AMD MI300X (192 GB, günstigstes $/GB > 80GB)
               oder H200 141G (HBM3e, höchste Bandwidth)
          Nein →
            Latenz < 50 ms TTFT + hohe Parallelität?
              Ja → H100 SXM5 (NVLink-Cluster)
              Nein → A100 80G SXM (ausreichend)
```

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
| GPU (Compute) | Nicht erforderlich | Empfohlen für Vector-Batch | 4× A100 SXM (312 TFLOPS je) |
| VRAM | Nicht erforderlich | – | – |
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
| GPU (Compute) | RTX 4060 Ti (22 TFLOPS) | RTX 4090 (82 TFLOPS) | 4× A100 80G SXM (312 TFLOPS je) |
| VRAM | 16 GB | 24 GB | 320 GB (4× 80 GB) |
| NVMe | 1 TB NVMe Gen3 | 4 TB NVMe Gen4 | 2× 8 TB NVMe Gen5 |
| **Max. concurrent LLM-Sessions (Mistral-7B)** | **2–3** | **8–16** | **60–120** |
| **Max. concurrent DB-Connections** | **~1.000** | **~10.000** | **~50.000** |

### 5.3 Cluster-Matrix nach Anfragelast

| Anfragelast | Knoten | CPU (gesamt) | RAM (gesamt) | NVMe (gesamt) | GPU Compute (gesamt) | VRAM (gesamt) |
|-------------|--------|--------------|-------------|---------------|---------------------|--------------|
| 100 | 1 | 4–8 Cores | 8–16 GB | 500 GB SSD | Optional | 0–16 GB opt. |
| 1.000 | 1 | 8–16 Cores | 32–64 GB | 1–2 TB NVMe | Optional | 0–24 GB opt. |
| 10.000 | 1–3 | 32–96 Cores | 128–384 GB | 4–12 TB NVMe | 1–2× RTX 4090 | 24–48 GB |
| 100.000 | 5–20 | 320–1.280 Cores | 1,6–5 TB | 20–80 TB NVMe | 8–40× A100 SXM | 640 GB–3,2 TB |
| 1.000.000 | 50–100 | 3.200–6.400 Cores | 12–25 TB | 200–400 TB NVMe | 80–200× A100 | 6,4–16 TB |
| 10.000.000 | 500+ | 32.000+ Cores | 128+ TB | 2+ PB NVMe | 500–2.000× H100/MI300X | 40–384 TB |

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
│  ├── p99 bei Idle→Burst → CPU C-States (C6+) aktiv          │
│  │    → Lösung: processor.max_cstate=1 setzen                │
│  └── p999 extrem → Compaction Write-Stall oder GC-Pause      │
│                                                              │
│  Throughput sinkt?                                           │
│  ├── CPU sat. → Mehr Cores / Horizontal Scale                │
│  ├── RAM sat. → Block-Cache erhöhen oder Sharding            │
│  ├── NVMe sat. → io_uring + Universal Compaction + RAID      │
│  ├── PCIe sat. → Gen4→Gen5 oder NVLink (Multi-GPU)           │
│  └── Network sat. → 25/100 GbE, Connection Multiplexing      │
│                                                              │
│  LLM-Latenz hoch?                                            │
│  ├── GPU-Auslastung > 90 % + VRAM < 80 % → Compute-Engpass  │
│  │    → GPU-Compute-Upgrade (RTX 4090 → A100 / MI300X)       │
│  ├── VRAM > 90 % + GPU-Auslastung < 70 % → VRAM-Engpass     │
│  │    → VRAM-Upgrade (A100 80G / MI300X 192G) oder Multi-GPU │
│  ├── GPU-Auslastung > 95 % → Continuous Batching prüfen      │
│  ├── PCIe-Bandbreite > 80 % (GPU↔CPU) → NVLink / PCIe Gen5  │
│  └── Modell zu groß → Kleineres Modell / Quantisierung        │
│                                                              │
│  Cache-Probleme?                                             │
│  ├── CPU L3-Thrashing → Huge-Pages aktivieren (THP)          │
│  ├── GPU L2-Thrashing (HNSW) → RTX 4090 (72 MB L2) nutzen   │
│  └── TLB-Thrashing → 2 MB Huge-Pages für Block-Cache        │
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

## 9. Sweet-Spot-Analyse je Ressourcentyp

> **Ziel:** Den Punkt maximalen Kosten-Nutzens für jede Ressource identifizieren —
> nicht das technische Maximum, sondern den wirtschaftlich optimalen Einsatzpunkt.

---

### 9.1 CPU / Cores — ROI-Kurve

Der Grenznutzen weiterer Cores sinkt durch NUMA-Latenz, Lock-Contention und
TBB-Scheduling-Overhead ab einem workload-abhängigen Sättigungspunkt:

| Core-Stufe | Query-Throughput (AQL) | Marginalgewinn | Sweet Spot? |
|-----------|----------------------|----------------|------------|
| 4 Cores | ~80K ops/s | Baseline | Dev/Test |
| 8 Cores | ~155K ops/s | +94 % | ✅ Dev Sweet Spot (< 500 Connections) |
| 16 Cores | ~290K ops/s | +87 % | ✅ SME Sweet Spot (500–5K) |
| 32 Cores | ~510K ops/s | +76 % | ✅ Production Sweet Spot (5K–20K) |
| 64 Cores | ~800K ops/s | +57 % | ✅ Enterprise Sweet Spot (20K–80K) |
| 96 Cores | ~1.000K ops/s | +25 % | ⚠️ Diminishing Returns (NUMA-Penalty > 30 %) |
| 128 Cores | ~1.100K ops/s | +10 % | ❌ Nur für Cluster-Flagshipknoten |

Empfehlung: **32–64 Cores** für Produktionsknoten. Ab 64 Cores überwiegt NUMA-Overhead.

---

### 9.2 GPU (Compute) — Durchsatz-Sweet-Spots

**GPU-Compute** (TFLOPS) bestimmt Inferenz-Geschwindigkeit und Batch-Throughput —
unabhängig von der VRAM-Kapazität.

Engpasszeichen: GPU-Auslastung > 90 % bei VRAM < 80 % → Compute-gebunden.

| GPU Modell | BF16 TFLOPS | GPU L2-Cache | Preis (ca.) | Mistral-7B tok/s | tok/s/€ | Eignung |
|------------|------------|-------------|-----------|----------------|---------|--------|
| RTX 4060 Ti 16 GB | 22 | 32 MB | ~400 € | ~40 | 0,100 | Dev/Test |
| RTX 4070 Ti Super 16 GB | 40 | 48 MB | ~800 € | ~90 | 0,113 | SME Balanced |
| RTX 4080 Super 16 GB | 52 | 64 MB | ~1.000 € | ~110 | 0,110 | ⚠️ VRAM-Deckel = 4070 Ti |
| **RTX 4090 24 GB** | **82** | **72 MB** | **~1.800 €** | **~180** | **0,100** | **✅ Single-GPU Sweet Spot** |
| A100 40 GB PCIe | 77 | 40 MB | ~8.000 € | ~160 | 0,020 | Enterprise (VRAM > Compute-ROI) |
| **A100 80 GB SXM** | **312** | **40 MB** | **~14.000 €** | **~350** | **0,025** | **✅ Enterprise Sweet Spot** |
| H100 80 GB SXM5 | 989 | 50 MB | ~30.000 € | ~700 | 0,023 | Hyperscale ROI ab ~100K req/day |
| **AMD MI300X 192 GB** | **1.307** | **32 MB** | **~20.000 €** | **~900** | **0,045** | **✅ Sweet Spot für 70B-Modelle** |

Fazit: Consumer-GPUs liefern 4–6× mehr tokens/€ als Data-Center-Karten. RTX 4090 ist
der beste Single-GPU-Wert. MI300X ist Sweet Spot für Llama-70B FP16 ohne Multi-GPU.

---

### 9.3 VRAM (Kapazität) — Session-Sweet-Spots

VRAM ist ein **harter Grenzwert** — kein transparentes Swap. Überschreitung führt zu OOM.

Engpasszeichen: VRAM > 90 % + GPU-Auslastung < 70 % → mehr VRAM, nicht mehr Compute.

#### Max. LLM-Sessions nach VRAM (Paged-Attention, seq_len=2048, 50 % Auslastung)

| VRAM | Phi-3-Mini 3.8B | Mistral-7B Q4 | Llama-3-8B Q4 | Llama-3-70B Q4 | Llama-3-70B FP16 |
|------|----------------|--------------|--------------|---------------|-----------------|
| 16 GB | ~20 | ~8–10 | ~8 | ❌ | ❌ |
| 24 GB | ~35 | ~16–20 | ~14 | ❌ | ❌ |
| 40 GB | ~65 | ~30–36 | ~28 | ~1–2 (knapp) | ❌ |
| 80 GB | ~130 | ~60–72 | ~56 | ~20–28 | ❌ |
| 141 GB (H200) | ~230 | ~110 | ~100 | ~50 | ~1 (knapp) |
| 192 GB (MI300X) | ~310 | ~150 | ~140 | ~70 | ~1–2 |
| 320 GB (4× A100) | ~500 | ~240 | ~220 | ~120 | ~10 |

#### €/GB VRAM — Effizienzvergleich

| GPU | VRAM | Preis | €/GB VRAM | Sweet Spot? |
|-----|------|-------|----------|------------|
| RTX 4060 Ti 16 GB | 16 GB | ~400 € | **25 €/GB** | ✅ Günstigste $/GB Consumer |
| RTX 4070 Ti Super 16 GB | 16 GB | ~800 € | 50 €/GB | Moderat |
| RTX 4080 Super 16 GB | 16 GB | ~1.000 € | 62 €/GB | ❌ VRAM-ineffizient |
| **RTX 4090 24 GB** | **24 GB** | **~1.800 €** | **75 €/GB** | **✅ 24-GB-Klasse Optimum** |
| A100 40 GB PCIe | 40 GB | ~8.000 € | 200 €/GB | Nur ECC + PCIe |
| **A100 80 GB SXM** | **80 GB** | **~14.000 €** | **175 €/GB** | **✅ Enterprise: bestes $/GB > 24 GB** |
| **AMD MI300X 192 GB** | **192 GB** | **~20.000 €** | **104 €/GB** | **✅ Optimum für 70B-FP16** |
| H100 80 GB SXM5 | 80 GB | ~30.000 € | 375 €/GB | ❌ Nur bei Throughput-ROI |

**GPU-Compute ≠ VRAM — Vergleich:**

```
Szenario A: RTX 4080 Super (52 TFLOPS, 16 GB)
  → Compute: ~110 tok/s    Sessions: ~10 concurrent
  → Hohe Geschwindigkeit, begrenzte Kapazität

Szenario B: RTX 4090 (82 TFLOPS, 24 GB) — SWEET SPOT
  → Compute: ~180 tok/s    Sessions: ~18 concurrent
  → Beide Dimensionen optimal

Szenario C: AMD MI300X (1.307 TFLOPS, 192 GB)
  → Compute: ~900 tok/s    Sessions: ~150 concurrent
  → Einzige Single-Card-Option für Llama-70B FP16
```

---

### 9.4 RAM (Systemspeicher) — Block-Cache-Hit-Rate-Kurve

| RAM | Block-Cache (65 %) | Hit-Rate (10 GB Hot) | Hit-Rate (100 GB) | Hit-Rate (1 TB) |
|-----|-------------------|---------------------|------------------|----------------|
| 16 GB | 10 GB | ~95 % | ~10 % | < 1 % |
| 32 GB | 21 GB | **~99 %** | ~21 % | < 3 % |
| 64 GB | 42 GB | ~99 % | ~42 % | ~4 % |
| 128 GB | 83 GB | ~99 % | ~83 % | ~8 % |
| 256 GB | 166 GB | ~99 % | **~99 %** | ~17 % |
| 512 GB | 333 GB | ~99 % | ~99 % | ~33 % |
| 1 TB | 666 GB | ~99 % | ~99 % | ~67 % |
| 2 TB | 1.300 GB | ~99 % | ~99 % | **~99 %** |

Sweet Spots: **32 GB** (bis 10 GB Hot) · **256 GB** (bis 100 GB) · **2 TB+** oder Sharding (1 TB).

---

### 9.5 NVMe-SSD — Generationen-Sweet-Spots

| NVMe Typ | Seq. Read | Rand. IOPS | Write-Latenz | Preis/TB | App-Write Ceiling | Sweet Spot |
|---------|----------|-----------|------------|---------|-----------------|-----------|
| SATA SSD | 550 MB/s | 100K | 0,10 ms | ~70 €/TB | ~5 MB/s | Dev/Test |
| NVMe Gen3 | 3.500 MB/s | 500K | 0,06 ms | ~80 €/TB | ~30 MB/s | SME |
| **NVMe Gen4** | **7.000 MB/s** | **1,2M** | **0,04 ms** | **~90 €/TB** | **~300 MB/s** | **✅ Production Sweet Spot** |
| NVMe Gen5 | 14.000 MB/s | 3M | 0,02 ms | ~180 €/TB | ~1 GB/s | Write-Heavy Enterprise |
| Intel Optane P5800X | 7.200 MB/s | 1,5M | **0,006 ms** | ~600 €/TB | ~500 MB/s | SLA < 1 ms p99 |

NVMe Gen4 ist der Production-Sweet-Spot: 9× günstiger als Optane bei 85 % der IOPS.
Optane nur bei p99-SLA < 1 ms ROI-positiv (Financial-Trading, Realtime-Fraud-Detection).

---

### 9.6 Threads & Connections — Ratio-Sweet-Spots

| Connections | Worker-Threads | Conn/Thread | I/O-Modell | Empfehlung |
|------------|--------------|------------|-----------|-----------|
| 100–500 | 20–50 | 5–25 | Blocking OK | Standard |
| 500–5K | 50–200 | 10–50 | Async bevorzugt | io_uring optional |
| 5K–20K | 200–600 | 25–100 | Async zwingend | io_uring + epoll |
| 20K–80K | 600–2K | 25–100 | Non-blocking | io_uring, SO_RCVBUF reduzieren |
| > 80K | ≥ 2K (max sinnvoll) | > 40 | Non-blocking + H-Scale | Horizontal Scaling |

Sweet Spot: **1 Thread pro 25–50 aktiven Connections** bei asynchronem I/O.
Über 4.000 Threads: OS-Scheduler-Overhead überwiegt den Parallelitätsgewinn.

---

### 9.7 L3-CPU-Cache — Plattformvergleich

| CPU-Plattform | L3-Cache | ThemisDB-Vorteil | Sweet Spot? |
|--------------|---------|-----------------|------------|
| Intel Core i9-14900K | 36 MB | Ausreichend < 30 MB Hot-Dataset | Dev/SME |
| AMD Ryzen 9 7950X3D (3D V-Cache) | **128 MB** | HNSW Neighbor-Lists im L3 | ✅ HNSW/Vector |
| AMD EPYC Genoa 9654 | **384 MB** | RocksDB Hot-Block-Index im L3 | ✅ Enterprise OLTP |
| Intel Xeon Platinum 8490H | 112 MB | Guter Mixed-Workload-Cache | Enterprise Mixed |
| AMD EPYC Bergamo 9754 | 256 MB | Viele Cores + großer L3 | Enterprise OLAP |

Sweet Spot: EPYC Genoa (384 MB L3) → p50 < 1 ms für Hot-Keys ohne RAM-Zugriff.
Ryzen 9 7950X3D: günstigste Option für Vector-Search-dominante Workloads.

---

## 10. Workload-spezifische Profilmatrix

> Ressourcenverteilung normiert auf 100 Punkte. GPU-Compute und VRAM sind separate Dimensionen.

| Workload-Profil | CPU | RAM | GPU Compute | VRAM | NVMe | Threads | Primärer Engpass |
|----------------|-----|-----|------------|------|------|---------|--------------------|
| OLTP (KV-heavy) | 35 | 30 | 0 | 0 | 25 | 10 | NVMe WAL + Compaction |
| OLAP / Analytics | 45 | 25 | 0 | 0 | 20 | 10 | CPU (TBB-Parallelism) |
| Vector Search GPU | 20 | 25 | 35 | 10 | 5 | 5 | GPU Compute (Batch) oder RAM (HNSW) |
| LLM Inference Only | 10 | 5 | 30 | 50 | 0 | 5 | VRAM (KV-Cache-Kapazität) |
| RAG (Vector + LLM) | 15 | 15 | 20 | 40 | 5 | 5 | VRAM, dann GPU Compute |
| Graph Traversal | 30 | 50 | 0 | 0 | 10 | 10 | RAM (Graph-Index in Memory) |
| Timeseries Ingest | 20 | 10 | 0 | 0 | 60 | 10 | NVMe Write-Throughput |
| Write-Heavy / CDC | 20 | 10 | 0 | 0 | 60 | 10 | NVMe Seq. Write + Compaction |
| Stable Diffusion | 5 | 5 | 40 | 50 | 0 | 0 | VRAM (Image Buffer) > GPU Compute |
| Mixed Polyglot | 20 | 25 | 15 | 20 | 15 | 5 | Situationsabhängig |

### Workload-spezifische Hardware-Empfehlungen

| Workload | CPU | RAM | GPU Compute | VRAM | NVMe |
|---------|-----|-----|------------|------|------|
| OLTP | EPYC Genoa 32–64C | 256 GB DDR5 | Nicht nötig | Nicht nötig | 2× NVMe Gen4 RAID-1 |
| OLAP | EPYC Bergamo 64–128C | 256–512 GB | Nicht nötig | Nicht nötig | 1× NVMe Gen4 |
| Vector Search | Ryzen 9 7950X3D | 256–512 GB | RTX 4090 (82 TFLOPS) | 24 GB | NVMe Gen4 |
| LLM Only | 16–32C | 64–128 GB | RTX 4090 / A100 SXM | **24–80 GB (kritisch)** | NVMe Gen3 |
| RAG | 32–64C | 128–256 GB | A100 80G / MI300X | 80–192 GB | NVMe Gen4 |
| Graph | EPYC Genoa (384 MB L3) | 512 GB–2 TB | Nicht nötig | Nicht nötig | NVMe Gen4 |
| Timeseries | 16–32C | 64 GB | Nicht nötig | Nicht nötig | 4× NVMe Gen5 RAID-0 |
| Write-Heavy | 16–32C | 64 GB | Nicht nötig | Nicht nötig | 4× NVMe Gen4 RAID-0 |
| Stable Diffusion | 8–16C | 32 GB | H100 SXM5 / A100 | **80 GB** | NVMe Gen3 |
| Mixed Polyglot | 64C EPYC Genoa | 256 GB | RTX 4090 | 24 GB | 2× NVMe Gen4 |

---

## 11. Granulierte Skalierungsstufen

> 12 Stufen von 50 bis 500K Connections. GPU-Compute und VRAM separat.
> Basis: Mixed-Workload (50 % Reads, 30 % Writes, 20 % AQL/Vector).

| Stufe | Connections | Cores | RAM | GPU Compute | VRAM | NVMe | Topologie | Engpass |
|-------|------------|-------|-----|------------|------|------|-----------|---------|
| S-01 | 50 | 2–4C | 4–8 GB | – | – | 256 GB SATA | 1 Knoten | Keiner |
| S-02 | 100–200 | 4–8C | 8–16 GB | – | – | 500 GB NVMe Gen3 | 1 Knoten | Keiner |
| S-03 | 200–500 | 8C | 16–32 GB | optional | opt. 16 GB | 1 TB NVMe Gen3 | 1 Knoten | Compaction-Bursts |
| S-04 | 500–1K | 8–16C | 32–64 GB | opt. 22 TFLOPS | opt. 16 GB | 2 TB NVMe Gen3/4 | 1 Knoten | CPU (AQL) |
| S-05 | 1K–2K | 16C | 64 GB | 82 TFLOPS empf. | 24 GB | 2× 2 TB NVMe Gen4 | 1 Knoten | NVMe Compaction |
| S-06 | 2K–5K | 16–32C | 128 GB | 82 TFLOPS | 24 GB | 2× 4 TB NVMe Gen4 | 1 Knoten | CPU + NVMe |
| S-07 | 5K–10K | 32C | 256 GB | 77 TFLOPS | 40 GB | 2× 4 TB NVMe Gen4 RAID-1 | 1–2 Knoten | CPU (TBB) |
| S-08 | 10K–20K | 64C | 256 GB | 312 TFLOPS | 80 GB | 4× 4 TB NVMe Gen4 | 2–3 Knoten | NVMe Array |
| S-09 | 20K–50K | 64C × 3 | 256 GB × 3 | 312 TFLOPS × 1–2 | 80–160 GB | 2× NVMe Gen4 × 3 | 3–5 Knoten | Sharding erforderlich |
| S-10 | 50K–100K | 64C × 10 | 256 GB × 10 | 312 TFLOPS × 4 | 320 GB | 2× NVMe Gen4 × 10 | 10–20 Knoten | Cluster-Koordination |
| S-11 | 100K–500K | 64C × 50 | 256 GB × 50 | 989 TFLOPS × 10–20 | 800 GB–1,6 TB | NVMe Gen4 × 50 | 50–100 Knoten | Netzwerk (100 GbE) |
| S-12 | > 500K | 64C × 200+ | 256 GB × 200+ | H100/MI300X × N | N × 80–192 GB | NVMe Gen5 × 200+ | 200+ Knoten | Gateway-Tier zwingend |

### Kritische Übergangspunkte

| Übergang | Was ändert sich | Konfigurationsänderung |
|---------|----------------|----------------------|
| S-02 → S-03 | Compaction spürbar | `rate_limiter_bytes_per_sec = 100MB` |
| S-04 → S-05 | CPU-dominiert bei AQL | 16C+, `tbb.max_parallelism = 12` |
| S-05 → S-06 | VRAM-Engpass bei LLM | RTX 4090 (24 GB) obligatorisch |
| S-06 → S-07 | NVMe + CPU saturiert | NVMe Gen4, io_uring aktivieren |
| S-07 → S-08 | Einzelknoten-Limit | Sharding aktivieren (Raft, 3 Knoten) |
| S-08 → S-09 | VRAM für Enterprise LLM | A100 80G SXM; Tensor-Parallel prüfen |
| S-09 → S-10 | Cluster-Koordinationsaufwand | Gossip-Intervall: 50 ms; 100 GbE |
| S-10 → S-11 | Netzwerk Engpass | 100 GbE + RDMA (RoCEv2) |
| S-11 → S-12 | ThemisDB allein nicht skalierbar | Gateway-Tier (MQTT/WebSocket) |

---

## 12. Upgrade-Pfad-Empfehlungen

### 12.1 Upgrade-Entscheidungsbaum

```
Symptom: Latenz steigt oder Throughput sinkt
├── CPU-Auslastung > 80 % sustained?
│   ├── Ja: +Cores (32→64) oder +Shards
│   └── Nein → weiter
│
├── RAM < 20 % frei oder Block-Cache Hit-Rate < 90 %?
│   ├── Ja: +RAM (verdoppeln) oder +Shards (> 1 TB Hot-Dataset)
│   └── Nein → weiter
│
├── NVMe > 70 % oder Write-Stall > 0?
│   ├── Ja: Gen3→Gen4, kUniversal Compaction, io_uring
│   └── Nein → weiter
│
├── GPU-Auslastung > 90 % + VRAM < 80 %?
│   ├── Ja: GPU-Compute-Upgrade (RTX 4090 → A100 / MI300X)
│   └── Nein → weiter
│
├── VRAM > 90 % oder LLM-Sessions in Warteschlange?
│   ├── Ja: VRAM-Upgrade (A100 80G / MI300X) oder Multi-GPU
│   └── Nein → weiter
│
├── PCIe > 80 % (GPU↔CPU, NVMe)?
│   ├── Ja: PCIe Gen5 Board, NVLink für Multi-GPU
│   └── Nein → weiter
│
└── Netzwerk > 80 % oder Inter-Shard-Latenz hoch?
    └── Ja: 25 GbE → 100 GbE, RDMA (RoCEv2)
```

### 12.2 Upgrade-Sequenz nach Kosten-Nutzen

| Prio | Upgrade | Kosten (ca.) | Erwarteter Gewinn | Wann sinnvoll |
|------|---------|------------|------------------|--------------||
| 1 | RAM verdoppeln | 500–2.000 € | +30–80 % Throughput | Cache Hit-Rate < 90 % |
| 2 | NVMe Gen3 → Gen4 | 200–800 €/TB | +50 % Write-Throughput | App-Write > 30 MB/s |
| 3 | CPU-Cores verdoppeln | 1.000–5.000 € | +40–70 % CPU-Throughput | CPU > 70 % sustained |
| 4 | 2× NVMe Gen4 RAID-0 | 400–1.600 € | +100 % NVMe Bandbreite | NVMe saturiert |
| 5 | GPU: RTX 4090 | ~1.800 € | +4× LLM-Speed, +2× VRAM | LLM p99 > 2 s |
| 6 | VRAM: A100 80G SXM | ~14.000 € | +5× concurrent Sessions | VRAM > 85 %, Sessions queued |
| 7 | VRAM: AMD MI300X | ~20.000 € | Llama-70B FP16 auf 1 Karte | 70B-FP16 benötigt |
| 8 | +1 Shard (Horizontal) | 5.000–20.000 € | Linear: CPU + RAM + NVMe | Einzelknoten-Limit erreicht |
| 9 | PCIe Gen5 Plattform | 3.000–10.000 € | +2× PCIe Bandwidth | NVLink-less Multi-GPU |
| 10 | H100 NVLink-Cluster | 30.000–120.000 € | +56× GPU-GPU Bandwidth | Tensor-Parallel ab 70B-FP16 |

### 12.3 Hardware-Generations-Wechsel

| Von → Nach | Grund | ThemisDB-Vorteil |
|-----------|-------|-----------------|
| DDR4 → DDR5 8-Channel | +5× Memory Bandwidth | Block-Cache Reads massiv schneller |
| PCIe Gen4 → Gen5 | +2× Bandwidth | NVMe Gen5 + H100 voll auslasten |
| NVMe Gen3 → Gen4 | +2× IOPS | Write-Stall um > 90 % reduziert |
| RTX 4090 → A100 80G | +3× VRAM, ECC | > 20 concurrent LLM-Sessions |
| A100 → AMD MI300X | +2,4× VRAM, +4× Bandwidth | Llama-70B FP16 auf einer Karte |
| A100 → H100 SXM5 | +3× Compute, NVLink v4 | Tensor-Parallel-Cluster ohne PCIe |

---

## 13. Fazit

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

*Erstellt: 2026-04-17 | Version: 1.1 | Maintainer: ThemisDB Engineering*
