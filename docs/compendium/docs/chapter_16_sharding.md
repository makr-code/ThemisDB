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

```mermaid
graph TB
    subgraph "Replication - Read Scaling"
        Client1[Client] --> Primary1[(Primary<br/>100 percent Data)]
        Primary1 -.sync.-> Replica1A[(Replica 1<br/>100 percent Data)]
        Primary1 -.sync.-> Replica1B[(Replica 2<br/>100 percent Data)]
        Client1 -.read.-> Replica1A
        Client1 -.read.-> Replica1B
    end
    
    subgraph "Sharding - Full Scaling"
        Client2[Client] --> Router[Shard Router<br/>Hash-based]
        Router --> Shard1[(Shard 1<br/>33 percent Data)]
        Router --> Shard2[(Shard 2<br/>33 percent Data)]
        Router --> Shard3[(Shard 3<br/>34 percent Data)]
    end
    
    style Primary1 fill:#667eea
    style Replica1A fill:#4facfe
    style Replica1B fill:#4facfe
    style Router fill:#f093fb
    style Shard1 fill:#43e97b
    style Shard2 fill:#43e97b
    style Shard3 fill:#43e97b
```

Abb. 16.1: Replication vs Sharding Vergleich

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

```mermaid
sequenceDiagram
    participant Client
    participant Router as Shard Router
    participant S1 as Shard 1
    participant S2 as Shard 2
    participant S3 as Shard 3
    
    Client->>Router: INSERT user_12345 {data}
    Note over Router: MurmurHash3("user_12345")<br/>= 3847592834<br/>3847592834 mod 3 = 2
    Router->>S2: INSERT {data}
    S2-->>Router: [OK] Success
    Router-->>Client: [OK] Inserted
    
    Client->>Router: SELECT * WHERE id='user_12345'
    Note over Router: Hash("user_12345") mod 3 = 2
    Router->>S2: SELECT * WHERE id='user_12345'
    S2-->>Router: {user data}
    Router-->>Client: {user data}
```

Abb. 16.2: Hash-Based-Routing-Flow

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

## 16.5 RAID-äquivalente Redundanz-Modi

ThemisDB unterstützt **6 Redundanzmodi** analog zu RAID-Levels, die unterschiedliche Trade-offs zwischen Performance, Speichereffizienz und Fault Tolerance bieten.

### Überblick: Redundanzmodi im Vergleich

| Modus | RF* | Speicher-<br/>effizienz | Through-<br/>put | Latenz | Ausfallsicher |
|-------|-----|-------------------------|------------------|--------|---------------|
| **NONE** | 1 | 100% | 1× | 1× | 0 Shards |
| **MIRROR (RF=2)** | 2 | 50% | 2× | 1.2× | 1 Shard |
| **MIRROR (RF=3)** | 3 | 33% | 3× | 1.5× | 2 Shards |
| **STRIPE** | 1 | 100% | 4× | 0.8× | 0 Shards |
| **STRIPE_MIRROR** | 2 | 50% | 2-3× | 1× | 1 Shard |
| **PARITY (4+2)** | 4+2 | 67% | 1.5× | 1.3× | 2 Shards |
| **PARITY (8+3)** | 8+3 | 73% | 1.3× | 1.5× | 3 Shards |
| **GEO_MIRROR** | 3 | 33% | Lokal 3× | Lokal 2× | 2 DCs |

*RF = Replication Factor

### 16.5.1 NONE Mode - Keine Redundanz

**RAID-Äquivalent:** Einzelne Disk ohne Redundanz

**Charakteristiken:**
- ✅ Maximale Speichereffizienz (100%)
- ✅ Keine Replikations-Latenz
- ❌ **Kein Datenverlust-Schutz** - Bei Shard-Ausfall sind Daten verloren
- ❌ Kein automatisches Failover

**Use Case:** Entwicklung, Test-Umgebungen, ephemerale Caches

**Beispiel-Konfiguration:**
```yaml
cluster:
  name: "themis-dev"
  mode: "NONE"

shard:
  id: "shard_001"
  replication:
    mode: NONE
    replication_factor: 1
    write_concern: IMMEDIATE
```

**Wann verwenden:**
- Nicht für Production!
- Nur für unkritische Daten oder Dev/Test
- Wenn regelmäßige externe Backups vorhanden sind

---

### 16.5.2 MIRROR Mode - Vollständige Spiegelung (RAID-1)

**RAID-Äquivalent:** RAID-1 (gespiegelt)

**Charakteristiken:**
- ✅ **Hohe Verfügbarkeit** - N-1 Shards können ausfallen (bei RF=N)
- ✅ Read-Skalierung (2× bei RF=2, 3× bei RF=3)
- ⚠️ Speichereffizienz: 50% (RF=2) oder 33% (RF=3)
- ⚠️ Write-Latenz: +20% (RF=2) bis +50% (RF=3)

**Use Case:** Production-Deployments mit High Availability Anforderungen

**Beispiel-Konfiguration (RF=2):**
```yaml
cluster:
  name: "themis-prod"
  mode: "MIRROR"

shard:
  id: "shard_001"
  replication:
    mode: MIRROR
    replication_factor: 2
    write_concern: QUORUM  # Warte auf 2/2 Replicas
    sync_mode: ASYNC_WAL   # WAL sync, dann async replication
    
  failover:
    auto_failover: true
    election_timeout_ms: 5000
    heartbeat_interval_ms: 1000
```

**Failover-Verhalten:**
```
Primary-Ausfall:          Automatisches Failover:
┌─────────┐ ✗             ┌─────────────┐
│ Primary │               │ New Primary │ (promoted)
│ (DEAD)  │               │ (ex-Replica)│
└─────────┘               └─────────────┘
     │                           │
┌─────────┐               ┌─────────────┐
│ Replica │──────────────>│ New Replica │ (rebuilt)
│  (ok)   │               │   (new)     │
└─────────┘               └─────────────┘

Failover-Zeit: <15 Sekunden
Data Loss: 0 (mit QUORUM write)
```

**Wann verwenden:**
- **Standard für Production** - Beste Balance zwischen Verfügbarkeit und Kosten
- RF=2 ausreichend für die meisten Szenarien
- RF=3 für kritische Systeme (Banking, Healthcare)

---

### 16.5.3 STRIPE Mode - Daten-Striping (RAID-0)

**RAID-Äquivalent:** RAID-0 (striped, kein Parity)

**Charakteristiken:**
- ✅ **Maximaler Throughput** (4× bei 4 Shards)
- ✅ Niedrige Latenz (0.8× Single-Node durch Parallelität)
- ✅ 100% Speichereffizienz
- ❌ **Kein Redundanz** - 1 Shard-Ausfall = Datenverlust

**Use Case:** HPC, Analytics mit externem Backup, temporäre Verarbeitung

**Beispiel-Konfiguration:**
```yaml
cluster:
  name: "themis-analytics"
  mode: "STRIPE"

shard:
  stripe_width: 4  # Daten über 4 Shards verteilen
  chunk_size_kb: 256
  
  replication:
    mode: STRIPE
    replication_factor: 1
```

**Datenverteilung:**
```
Dokument A (1 MB):
┌──────┬──────┬──────┬──────┐
│256KB │256KB │256KB │256KB │
└──┬───┴──┬───┴──┬───┴──┬───┘
   │      │      │      │
   v      v      v      v
Shard1 Shard2 Shard3 Shard4

Read: 4× parallele Chunks = 4× Throughput
Write: 4× parallele Writes = 4× Throughput
```

**Wann verwenden:**
- Nur wenn **externe Backup-Strategie** vorhanden!
- Big Data Analytics mit S3/HDFS Backup
- Temporäre ETL-Verarbeitung
- Niemals für kritische Produktionsdaten ohne Backup

---

### 16.5.4 STRIPE_MIRROR Mode - Kombiniert (RAID-10)

**RAID-Äquivalent:** RAID-10 (striped + mirrored)

**Charakteristiken:**
- ✅ **Hoher Throughput** (2-3× Single-Node)
- ✅ **High Availability** (1 Shard pro Stripe-Gruppe kann ausfallen)
- ⚠️ Speichereffizienz: 50% (wie MIRROR)
- ⚠️ Moderate Latenz (+0-20%)

**Use Case:** Production mit hohen Throughput-Anforderungen

**Beispiel-Konfiguration:**
```yaml
cluster:
  name: "themis-prod-highperf"
  mode: "STRIPE_MIRROR"

shard:
  stripe_groups:
    - group_id: 1
      shards: [shard_001, shard_002]  # Stripe-Gruppe 1
      replication_factor: 2
    - group_id: 2
      shards: [shard_003, shard_004]  # Stripe-Gruppe 2
      replication_factor: 2
```

**Architektur:**
```
┌────────────────────────────────────┐
│ Stripe-Gruppe 1 (50% der Daten)   │
│ ┌─────────┐      ┌─────────┐      │
│ │ Shard 1 │ ◄──► │ Shard 1'│      │
│ │ Primary │      │ Replica │      │
│ └─────────┘      └─────────┘      │
└────────────────────────────────────┘

┌────────────────────────────────────┐
│ Stripe-Gruppe 2 (50% der Daten)   │
│ ┌─────────┐      ┌─────────┐      │
│ │ Shard 2 │ ◄──► │ Shard 2'│      │
│ │ Primary │      │ Replica │      │
│ └─────────┘      └─────────┘      │
└────────────────────────────────────┘

Throughput: 2× (Striping über 2 Gruppen)
Availability: Jede Gruppe toleriert 1 Ausfall
```

**Wann verwenden:**
- Production mit >100K ops/sec Anforderung
- Balance zwischen Performance und Verfügbarkeit
- Bevorzugt gegenüber reinem MIRROR wenn Throughput wichtiger als Latenz

---

### 16.5.5 PARITY Mode - Erasure Coding (RAID-5/6)

**RAID-Äquivalent:** RAID-5/6 (mit Parity Chunks)

**Charakteristiken:**
- ✅ **Hohe Speichereffizienz** (67% bei 4+2, 73% bei 8+3)
- ✅ **Multi-Fault-Tolerance** (k Shards können ausfallen, k=Parity-Chunks)
- ⚠️ Read: Moderate Performance (1.5× Single-Node)
- ⚠️ Write: Parity-Overhead (+30-50% Latenz)
- ⚠️ Rebuild: Langsamer als MIRROR (Parity-Berechnungen)

**Use Case:** Large-Scale Data Warehouse, Cold Storage, kostenoptimiert

**Beispiel-Konfiguration (4+2):**
```yaml
cluster:
  name: "themis-warehouse"
  mode: "PARITY"

shard:
  erasure_coding:
    data_shards: 4      # 4 Daten-Shards
    parity_shards: 2    # 2 Parity-Shards
    algorithm: "reed_solomon"
    
  replication:
    mode: PARITY
    write_concern: ALL  # Warte auf alle 6 Shards
```

**Datenverteilung:**
```
Dokument A (1 MB):
┌──────┬──────┬──────┬──────┐──────┬──────┐
│ D1   │ D2   │ D3   │ D4   │ P1   │ P2   │
│250KB │250KB │250KB │250KB │125KB │125KB │
└──┬───┴──┬───┴──┬───┴──┬───┴──┬───┴──┬───┘
   │      │      │      │      │      │
   v      v      v      v      v      v
Shard1 Shard2 Shard3 Shard4 Shard5 Shard6

Speichereffizienz: 1 MB / 1.5 MB = 67%
Ausfallsicher: Bis zu 2 Shards (P1, P2)
```

**Rebuild bei Ausfall:**
```
Shard 3 fällt aus:
D3 = D1 ⊕ D2 ⊕ D4 ⊕ P1  (XOR-Berechnung)

Rebuild-Zeit: ~2× MIRROR (wegen Parity-Berechnung)
```

**Wann verwenden:**
- **Cold/Warm Storage** (weniger häufig geschrieben)
- **Cost-Optimierung** für große Datenmengen (>100 TB)
- Wenn 2+ Shard-Ausfälle toleriert werden müssen
- Data Warehouses mit Read-Heavy Workload

**Cost-Vergleich:**
```
10 TB Nutzbare Kapazität:

MIRROR (RF=2):  20 TB Raw Storage = 100% Overhead
PARITY (4+2):   15 TB Raw Storage =  50% Overhead
PARITY (8+3):   13.75 TB Raw Storage = 37.5% Overhead

Savings: 25-62.5% Speicherkosten
```

---

### 16.5.6 GEO_MIRROR Mode - Multi-Region

**RAID-Äquivalent:** RAID-1 über Data Centers

**Charakteristiken:**
- ✅ **Disaster Recovery** - N-1 Data Centers können ausfallen
- ✅ **Geo-Distribution** - Niedrige Latenz für regionale Clients
- ⚠️ Speichereffizienz: 33% (RF=3 über 3 DCs)
- ⚠️ Cross-DC Latenz: +50-200ms (abhängig von Entfernung)

**Use Case:** Multi-Region Deployment, Disaster Recovery

**Beispiel-Konfiguration:**
```yaml
cluster:
  name: "themis-global"
  mode: "GEO_MIRROR"

shard:
  id: "shard_001"
  replication:
    mode: GEO_MIRROR
    replication_factor: 3
    geo_regions:
      - region: "eu-central-1"   # Frankfurt
        datacenter: "dc1"
      - region: "us-east-1"      # Virginia
        datacenter: "dc2"
      - region: "ap-southeast-1" # Singapore
        datacenter: "dc3"
    
    write_concern: LOCAL_QUORUM  # 2/3 im lokalen DC
    read_preference: NEAREST     # Nächster DC
```

**Architektur:**
```
┌────────────────────────┐
│   EU (Frankfurt)       │
│ ┌────────┐             │
│ │Primary │───────┐     │
│ └────────┘       │     │
└──────────────────┼─────┘
                   │
    ┌──────────────┼──────────────┐
    │              │              │
    │         ┌────▼────┐         │
┌───▼─────────┴─────────▼─────────▼───┐
│   US (Virginia)   │   APAC (Singapore)│
│ ┌────────┐        │   ┌────────┐     │
│ │Replica │        │   │Replica │     │
│ └────────┘        │   └────────┘     │
└───────────────────┴──────────────────┘

Lokale Reads: <5ms (innerhalb DC)
Cross-DC Writes: +100ms (async replication)
```

**Failover-Strategie:**
```
DC-Ausfall (z.B. EU):
1. Auto-Promote Replica in US oder APAC
2. Reroute Clients zum neuen Primary
3. Rebuild in anderem DC (eu-west-1)

Failover-Zeit: <60 Sekunden
Data Loss: 0 (mit LOCAL_QUORUM)
```

**Wann verwenden:**
- **Global Services** mit Benutzern in mehreren Kontinenten
- **Disaster Recovery** für kritische Systeme
- **Compliance** (Daten-Residency in bestimmten Regionen)
- Banking, Healthcare, Government

---

### 16.5.7 Entscheidungsmatrix

**Welcher Modus für welchen Use Case?**

```mermaid
graph TD
    Start[Welcher Redundanzmodus?] --> Q1{Production?}
    Q1 -->|Nein| NONE[NONE Mode<br/>Dev/Test only]
    Q1 -->|Ja| Q2{Geo-Distributed?}
    
    Q2 -->|Ja| GEO[GEO_MIRROR<br/>Multi-Region]
    Q2 -->|Nein| Q3{Throughput<br/>>100K ops/sec?}
    
    Q3 -->|Ja| Q4{Kann Failover<br/>tolerieren?}
    Q4 -->|Nein| STRIPE_M[STRIPE_MIRROR<br/>High Perf + HA]
    Q4 -->|Ja| STRIPE[STRIPE<br/>Max Perf<br/>⚠️ Mit Backup!]
    
    Q3 -->|Nein| Q5{Cold Storage<br/>>100 TB?}
    Q5 -->|Ja| PARITY[PARITY 8+3<br/>Cost-optimiert]
    Q5 -->|Nein| MIRROR[MIRROR RF=2<br/>Standard Prod]
    
    style NONE fill:#ff6b6b
    style GEO fill:#4ecdc4
    style STRIPE_M fill:#ffe66d
    style MIRROR fill:#95e1d3
    style PARITY fill:#a8e6cf
    style STRIPE fill:#ffd93d
```

Abb. 16.5.7: Entscheidungsbaum für Redundanzmodus-Wahl

**Zusammenfassung:**
- **Dev/Test:** NONE
- **Standard Production:** MIRROR (RF=2)
- **High Throughput Production:** STRIPE_MIRROR
- **Cold Storage / Data Warehouse:** PARITY (4+2 oder 8+3)
- **Global / Disaster Recovery:** GEO_MIRROR
- **Max Performance (mit Backup):** STRIPE

---

## 16.6 Fault Tolerance

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

## 16.7 Performance-Benchmarks

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

## 16.8 Hyperscaler-Vergleich

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

## 16.9 Production Deployment

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

## 16.10 Best Practices

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

## 16.11 High Availability Features (v1.4.0-alpha)

### 16.11.1 Hot Spare

**Neu in v1.4.0-alpha:** Automatisches Failover bei Node-Ausfällen mit Hot-Spare-Nodes für maximale Verfügbarkeit.

**Konzept:**

Ein Hot Spare ist ein vollständig konfigurierter Standby-Node, der im Cluster registriert ist und bei Ausfall eines aktiven Shards sofort dessen Rolle übernehmen kann.

```mermaid
graph TB
    subgraph "Normale Operation"
        Client1[Clients] --> Router1[Shard Router]
        Router1 --> Shard1[(Shard 1<br/>ACTIVE)]
        Router1 --> Shard2[(Shard 2<br/>ACTIVE)]
        Router1 --> Shard3[(Shard 3<br/>ACTIVE)]
        HotSpare1[(Hot Spare<br/>STANDBY)]
        
        style Shard1 fill:#43e97b
        style Shard2 fill:#43e97b
        style Shard3 fill:#43e97b
        style HotSpare1 fill:#ffd32a
    end
    
    subgraph "Nach Shard 2 Ausfall"
        Client2[Clients] --> Router2[Shard Router]
        Router2 --> Shard1A[(Shard 1<br/>ACTIVE)]
        Router2 -.failover.-> HotSpare2[(Hot Spare<br/>-> ACTIVE<br/>replaces Shard 2)]
        Router2 --> Shard3A[(Shard 3<br/>ACTIVE)]
        Shard2X[(Shard 2<br/>FAILED)] -.->|recovery| Recovery[Recovery Process]
        
        style Shard1A fill:#43e97b
        style HotSpare2 fill:#43e97b
        style Shard3A fill:#43e97b
        style Shard2X fill:#ff6b6b
    end
```

Abb. 16.3: Shard-Key-Selection-Matrix

**Konfiguration:**

Die folgende YAML-Konfiguration zeigt ein Production-Setup mit 4 aktiven Shards und 2 Hot Spares für automatisches Failover. Die Hot Spares führen kontinuierliche Replikation durch und können bei Ausfall eines Active Shards innerhalb von 5 Sekunden übernehmen.

📁 **Vollständiger Code:** `config/sharding/cluster.yaml` (~120 Zeilen mit allen Nodes)

```yaml
sharding:
  cluster:
    active_shards: 4
    hot_spares: 2  # 2 Hot Spare Nodes für HA
    
  hot_spare:
    enabled: true
    promotion_timeout_ms: 5000  # Max 5s für Failover
    health_check_interval_ms: 1000
    failure_threshold: 3  # 3 Fehlversuche bis Promotion
    
  nodes:
    # Aktive Shards (Beispiel: Shard 1 & 2)
    - id: shard-1
      host: 10.0.1.10
      port: 8765
      role: active
      
    - id: shard-2
      host: 10.0.1.11
      port: 8765
      role: active
      
    # Hot Spares mit kontinuierlicher Replikation
    - id: hot-spare-1
      host: 10.0.1.20
      port: 8765
      role: hot_spare
      replicate_from: shard-1  # Sync von shard-1
```

**Weitere Node-Definitionen im vollständigen Config:**
- Shard 3 & 4 (weitere aktive Nodes)
- Hot Spare 2 (Backup für Shard 2)
- Monitoring-Endpunkte für Health Checks
- Replication-Lag-Alerts (>100ms Verzögerung)

**Deployment-Szenarien:**

**1. Minimal HA Setup (4 Shards + 1 Hot Spare):**
```
Active: [S1, S2, S3, S4]
Spare:  [HS1]
Cost:   5 Nodes
HA:     Einzelner Ausfall abgedeckt
```

**2. Standard HA Setup (4 Shards + 2 Hot Spares):**
```
Active: [S1, S2, S3, S4]
Spares: [HS1, HS2]
Cost:   6 Nodes
HA:     Zwei gleichzeitige Ausfälle abgedeckt
```

**3. Enterprise HA Setup (8 Shards + 3 Hot Spares):**
```
Active: [S1, S2, S3, S4, S5, S6, S7, S8]
Spares: [HS1, HS2, HS3]
Cost:   11 Nodes
HA:     Drei gleichzeitige Ausfälle abgedeckt
```

**Failover-Ablauf:**

1. **Detektion (0-3s):**
   ```
   Health Check fails → Retry 3x (3s) → Declare failure
   ```

2. **Promotion (3-8s):**
   ```
   Select Hot Spare → Apply latest WAL → Update routing table → Promote to active
   ```

3. **Recovery (8s+):**
   ```
   Client requests → Hot Spare (now active) → Background: Sync data from other shards
   ```

**Detaillierte Failover-Timeline:**

```mermaid
gantt
    title Hot Spare Failover Timeline (Total: ~5s)
    dateFormat X
    axisFormat %Ls
    
    section Detection Phase
    Health Check 1 (OK)     :done, h1, 0, 1000
    Health Check 2 (FAIL)   :crit, h2, 1000, 2000
    Health Check 3 (FAIL)   :crit, h3, 2000, 3000
    Declare Failure         :milestone, m1, 3000, 3000
    
    section Promotion Phase
    Select Hot Spare        :active, p1, 3000, 3500
    Apply WAL Entries       :active, p2, 3500, 4500
    Update Routing Table    :active, p3, 4500, 5000
    Promote to Active       :milestone, m2, 5000, 5000
    
    section Recovery Phase
    Accept Client Requests  :done, r1, 5000, 8000
    Background Sync         :active, r2, 5000, 120000
```

Abb. 16.4: Hot-Spare-Failover-Timeline

```mermaid
sequenceDiagram
    participant Client
    participant Router as Shard Router
    participant Shard as Shard 2 (Primary)
    participant HS as Hot Spare 1
    participant Monitor as Health Monitor
    
    Note over Shard,HS: Normal Operation
    Client->>Router: Request
    Router->>Shard: Route to Shard 2
    Shard-->>Router: Response
    Router-->>Client: Response
    Monitor->>Shard: Health Check (OK)
    
    Note over Shard: [ERROR] Shard 2 Crashes
    Monitor->>Shard: Health Check (FAIL)
    Monitor->>Shard: Health Check (FAIL)
    Monitor->>Shard: Health Check (FAIL)
    Monitor->>Monitor: Declare Failure (3s)
    
    Note over HS: Promotion Starts
    Monitor->>HS: Select as Replacement
    HS->>HS: Apply Latest WAL (1s)
    Monitor->>Router: Update Routing Table (0.5s)
    HS->>HS: Promote to ACTIVE (0.5s)
    
    Note over HS: Now Serving Traffic
    Client->>Router: Request
    Router->>HS: Route to Hot Spare 1 (now active)
    HS-->>Router: Response
    Router-->>Client: Response (5s total failover)
```

Abb. 16.5: Failover-Sequenzdiagramm

**Diagramm-Erklärung:**
- **Gantt-Chart (oben):** Zeigt die zeitliche Abfolge der Failover-Phasen
  - Detection: 0-3s (3 Health Checks à 1s)
  - Promotion: 3-5s (Spare auswählen, WAL anwenden, Routing aktualisieren)
  - Total: ~5s bis Traffic wieder fließt
  
- **Sequence-Diagram (unten):** Zeigt die Interaktion zwischen Komponenten
  - Health Monitor erkennt Ausfall nach 3 Fehlversuchen
  - Hot Spare wird automatisch promoted
  - Routing-Tabelle wird aktualisiert
  - Clients merken nichts vom Failover (transparent)

**Performance-Charakteristiken:**

| Metric | Wert | Beschreibung |
|--------|------|--------------|
| **Failover Time (p50)** | 4.5s | Median Zeit bis Hot Spare aktiv |
| **Failover Time (p99)** | 7.8s | 99%-Perzentil Failover-Zeit |
| **Data Loss** | 0 bytes | Bei WAL Replication aktiviert |
| **Throughput Impact** | <5% | Während Failover |
| **Hot Spare Overhead** | ~2% CPU | Kontinuierliche Replikation |

**Monitoring:**

```aql
// Hot Spare Status überwachen
FOR node IN _cluster_nodes
  FILTER node.role IN ['hot_spare', 'active']
  RETURN {
    id: node.id,
    role: node.role,
    status: node.status,
    last_heartbeat: node.last_heartbeat,
    replication_lag_ms: node.replication_lag_ms,
    failover_ready: node.failover_ready
  }
```

**Prometheus Metriken:**
```
themis_hot_spare_active{node="hot-spare-1"} 0
themis_hot_spare_replication_lag_seconds{node="hot-spare-1"} 0.045
themis_hot_spare_failover_count_total{node="hot-spare-1"} 0
themis_shard_health{shard="shard-1"} 1
```

**Failover-Test:**

```bash
# Simuliere Shard-Ausfall
docker stop themisdb-shard-2

# Überwache Failover
watch -n 0.5 'curl -s http://localhost:8765/cluster/status | jq .nodes'

# Erwartete Ausgabe nach ~5s:
# hot-spare-1: role=ACTIVE (promoted)
# shard-2: role=FAILED
```

**Best Practices:**

1. **1 Hot Spare pro 3-4 aktive Shards** - Balance zwischen Kosten und Verfügbarkeit
2. **Geografische Trennung** - Hot Spare in anderem Datacenter/Availability Zone
3. **Regelmäßige Failover-Tests** - Monatlich testen, ob Promotion funktioniert
4. **Monitoring kritisch** - Alerts bei hoher Replication Lag (>100ms)
5. **Hot Spare kontinuierlich synchronisieren** - Via WAL Replication (siehe unten)

### 16.11.2 WAL Replication

**Neu in v1.4.0-alpha:** Write-Ahead-Log basierte Replikation für Zero-Data-Loss und kontinuierliche Synchronisation.

**Konzept:**

WAL (Write-Ahead Log) Replication überträgt alle Schreiboperationen über ein transaktionales Log an Replicas und Hot Spares, bevor sie bestätigt werden.

```mermaid
sequenceDiagram
    participant Client
    participant Primary as Primary Shard
    participant WAL as WAL Buffer
    participant Replica as Hot Spare
    
    Client->>Primary: WRITE Operation
    Primary->>WAL: Append to WAL
    WAL->>WAL: Flush to disk (fsync)
    
    par Synchronous Replication
        WAL->>Replica: Stream WAL entry
        Replica->>Replica: Apply & ACK
        Replica-->>Primary: ACK received
    end
    
    Primary-->>Client: Write confirmed
    
    Note over Primary,Replica: Zero data loss guarantee
```

Abb. 16.6: Elastic-Sharding-Workflow

**Replikations-Modi:**

**1. Synchronous Replication:**
```yaml
replication:
  mode: synchronous
  quorum: 1  # Mindestens 1 Replica muss bestätigen
  timeout_ms: 100  # Timeout für Sync-Bestätigung
```

**Vorteile:**
- ✅ Zero Data Loss garantiert
- ✅ Replica immer auf aktuellem Stand

**Nachteile:**
- ⚠️ Höhere Write-Latenz (+50-100ms)
- ⚠️ Write blockiert bei Replica-Ausfall (ohne Degradation)

**2. Asynchronous Replication:**
```yaml
replication:
  mode: asynchronous
  lag_target_ms: 50  # Ziel Replication Lag
  buffer_size_mb: 256  # WAL Buffer Size
```

**Vorteile:**
- ✅ Minimale Write-Latenz
- ✅ Primary nicht abhängig von Replica

**Nachteile:**
- ⚠️ Potentieller Data Loss bei Primary-Ausfall (Lag-abhängig)
- ⚠️ Replica kann hinterherhinken

**3. Hybrid Mode (Empfohlen):**
```yaml
replication:
  mode: hybrid
  synchronous_shards: [shard-1, shard-2]  # Kritische Daten
  asynchronous_shards: [shard-3, shard-4]  # Unkritische Daten
  quorum: 1
```

**Multi-SSD WAL Konfiguration:**

Für maximale Performance: WAL auf separaten SSDs mit hohem Write-Throughput.

```yaml
# config/storage/wal.yaml
wal:
  directories:
    # Primary WAL auf dediziertem NVMe
    - path: /mnt/nvme0/wal
      type: primary
      fsync_mode: always  # Garantierte Durability
      
    # Backup WAL auf zweitem NVMe (optional)
    - path: /mnt/nvme1/wal-backup
      type: mirror
      fsync_mode: always
  
  # Performance-Tuning
  segment_size_mb: 64  # WAL Segment-Größe
  buffer_size_mb: 256  # In-Memory WAL Buffer
  compression: lz4  # WAL Kompression
  
  # Retention
  keep_segments: 100  # Anzahl Segments aufbewahren
  max_size_gb: 10  # Maximale WAL Größe
```

**Replication Slots:**

Verfolge Replikations-Fortschritt und verhindere vorzeitiges WAL-Purging.

```aql
// Replication Slot erstellen
CALL CREATE_REPLICATION_SLOT('hot-spare-1', {
  slot_type: 'physical',
  temporary: false
})

// Slot-Status abfragen
FOR slot IN _replication_slots
  RETURN {
    slot_name: slot.name,
    slot_type: slot.type,
    active: slot.active,
    restart_lsn: slot.restart_lsn,
    confirmed_flush_lsn: slot.confirmed_flush_lsn,
    wal_lag_bytes: slot.wal_lag_bytes,
    lag_seconds: slot.lag_seconds
  }
```

**Replication Lag Monitoring:**

```aql
// Echtzeit Replication Lag
RETURN WAL_REPLICATION_STATS()
```

Output:
```json
{
  "primary": {
    "current_lsn": "0/5A2F3C40",
    "insert_lsn": "0/5A2F3C40",
    "flush_lsn": "0/5A2F3C40"
  },
  "replicas": [
    {
      "name": "hot-spare-1",
      "state": "streaming",
      "sent_lsn": "0/5A2F3C30",
      "flush_lsn": "0/5A2F3C30",
      "replay_lsn": "0/5A2F3C30",
      "lag_bytes": 16,
      "lag_seconds": 0.002,
      "sync_state": "async"
    },
    {
      "name": "hot-spare-2",
      "state": "streaming",
      "sent_lsn": "0/5A2F3C40",
      "flush_lsn": "0/5A2F3C40",
      "replay_lsn": "0/5A2F3C40",
      "lag_bytes": 0,
      "lag_seconds": 0.000,
      "sync_state": "sync"
    }
  ]
}
```

**Prometheus Metriken:**
```
themis_wal_replication_lag_seconds{replica="hot-spare-1"} 0.002
themis_wal_replication_lag_bytes{replica="hot-spare-1"} 16
themis_wal_segments_total 156
themis_wal_size_bytes 4294967296
themis_wal_sync_operations_total 125840
themis_wal_sync_duration_seconds_sum 45.2
```

**Recovery-Szenarien:**

**Szenario 1: Hot Spare Promotion**
```bash
# Primary fällt aus
Primary: FAILED

# Hot Spare wird promoted (automatisch)
Hot Spare: STANDBY → RECOVERY → ACTIVE

# Steps:
1. Apply remaining WAL entries (0-2s)
2. Update routing table (1-2s)
3. Accept new writes (2-3s)
Total: ~5s
```

**Szenario 2: Point-in-Time Recovery (PITR)**
```aql
// Restore auf bestimmten Zeitpunkt
CALL RESTORE_FROM_WAL({
  target_time: '2026-01-06T12:00:00Z',
  wal_archive_path: '/backup/wal-archive',
  recovery_mode: 'pause'  // Pause nach Recovery für Validation
})
```

**Szenario 3: Failed Replica Resync**
```bash
# Replica ist zu weit zurück (>max_wal_size)
# Benötigt Base Backup + WAL Replay

# 1. Base Backup erstellen
pg_basebackup -D /data/hot-spare-1 -Fp -Xs -P

# 2. WAL Replay starten
# Automatisch, sobald Replica neu startet

# 3. Catch-up Phase
# Replica replayed ~2GB WAL in ~30s
# Streaming replication continues
```

**Performance-Impact:**

| Mode | Write Latency | Throughput | Data Loss Risk |
|------|---------------|------------|----------------|
| **Keine Replikation** | 1.2ms | 100% | High |
| **Async Replication** | 1.3ms (+8%) | 98% | Low (Lag-dependent) |
| **Sync Replication (1 Replica)** | 2.5ms (+108%) | 85% | None |
| **Sync Replication (2 Replicas)** | 3.8ms (+217%) | 70% | None |

**Best Practices:**

1. **Hybrid Mode für Production** - Sync für kritische, Async für unkritische Daten
2. **Monitoring essentiell** - Alert bei Lag >100ms
3. **Dedizierte SSDs für WAL** - Mindestens 1 IOPS pro Write
4. **Replication Slots verwenden** - Verhindert WAL-Deletion bei Replica-Ausfall
5. **Regelmäßige PITR-Tests** - Monatlich Recovery testen
6. **WAL Archivierung aktivieren** - Für Long-Term Backups

**Zusammenspiel Hot Spare + WAL Replication:**

```yaml
# Optimale Konfiguration für HA
sharding:
  cluster:
    active_shards: 4
    hot_spares: 2
    
  replication:
    mode: hybrid
    hot_spares_sync: true  # Hot Spares immer synchron
    quorum: 1
    
  wal:
    segment_size_mb: 64
    keep_segments: 200  # ~12GB für 1h Lag-Toleranz
    compression: lz4
    fsync: true
```

**Ergebnis:**
- ✅ Failover in <5s
- ✅ Zero Data Loss
- ✅ <10% Write-Latenz Overhead
- ✅ Vollautomatische Recovery

## 16.12 Troubleshooting

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

## 16.13 Zusammenfassung

**ThemisDB Sharding bietet:**
- ✅ **Production-Ready:** 91% Scaling Efficiency bis 8 Nodes
- ✅ **Auto-Rebalancing:** <15% Dip, <5min Recovery
- ✅ **Fault Tolerant:** <60s Recovery bei Replica-Failure
- ✅ **Cost-Efficient:** 67-92% günstiger als Hyperscaler
- ✅ **Native Multi-Model:** Sharding funktioniert über alle Datenmodelle

**Nächste Schritte:**
- Kapitel 19: Monitoring für detailliertes Observability-Setup
- Kapitel 21: Performance-Tuning für Sharding-spezifische Optimierungen
- Kapitel 18: High Availability für Multi-DC-Szenarien (siehe auch 16.5.6 GEO_MIRROR)
- `docs/de/SHARDING_DOCUMENTATION_INDEX.md`: Vollständige technische Dokumentation
- `docs/de/SHARDING_RAID_MODES_CONFIGURATION_v1.4.md`: Detaillierte RAID-Modus-Konfigurationen

**Production-Deployment?** Starten Sie mit einem 2-Node Cluster und skalieren Sie bei Bedarf auf 4 oder 8 Nodes!

---

## Weiterführende Ressourcen

**Dokumentation:**
- `docs/de/SHARDING_INTEGRATION_SUMMARY.md` - Master-Übersicht
- `docs/de/SHARDING_RAID_MODES_CONFIGURATION_v1.4.md` - RAID-Redundanzmodi Details
- `docs/de/SHARDING_BENCHMARK_PLAN_v1.4.md` - Enterprise Benchmark-Spezifikation
- `docs/de/SHARDING_PRODUCTION_DEPLOYMENT_RAID_v1.4.md` - Production Deployment Guide
- `docs/de/SHARDING_MONITORING_OBSERVABILITY_RAID_v1.4.md` - Monitoring & Observability

**Tools:**
- `tools/shard_loader.py` - Daten-Loader für Sharding-Tests
- `tools/shard_bench.py` - Benchmark-Runner
- `tools/fault_injector.py` - Chaos-Engineering-Tool

**Konfiguration:**
- `config/sharding/shard-router-example.yaml` - Production-Ready Config
- `.github/workflows/sharding-benchmark.yml` - CI/CD Benchmarks

## 16.14 Sharding C++ Produktions-API (v1.x) {#sharding-cpp-api}

Dieses Kapitel dokumentiert die C++-Schnittstellen des Sharding-Moduls (`include/sharding/`).

### 16.14.1 AdaptiveShardRouter — Lernender Query-Router

```cpp
#include "sharding/adaptive_shard_router.h"

themis::sharding::AdaptiveShardRouter::AdaptiveConfig acfg;
acfg.learning_rate           = 0.1;
acfg.exploration_factor      = 0.05;  // 5 % Exploration
acfg.enable_predictive_routing = true;
acfg.latency_weight          = 0.7;
acfg.load_weight             = 0.3;

themis::sharding::AdaptiveShardRouter router(topology, acfg);

// Request routen
auto decision = router.route(request_key);
// decision.shard_id, decision.endpoint, decision.used_adaptive_routing

// Feedback nach Query-Ausführung (für Lernschritt)
router.recordOutcome(request_key, decision.shard_id,
    latency_ms, error_occurred);

// Statistiken
auto stats = router.getAdaptiveStats();
// stats.total_queries, stats.adaptive_routing_pct, stats.stopped_early
```

### 16.14.2 ConsistentHashRing — Virtueller Knoten-Ring

```cpp
#include "sharding/consistent_hash.h"

themis::sharding::ConsistentHashRing ring;

// Shards hinzufügen (150 virtuelle Knoten je Shard)
ring.addShard("shard-0", 150);
ring.addShard("shard-1", 150);
ring.addShard("shard-2", 150);

// Key → Shard mappen
auto shard = ring.getShardForKey("user:alice");

// Shard entfernen (Rehash automatisch)
ring.removeShard("shard-1");

// N Shards für Replikation ermitteln
auto replicas = ring.getNShardsForKey("user:alice", 3);
```

### 16.14.3 CrossShardTransaction — Verteilte Transaktionen

```cpp
#include "sharding/cross_shard_transaction.h"

themis::sharding::CrossShardTransactionConfig txcfg;
txcfg.protocol         = themis::sharding::TransactionProtocol::TWO_PHASE_COMMIT;
txcfg.isolation_level  = themis::sharding::IsolationLevel::SERIALIZABLE;
txcfg.timeout_ms       = 5000;

themis::sharding::CrossShardTransaction tx(coordinator, txcfg);

// Transaktions-Teilnehmer registrieren
tx.addParticipant({"shard-0", "node0:8766"});
tx.addParticipant({"shard-1", "node1:8766"});

// 2PC ausführen
tx.begin();
tx.execute("shard-0", write_op_a);
tx.execute("shard-1", write_op_b);

auto result = tx.commit();  // Prepare + Commit beide Shards
// result.committed, result.participating_shards

// WAL-Eintrag für Crash-Recovery
auto wal_entry = tx.getWALEntry();
```

### 16.14.4 EpochFencingManager — Stale-Leader-Schutz

```cpp
#include "sharding/epoch_fencing.h"

themis::sharding::EpochFencingManager fence_mgr(db);

// Aktuelle Epoch abfragen
auto epoch = fence_mgr.getCurrentEpoch();

// Token für kritische Operation ausstellen
auto token = fence_mgr.acquireToken(epoch);
// token.epoch, token.expires_at

// Vor einer Schreiboperation validieren
auto result = fence_mgr.checkFence(token);
// result == FencingResult::VALID / STALE_EPOCH / EXPIRED

// Neue Epoch starten (nach Leader-Wechsel)
fence_mgr.advanceEpoch();

// Lease-Management
auto lease = fence_mgr.acquireLease("shard-coordinator", std::chrono::seconds(30));
fence_mgr.renewLease(lease.lease_id);
fence_mgr.revokeLease(lease.lease_id);
```

**FencingResult:** `VALID` / `STALE_EPOCH` / `EXPIRED` / `REVOKED`

### 16.14.5 AutoRebalancer — Automatische Shard-Aufteilung

```cpp
#include "sharding/auto_rebalancer.h"

themis::sharding::HotShardSplitPolicy::Config splcfg;
splcfg.qps_threshold                = 50000;
splcfg.enable_predictive_splitting  = true;  // ML-basiert
splcfg.min_split_interval_seconds   = 300;

themis::sharding::HotShardSplitPolicy policy(splcfg);
policy.setPredictiveDetector(&failure_detector);

// Hot-Shard erkennen
auto proposals = policy.analyzeShard("shard-3", current_qps, current_size);
for (auto& p : proposals) {
    // p.shard_id, p.suggested_split_key, p.is_predictive, p.priority_score
    if (p.priority_score > 0.8) {
        rebalancer.executeSplit(p);
    }
}
```

### 16.14.6 DataMigrator — Live-Migration

```cpp
#include "sharding/data_migrator.h"

themis::sharding::LiveMigrationConfig migcfg;
migcfg.enable_dual_write        = true;  // Schreibt gleichzeitig auf alt + neu
migcfg.verify_after_bulk_copy   = true;
migcfg.rate_limit_keys_per_sec  = 10000;
migcfg.batch_size               = 500;

themis::sharding::DataMigrator migrator(source_shard, dest_shard, migcfg);

// Fortschritt überwachen
migrator.setProgressCallback([](const themis::sharding::MigrationProgress& p) {
    std::cout << "Migrated " << p.keys_migrated
              << "/" << p.total_keys
              << " (" << p.percent_complete << "%)\n";
});

auto result = migrator.migrate("user:0000" /* from key */, "user:ffff" /* to key */);
// result.success, result.keys_migrated, result.errors, result.duration_ms
```
