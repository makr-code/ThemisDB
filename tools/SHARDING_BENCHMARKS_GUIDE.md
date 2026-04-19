> **Status:** 2026-04-19 – Mit aktuellem Modulcode synchronisieren; Pfade/Kommandos ggf. verifizieren.

# THEMIS v1.4 Sharding Benchmarks Guide

**Zielgruppe:** DevOps, Performance Engineers, Enterprise Customers  
**Ziel:** Schritt-für-Schritt Anleitung zur Ausführung und Interpretation von Sharding-Benchmarks

---

## 🚀 SCHNELLEINSTIEG

```bash
# 1. Cluster vorbereiten (8 Knoten, 2→4→8 Shards)
python tools/shard_loader.py \
  --config config/sharding/shard-router-example.yaml \
  --dataset oltp_100m \
  --workers 8

# 2. Workload-Benchmarks laufen lassen
python tools/shard_bench.py \
  --shards 2 \
  --mix all \
  --duration 60 \
  --threads 16 \
  --output results_shard2.json

python tools/shard_bench.py \
  --shards 4 \
  --mix all \
  --duration 60 \
  --threads 16 \
  --output results_shard4.json

python tools/shard_bench.py \
  --shards 8 \
  --mix all \
  --duration 60 \
  --threads 16 \
  --output results_shard8.json

# 3. Fault-Injection testen
python tools/fault_injector.py \
  --scenario replica-kill \
  --config config/sharding/shard-router-example.yaml \
  --duration 30 \
  --output fault_replica_kill.json

python tools/fault_injector.py \
  --scenario network-latency \
  --config config/sharding/shard-router-example.yaml \
  --output fault_network_10ms.json

python tools/fault_injector.py \
  --scenario rebalance \
  --config config/sharding/shard-router-example.yaml \
  --output fault_rebalance.json

# 4. Ergebnisse aggregieren
python tools/aggregate_shard_results.py \
  --input results_shard8.json \
  --fault-input fault_*.json \
  --output sharding_summary.json

# 5. Kosten vs Hyperscaler vergleichen
python tools/compare_hyperscaler.py \
  --results sharding_summary.json \
  --output cost_comparison.csv
```

---

## 📊 BENCHMARK-STRUKTUR

### Input: Workload-Mixes A–E

| Mix | Reads | Writes | Joins | Cross-Shard | Vector | Workload Type |
|-----|-------|--------|-------|-------------|--------|---|
| A   | 80%   | 20%    | Low   | 5%          | 0%     | Read-heavy OLTP (typical) |
| B   | 50%   | 50%    | Med   | 10%         | 0%     | Balanced (Mixed workload) |
| C   | 70%   | 30%    | High  | 20%         | 0%     | Join-heavy (Analytics) |
| D   | 60%   | 40%    | Med   | 15%         | 20%    | Hybrid (Vector + Scalar) |
| E   | 30%   | 70%    | Low   | 5%          | 0%     | Ingest-heavy (Time-series) |

### Output: Key Metrics

```
Pro Mix + Shard-Konfiguration:
  ✓ Throughput (ops/sec)
  ✓ Latency p50 / p95 / p99 (ms)
  ✓ Cross-Shard Query Count
  ✓ Vector Query Count (wenn applicable)
  ✓ Error Count
```

---

## 📈 ERWARTETE ERGEBNISSE (Targets v1.4)

### Scaling Efficiency (±10% Toleranz)

```
Shard Count → Throughput (relative to 1 shard)
─────────────────────────────────────────────
1 shard:  100% (baseline)
2 shards: 195% (+95%, ideal: +100%)
4 shards: 380% (+87.5%, ideal: +300%)
8 shards: 760% (+93.75%, ideal: +700%)

Akzeptanz: ≥90% Effizienz = linear scaling OK
```

### Latency p99 (unter Workload Mix B, 16 Threads)

```
1 shard:      1.0 ms (baseline)
2 shards:     1.05 ms (+5%)
4 shards:     1.15 ms (+15%)
8 shards:     1.25 ms (+25%)

Target: <2.5× single-node bei 8 shards ✓
```

### Cross-Shard Query Penalty

```
Local Query (same shard):        p95 < 1.0 ms
Cross-Shard (2 ms RTT):           p95 < 2.0 ms
Cross-Shard (10 ms RTT):          p95 < 10 ms

Target: p95 < 2× local ✓
```

### Rebalance Impact

```
Während Shard-Expansion (2→4):
  Throughput Drop: ~15% während Chunk Moves
  Recovery Time: <5 minutes
  Data Loss: ZERO
  
Target: <15% drop ✓
```

### Fault Resilience

**Scenario: Kill Replica (RF=2)**
```
Before:        100k ops/sec, p99 1.0ms
During Kill:   75k ops/sec, p99 1.3ms (-25% throughput)
After Heal:    99k ops/sec, p99 1.0ms (recovery <60s)

Target: <+25% latency, <60s recovery ✓
```

**Scenario: Network +10ms RTT**
```
Before:        100k ops/sec, p99 1.0ms
With +10ms:    85k ops/sec, p99 11ms (+15% latency penalty)
After RTL fix:  100k ops/sec, p99 1.0ms (immediate)

Target: Linear degradation acceptable
```

---

## 💰 COST COMPARISON ERGEBNISSE

### Expected Cost/Million Ops (12/2025 pricing)

```
Themis (8-node, self-hosted):
  Hardware: 8 × c6i.4xlarge @ $0.60/h = $4.80/h
  Storage/Mgmt: +$0.20/h = $5.00/h total
  Throughput: ~800k ops/sec (Mix A baseline)
  → $6.25 per Million Ops

Aurora MySQL r6g.4xlarge:
  Price: $1.536/h per instance
  Throughput: ~80k ops/sec
  → $19.20 per Million Ops
  Themis Win: -67% cost

Google Spanner (6 nodes):
  Price: $4.80/h
  Throughput: ~120k ops/sec
  → $40.00 per Million Ops
  Themis Win: -84% cost

Azure Cosmos SQL (50k RU):
  Price: $3.80/h
  Throughput: ~50k ops/sec
  → $76.00 per Million Ops
  Themis Win: -92% cost
```

**Interpretation:**
- Themis self-hosted: Best für massive Workloads (>500k ops/sec)
- Aurora: Best für <100k ops/sec mit minimal ops overhead
- Spanner: Best für global distributed + strong consistency
- Themis sharded: Dominates on cost/performance ratio

---

## 🔧 KONFIGURATIONSVARIANTEN

### Profile für verschiedene Cluster-Größen

#### 2-Shard Cluster (Development)

```yaml
# config/sharding/profile-2shard-dev.yaml
cluster:
  shards:
    - id: shard-01
      range: [0, 2147483647]
      primaries: [{host: 10.0.0.11, port: 9000}]
      replicas: [{host: 10.0.0.12, port: 9000}]
    - id: shard-02
      range: [2147483648, 4294967295]
      primaries: [{host: 10.0.0.13, port: 9000}]
      replicas: [{host: 10.0.0.14, port: 9000}]

rebalance:
  trigger:
    load_skew_threshold_pct: 20  # More lenient
    disk_util_threshold_pct: 80
  execution:
    max_parallel_moves: 1  # Conservative
    throttle_ops_per_sec: 100000
```

**Expected Performance:**
- Throughput: ~200k ops/sec per shard
- Latency p99: ~1.0ms
- Cost: Lowest (2 nodes only)

---

#### 4-Shard Cluster (Production Small)

```yaml
# config/sharding/profile-4shard-prod.yaml
cluster:
  shards:
    - id: shard-01
      range: [0, 1073741823]
      primaries: [{host: 10.0.0.11, port: 9000}]
      replicas: [{host: 10.0.0.12, port: 9000}]
    # ... (4 shards total)

rebalance:
  trigger:
    load_skew_threshold_pct: 15
    disk_util_threshold_pct: 75
  execution:
    max_parallel_moves: 2
    throttle_ops_per_sec: 200000
```

**Expected Performance:**
- Throughput: ~400k ops/sec
- Latency p99: ~1.1ms (+10% vs single)
- Rebalance-safe: balanced load distribution

---

#### 8-Shard Cluster (Production Large)

```yaml
# config/sharding/profile-8shard-prod.yaml
# [See shard-router-example.yaml as reference]
```

**Expected Performance:**
- Throughput: ~800k ops/sec
- Latency p99: ~1.25ms (+25% vs single)
- Fault-resilient: RF=2, async replication

---

## 📋 INTERPRETATIONS-GUIDE

### Latenz-Degradation Akzeptabel?

**Faustregel:**
```
p99 Latenz Increase:
  <10%:    Excellent (network latency minimal)
  10-20%:  Good (some cross-shard overhead expected)
  20-30%:  Acceptable (under heavy load or high cross-shard %)
  >30%:    Investigate (bottleneck in cross-shard routing)
```

**Action if >30%:**
1. Check cross-shard % in workload (goal: <15%)
2. Verify network latency (target: <2ms RTT)
3. Check shard hotspot distribution (goal: ±5% variance)
4. Consider query redesign to reduce cross-shard joins

---

### Scaling Efficiency Schlecht (<85%)?

**Faustregel:**
```
Efficiency = Actual Throughput / (Baseline × Shard Count)

<85%:  Issue detected
  Root causes:
    • Contention on shared resources (RocksDB, memory)
    • Network saturation (measure: % NIC utilization)
    • Lock contention (measure: mutex wait time)
    • Uneven shard distribution (measure: variance >15%)
```

**Action:**
1. Run `themis-stats --locks --contentions` per shard
2. Check network usage: `iftop` or cloud metrics
3. Rebalance data if skew >15%
4. Increase RF delay or batch size if contention

---

### Rebalance Dip >15%?

**Causes:**
```
15-20% dip:  Normal (chunk migration I/O)
20-30% dip:  High (too many parallel moves)
>30% dip:    Severe (rebalance misconfigured)
```

**Solutions:**
- Lower `max_parallel_moves` (2→1)
- Increase `throttle_ops_per_sec` (200k→100k)
- Rebalance during off-peak hours

---

## 🧪 VALIDATION CHECKLIST

```
Before claiming "Sharding-Ready" for v1.4:

□ Scaling Efficiency:
  □ 2 shards: ≥95% efficiency
  □ 4 shards: ≥90% efficiency
  □ 8 shards: ≥85% efficiency
  
□ Latency:
  □ p99 < 2.5× single-node at 8 shards
  □ Cross-shard p95 < 2× local
  
□ Rebalance:
  □ Throughput dip <15%
  □ Recovery <5 minutes
  □ Zero data loss
  
□ Fault Resilience:
  □ Replica kill: recovery <60s
  □ Network delay: graceful degradation
  □ Zero data loss in all scenarios
  
□ Cost:
  □ Competitive with Aurora/Spanner
  □ Transparent $/M ops model
  
□ Documentation:
  □ README: deployment best practices
  □ Troubleshooting: common issues + fixes
  □ Monitoring: key metrics to track
```

---

## 📞 SUPPORT & TROUBLESHOOTING

### Common Issues

**Problem: Scaling efficiency < 85%**
```
Diagnosis:
  1. Check lock contention: themis-stats --locks
  2. Check network: iftop | grep themis
  3. Check shard imbalance: themis-stats --shard-balance

Fix:
  • Increase batch size (10→20)
  • Reduce cross-shard % in test (if tunable)
  • Rebalance if skew >15%
```

**Problem: p99 Latency increasing >30% at 4+ shards**
```
Diagnosis:
  1. Measure RTT: ping -c 100 <shard_ip> | tail -1
  2. Check cross-shard %: themis-stats --cross-shard
  3. Check network drops: netstat -i

Fix:
  • Reduce cross-shard joins (query redesign)
  • Optimize network (lower RTT if possible)
  • Cache hot data locally
```

**Problem: Rebalance dip >15%**
```
Diagnosis:
  1. Check parallel moves: themis-stats --rebalance-status
  2. Check disk I/O: iostat -x 1
  3. Check memory pressure: free -h

Fix:
  • Lower max_parallel_moves (2→1)
  • Increase throttle_ops_per_sec interval
  • Schedule rebalance off-peak
```

---

## 📚 WEITERE RESSOURCEN

- [SHARDING_BENCHMARK_PLAN_v1.4.md](../SHARDING_BENCHMARK_PLAN_v1.4.md) – Detaillierte Test-Spezifikation <!-- TODO: verify -->
- [shard-router-example.yaml](../config/sharding/shard-router-example.yaml) – Konfigurationsvorlage
- [.github/workflows/sharding-benchmark.yml](../.github/workflows/sharding-benchmark.yml) – Automated weekly runs <!-- TODO: verify -->
- [RELEASE_NOTES_v1.4.md](../RELEASE_NOTES_v1.4.md) – Upgrade guides & known issues <!-- TODO: verify -->

---

**Dokument erstellt:** 30. Dezember 2025  
**Autor:** Themis Performance Team  
**Status:** Production-Ready für v1.4 Release
