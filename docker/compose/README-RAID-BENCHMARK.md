# ThemisDB RAID Benchmark Suite - Hyperscaler Edition

Comprehensive Docker-based RAID testing infrastructure für ThemisDB Hyperscaler Edition v1.4.0.

## Übersicht

Diese Benchmark-Suite testet RAID-Konfigurationen (RAID0, RAID1, RAID5, RAID6, RAID10) über mehrere Docker-Container hinweg mit realistischen Workloads.

### Benchmark-Suiten

Die `bench_docker_raid_comprehensive` Suite umfasst:

1. **SmallDocumentWrite** - Kleine JSON-Dokumente (1-10 KB)
2. **MediumDocumentWrite** - Mittlere Dokumente (100-500 KB)
3. **LargeBlobWrite** - Große Binärdaten (1-100 MB)
4. **RandomRead** - Zufällige Lesezugriffe über Containers
5. **ContainerFailover** - Ausfallszenarien und Recovery
6. **ConcurrentOperations** - Gleichzeitige Lese-/Schreibvorgänge
7. **MixedReadWrite** - Realistische gemischte Workloads
8. **SynchronizationLatency** - RAID-Synchronisierungszeiten
9. **CrossContainerQuery** - AQL-Queries über Container-Grenzen
10. **DynamicRebalancing** - Dynamisches Rebalancing bei Last

**Geschätzte Laufzeit**: 1-2 Stunden (je nach Hardware)

## Quick Start

### 1. Docker Image Build Status überprüfen

```bash
# Status des laufenden Builds
docker images themisdb:1.4.0-hyperscaler

# Sobald fertig (nach ~20-30 Min):
docker images | grep themisdb
```

### 2. RAID-Umgebung starten

```bash
# In docker/compose Verzeichnis wechseln
cd docker/compose

# RAID-Cluster mit 6 Nodes starten
docker-compose -f docker-compose-raid-hyperscaler.yml up -d

# Status überprüfen
docker-compose -f docker-compose-raid-hyperscaler.yml ps

# Logs verfolgen
docker-compose -f docker-compose-raid-hyperscaler.yml logs -f themis-raid-controller
```

### 3. Cluster Health Check

```bash
# Controller Health
curl http://localhost:8080/health

# Prometheus Metriken
curl http://localhost:4318/metrics

# Grafana Dashboard (Browser)
open http://localhost:3000
# Login: admin / themis_admin
```

### 4. RAID-Konfiguration initialisieren

```bash
# RAID0 (Striping) - 3 Nodes
curl -X POST http://localhost:8080/api/raid/configure \
  -H "Content-Type: application/json" \
  -d '{
    "type": "RAID0",
    "nodes": ["raid-node-1", "raid-node-2", "raid-node-3"],
    "stripe_size_kb": 64
  }'

# RAID1 (Mirroring) - 2 Nodes
curl -X POST http://localhost:8080/api/raid/configure \
  -H "Content-Type: application/json" \
  -d '{
    "type": "RAID1",
    "nodes": ["raid-node-1", "raid-node-2"],
    "mirrors": 2
  }'

# RAID5 (Striping + Parity) - 4 Nodes
curl -X POST http://localhost:8080/api/raid/configure \
  -H "Content-Type: application/json" \
  -d '{
    "type": "RAID5",
    "nodes": ["raid-node-1", "raid-node-2", "raid-node-3", "raid-node-4"],
    "stripe_size_kb": 64,
    "parity_rotation": "left-symmetric"
  }'

# RAID6 (Double Parity) - 6 Nodes
curl -X POST http://localhost:8080/api/raid/configure \
  -H "Content-Type: application/json" \
  -d '{
    "type": "RAID6",
    "nodes": ["raid-node-1", "raid-node-2", "raid-node-3", "raid-node-4", "raid-node-5", "raid-node-6"],
    "stripe_size_kb": 128,
    "parity_scheme": "reed-solomon"
  }'

# RAID10 (Mirroring + Striping) - 4 Nodes
curl -X POST http://localhost:8080/api/raid/configure \
  -H "Content-Type: application/json" \
  -d '{
    "type": "RAID10",
    "nodes": ["raid-node-1", "raid-node-2", "raid-node-3", "raid-node-4"],
    "mirrors": 2,
    "stripe_size_kb": 64
  }'
```

### 5. Benchmarks ausführen

#### Option A: Im Build-Container (empfohlen)

```bash
# MSVC Build-Container
cd c:\VCC\themis\build-msvc

# Vollständige Suite (1+ Stunde)
.\Release\bench_docker_raid_comprehensive.exe --benchmark_out=raid_results.json --benchmark_out_format=json

# Einzelne Suite schneller
.\Release\bench_docker_raid_comprehensive.exe --benchmark_filter="SmallDocumentWrite" --benchmark_repetitions=3

# Mit Prometheus Export
.\Release\bench_docker_raid_comprehensive.exe --benchmark_out=raid_results.json --prometheus_export=true --prometheus_port=9091
```

#### Option B: Im Docker-Container ausführen

```bash
# Benchmark Binary in Controller kopieren
docker cp build-msvc/Release/bench_docker_raid_comprehensive.exe themis-raid-controller:/tmp/

# Im Container ausführen
docker exec -it themis-raid-controller /tmp/bench_docker_raid_comprehensive.exe --benchmark_out=/tmp/results.json

# Ergebnisse zurückkopieren
docker cp themis-raid-controller:/tmp/results.json ./raid_benchmark_results.json
```

### 6. Ergebnisse analysieren

```bash
# Python-Analyse-Skript
python benchmarks/analyze_raid_benchmarks.py raid_results.json

# Prometheus Queries (Browser)
open http://localhost:9090

# Grafana Dashboards
open http://localhost:3000/d/raid-benchmarks
```

## Architektur

```
┌─────────────────────────────────────────────────────────────────┐
│                     RAID Controller                              │
│  (themis-raid-controller:8080, :18765, :4318)                   │
│  - Orchestriert RAID-Konfiguration                              │
│  - Verteilt Schreibvorgänge                                      │
│  - Sammelt Metriken                                              │
└─────────────┬──────────┬──────────┬──────────┬──────────────────┘
              │          │          │          │
    ┌─────────┴──┐  ┌────┴─────┐  ┌┴─────────┐│
    │            │  │          │  │          ││
    v            v  v          v  v          vv
┌───────┐   ┌───────┐   ┌───────┐   ┌───────┐
│Node 1 │   │Node 2 │   │Node 3 │   │Node 4 │ ...
│:18766 │   │:18767 │   │:18768 │   │:18769 │
└───────┘   └───────┘   └───────┘   └───────┘
    │           │           │           │
    └───────────┴───────────┴───────────┴────────────────┐
                                                          │
                    ┌────────────────────────────────────┘
                    v
            ┌──────────────┐       ┌──────────┐
            │  Prometheus  │──────>│ Grafana  │
            │    :9090     │       │  :3000   │
            └──────────────┘       └──────────┘
```

## Container-Konfiguration

### RAID Controller
- **Ports**: 8080 (HTTP), 18765 (Wire Protocol), 4318 (Metrics)
- **Ressourcen**: 2 CPU, 4 GB RAM
- **Rolle**: Orchestrierung, Load Balancing, Failover

### RAID Nodes (1-6)
- **Ports**: 8081-8086 (HTTP), 18766-18771 (Wire Protocol)
- **Ressourcen**: 2 CPU, 4 GB RAM pro Node
- **Rolle**: Datenreplikation, Parity-Berechnung

### Monitoring
- **Prometheus**: Metriken-Sammlung (15s Intervall)
- **Grafana**: Visualisierung und Alerting

## Benchmark-Parameter

### Workload-Größen
- **Small**: 1-10 KB (JSON-Dokumente)
- **Medium**: 100-500 KB (strukturierte Daten)
- **Large**: 1-100 MB (Blobs, Binärdaten)

### RAID-Stripe-Sizes
- **RAID0/1**: 64 KB (Standard)
- **RAID5**: 64 KB (optimiert für sequentielle I/O)
- **RAID6**: 128 KB (optimiert für Parity-Berechnung)
- **RAID10**: 64 KB (Mirror + Stripe)

### Benchmark-Optionen

```bash
# Minimum: Schnelltest (5-10 Min)
--benchmark_min_time=1s --benchmark_repetitions=1

# Standard: Balanced (30-60 Min)
--benchmark_min_time=5s --benchmark_repetitions=3

# Extensive: Volle Suite (1-2 Stunden)
--benchmark_min_time=10s --benchmark_repetitions=10

# Filter für spezifische Tests
--benchmark_filter="SmallDocumentWrite|RandomRead"
--benchmark_filter="RAID5.*"

# Output-Formate
--benchmark_out=results.json --benchmark_out_format=json
--benchmark_out=results.csv --benchmark_out_format=csv
```

## Fehlerszenarien

### Node-Ausfall simulieren

```bash
# Node 3 stoppen
docker-compose -f docker-compose-raid-hyperscaler.yml stop themis-raid-node-3

# RAID-Recovery-Zeit messen
time docker-compose -f docker-compose-raid-hyperscaler.yml start themis-raid-node-3

# Logs prüfen
docker-compose -f docker-compose-raid-hyperscaler.yml logs themis-raid-controller | grep -i "failover\|recovery"
```

### Netzwerk-Latenz simulieren

```bash
# 100ms Latenz zu Node 2 hinzufügen
docker exec themis-raid-controller tc qdisc add dev eth0 root netem delay 100ms

# Latenz entfernen
docker exec themis-raid-controller tc qdisc del dev eth0 root
```

### Disk-Pressure simulieren

```bash
# Node mit vielen Schreibvorgängen belasten
docker exec themis-raid-node-1 dd if=/dev/zero of=/var/lib/themisdb/stress bs=1M count=1000
```

## Metriken und Monitoring

### Prometheus Queries

```promql
# Durchsatz pro Node
rate(themis_bytes_written_total[5m])

# Durchschnittliche Latenz
rate(themis_operation_duration_seconds_sum[5m]) / rate(themis_operation_duration_seconds_count[5m])

# RAID-Synchronisierung
themis_raid_sync_lag_seconds

# Fehlerrate
rate(themis_operation_errors_total[5m])
```

### Grafana Dashboards

1. **RAID Overview**: Cluster-Status, Node Health, Throughput
2. **Performance Metrics**: Latenz, IOPS, Bandbreite
3. **Failure Analysis**: Failover-Zeiten, Recovery-Status
4. **Resource Usage**: CPU, Memory, Network pro Node

## Bereinigung

```bash
# Containers stoppen und entfernen
docker-compose -f docker-compose-raid-hyperscaler.yml down

# Mit Volumes (Daten löschen)
docker-compose -f docker-compose-raid-hyperscaler.yml down -v

# Alle ThemisDB Images entfernen
docker rmi themisdb:1.4.0-hyperscaler themisdb:hyperscaler

# System cleanup
docker system prune -a --volumes
```

## Troubleshooting

### Build schlägt fehl

```bash
# Docker Build Logs prüfen
docker build -f docker/Dockerfile.hyperscaler --progress=plain --no-cache .

# Vcpkg Cache leeren
rm -rf vcpkg_installed/
```

### Container startet nicht

```bash
# Logs anzeigen
docker-compose -f docker-compose-raid-hyperscaler.yml logs themis-raid-controller

# Healthcheck manuell
docker exec themis-raid-controller curl -f http://localhost:8080/health
```

### Benchmark hängt

```bash
# Prozesse prüfen
docker exec themis-raid-controller ps aux | grep bench

# Strace für Debugging
docker exec themis-raid-controller strace -p <PID>
```

## Erweiterte Konfiguration

### Custom Benchmark-Parameter

Erstelle `benchmarks/raid_benchmark_config.json`:

```json
{
  "raid_types": ["RAID0", "RAID1", "RAID5", "RAID6", "RAID10"],
  "stripe_sizes_kb": [32, 64, 128, 256],
  "document_sizes_bytes": [1024, 10240, 102400, 1048576, 10485760],
  "concurrent_clients": [1, 4, 8, 16, 32],
  "test_duration_seconds": 300,
  "repetitions": 5
}
```

### Custom Docker-Compose

Für größere Cluster (12+ Nodes):

```bash
cp docker-compose-raid-hyperscaler.yml docker-compose-raid-large.yml
# Nodes 7-12 hinzufügen mit entsprechenden Ports
```

## Performance-Erwartungen

### Typische Durchsätze (Pro Node)

- **RAID0**: ~500 MB/s Write, ~600 MB/s Read
- **RAID1**: ~250 MB/s Write, ~500 MB/s Read
- **RAID5**: ~350 MB/s Write, ~550 MB/s Read
- **RAID6**: ~280 MB/s Write, ~520 MB/s Read
- **RAID10**: ~400 MB/s Write, ~550 MB/s Read

### Typische Latenzen

- **Small Documents (1-10 KB)**: 1-5 ms
- **Medium Documents (100-500 KB)**: 10-50 ms
- **Large Blobs (1-100 MB)**: 200-2000 ms

*Hinweis: Abhängig von Hardware, Docker-Overhead und Netzwerk-Latenz*

## Weitere Ressourcen

- [ThemisDB HYPERSCALER Documentation](../../docs/editions/hyperscaler.md)
- [RAID Configuration Guide](../../docs/storage/raid.md)
- [Benchmark Analysis Guide](../../benchmarks/ADVANCED_BENCHMARKS_GUIDE.md)
- [Performance Tuning](../../docs/performance/tuning.md)

## Lizenz

ThemisDB HYPERSCALER Edition - Copyright © 2024 VCC
Siehe [LICENSE](../../LICENSE) für Details.
