# BENCHMARK_STATUS (Archived)

Status: Archived redirect stub (2026-08-21)

This status document was moved to historical docs during benchmark cleanup.

Archived copy:
- [historical/2026-08/BENCHMARK_STATUS.md](historical/2026-08/BENCHMARK_STATUS.md)

Canonical sources:
- [../BENCHMARK_STANDARDS.md](../BENCHMARK_STANDARDS.md)
- [../MEASUREMENT_HYGIENE.md](../MEASUREMENT_HYGIENE.md)
- [../README.md](../README.md)

Usage:
- Use canonical sources for current benchmark rules and active status.
- Use archived copy only for historical context.

```bash
# Scenario S1 (4 Stunden, schneller Test)
./benchmarks/run_benchmark.ps1 -Scenario S1 -RaidLevel RAID10 -NumShards 3 -DurationHours 4

# Scenario S4 (Production Standard)
./benchmarks/run_benchmark.ps1 -Scenario S4 -RaidLevel RAID10 -NumShards 6 -DurationHours 12

# Scenario S5 (Data Warehouse)
./benchmarks/run_benchmark.ps1 -Scenario S5 -RaidLevel RAID6 -NumShards 12 -DurationHours 18
```

## 📈 Erwartete Metriken

### Ausgabe-Format (JSON)
```json
{
  "scenario": "S4",
  "shard_count": 6,
  "raid_level": "RAID10",
  "workload_type": "Mixed",
  "throughput_qps": 29.98,
  "latency_p50_ms": 8.2,
  "latency_p95_ms": 24.5,
  "latency_p99_ms": 45.3,
  "successful_queries": 1294850,
  "failed_queries": 1150,
  "cpu_usage_avg_pct": 65.3,
  "disk_iops_avg": 18500,
  "network_tx_mbps": 450.2
}
```

## 🔧 Konfigurierbar

- **RAID-Level:** RAID0, RAID1, RAID5, RAID6, RAID10
- **Shard-Count:** 3, 6, 12, 24 (beliebig erweiterbar)
- **Workload-Typen:** OLTP, OLAP, Mixed, TimeSeries, VectorSearch
- **Datenvolumen:** 100GB, 500GB, 1TB (skalierbar)
- **Dauer:** 4-96 Stunden (konfigurierbar)

## 📊 Monitoring

- **Grafana:** http://localhost:3000 (admin/admin)
- **Prometheus:** http://localhost:9090
- **Shard Health:** http://localhost:8080-8005 (abhängig von Shard-Count)

## 📁 Verzeichnisstruktur

```
benchmarks/
├── docker-compose.multi-shard-raid.yml
├── run_multi_shard_raid_benchmark.py
├── run_benchmark.ps1
├── run_benchmark.sh
├── scripts/
│   ├── load_test_data.py
│   └── run_benchmark.sh
├── monitoring/
│   ├── prometheus.yml
│   └── grafana/
│       └── datasources.yml
├── results/                           # Benchmark-Ergebnisse
├── logs/                              # Logs der Shards
├── MULTI_SHARD_RAID_BENCHMARK_PLAN.md
├── MULTI_SHARD_RAID_QUICKSTART.md
└── analyze_results.py
```

## ⚙️ Systemanforderungen

- **RAM:** Mindestens 64GB (für 12+ Shards)
- **CPU:** 16+ Kerne (für parallele Workloads)
- **Speicher:** 500GB+ (für Test-Daten)
- **Docker:** 20.10+ mit Compose v2.0+

## 🔍 Häufig verwendete Befehle

```bash
# Alle Szenarien auflisten
ls -la benchmarks/results/

# Letzte Ergebnisse anzeigen
cat benchmarks/results/*/summary.json | jq .

# Vergleich über alle Tests
python benchmarks/analyze_results.py

# Logs eines Shards ansehen
docker-compose -f benchmarks/docker-compose.multi-shard-raid.yml logs themis-shard-0

# Cluster stoppen
docker-compose -f benchmarks/docker-compose.multi-shard-raid.yml down -v
```

## 📋 Nächste Schritte

1. **Erste Tests ausführen:**
   ```bash
   ./benchmarks/run_benchmark.ps1 -Scenario S1 -RaidLevel RAID10 -NumShards 3 -DurationHours 4
   ```

2. **Ergebnisse überprüfen:**
   ```bash
   python benchmarks/analyze_results.py
   ```

3. **Grafana öffnen:**
   ```
   http://localhost:3000
   ```

4. **Production-Szenarios testen:**
   - S4 (Standard 6 Shards)
   - S5 (Data Warehouse 12 Shards)

## 🐛 Troubleshooting

**Problem:** Docker-Fehler beim Start
```bash
# Cleanup und Retry
docker-compose -f benchmarks/docker-compose.multi-shard-raid.yml down -v
docker system prune -f
```

**Problem:** Zu hohe RAM-Nutzung
```bash
# Shard-Ressourcen begrenzen in docker-compose.yml
# Oder kleinere Datenvolumina verwenden (S1-S3 statt S5-S8)
```

**Problem:** Netzwerk-Timeouts
```bash
# Health-Check verbessern
docker-compose -f benchmarks/docker-compose.multi-shard-raid.yml exec themis-shard-0 curl http://localhost:8080/health
```

## 📞 Support

- Logs: `benchmarks/logs/`
- Metriken: Prometheus + Grafana
- Rohdaten: `benchmarks/results/*/`
- Dokumentation: `benchmarks/MULTI_SHARD_RAID_*.md`

---
**Status:** ✅ Produktionsreif
**Letzte Aktualisierung:** 11. Dezember 2025
