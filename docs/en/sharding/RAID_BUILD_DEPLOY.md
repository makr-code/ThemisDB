# Themis RAID Setup – Build & Deploy Guide

## 1) Voraussetzungen
- Docker (Desktop) mit Linux Backend
- Optional: Docker Buildx, WSL2 (für schnellere Builds unter Windows)
- Ports frei: 18765-18772, 8080-8087, 9090, 3000

## 2) Image bauen (Hyperscaler, alle Features aktiv)
- Standard: LLM=ON, GPU=ON, Metrics=ON (Default in Dockerfile.themis-server)
- Tests/Benchmarks sind im Dockerfile deaktiviert (`THEMIS_BUILD_TESTS=OFF`, `THEMIS_BUILD_BENCHMARKS=OFF`), um fehlende Testquellen zu umgehen.

Schneller Build (mit Cache):
```powershell
cd C:\VCC\themis
docker build -f Dockerfile.themis-server -t themisdb/themisdb:hyperscaler .
```

Sauberer Build (ohne Cache, wenn vorheriger Build fehlschlug):
```powershell
cd C:\VCC\themis
docker build --no-cache -f Dockerfile.themis-server -t themisdb/themisdb:hyperscaler .
```

Tags ergänzen (optional Latest):
```powershell
docker tag themisdb/themisdb:hyperscaler themisdb/themisdb:latest
```

## 3) Deployment (RAID-Cluster)
Compose-Datei: [docker/compose/docker-compose-sharding.yml](docker/compose/docker-compose-sharding.yml)
- 9 Shards: RAID0 (3), RAID1 (2), RAID5 (3)
- Prometheus + Grafana enthalten
- Alle Services nutzen `themisdb/themisdb:hyperscaler`

Start (alte Container stoppen/entfernen, dann neu starten):
```bash
cd docker/compose
docker-compose -f docker-compose-sharding.yml down
docker-compose -f docker-compose-sharding.yml up -d
```

Healthcheck & Status:
```bash
docker ps --format "table {{.Names}}\t{{.Status}}"
```

## 4) Metriken & Monitoring
- REST + /metrics pro Shard: Ports 8080-8087 (siehe Compose-Portmapping)
- Prometheus: http://localhost:9090
- Grafana: http://localhost:3000 (admin/admin)
- Dashboard-JSON: [benchmarks/monitoring/themis_raid_benchmark_dashboard.json](benchmarks/monitoring/themis_raid_benchmark_dashboard.json)
  - Import in Grafana unter Dashboards → Import → JSON hochladen

Schnelle Checks:
```bash
# Metrics eines Shards
curl http://localhost:8080/metrics | head -5

# Prometheus Targets
curl http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | {job: .labels.job, instance: .labels.instance}'
```

## 5) RAID-Benchmark/Smoke-Test
(Optional, wenn binary im Container vorhanden)
```bash
docker exec themis-raid0-shard1 /usr/local/bin/themis_server --benchmark-raid
```

## 6) Häufige Build-Probleme
- Fehlende Tests/Benchmarks: im Dockerfile deaktiviert (`THEMIS_BUILD_TESTS=OFF`, `THEMIS_BUILD_BENCHMARKS=OFF`).
- Großes Build-Context: `.dockerignore` ist aggressiv (51MB Kontext). Nicht zurückdrehen.
- Fehlende Tools (flex/bison): bereits in Dockerfile installiert.

## 7) Cleanup
```bash
docker-compose -f docker-compose-sharding.yml down -v
docker image rm themisdb/themisdb:hyperscaler
```
