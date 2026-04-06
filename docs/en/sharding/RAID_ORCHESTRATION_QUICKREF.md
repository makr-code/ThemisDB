# RAID LoRA Orchestration - Quick Reference

## 🚀 Start (5 Minutes)

```bash
cd C:\VCC\themis

# 1. Build & Start (first time: ~5 min)
make -f Makefile.raid-tests build up

# 2. Wait for Shards (watch this)
watch -n 1 "docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml ps"

# 3. Run Tests
make -f Makefile.raid-tests test-all

# 4. View Results
make -f Makefile.raid-tests results
cat test_results/test_report.html  # Open in browser
cat test_results/orchestrator_results.json | jq .

# 5. Cleanup
make -f Makefile.raid-tests clean
```

## 📊 Monitoring

```bash
# Prometheus
http://localhost:9090

# Grafana
http://localhost:3000  (admin/themis)

# Logs
docker logs -f themis-llm-raid-tests
docker logs -f themis-raid0-shard1

# Health Check
docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml ps
```

## 🧪 Test Modes

| Mode | Command | Tests | Benchmarks | Duration |
|------|---------|-------|-----------|----------|
| **inline** | `make test-inline` | LoRA | — | ~5s |
| **pipeline** | `make test-pipeline` | RAID | — | ~8s |
| **all_tests** | `make test` | ALL | — | ~15s |
| **all_bench** | `make test-bench` | — | ALL | ~30s |
| **all** | `make test-all` | ALL | ALL | ~45s |

## 📂 Files & Locations

| Component | File | Lines | Purpose |
|-----------|------|-------|---------|
| **Orchestrator** | `docker/raid_lora_orchestrator.py` | 480 | Main coordinator |
| **Data Pusher** | `include/raid_data_pusher.h` | 280 | HTTP client |
| **Tests** | `tests/test_llm_raid_data_push.cpp` | 480 | Push + metrics tests |
| **Compose** | `docker/compose/docker-compose-llm-raid-tests-orchestrated.yml` | 350 | Services |
| **Entrypoint** | `docker/test-entrypoint-orchestrated.sh` | 40 | Docker entry |
| **Makefile** | `Makefile.raid-tests` | 180 | Build automation |
| **Docs** | `docker/compose/ORCHESTRATION_QUICKSTART.md` | 350 | Full guide |

## 🔧 Makefile Targets

```makefile
make -f Makefile.raid-tests build          # Build image
make -f Makefile.raid-tests up             # Start cluster
make -f Makefile.raid-tests down           # Stop services
make -f Makefile.raid-tests test           # Run default tests
make -f Makefile.raid-tests test-all       # Run everything
make -f Makefile.raid-tests logs           # View logs
make -f Makefile.raid-tests health         # Check status
make -f Makefile.raid-tests results        # Get results
make -f Makefile.raid-tests clean          # Cleanup
make -f Makefile.raid-tests grafana        # Open Grafana
make -f Makefile.raid-tests prometheus     # Open Prometheus
```

## 📈 Metrics Collected

### Before Tests (Baseline)
- `themis_documents_total` = 0
- `themis_disk_usage_bytes` ≈ 1GB
- `themis_lora_cache_hits` = 0
- LLM Status = IDLE

### After Data Push
- `themis_documents_total` = 1000 (pushed)
- `themis_disk_usage_bytes` ≈ 1.05GB
- Per-shard: ~111 docs (RAID0 round-robin)

### After Tests
- `themis_documents_total` = 1000 (unchanged)
- `themis_disk_usage_bytes` ≈ 1.2GB (models + LoRA cached)
- `themis_lora_cache_hits` = 50+ (from inference)
- LLM Status = LOADED

## 🎯 Expected Output

```
Phase 1: Data Push ✓
├─ Healthy shards: 9/9
├─ Pushed records: 1000/1000
├─ Duration: 2.5s
└─ Distribution: OK (111 per shard ±5%)

Phase 2: Tests Executed ✓
├─ test_llm_raid_data_push: PASSED (7 tests)
├─ test_llm_lora_inline: PASSED (6 tests)
├─ test_llm_plugin: PASSED (8+ tests)
├─ test_llm_raid_pipeline: PASSED (9 tests)
├─ bench_lora_inline: PASSED (6 benchmarks)
└─ bench_llm_raid_pipeline: PASSED (12 benchmarks)

Phase 3: Validation ✓
├─ Result files: 8 found
├─ Metrics collected: 2 snapshots
├─ Shards monitored: 9/9
└─ Overall Status: PASSED ✅
```

## 🐛 Troubleshooting

### Shard nicht erreichbar
```bash
docker logs themis-raid0-shard1
docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml \
  restart themis-raid0-shard1
```

### Tests schlagen fehl
```bash
docker logs -f themis-llm-raid-tests
# Oder direct in container
docker exec themis-llm-raid-tests cat /test_results/pipeline_results.xml
```

### Metriken nicht gesammelt
```bash
curl http://localhost:9090/api/v1/targets
curl http://localhost:9090/api/v1/query?query=themis_documents_total
```

### Cleanup fehlgeschlagen
```bash
docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml down -v
docker system prune -f
```

## 📊 Performance Reference

```
Operation                Duration
──────────────────────────────────
Start Cluster            60-90s
Push 1000 Records        ~2.5s
Run Inline Tests         ~5s
Run Pipeline Tests       ~8s
Run All Tests            ~15s
Run All Benchmarks       ~30s
Collect Metrics          ~1s
Generate Report          ~1s
─────────────────────────────────
Full Pipeline            ~45-60s
```

## 🔍 Inspection Commands

```bash
# Shard Status
curl http://localhost:8080/health
curl http://localhost:8080/api/v1/collections

# Metrics (Prometheus)
curl http://localhost:9090/api/v1/targets
curl http://localhost:9090/api/v1/query?query=up

# Test Results
docker cp themis-llm-raid-tests:/test_results/ ./results/
cat results/orchestrator_results.json | python -m json.tool

# Container Status
docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml ps
docker stats
```

## 🌐 Web Dashboards

| Service | URL | Purpose |
|---------|-----|---------|
| Prometheus | http://localhost:9090 | Time-series metrics |
| Grafana | http://localhost:3000 | Dashboards & alerts |
| Shard 1 | http://localhost:8080 | RAID0-shard1 API |
| Shard 2 | http://localhost:8081 | RAID0-shard2 API |
| Shard 3 | http://localhost:8082 | RAID0-shard3 API |

## 🚨 Important Notes

1. **First Run:** Image build takes ~5 minutes (C++ compilation)
2. **Shard Startup:** Takes 60-90 seconds for all shards to be healthy
3. **Healthcheck:** Watch `docker-compose ps` during startup
4. **Results:** Automatically saved to `/test_results/` in container
5. **Cleanup:** Always `make clean` to avoid dangling resources
6. **Logs:** Enable with `docker logs -f` during troubleshooting

## 🔐 Credentials

| Service | User | Pass |
|---------|------|------|
| Grafana | admin | themis |
| Prometheus | — | — |
| RAID APIs | — | — |

## 📝 Example: Complete Workflow

```bash
# Terminal 1: Start services
cd C:\VCC\themis
make -f Makefile.raid-tests build
make -f Makefile.raid-tests up
make -f Makefile.raid-tests health

# Terminal 2: Monitor (in separate window)
watch -n 1 "docker-compose -f docker/compose/docker-compose-llm-raid-tests-orchestrated.yml ps"

# Terminal 3: Run tests (once all shards healthy)
make -f Makefile.raid-tests test-all
make -f Makefile.raid-tests results

# Terminal 4: Check Prometheus/Grafana (optional)
open http://localhost:9090    # Prometheus
open http://localhost:3000    # Grafana

# Final: Cleanup
make -f Makefile.raid-tests clean
```

## 💡 Tips & Tricks

```bash
# Watch test progress
watch -c docker logs -f themis-llm-raid-tests

# Get test duration
docker logs themis-llm-raid-tests | grep -i duration

# View specific result type
cat test_results/orchestrator_results.json | jq '.phases'
cat test_results/pipeline_results.xml | grep -o 'tests="[0-9]*"'

# Export metrics to CSV
curl -s 'http://localhost:9090/api/v1/query?query=themis_documents_total' | jq '.data.result'

# Monitor in real-time
for i in {1..10}; do clear; docker-compose ps; sleep 5; done

# Save logs before cleanup
docker cp themis-llm-raid-tests:/test_results/ ./backup_results/$(date +%s)
```

## 🎓 Learn More

- **Orchestration Guide:** `docker/compose/ORCHESTRATION_QUICKSTART.md`
- **Architecture:** `RAID_ORCHESTRATION_ARCHITECTURE.md`
- **Implementation Summary:** `RAID_ORCHESTRATION_SUMMARY.md`
- **RAID Docs:** `README.md`

---

**Last Updated:** 2026-04-06  
**Status:** ✅ Ready for Production  
**Next Command:** `make -f Makefile.raid-tests build`
