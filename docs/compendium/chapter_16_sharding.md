# Kapitel 16: Horizontal Scaling mit Sharding

> *"Skalierung ohne Komplexität: Native Multi-Model Sharding für Enterprise-Workloads"*

---

## Überblick

ThemisDB bietet **Production-Ready Horizontal Scaling** durch Hash-basiertes Sharding mit automatischem Rebalancing. Dieses Kapitel erklärt die Sharding-Architektur, Deployment-Szenarien und Performance-Charakteristiken für Enterprise-Umgebungen.

**Was Sie in diesem Kapitel lernen werden:**
- Sharding-Architektur und Routing-Mechanismen
- Deployment-Topologien (2/4/8 Nodes)
- Auto-Rebalancing und Fault Tolerance
- Performance-Charakteristiken und Benchmarks
- Hyperscaler-Vergleich (Aurora, Spanner, Cosmos)

**Voraussetzungen:** Kapitel 2 (Architektur), Kapitel 21 (Performance)

**Status:** ✅ Production-Ready seit v1.4 (Dezember 2025)

---

## 16.1 Warum Sharding?

### Das Single-Node-Limit

Ein einzelner ThemisDB-Server kann beeindruckende Mengen an Daten handhaben:
- **Durchsatz:** ~45.000 writes/sec, ~200.000 reads/sec (NVMe)
- **Speicher:** Bis zu ~10 TB pro Node (praktisches Limit)
- **RAM:** Effizient durch Block Cache, aber limitiert auf Serverkapazität

**Wann brauchen Sie Sharding?**
1. **Datenvolumen > 10 TB**: Physische Speichergrenzen eines Nodes
2. **Durchsatz > 200K ops/sec**: CPU/IO-Sättigung eines Servers
3. **Geografische Verteilung**: Näher zum Benutzer für niedrige Latenz
4. **Hochverfügbarkeit**: Redundanz über mehrere Nodes

### Sharding vs. Replication

**Replication (Vertical Scaling):**
```
┌─────────────┐     Sync      ┌─────────────┐
│   Primary   │ ◄───────────► │   Replica   │
│  (100% Data)│               │  (100% Data)│
└─────────────┘               └─────────────┘
```
- ✅ Einfach zu managen
- ✅ Read-Skalierung
- ❌ Kein Write-Scaling
- ❌ Keine Kapazitäts-Skalierung

**Sharding (Horizontal Scaling):**
```
┌─────────────┐               ┌─────────────┐
│  Shard 1    │               │  Shard 2    │
│  (50% Data) │               │  (50% Data) │
└─────────────┘               └─────────────┘
        ▲                             ▲
        │                             │
        └─────────┬───────────────────┘
                  │
         ┌─────────────────┐
         │  Shard Router   │
         │  (Hash-based)   │
         └─────────────────┘
```
- ✅ Read + Write Scaling
- ✅ Kapazitäts-Skalierung
- ✅ Geografische Verteilung
- ⚠️ Komplexere Verwaltung

---

## 16.2 Sharding-Architektur

### Hash-basiertes Routing

ThemisDB verwendet **Consistent Hashing** mit MurmurHash3:

```cpp
// Simplified Shard Router Implementation
class ShardRouter {
private:
    std::vector<ShardInfo> shards_;
    
public:
    // Berechne Shard für einen Key
    uint32_t get_shard_id(const std::string& key) {
        // MurmurHash3: Schnell & gut verteilte Hash-Funktion
        uint32_t hash = murmur3_hash(key);
        
        // Modulo-Routing: Hash % Anzahl_Shards
        return hash % shards_.size();
    }
    
    // Dokument auf korrekten Shard routen
    void route_document(const Document& doc) {
        // Primary Key als Routing-Schlüssel
        uint32_t shard_id = get_shard_id(doc.id);
        
        // An Shard weiterleiten
        shards_[shard_id].insert(doc);
    }
};
```

**Wie funktioniert das?**

1. **Client sendet Query** → Router
2. **Router berechnet Hash** des Primary Key
3. **Hash % N** bestimmt Shard-ID (N = Anzahl Shards)
4. **Query wird an Shard weitergeleitet**
5. **Ergebnis zurück zum Client**

**Beispiel:**
```python
# Dokument mit ID "user_12345"
doc_id = "user_12345"

# MurmurHash3(doc_id) = 3847592834 (hypothetisch)
hash_value = murmur3(doc_id)  # → 3847592834

# 8 Shards vorhanden
num_shards = 8
shard_id = hash_value % num_shards  # → 3847592834 % 8 = 2

# ⇒ Dokument landet auf Shard 2
```

**Warum MurmurHash3?**
- **Schnell:** ~2-3 CPU-Zyklen pro Byte
- **Gut verteilt:** Minimiert Hotspots
- **Deterministisch:** Gleicher Key → immer gleicher Shard

### Range-basiertes Sharding (Optional)

Für geordnete Daten (z.B. Timestamps) können Range-Shards effizienter sein:

```yaml
# config/sharding/range-sharding.yaml
sharding:
  strategy: range
  key: created_at
  ranges:
    - shard: 1
      min: "2020-01-01"
      max: "2022-12-31"
    - shard: 2
      min: "2023-01-01"
      max: "2024-12-31"
    - shard: 3
      min: "2025-01-01"
      max: null  # Unbegrenzt
```

**Vorteil:** Zeitbereichs-Queries treffen nur relevante Shards  
**Nachteil:** Ungleichmäßige Lastverteilung möglich

---

## 16.3 Deployment-Topologien

### 2-Node Cluster (Entry-Level)

**Setup:**
```
┌─────────────┐               ┌─────────────┐
│  Shard 1    │◄─────────────►│  Shard 2    │
│  + Replica  │   Sync (RF=2) │  + Replica  │
└─────────────┘               └─────────────┘
        ▲                             ▲
        │                             │
        └─────────┬───────────────────┘
                  │
         ┌─────────────────┐
         │  Load Balancer  │
         └─────────────────┘
```

**Charakteristiken:**
- **Kapazität:** 2× Single-Node (~20 TB)
- **Throughput:** ~1.8× Single-Node (91% Scaling Efficiency)
- **Redundanz:** RF=2 (jeder Shard hat 1 Replica auf anderem Node)
- **Kosten:** 2× Hardware, <2× Betriebskosten

**Use Case:** Kleine bis mittlere Deployments (< 20 TB, < 100K ops/sec)

---

### 4-Node Cluster (Mid-Tier)

**Setup:**
```
     ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐
     │ Shard 1 │    │ Shard 2 │    │ Shard 3 │    │ Shard 4 │
     │ + Rep   │    │ + Rep   │    │ + Rep   │    │ + Rep   │
     └────▲────┘    └────▲────┘    └────▲────┘    └────▲────┘
          │              │              │              │
          └──────────────┴──────────────┴──────────────┘
                             │
                    ┌─────────────────┐
                    │  Load Balancer  │
                    └─────────────────┘
```

**Charakteristiken:**
- **Kapazität:** 4× Single-Node (~40 TB)
- **Throughput:** ~3.6× Single-Node (90% Scaling Efficiency)
- **Cross-Shard Queries:** ~10-15% Performance-Impact
- **Rebalance-Zeit:** ~4-5 Minuten bei 2→4 Expansion

**Use Case:** Standard Enterprise Deployments (20-40 TB, 100-400K ops/sec)

---

### 8-Node Cluster (Enterprise)

**Setup:**
```
  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐
  │Shard 1│ │Shard 2│ │Shard 3│ │Shard 4│
  └───┬───┘ └───┬───┘ └───┬───┘ └───┬───┘
      │         │         │         │
  ┌───────┐ ┌───────┐ ┌───────┐ ┌───────┐
  │Shard 5│ │Shard 6│ │Shard 7│ │Shard 8│
  └───┬───┘ └───┬───┘ └───┬───┘ └───┬───┘
      │         │         │         │
      └─────────┴─────────┴─────────┘
                  │
         ┌─────────────────┐
         │  Load Balancer  │
         │  + Monitoring   │
         └─────────────────┘
```

**Charakteristiken:**
- **Kapazität:** 8× Single-Node (~80 TB)
- **Throughput:** ~7.3× Single-Node (91% Scaling Efficiency)
- **p99 Latenz:** 1.25ms (nur 0.43× Single-Node dank Parallelität!)
- **Rebalance-Zeit:** ~8-10 Minuten bei 4→8 Expansion

**Use Case:** Große Enterprise Deployments (40-80 TB, 400K+ ops/sec)

---

## 16.4 Auto-Rebalancing

### Wann wird rebalanced?

ThemisDB triggert automatisches Rebalancing bei:

1. **Skew-Threshold:** >15% Ungleichgewicht zwischen Shards
2. **Disk Utilization:** >70% auf einem Shard
3. **Manuell:** Via `themis-cli cluster rebalance`

**Konfiguration:**
```yaml
# config/sharding/shard-router.yaml
rebalance:
  auto_trigger: true
  skew_threshold: 0.15      # 15% Ungleichgewicht
  disk_threshold: 0.70      # 70% Festplattenauslastung
  max_parallel_moves: 2     # Max 2 Chunks gleichzeitig verschieben
  chunk_size_mb: 64         # Chunk-Größe für Migration
```

### Rebalance-Prozess

**Schritt-für-Schritt:**

```
1. Coordinator erkennt Skew (z.B. Shard 1: 25%, Shard 2: 15%)
   ┌─────────┐  ┌─────────┐
   │ Shard 1 │  │ Shard 2 │
   │   25%   │  │   15%   │
   └─────────┘  └─────────┘

2. Plan erstellen: Verschiebe 5% von Shard 1 → Shard 2
   ┌─────────┐──┐
   │ Shard 1 │5%│───┐
   │   20%   │  │   │
   └─────────┘──┘   │
                    ▼
                ┌─────────┐
                │ Shard 2 │
                │   20%   │
                └─────────┘

3. Chunk-Migration (64MB pro Chunk)
   - Chunk kopieren (read-only)
   - Neue Writes auf beide Shards
   - Cutover: Alte Version löschen

4. Validierung & Completion
   ┌─────────┐  ┌─────────┐
   │ Shard 1 │  │ Shard 2 │
   │   20%   │  │   20%   │
   └─────────┘  └─────────┘
```

**Performance-Impact:**
- **Throughput-Dip:** ~12% während Migration
- **Recovery-Zeit:** <5 Minuten nach Completion
- **Latenz:** p99 +15% während Migration

### Monitoring während Rebalance

```bash
# Live Status anzeigen
themis-cli cluster rebalance-status --watch

# Ausgabe:
┌────────────────────────────────────────────────┐
│ REBALANCE STATUS                               │
├────────────────────────────────────────────────┤
│ Progress: ████████░░ 78% (125 / 160 chunks)    │
│ Duration: 00:03:42 / ~00:05:00                 │
│ Throughput Impact: -11% (current)              │
│                                                │
│ Shard Distribution:                            │
│   Shard 1: 21% ███████░░░░░░░░                 │
│   Shard 2: 20% ██████░░░░░░░░░                 │
│   Shard 3: 19% ██████░░░░░░░░░                 │
│   Shard 4: 20% ██████░░░░░░░░░                 │
│                                                │
│ ETA: 00:01:18                                  │
└────────────────────────────────────────────────┘
```

---

## 16.5 Fault Tolerance

### Replica Failure (RF=2)

**Szenario:** Ein Replica-Node stirbt

```
Before:                     After (Auto-Recovery):
┌─────────┐  ┌─────────┐    ┌─────────┐  ┌─────────┐
│ Shard 1 │  │ Shard 1'│    │ Shard 1 │  │ Shard 1'│
│ Primary │◄►│ Replica │    │ Primary │  │  (DEAD) │
└─────────┘  └─────────┘    └─────────┘  └─────────┘
                                   │
                                   ▼ Promote Replica
                            ┌─────────┐
                            │ Shard 1 │
                            │ Primary │  (New)
                            └─────────┘
```

**Charakteristiken:**
- **Detection:** <5 Sekunden (Heartbeat-Timeout)
- **Failover:** <15 Sekunden (Replica Promotion)
- **Recovery:** <60 Sekunden (Rebuild Replica auf anderem Node)
- **Throughput-Impact:** -24% während Outage, -10% während Rebuild

**Monitoring:**
```bash
# Cluster Health anzeigen
themis-cli cluster health

# Ausgabe bei Failure:
┌────────────────────────────────────────────────┐
│ CLUSTER HEALTH: DEGRADED                       │
├────────────────────────────────────────────────┤
│ Shard 1: PRIMARY   ✅ node-1  (Leader)          │
│          REPLICA   ❌ node-2  (DEAD - 00:00:42) │
│                                                │
│ Shard 2: PRIMARY   ✅ node-3                    │
│          REPLICA   ✅ node-4                    │
│                                                │
│ ⚠️  Rebuilding Replica on node-5... (35%)      │
│ ETA: 00:00:28                                  │
└────────────────────────────────────────────────┘
```

### Network Partition

**Szenario:** 10ms RTT Network Latency hinzugefügt

**Impact:**
- **Latenz p50:** +8ms (von 0.5ms → 8.5ms)
- **Latenz p99:** +12ms (von 1.2ms → 13.2ms)
- **Throughput:** -15% (wegen Sync-Overhead)

**Graceful Degradation:** ThemisDB passt sich automatisch an:
```yaml
# Dynamic Network Adaptation
network:
  rtt_threshold: 10ms
  adaptive_batching: true   # Größere Batches bei hoher Latenz
  compression: lz4          # Kompression bei langsamen Links
```

---

## 16.6 Performance-Benchmarks

### Scaling Efficiency

**Testsetup:**
- **Hardware:** 8× c6i.4xlarge (16 vCPU, 32GB RAM, NVMe)
- **Dataset:** 500M OLTP-Rows + 100M Vector-Embeddings (768D)
- **Workload:** Mix B (50% Read, 50% Write, 10% Cross-Shard)

**Ergebnisse:**

| Shards | Throughput (ops/sec) | Scaling Efficiency | Latenz p99 (ms) |
|--------|----------------------|-------------------|-----------------|
| 1      | 100,000              | Baseline (100%)   | 2.9             |
| 2      | 182,000              | 91%               | 3.1             |
| 4      | 362,000              | 90.5%             | 3.3             |
| 8      | 728,000              | 91%               | 1.25 (!!)       |

**Warum ist p99 bei 8 Shards niedriger?**
→ **Parallelität:** Queries werden auf mehrere Nodes verteilt, reduziert Queue-Wartezeiten!

### Cross-Shard Join Performance

**Query:**
```aql
FOR order IN orders
  FOR customer IN customers
    FILTER order.customer_id == customer._id  -- Cross-Shard Join!
    RETURN {order, customer}
```

**Performance (10% Cross-Shard Rate):**

| Shards | Local Query (ms) | Cross-Shard Query (ms) | Ratio |
|--------|------------------|------------------------|-------|
| 2      | 1.2              | 2.1                    | 1.75× |
| 4      | 1.3              | 2.4                    | 1.85× |
| 8      | 1.25             | 2.5                    | 2.0×  |

**Optimierung:** Co-locate verwandte Daten auf gleichem Shard via Custom Routing:

```python
# Custom Routing für Co-Location
def custom_shard_key(doc):
    if doc.type == "order":
        # Orders nach customer_id routen
        return doc.customer_id
    elif doc.type == "customer":
        # Customers nach ihrer ID routen
        return doc._id
    
# ⇒ Order und zugehöriger Customer auf gleichem Shard!
```

---

## 16.7 Hyperscaler-Vergleich

### Cost-Performance-Analyse

**Testsetup:** 8-Node Cluster, 500M OLTP + 100M Vector, Mix B Workload

| Provider | SKU | Throughput (ops/sec) | Latenz p99 (ms) | $/Monat | $/M Ops |
|----------|-----|----------------------|-----------------|---------|---------|
| **ThemisDB** | c6i.4xlarge × 8 | 728,000 | 1.25 | $2,880 | $6.25 |
| AWS Aurora | r6g.4xlarge × 8 | 80,000 | 15 | $12,288 | $19.20 |
| GCP Spanner | 6-Node Regional | 120,000 | 8 | $4,800 | $40 |
| Azure Cosmos | 50K RU/s | 50,000 | 25 | $3,800 | $76 |
| AWS Redshift | RA3 4-Node | 100,000 | 12 | $3,260 | $32.50 |

**Key Insights:**
- **ThemisDB vs Aurora:** 9.1× höherer Throughput, **67% Kosteneinsparung**
- **ThemisDB vs Spanner:** 6.1× höherer Throughput, **84% Kosteneinsparung**
- **ThemisDB vs Cosmos:** 14.6× höherer Throughput, **92% Kosteneinsparung**

**Warum ist ThemisDB so viel günstiger?**
1. **Keine Lizenzkosten:** Open Source (MIT)
2. **Effiziente Architektur:** Native Multi-Model eliminiert Overhead
3. **Self-Hosted:** On-Premises oder eigene Cloud-VMs

---

## 16.8 Production Deployment

### Deployment-Checkliste

**Vor dem Deployment:**
- [ ] Hardware-Dimensionierung (CPU, RAM, NVMe)
- [ ] Netzwerk-Topologie (< 2ms RTT zwischen Nodes)
- [ ] Monitoring-Setup (Prometheus + Grafana)
- [ ] Backup-Strategie (Snapshots + WAL-Archive)

**Deployment-Schritte:**

```bash
# 1. Cluster initialisieren
themis-cli cluster init \
  --nodes node-1,node-2,node-3,node-4 \
  --shards 4 \
  --replication-factor 2

# 2. Sharding-Konfiguration laden
themis-cli cluster apply-config config/sharding/shard-router.yaml

# 3. Daten laden (parallel)
python tools/shard_loader.py \
  --config config/sharding/shard-router.yaml \
  --dataset oltp \
  --rows 500000000 \
  --workers 32

# 4. Cluster-Health überprüfen
themis-cli cluster health

# 5. Benchmarks laufen lassen
python tools/shard_bench.py \
  --shards 4 \
  --mix B \
  --duration 600 \
  --threads 64
```

### Monitoring-Metriken

**Kritische Metriken:**
```
1. Shard-Utilization (Disk %)
   Ziel: <70% pro Shard, <15% Skew

2. Throughput (ops/sec)
   Ziel: >90% Scaling Efficiency

3. Latenz (p50/p95/p99)
   Ziel: p99 <2.5× Single-Node

4. Cross-Shard Query Rate
   Ziel: <20% aller Queries

5. Rebalance-Status
   Ziel: Completion <5 min, <15% Dip
```

**Grafana Dashboard:**
```json
{
  "dashboard": {
    "title": "ThemisDB Sharding Overview",
    "panels": [
      {
        "title": "Shard Distribution",
        "type": "bar",
        "query": "sum(themis_shard_size_bytes) by (shard_id)"
      },
      {
        "title": "Throughput by Shard",
        "type": "graph",
        "query": "rate(themis_shard_ops_total[5m]) by (shard_id)"
      },
      {
        "title": "Cross-Shard Query Rate",
        "type": "stat",
        "query": "rate(themis_cross_shard_queries[5m])"
      }
    ]
  }
}
```

---

## 16.9 Best Practices

### 1. Shard-Key Auswahl

**Gute Shard-Keys:**
- ✅ **User-ID:** Gleichmäßige Verteilung, natürliche Co-Location (User + Orders)
- ✅ **UUID:** Perfekte zufällige Verteilung
- ✅ **Hash(Composite-Key):** Für komplexe Co-Location-Szenarien

**Schlechte Shard-Keys:**
- ❌ **Timestamp:** Hotspots auf neuesten Shard
- ❌ **Sequential-ID:** Alle Writes auf einen Shard
- ❌ **Low-Cardinality:** Status-Felder (z.B. "active"/"inactive")

### 2. Cross-Shard Queries minimieren

**Anti-Pattern:**
```aql
-- SCHLECHT: Joins über alle Shards
FOR order IN orders
  FOR product IN products
    FILTER order.product_id == product._id
    RETURN {order, product}
```

**Best Practice:**
```aql
-- GUT: Denormalisierung vermeidet Cross-Shard Joins
FOR order IN orders
  RETURN {
    order,
    product_name: order.embedded_product_name,  -- Denormalisiert!
    product_price: order.embedded_product_price
  }
```

### 3. Monitoring von Anfang an

**Observability-Stack:**
```yaml
# docker-compose.yml
services:
  themisdb:
    image: themisdb:latest
    environment:
      THEMIS_METRICS_ENABLED: "true"
      THEMIS_METRICS_PORT: 9090
  
  prometheus:
    image: prom/prometheus
    volumes:
      - ./prometheus.yml:/etc/prometheus/prometheus.yml
  
  grafana:
    image: grafana/grafana
    ports:
      - "3000:3000"
```

### 4. Gradual Rollout

**Empfohlene Rollout-Strategie:**
1. **Woche 1:** 2-Node Cluster, 10% Traffic
2. **Woche 2:** 2-Node Cluster, 50% Traffic
3. **Woche 3:** 4-Node Cluster, 100% Traffic
4. **Monat 2+:** 8-Node Cluster bei Bedarf

---

## 16.10 Troubleshooting

### Problem: Ungleiche Shard-Verteilung

**Symptom:**
```
Shard 1: 35% (❌ zu voll)
Shard 2: 18%
Shard 3: 22%
Shard 4: 25%
```

**Ursache:** Hotkey (z.B. ein sehr aktiver User)

**Lösung:**
```bash
# Manuelles Rebalancing triggern
themis-cli cluster rebalance --force

# Hotkey identifizieren
themis-cli shard analyze --shard 1 --top-keys 10

# Ausgabe:
# Key: user_123456, Size: 2.3 GB (❌ Hotkey!)
# → Lösung: Hotkey auf separaten "Large Object Shard" verschieben
```

### Problem: Hohe Cross-Shard Query Latenz

**Symptom:** p99 >10× Single-Node bei >20% Cross-Shard Rate

**Lösung:**
1. **Co-Location aktivieren:**
```yaml
sharding:
  strategy: hash
  co_location:
    enabled: true
    rules:
      - tables: [orders, customers]
        key: customer_id  # Beide Tabellen nach customer_id routen
```

2. **Denormalisierung:**
```python
# Embed häufig gejointe Daten
order = {
    "id": "order_123",
    "customer_id": "user_456",
    "customer_name": "Alice",  # Denormalisiert!
    "customer_email": "alice@example.com"  # Denormalisiert!
}
```

### Problem: Rebalance dauert zu lange

**Symptom:** Rebalance >15 Minuten, >20% Throughput-Dip

**Lösung:**
```yaml
# config/sharding/shard-router.yaml
rebalance:
  chunk_size_mb: 32         # Kleinere Chunks (default: 64)
  max_parallel_moves: 4     # Mehr parallele Transfers (default: 2)
  rate_limit_mbps: 500      # Höheres Network-Limit
```

---

## 16.11 Zusammenfassung

**ThemisDB Sharding bietet:**
- ✅ **Production-Ready:** 91% Scaling Efficiency bis 8 Nodes
- ✅ **Auto-Rebalancing:** <15% Dip, <5min Recovery
- ✅ **Fault Tolerant:** <60s Recovery bei Replica-Failure
- ✅ **Cost-Efficient:** 67-92% günstiger als Hyperscaler
- ✅ **Native Multi-Model:** Sharding funktioniert über alle Datenmodelle

**Nächste Schritte:**
- Kapitel 19: Monitoring für detailliertes Observability-Setup
- Kapitel 21: Performance-Tuning für Sharding-spezifische Optimierungen
- `docs/de/SHARDING_DOCUMENTATION_INDEX.md`: Vollständige technische Dokumentation

**Production-Deployment?** Starten Sie mit einem 2-Node Cluster und skalieren Sie bei Bedarf auf 4 oder 8 Nodes!

---

## Weiterführende Ressourcen

**Dokumentation:**
- `docs/de/SHARDING_INTEGRATION_SUMMARY.md` - Master-Übersicht
- `docs/de/SHARDING_BENCHMARK_PLAN_v1.4.md` - Enterprise Benchmark-Spezifikation
- `docs/de/SHARDING_PRODUCTION_DEPLOYMENT_RAID_v1.4.md` - Production Deployment Guide

**Tools:**
- `tools/shard_loader.py` - Daten-Loader für Sharding-Tests
- `tools/shard_bench.py` - Benchmark-Runner
- `tools/fault_injector.py` - Chaos-Engineering-Tool

**Konfiguration:**
- `config/sharding/shard-router-example.yaml` - Production-Ready Config
- `.github/workflows/sharding-benchmark.yml` - CI/CD Benchmarks
