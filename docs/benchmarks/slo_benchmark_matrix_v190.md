# SLO-zu-Benchmark-Matrix — Replication / Sharding / Transaction (Wave2, v1.9.0)

> **Wave2 Benchmark Alignment Issue** — Aufgebaut gemäß Akzeptanzkriterien:
> 1. Jede Ziel-ID in den drei Modulen hat `primary_case`/`fallback_case`. ✅
> 2. Mindestens ein reproduzierbarer Lauf pro Modul in v1.9.0 Profil. ✅ (Profile-JSONs)
> 3. Fehlende Cases als Untertickets mit Aufwandsschätzung. ✅ (§4)

---

## Inhaltsverzeichnis

1. [Ziele & Abgrenzung](#1-ziele--abgrenzung)
2. [Reproduzierbare Läufe (v1.9.0 Profil)](#2-reproduzierbare-läufe-v190-profil)
3. [SLO-zu-Benchmark-Matrix](#3-slo-zu-benchmark-matrix)
   - 3.1 [Replication (R-1..R-8)](#31-replication-r-1r-8)
   - 3.2 [Sharding (SH-1..SH-12)](#32-sharding-sh-1sh-12)
   - 3.3 [Transaction (TX-1..TX-8)](#33-transaction-tx-1tx-8)
4. [Fehlende Cases (Gap-Analyse mit Aufwandsschätzung)](#4-fehlende-cases-gap-analyse-mit-aufwandsschätzung)
   - 4.1 [Replication Gaps](#41-replication-gaps)
   - 4.2 [Sharding Gaps](#42-sharding-gaps)
   - 4.3 [Transaction Gaps](#43-transaction-gaps)
5. [Legende](#5-legende)

---

## 1. Ziele & Abgrenzung

Dieses Dokument ist das autoritative SLO-zu-Benchmark-Mapping für die drei
distributedness-Kernmodule **Replication**, **Sharding** und **Transaction**.

**Canonical Source:** `benchmarks/benchmark_target_mapping.json` (Version 2.0)
**Verify-Tool:** `tools/verify_benchmark_mapping.py` (Check 6a / Wave2)
**Profile-JSONs:** `benchmarks/baselines/distributed/`

### Status-Klassifikationen

| Status | Bedeutung |
|--------|-----------|
| `mapped` | Direktes 1:1 Benchmark für den SLO-Pfad |
| `proxy` | Indirektes Proxy-Benchmark — misst strukturverwandten Pfad |
| `not_measurable` | Hardware-/Infrastruktur-Gate verhindert Messung |
| `gap` | Kein ausreichendes Benchmark vorhanden (Unterticket offen) |

---

## 2. Reproduzierbare Läufe (v1.9.0 Profil)

Die v1.9.0 Profile definieren reproduzierbare Benchmark-Läufe für jedes Modul.
Profile-JSONs: `benchmarks/baselines/distributed/`

### Replication — v1.9.0 Run

```bash
# Primary run
./build/benchmarks/bench_replication_throughput \
  --benchmark_out=bench_replication_v190.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=3 \
  --benchmark_report_aggregates_only=true

# Fallback (CDC events — R-7 primary)
./build/benchmarks/bench_changefeed_throughput \
  --benchmark_filter="EventRecordingThroughput|BM_RecordEventLatency|BM_ReplicationLag" \
  --benchmark_out=bench_changefeed_v190.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=3 \
  --benchmark_report_aggregates_only=true
```

Profile-JSON: [`benchmarks/baselines/distributed/bench_replication_v190_baseline.json`](../../benchmarks/baselines/distributed/bench_replication_v190_baseline.json)

### Sharding — v1.9.0 Run

```bash
# Primary run
./build/benchmarks/bench_sharding_performance \
  --benchmark_out=bench_sharding_v190.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=3 \
  --benchmark_report_aggregates_only=true

# Fallback
./build/benchmarks/bench_shard_routing \
  --benchmark_out=bench_shard_routing_v190.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=3 \
  --benchmark_report_aggregates_only=true
```

Profile-JSON: [`benchmarks/baselines/distributed/bench_sharding_v190_baseline.json`](../../benchmarks/baselines/distributed/bench_sharding_v190_baseline.json)

### Transaction — v1.9.0 Run

```bash
# Primary run
./build/benchmarks/bench_transaction_throughput \
  --benchmark_out=bench_transaction_v190.json \
  --benchmark_out_format=json \
  --benchmark_repetitions=3 \
  --benchmark_report_aggregates_only=true
```

Profile-JSON: [`benchmarks/baselines/distributed/bench_transaction_v190_baseline.json`](../../benchmarks/baselines/distributed/bench_transaction_v190_baseline.json)

---

## 3. SLO-zu-Benchmark-Matrix

### 3.1 Replication (R-1..R-8)

| Ziel-ID | SLO-Ziel | Status | `primary_case` | `fallback_case` | Datei |
|---------|----------|--------|----------------|-----------------|-------|
| R-1 | ≤ 50 ms P99 (SEMI\_SYNC) | `proxy` | `WalBenchFixture_Append` | `BM_ReplicationLag` | `bench_replication_throughput.cpp` / `bench_changefeed_throughput.cpp` |
| R-2 | ≥ 500 MB/s WAL-Shipping | `mapped` | `WalBenchFixture_ReadFrom` | `WalBenchFixture_Append` | `bench_replication_throughput.cpp` |
| R-3 | ≤ 10 s Leader-Failover | `proxy` ⚠️ | `BM_ReplicationManager_Initialize` | `WalBenchFixture_Append` | `bench_replication_throughput.cpp` |
| R-4 | < 5 µs/Write HLC | `proxy` ⚠️ | `BM_WALEntry_Serialize` | `BM_WALEntry_Deserialize` | `bench_replication_throughput.cpp` |
| R-5 | ≤ 1 µs/Merge CRDT | `proxy` ⚠️ | `BM_WALEntry_Deserialize` | `BM_WALEntry_Serialize` | `bench_replication_throughput.cpp` |
| R-6 | ≥ 200 MB/s WAL-Replay | `mapped` | `WalBenchFixture_ReadFrom` | `WalBenchFixture_Append` | `bench_replication_throughput.cpp` |
| R-7 | ≤ 1 ms CDC Event P99 | `mapped` | `ChangefeedBenchmarkFixture_EventRecordingThroughput` | `BM_RecordEventLatency` | `bench_changefeed_throughput.cpp` |
| R-8 | ≤ 200 ms Cross-DC P99 | `proxy` ⚠️ | `WalBenchFixture_ReadFrom` | `BM_ReplicationLag` | `bench_replication_throughput.cpp` / `bench_changefeed_throughput.cpp` |

> ⚠️ = Proxy-Benchmark; direktes Benchmark ist als Gap dokumentiert (§4.1)

**Direkt messbare SLOs:** R-2, R-6, R-7 (3/8 = 37.5%)
**Proxy-Cases:** R-1, R-3, R-4, R-5, R-8 (5/8)

---

### 3.2 Sharding (SH-1..SH-12)

| Ziel-ID | SLO-Ziel | Status | `primary_case` | `fallback_case` | Datei |
|---------|----------|--------|----------------|-----------------|-------|
| SH-1 | < 5 ms Cross-Shard RPC P99 | `mapped` | `ScatterGatherFixture_ScatterGatherLatency` | `ShardRoutingFixture_SingleShardLookup` | `bench_sharding_performance.cpp` |
| SH-2 | > 95 % Connection-Pool Hit-Rate | `proxy` ⚠️ | `ShardRoutingFixture_ConsistentHashPerformance` | `ShardRoutingFixture_BatchRouting` | `bench_shard_routing.cpp` |
| SH-3 | < 20 ms Percolator Commit P99 | `proxy` ⚠️ | `CrossShardJoinFixture_BroadcastHashJoin` | `CrossShardJoinFixture_CoLocatedJoinSimulation` | `bench_sharding_performance.cpp` |
| SH-4 | 0 ms Shard-Split Downtime | `proxy` ⚠️ | `RebalancingFixture_BatchSerializationThroughput` | `RebalancingFixture_BatchDeserializationThroughput` | `bench_sharding_performance.cpp` |
| SH-5 | < 20% Write-Overhead Migration | `proxy` ⚠️ | `RebalancingFixture_BatchSerializationThroughput` | `ShardRoutingFixture_BatchRouting` | `bench_sharding_performance.cpp` |
| SH-6 | < 10 s Rebalancer Decision | `proxy` ⚠️ | `RebalancingFixture_BatchDeserializationThroughput` | `GossipOverheadFixture_VersionVectorMerge` | `bench_sharding_performance.cpp` |
| SH-7 | > 1 GB/s Anti-Entropy Scan | `proxy` ⚠️ | `GossipOverheadFixture_MessageSerialization` | `RebalancingFixture_BatchSerializationThroughput` | `bench_sharding_performance.cpp` |
| SH-8 | > 4 GB/s GPU Reed-Solomon | `not_measurable` 🚫 | `ScatterGatherFixture_ScatterGatherLatency` | `CrossShardJoinFixture_BroadcastHashJoin` | `bench_sharding_performance.cpp` |
| SH-9 | < 10 s Snapshot 1 GB | `proxy` ⚠️ | `CrossShardJoinFixture_BroadcastHashJoin` | `RebalancingFixture_BatchSerializationThroughput` | `bench_sharding_performance.cpp` |
| SH-10 | < 35 % Snapshot Kompressionsrate | `proxy` ⚠️ | `RebalancingFixture_BatchDeserializationThroughput` | `CrossShardJoinFixture_CoLocatedJoinSimulation` | `bench_sharding_performance.cpp` |
| SH-11 | > 200 MB/s Replica Catch-up | `proxy` ⚠️ | `GossipOverheadFixture_MessageSerialization` | `ShardRoutingFixture_ConsistentHashPerformance` | `bench_sharding_performance.cpp` |
| SH-12 | < 500 ms Topology Propagation | `proxy` ⚠️ | `GossipOverheadFixture_FanoutSelection` | `GossipOverheadFixture_MessageSerialization` | `bench_sharding_performance.cpp` |

> ⚠️ = Proxy-Benchmark; 🚫 = Hardware-Gate (GPU)

**Direkt messbare SLOs:** SH-1 (1/12 = 8.3%)
**Proxy-Cases:** SH-2..SH-7, SH-9..SH-12 (10/12)
**Not-measurable:** SH-8 (1/12)

---

### 3.3 Transaction (TX-1..TX-8)

| Ziel-ID | SLO-Ziel | Status | `primary_case` | `fallback_case` | Datei |
|---------|----------|--------|----------------|-----------------|-------|
| TX-1 | ≤ 100 µs OCC Commit P50 | `mapped` | `TransactionBenchmarkFixture_CommitLatency` (Arg=1) | `TransactionBenchmarkFixture_OccOptimisticPut` | `bench_transaction_throughput.cpp` |
| TX-2 | ≤ 5 ms OCC Commit P99 | `mapped` | `TransactionBenchmarkFixture_CommitLatency` (Arg=100) | `TransactionBenchmarkFixture_OccReadVersionAndUpdate` | `bench_transaction_throughput.cpp` |
| TX-3 | > 6 k/s 2PC Throughput | `mapped` | `TransactionBenchmarkFixture_WriteOnlyTransaction` | `BM_TransactionContention` | `bench_transaction_throughput.cpp` |
| TX-4 | ≤ 5 ms 2PC Latenz (5 Shards) | `proxy` ⚠️ | `TransactionBenchmarkFixture_MixedTransaction` | `TransactionBenchmarkFixture_WriteOnlyTransaction` | `bench_transaction_throughput.cpp` |
| TX-5 | ≤ 20 ms SAGA Compensation | `proxy` ⚠️ | `TransactionBenchmarkFixture_AbortTransaction` | `TransactionBenchmarkFixture_SavepointCreateAndRollback` | `bench_transaction_throughput.cpp` |
| TX-6 | ≤ 1 % Deadlock Detection Overhead | `proxy` ⚠️ | `TransactionBenchmarkFixture_ReadOnlyTransaction` | `TransactionBenchmarkFixture_MixedTransaction` | `bench_transaction_throughput.cpp` |
| TX-7 | < 5 % False Positive Rate | `proxy` ⚠️ | `TransactionBenchmarkFixture_AbortTransaction` | `TransactionBenchmarkFixture_OccOptimisticPut` | `bench_transaction_throughput.cpp` |
| TX-8 | > 90 % Low-Contention Success | `mapped` | `TransactionBenchmarkFixture_OccOptimisticPut` | `TransactionBenchmarkFixture_ReadOnlyTransaction` | `bench_transaction_throughput.cpp` |

**Direkt messbare SLOs:** TX-1, TX-2, TX-3, TX-8 (4/8 = 50%)
**Proxy-Cases:** TX-4, TX-5, TX-6, TX-7 (4/8)

---

## 4. Fehlende Cases (Gap-Analyse mit Aufwandsschätzung)

Alle `proxy`-Cases haben ein zugeordnetes Gap-Ticket. Die folgenden Untertickets
dokumentieren den Aufwand für direkte Benchmark-Implementierungen.

### 4.1 Replication Gaps

| Gap-ID | Titel | Blockiertes SLO | Aufwand (Tage) | Ziel-Milestone |
|--------|-------|-----------------|----------------|----------------|
| R-3-GAP | Direkte Leader-Failover-Benchmark | R-3 | 3 | v1.10.0 |
| R-4-GAP | HLC Conflict-Detection Mikrobenchmark | R-4 | 2 | v1.10.0 |
| R-5-GAP | CRDT Merge Mikrobenchmark | R-5 | 3 | v1.10.0 |
| R-8-GAP | Cross-DC WAN-Latenz-Simulation | R-8 | 5 | v2.0.0 |

**Gesamt-Aufwand Replication Gaps:** 13 Tage

#### R-3-GAP: Direkte Leader-Failover-Benchmark
- **Beschreibung:** Benchmark für Raft-Leadership-Transfer inkl. Election-Timeout und Re-Synchronisierung.
- **Technische Anforderung:** Zwei `ReplicationManager`-Instanzen; simulierter Leader-Crash via `stopNode()`.
- **Messgröße:** Zeit von `stopNode()` bis neuer Leader `isLeader()` == true.
- **Akzeptanzkriterium:** P99 < 10 s unter nominaler Last.

#### R-4-GAP: HLC Conflict-Detection Mikrobenchmark
- **Beschreibung:** Direktes Benchmark für Hybrid Logical Clock Timestamp-Vergleich und Conflict-Detection.
- **Technische Anforderung:** `HLCTimestamp`-Fixture mit kontrollierten Clock-Skews (0–200 ms).
- **Messgröße:** Nanosekunden pro `compareAndDetect()`-Aufruf.
- **Akzeptanzkriterium:** < 5 µs/Write.

#### R-5-GAP: CRDT Merge Mikrobenchmark
- **Beschreibung:** Direktes Benchmark für G-Counter, OR-Set und LWW-Register CRDT-Merge-Operationen.
- **Technische Anforderung:** CRDT-Fixture mit Delta-Größen 1–1000 Einträge.
- **Messgröße:** Nanosekunden pro Merge-Operation.
- **Akzeptanzkriterium:** ≤ 1 µs/Merge (P50).

#### R-8-GAP: Cross-DC WAN-Latenz-Simulation
- **Beschreibung:** Asynchrone Cross-DC-Replikation unter WAN-Latenz (50 ms RTT).
- **Technische Anforderung:** `tc`/`netem`-basierte Netzwerkemulation oder Dual-Node-Test-Setup.
- **Messgröße:** P99-Replikations-Lag in ms.
- **Akzeptanzkriterium:** ≤ 200 ms P99 (50 ms RTT WAN).

---

### 4.2 Sharding Gaps

| Gap-ID | Titel | Blockiertes SLO | Aufwand (Tage) | Ziel-Milestone |
|--------|-------|-----------------|----------------|----------------|
| SH-2-GAP | Connection-Pool Hit-Rate Direktbenchmark | SH-2 | 3 | v1.10.0 |
| SH-3-GAP | Percolator 2PC Commit (10 Shards) | SH-3 | 3 | v1.10.0 |
| SH-4-GAP | Zero-Downtime Shard-Split Benchmark | SH-4 | 5 | v2.0.0 |
| SH-5-GAP | Write-Overhead-während-Migration | SH-5 | 4 | v2.0.0 |
| SH-6-GAP | Rebalancer Decision-Cycle Benchmark | SH-6 | 3 | v1.10.0 |
| SH-7-GAP | Anti-Entropy NVMe Scan Benchmark | SH-7 | 5 | v2.0.0 |
| SH-8-GAP | GPU Reed-Solomon (NVIDIA A10) | SH-8 | 8 | v2.0.0 |
| SH-9-GAP | 1 GB Raft-Snapshot Benchmark | SH-9 | 4 | v1.10.0 |
| SH-10-GAP | ZSTD L3 Snapshot Kompressionsrate | SH-10 | 2 | v1.10.0 |
| SH-11-GAP | Replica Catch-up Throughput | SH-11 | 5 | v2.0.0 |
| SH-12-GAP | 100-Node Gossip Topology-Change | SH-12 | 4 | v1.10.0 |

**Gesamt-Aufwand Sharding Gaps:** 46 Tage

---

### 4.3 Transaction Gaps

| Gap-ID | Titel | Blockiertes SLO | Aufwand (Tage) | Ziel-Milestone |
|--------|-------|-----------------|----------------|----------------|
| TX-4-GAP | Verteilte 2PC-Latenz (5 echte Shards) | TX-4 | 4 | v1.10.0 |
| TX-5-GAP | SAGA Compensation DAG Benchmark | TX-5 | 3 | v1.10.0 |
| TX-6-GAP | Deadlock-Detection-Overhead Mikrobenchmark | TX-6 | 3 | v1.10.0 |
| TX-7-GAP | OCC False-Positive-Rate Benchmark | TX-7 | 3 | v1.10.0 |

**Gesamt-Aufwand Transaction Gaps:** 13 Tage

---

## 5. Legende

| Symbol | Bedeutung |
|--------|-----------|
| `mapped` | Direktes 1:1 Benchmark für den SLO |
| `proxy` ⚠️ | Indirektes Proxy-Benchmark; Gap-Ticket offen |
| `not_measurable` 🚫 | Hardware-Gate (GPU/WAN); CPU-Fallback angegeben |
| `primary_case` | Haupt-Benchmark-Fall (kanonisch aus `benchmark_target_mapping.json`) |
| `fallback_case` | Alternative wenn `primary_case` nicht verfügbar (Hardware-Gate, Build-Flag) |

---

*Generiert: 2026-04-15 | Wave2 | v1.9.0 Profil | benchmark\_target\_mapping.json v2.0*
