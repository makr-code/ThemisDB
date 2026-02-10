# RAID Test - Quick Start Guide

## 1. Warten Sie bis Docker Build abgeschlossen ist

```powershell
docker images | Select-String "hyperscaler"
# Sollte anzeigen: themisdb/themisdb  hyperscaler
```

## 2. RAID Cluster starten

```bash
cd docker/compose
docker-compose -f docker-compose-sharding.yml down   # Alte Container löschen
docker-compose -f docker-compose-sharding.yml up -d  # Alle 9 RAID Shards + Monitoring
```

## 3. Statuscheck (alle Container sollten "healthy" sein)

```bash
docker ps --format "table {{.Names}}\t{{.Status}}" | grep -E "themis|prometheus|grafana"
```

## 4. Metriken Endpoints überprüfen

```bash
# RAID0-Shard1 Metriken
curl http://localhost:8080/metrics

# Alle anderen Shards (verschiedene Ports)
for i in {8080..8087}; do echo "=== Port $i ==="; curl -s http://localhost:$i/metrics | head -5; done
```

## 5. Prometheus Datenquellen validieren

```bash
curl http://localhost:9090/api/v1/targets | jq '.data.activeTargets[] | {job: .labels.job, instance: .labels.instance}'
```

## 6. Grafana Dashboard öffnen

```
URL: http://localhost:3000
User: admin
Password: admin
Dashboard: "Themis RAID Benchmark"
```

## 7. RAID Benchmark Test durchführen

```bash
docker exec themis-raid0-shard1 /usr/local/bin/themis_server --benchmark-raid

# Oder manuelle Last erzeugen mit:
docker exec themis-raid0-shard1 curl -X POST http://localhost:18765/api/v1/insert \
  -H "Content-Type: application/json" \
  -d '{"data": "test", "count": 1000000}'
```

## 8. Monitoring Dashboard

Öffnen Sie Grafana und überprüfen Sie folgende Panels:
- **RAID I/O Throughput** - Durchsatz aller 9 Shards
- **Operation Latency (p95/p99)** - Latenzverteilung
- **Operations per Second** - Transaktionen pro Sekunde
- **Average Throughput by RAID Level** - Vergleich RAID0 vs RAID1 vs RAID5

## 9. Troubleshooting

```bash
# Container Logs überprüfen
docker logs themis-raid0-shard1

# Netzwerk überprüfen
docker network inspect themis-network

# Prometheus Ziele überprüfen
docker logs themis-prometheus | grep -i error
```

## Erwartete Performance

### RAID 0 (Striping)
- **Durchsatz:** ~1.5GB/s (höchster)
- **Latenz p99:** 50-100ms
- **Nutzung:** Reine Performance

### RAID 1 (Mirror)
- **Durchsatz:** ~750MB/s (Hälfte von RAID0)
- **Latenz p99:** 100-150ms
- **Nutzung:** Redundanz mit akzeptabler Performance

### RAID 5 (Striping + Parity)
- **Durchsatz:** ~1GB/s (besser als RAID1, schlechter als RAID0)
- **Latenz p99:** 75-125ms
- **Nutzung:** Balance zwischen Performance und Redundanz
