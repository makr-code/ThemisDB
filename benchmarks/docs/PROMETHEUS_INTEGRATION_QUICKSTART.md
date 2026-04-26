> **Aktueller Build-Flow:** `cmake --preset linux-ninja-perf && cmake --build --preset linux-ninja-perf`

# Prometheus Integration - Schnellstart

## Status: ✅ Integration abgeschlossen

### Was wurde integriert:

1. **Prometheus C++ Client** hinzugefügt zu vcpkg.json
2. **CMakeLists.txt** aktualisiert mit prometheus-cpp Linking
3. **Benchmark-Code** erweitert um Metrics-Export auf Port 9091
4. **Monitoring-Stack** läuft auf Docker:
   - Grafana: http://localhost:3000 (admin/admin)
   - Prometheus: http://localhost:19090
   - ThemisDB Metrics: http://localhost:9091/metrics

### Aktueller Stack-Status:

```
✅ themisdb-prometheus   running   0.0.0.0:19090->9090/tcp
✅ themisdb-grafana      running   0.0.0.0:3000->3000/tcp  
✅ themisdb              running   0.0.0.0:19091->9091/tcp
```

### Metrics die exportiert werden:

1. **themis_raid_throughput_bytes_per_second** (Gauge)
   - RAID write/read throughput
   - Labels: raid_level, containers

2. **themis_raid_operation_latency_seconds** (Histogram)
   - Operation latency distribution
   - Buckets: .001, .005, .01, .05, .1, .5, 1, 5, 10

3. **themis_raid_operations_total** (Counter)
   - Total operations completed
   - Labels: operation_type, raid_level

### Benchmark starten mit Metrics:

```powershell
cd C:\VCC\themis\build-msvc\Release
.\bench_docker_raid_comprehensive.exe --benchmark_min_time=1s
```

Metrics-Server startet automatisch auf Port 9091 beim Benchmark-Start.

### Grafana Dashboard einrichten:

1. Öffne http://localhost:3000 (admin/admin)
2. Configuration → Data Sources → Add Prometheus
   - URL: http://themisdb-prometheus:9090
3. Create Dashboard → Add Panel
4. Query examples:
   ```promql
   # Durchsatz über Zeit
   rate(themis_raid_throughput_bytes_per_second[5m])
   
   # 95th Percentile Latenz
   histogram_quantile(0.95, 
     rate(themis_raid_operation_latency_seconds_bucket[5m])
   )
   
   # Operations pro Sekunde
   rate(themis_raid_operations_total[5m])
   ```

### Nächste Schritte:

1. ✅ Monitoring-Stack läuft
2. ✅ Prometheus-Export integriert
3. ⏳ Längeren Benchmark-Lauf starten (10+ Minuten)
4. ⏳ Grafana-Dashboard mit RAID-Metriken erstellen
5. ⏳ Alerting-Regeln definieren

### Port-Änderungen (wegen Konflikten):

- Prometheus: 9090 → 19090 (Port 9090 war belegt)
- ThemisDB Metrics: 9091 (direkter Host-Zugriff)
- Grafana: 3000 (unverändert)

### Troubleshooting:

**Keine Metriken in Grafana?**
```powershell
# 1. Check Metrics-Endpoint
curl http://localhost:9091/metrics

# 2. Check Prometheus Targets
# http://localhost:19090/targets

# 3. Restart Prometheus
cd C:\VCC\themis\grafana
docker-compose restart prometheus
```

**Benchmark zeigt keine Metriken?**
- Stelle sicher, dass Benchmark läuft (mindestens 30s für sichtbare Daten)
- Check dass Port 9091 nicht blockiert ist
- Logs prüfen: Der Benchmark gibt beim Start aus:
  ```
  Prometheus metrics server started on http://0.0.0.0:9091/metrics
  ```

---

**Erstellt:** 2026-01-03 16:25 CET  
**Status:** Ready for Production Testing
