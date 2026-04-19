> **Aktueller Build-Flow:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`

# ThemisDB Multi-Shard RAID Benchmark - Quick Start

## Übersicht

Vollautomatisierte Benchmark-Suite zur Evaluation von ThemisDB in Multi-Shard-Deployments mit verschiedenen RAID-Konfigurationen.

## Voraussetzungen

- Docker 20.10+ mit Buildx
- Docker Compose v2.0+
- Mindestens 64GB RAM
- 500GB+ freier Speicher
- Linux-Host mit mdadm (für echte RAID-Konfiguration)

## Quick Start

### 1. Einfacher Test (3 Shards, RAID10, 4 Stunden)

```bash
cd benchmarks
chmod +x scripts/run_benchmark.sh
./scripts/run_benchmark.sh S1 RAID10 3 4
```

### 2. Production-Szenario (6 Shards, RAID10, 12 Stunden)

```bash
./scripts/run_benchmark.sh S4 RAID10 6 12
```

### 3. Large-Scale OLAP (12 Shards, RAID6, 18 Stunden)

```bash
./scripts/run_benchmark.sh S5 RAID6 12 18
```

## Verfügbare Szenarien

| Szenario | Shards | RAID | Workload | Daten | Dauer | Beschreibung |
|----------|--------|------|----------|-------|-------|--------------|
| **S1** | 3 | RAID0 | OLTP | 100GB | 4h | Baseline Performance |
| **S2** | 3 | RAID1 | OLTP | 100GB | 6h | High Availability |
| **S3** | 3 | RAID5 | OLTP | 100GB | 8h | Balanced |
| **S4** | 6 | RAID10 | Mixed | 500GB | 12h | Production Standard |
| **S5** | 12 | RAID6 | OLAP | 1TB | 18h | Data Warehouse |
| **S6** | 24 | RAID10 | TimeSeries | 1TB | 24h | Time-Series Workload |
| **S7** | 6 | RAID5 | VectorSearch | 500GB | 10h | Vector Similarity |
| **S8** | 12 | RAID1 | Mixed | 500GB | 16h | Multi-DC Failover |

## Manuelle Ausführung

### Cluster starten

```bash
# Umgebungsvariablen setzen
export SCENARIO=S4
export RAID_LEVEL=RAID10
export NUM_SHARDS=6
export WORKLOAD_TYPE=Mixed
export DURATION_HOURS=12

# Cluster mit 6 Shards starten
docker-compose -f docker-compose.multi-shard-raid.yml --profile 6-shards up -d
```

### Daten laden

```bash
export DOCUMENTS_TOTAL=25000000  # 25M docs für 500GB
docker-compose -f docker-compose.multi-shard-raid.yml --profile data-load up data-loader
```

### Benchmark ausführen

```bash
docker-compose -f docker-compose.multi-shard-raid.yml --profile benchmark up benchmark-controller
```

### Monitoring

- **Grafana:** http://localhost:3000 (admin/admin)
- **Prometheus:** http://localhost:9090
- **ThemisDB Shard 0:** http://localhost:8080/health

### Ergebnisse analysieren

```bash
# Ergebnisse anzeigen
ls -lh results/

# Letzte Ergebnisse anzeigen
cat results/S4_RAID10_*.json | jq .

# Vergleich mehrerer Läufe
python analyze_results.py results/S*.json
```

### Cluster stoppen

```bash
docker-compose -f docker-compose.multi-shard-raid.yml down -v
```

## Erwartete Ergebnisse

### S1: RAID0 Baseline (3 Shards)
- **Throughput:** >10,000 QPS
- **Latency P99:** <10ms (Point Operations)
- **CPU:** 60-70% average

### S4: RAID10 Production (6 Shards)
- **OLTP Throughput:** >50,000 QPS
- **OLAP Throughput:** >500 complex queries/sec
- **Latency P99:** <15ms (OLTP), <200ms (Cross-Shard Join)

### S5: RAID6 Data Warehouse (12 Shards)
- **Query Throughput:** 200-500 concurrent complex queries
- **Scan Throughput:** 3-5 GB/s aggregate
- **Join Latency P95:** 2-5 seconds (Star Schema)

## Troubleshooting

### Port-Konflikte

```bash
# Prüfen, ob Ports belegt sind
netstat -tulpn | grep -E '8080|9090|3000'

# Andere Ports verwenden (docker-compose.yml anpassen)
# Oder bestehende Services stoppen
```

### Speicherplatz

```bash
# Verfügbaren Speicher prüfen
df -h

# Docker-Volumes bereinigen
docker volume prune -f
docker system prune -a --volumes -f
```

### Performance-Probleme

```bash
# CPU/Memory-Limits anpassen
export SHARD_CPU_LIMIT=16
export SHARD_MEMORY_LIMIT=64G

# Oder in docker-compose.yml direkt anpassen
```

### Logs ansehen

```bash
# Alle Logs
docker-compose -f docker-compose.multi-shard-raid.yml logs -f

# Nur Shard 0
docker-compose -f docker-compose.multi-shard-raid.yml logs -f themis-shard-0

# Benchmark Controller
docker-compose -f docker-compose.multi-shard-raid.yml logs -f benchmark-controller
```

## Konfiguration

### RAID-Level ändern

Unterstützte RAID-Level: `RAID0`, `RAID1`, `RAID5`, `RAID6`, `RAID10`

```bash
export RAID_LEVEL=RAID6
./scripts/run_benchmark.sh S5 RAID6 12 18
```

### Shard-Count ändern

Unterstützte Werte: 3, 6, 12, 24

```bash
export NUM_SHARDS=12
# Passenden Docker Compose Profile verwenden
docker-compose --profile 12-shards up -d
```

### Workload-Parameter

```bash
export TARGET_QPS=100000      # Ziel-Durchsatz
export CONCURRENT_CLIENTS=256 # Parallele Clients
export DURATION_HOURS=8       # Test-Dauer
```

## Ergebnis-Format

```json
{
  "scenario": "S4",
  "shard_count": 6,
  "raid_level": "RAID10",
  "workload_type": "Mixed",
  "duration_seconds": 43200.5,
  "total_queries": 1296000,
  "successful_queries": 1294850,
  "failed_queries": 1150,
  "throughput_qps": 29.98,
  "latency_p50_ms": 8.2,
  "latency_p95_ms": 24.5,
  "latency_p99_ms": 45.3,
  "latency_p999_ms": 123.7,
  "latency_max_ms": 1205.4,
  "cpu_usage_avg_pct": 65.3,
  "disk_iops_avg": 18500,
  "network_tx_mbps": 450.2
}
```

## Best Practices

1. **Baseline zuerst:** Starte mit S1 (RAID0, 3 Shards) für Baseline-Daten
2. **Inkrementell skalieren:** S1 → S2 → S3 → S4 für Vergleichbarkeit
3. **Warmup beachten:** Erste 10-15 Minuten jedes Tests sind Warmup
4. **Monitoring aktivieren:** Grafana-Dashboards kontinuierlich überwachen
5. **Mehrere Runs:** Mindestens 3 Runs pro Szenario für statistische Signifikanz

## Support

- **Logs:** `./logs/shard-*/` und Docker Logs
- **Metriken:** Prometheus + Grafana
- **Rohdaten:** `./results/*.jsonl` (alle Einzelmessungen)

## Nächste Schritte

Nach erfolgreichem Benchmark:

1. Ergebnisse analysieren und vergleichen
2. Optimale RAID-Konfiguration pro Workload identifizieren
3. Shard-Count-Sizing basierend auf Datenvolumen
4. Cost-Performance-Matrix erstellen
5. Production-Deployment-Guide ableiten
