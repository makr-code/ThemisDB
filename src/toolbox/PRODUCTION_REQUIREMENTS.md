> **Status:** 2026-06-01 – mit aktuellem Toolbox-Code (`text_normalizer.cpp`) abgeglichen.

# ThemisDB Toolbox Module - Production Requirements

<!-- Status: current | validated: 2026-06-01 | re-verified: 2026-08-07 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · PERFORMANCE_EXPECTATIONS.md · SECURITY.md -->

## Zweck und Geltungsbereich

Dieses Dokument ist der **kanonische Referenzpunkt für produktive Mindestanforderungen** des Toolbox-Moduls.
Es definiert verbindliche Betriebs- und Sicherheitsanforderungen für Text-Normalizer, Language-Detector, Toolbox-Utilities.

## Dokumentabgrenzung (Canonical Split)

- **`src/toolbox/PRODUCTION_REQUIREMENTS.md` (dieses Dokument):** verpflichtende Produktionsanforderungen (MUST/MUST NOT), Sicherheitsannahmen, Betriebsgrenzen.
- **`src/toolbox/README.md`:** Funktionsübersicht, Architekturkontext, API- und Nutzungsbeispiele.
- **`src/toolbox/ROADMAP.md`:** Lieferphasen, offene/abgeschlossene Features, Readiness-Planung.
- **`src/toolbox/FUTURE_ENHANCEMENTS.md`:** mittelfristige/langfristige Erweiterungen und Forschungsfelder.

## Verbindliche Produktionsanforderungen

- **MUST:** Toolbox-Input-Validierung aktiv; malformed Input wird mit explizitem Fehlercode abgewiesen.
- **MUST:** Language-Detector-Ergebnisse werden mit Konfidenz-Threshold versehen; Ergebnisse unter Threshold werden geflaggt.
- **MUST:** Alle sicherheitsrelevanten Konfigurationswerte beim Start validiert; fehlende oder ungültige Werte führen zu Fail-Closed-Verhalten.
- **MUST NOT:** Sicherheits- oder Autorisierungs-Checks in Produktionspfaden deaktivieren.

## Verbindliche Sicherheitsanforderungen

- Sicherheitsrelevante Operationen werden über dedizierte Kontroll-Surfaces geleitet.
- Fehler in sicherheitskritischen Pfaden werden als explizite Fehlercodes propagiert; kein Silent-Permit.
- Audit-Logging für sicherheitsrelevante Operationen aktiv in Produktionsdeployments.

## Incident Taxonomy & Observable Diagnostics

Phase 3 implements a unified incident taxonomy across 4 execution planes (Orchestration, Bridge, Registry, Helper).
All incident classes are observable via Prometheus metrics and logging.

Refer to `src/toolbox/SECURITY.md` for the complete **Unified Incident Taxonomy** section, which defines:
- **Layer 1 (Orchestration):** extraction_empty, extraction_failed, extraction_timeout, extraction_overflow (EX-*)
- **Layer 2 (Bridge):** bridge_no_text, bridge_writer_failed, bridge_toolbox_failed, bridge_empty_result (BR-*)
- **Layer 3 (Registry):** registry_not_initialized, registry_double_init, registry_reset_during_active (REG-*)
- **Layer 4 (Helper):** helper_empty_input, helper_encoding_unsupported, helper_size_exceeded, helper_malformed_input (HLP-*)

All incidents are tracked via Prometheus metrics:
- `toolbox_extraction_failures_total` — Layer 1 (Orchestration) errors
- `toolbox_bridge_failures_total` + `toolbox_bridge_latency_us` — Layer 2 (Bridge) errors and latency
- `toolbox_registry_misuse_total` — Layer 3 (Registry) errors
- `toolbox_text_*_errors_total` — Layer 4 (Helper) errors

## Betriebsgrenzen

- Konfigurationswerte müssen deployment-spezifisch gesetzt sein; Default-Werte gelten nicht als produktionssicher.
- Ressourcen-Limits (Größen, Counts, Timeouts) müssen mit den Deployment-Anforderungen übereinstimmen.
- Externe Abhängigkeiten müssen mit expliziten Verbindungs-Timeouts und Retry-Policies konfiguriert sein.

## Minimaler Produktions-Check (Audit-fähig)

- [ ] Modul-Konfiguration vollständig und beim Start validiert
- [ ] Sicherheits- und Autorisierungs-Checks aktiv
- [ ] Ressourcen-Limits explizit konfiguriert (keine Unlimited-Defaults)
- [ ] Audit-Logging aktiv
- [ ] Externe Abhängigkeiten mit Timeout und Retry konfiguriert
- [ ] Produktionsmodus via `THEMIS_PRODUCTION_MODE` oder `THEMIS_ENVIRONMENT` gesetzt

## A. Environment & Dependencies

### Required Libraries

| Library | Minimum Version | Purpose | Install Command (Ubuntu/Debian) |
|---------|-----------------|---------|--------------------------------|
| fmt | 8.0+ | String formatting, logging output | `apt-get install libfmt-dev` |
| spdlog | 1.10+ | Structured logging with severity levels | `apt-get install libspdlog-dev` |
| nlohmann-json | 3.2+ | JSON configuration and data exchange | `apt-get install nlohmann-json3-dev` |
| RocksDB | 6.0+ (optional) | Optional persistent registry backend | `apt-get install librocksdb-dev` |
| GCC/Clang | 11+ (C++17) | Compiler with C++17 support | `apt-get install g++` or `clang` |
| CMake | 3.20+ | Build configuration | `apt-get install cmake` |

### macOS Installation

```bash
brew install fmt spdlog nlohmann-json rocksdb cmake
```

### Windows Installation (vcpkg)

```powershell
vcpkg install fmt:x64-windows spdlog:x64-windows nlohmann-json:x64-windows rocksdb:x64-windows
cmake -DCMAKE_TOOLCHAIN_FILE="[vcpkg root]/scripts/buildsystems/vcpkg.cmake"
```

### Build System Requirements

- CMake 3.20+ (required for toolbox module)
- Ninja 1.10+ (recommended for linux-release preset for parallel builds)
- GCC 11+ or Clang 12+ (C++17 minimum, C++20 recommended)

### Build Command

```bash
# Configure release build (recommended for production)
cmake --preset linux-release

# Build with parallel jobs
cmake --build --preset linux-release --parallel 16

# Alternatively, configure and build manually
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++
cmake --build build --parallel 16
```

---

## B. Deployment Configuration

### Ingestion Worker Thread Pool

**Purpose:** Controls concurrency for text extraction, normalization, and language detection operations.

**Configuration:**

```yaml
toolbox:
  # Worker thread pool size (CPU-bound tasks)
  extraction_worker_threads: 4      # Typical: 2-8 (set to CPU core count or CPU_count / 2)
  
  # Helper operations (text normalization, language detection)
  helper_worker_threads: 2           # Typical: 1-4 (lighter than extraction)
  
  # Bridge operations (graph/vector writes)
  bridge_worker_threads: 2           # Typical: 1-2 (I/O bound, fewer threads needed)
```

**Tuning Guidance:**

- **Extraction:** Set to CPU core count for maximum throughput (16 cores = 8-16 threads)
- **Helpers:** Set to CPU core count / 2 for balanced CPU + I/O
- **Bridge:** Keep small (1-2) to avoid writer contention; increase only if I/O becomes bottleneck
- **Memory Overhead:** Estimate ~1-2 MB per thread; for 8 threads, budget ~16 MB for thread stacks

### Registry Memory Budget

**Purpose:** Limits in-memory toolbox registry size and helper cache.

```yaml
toolbox:
  # Maximum toolbox registry in-memory size
  registry_max_memory_mb: 256         # Typical: 64-512 MB
  
  # Language detector result cache size
  language_cache_max_entries: 10000   # Typical: 1K-100K
  
  # Fingerprint hash cache (deduplication)
  fingerprint_cache_max_entries: 50000 # Typical: 10K-1M
```

**Estimation:**

- Registry entry: ~100 bytes average; 256 MB → ~2.5M entries
- Language cache entry: ~50 bytes; 10K entries → ~500 KB
- Fingerprint cache: ~32 bytes (SHA-256 hash); 50K → ~1.6 MB

### Prometheus Metrics Export

**Purpose:** Enable observability via Prometheus scraping.

```yaml
toolbox:
  # Prometheus metrics endpoint
  metrics_export_enabled: true
  metrics_port: 9090                  # Default Prometheus port
  metrics_endpoint_path: "/metrics"   # HTTP path for scraping
  
  # Metrics aggregation
  histogram_buckets: [1, 5, 10, 50, 100, 500, 1000]  # Latency buckets (ms)
  counter_reset_interval_sec: 3600    # Reset counters every hour (optional)
```

**Prometheus Configuration (prometheus.yml):**

```yaml
global:
  scrape_interval: 15s                # Scrape toolbox metrics every 15 seconds

scrape_configs:
  - job_name: 'themisdb-toolbox'
    static_configs:
      - targets: ['localhost:9090']
    metrics_path: '/metrics'
    scrape_interval: 10s              # More frequent for production
```

### Logging Configuration

**Purpose:** Control diagnostic log output and severity levels.

```yaml
toolbox:
  # Log level: SPDLOG_LEVEL_TRACE, DEBUG, INFO, WARN, ERR, CRITICAL
  log_level: "SPDLOG_LEVEL_INFO"      # Production: INFO; Debug: DEBUG
  
  # Log output
  log_file: "/var/log/themisdb/toolbox.log"
  log_max_size_mb: 100                # Rotate log at 100 MB
  log_max_files: 10                   # Keep 10 rotated log files
  
  # Structured logging
  log_format: "json"                  # json or text (json recommended for parsing)
  log_include_thread_id: true         # Include thread ID in logs
  log_include_timestamp: true         # ISO 8601 timestamp
```

**Example Log Entry:**

```json
{
  "timestamp": "2026-08-07T19:02:26.978Z",
  "level": "INFO",
  "module": "toolbox",
  "thread_id": 12345,
  "message": "[TOOLBOX] Bridge enrichment completed: 42 entities extracted",
  "incident_code": "BR-OK",
  "latency_us": 42000
}
```

---

## C. Operational Runbooks

### Initialization on Startup

**Procedure:**

1. **Load Configuration**
   ```cpp
   auto config = LoadToolboxConfig("/etc/themisdb/toolbox.yaml");
   ```

2. **Initialize Global Registry**
   ```cpp
   // Must be called BEFORE any toolbox operations
   auto& registry = ToolboxRegistry::instance();
   registry.initialize(config);
   
   // Verify initialization successful
   if (!registry.is_initialized()) {
     log.error("[TOOLBOX] Registry initialization failed; system shutdown");
     return false;  // Fail-closed
   }
   ```

3. **Verify Dependencies**
   ```cpp
   // Check that ContentManager is available
   if (!ContentManager::available()) {
     log.error("[TOOLBOX] ContentManager not available; startup blocked");
     return false;
   }
   ```

4. **Start Metrics Export**
   ```cpp
   MetricsExporter::start(config.metrics_port, config.metrics_endpoint_path);
   log.info("[TOOLBOX] Metrics export started on port {}", config.metrics_port);
   ```

**Checklist:**
- [x] Configuration file exists and is valid
- [x] Registry initialized successfully
- [x] ContentManager available
- [x] Metrics export enabled and accessible

### Registry Reset/Reconfiguration Procedure

**When Safe to Call `reset()`:**

- Only during deployment/maintenance windows
- After ensuring NO active extraction operations
- Recommended: during graceful shutdown before restart

**Procedure:**

```cpp
// 1. Drain active operations (wait for completion)
while (registry.active_operation_count() > 0) {
  sleep(100);  // Wait 100ms, check again
}

// 2. Stop accepting new operations
registry.set_accepting_new_operations(false);

// 3. Final verification
if (registry.active_operation_count() == 0) {
  // Safe to reset
  registry.reset();
  log.info("[TOOLBOX] Registry reset successfully");
} else {
  log.error("[TOOLBOX] Cannot reset: {} operations still active",
            registry.active_operation_count());
  return false;  // Fail-closed
}

// 4. Re-initialize for next deployment
registry.initialize(new_config);
```

**Monitoring During Reset:**
- Track `toolbox_registry_misuse_total` (should not increment)
- Verify `toolbox_extraction_failures_total` stable (not increasing)
- Log all reset attempts with timestamp

### Metric Scraping Setup

**Prometheus HTTP Endpoint:**

```bash
# Verify endpoint is responding
curl -s http://localhost:9090/metrics | head -20

# Expected output (first few lines):
# HELP toolbox_extraction_failures_total Total extraction failures
# TYPE toolbox_extraction_failures_total counter
# toolbox_extraction_failures_total{incident="EX-EMPTY"} 42
# toolbox_extraction_failures_total{incident="EX-FAILED"} 5
```

**Grafana Dashboard Setup:**

1. Add Prometheus data source: `http://localhost:9090`
2. Import toolbox dashboard from:
   - File: `src/toolbox/grafana/toolbox_dashboard.json` (if provided)
   - Or manually create dashboard with key metrics:

**Key Metrics to Display:**
- `toolbox_extraction_failures_total` (counter, per incident type)
- `toolbox_bridge_failures_total` (counter, per incident type)
- `toolbox_extraction_latency_us` (histogram, p50/p95/p99)
- `toolbox_bridge_latency_us` (histogram, p50/p95/p99)
- `toolbox_registry_misuse_total` (counter, per incident type)

### Error Recovery Procedures

**Soft-Fail Graceful Degradation:**

The toolbox module implements soft-fail behavior for certain error conditions to avoid service interruption:

1. **Bridge Write Failures (BR-WRITER):**
   - Incident: Graph or vector sink write fails
   - Behavior: Bridge operation completes with empty result (no exception)
   - Recovery: Application continues; user sees incomplete enrichment
   - Observable: `toolbox_bridge_failures_total` incremented
   - Action: Log warning; alert if failure rate > 5% per minute

2. **Helper Encoding Issues (HLP-ENCODING):**
   - Incident: Text encoding not recognized
   - Behavior: Fall back to UTF-8 interpretation
   - Recovery: Processing continues with degraded accuracy
   - Observable: `toolbox_helper_errors_total[HLP-ENCODING]` incremented
   - Action: Log and monitor; may need content inspection

3. **Registry Not Initialized (REG-NOT-INIT):**
   - Incident: Operation attempted before registry.initialize()
   - Behavior: Exception thrown immediately
   - Recovery: Application MUST initialize registry before proceeding
   - Observable: Exception in logs + `toolbox_registry_misuse_total` incremented
   - Action: CRITICAL - requires application code fix

**What NOT to Do (Hard Failures):**
- Do NOT suppress `REG-NOT-INIT` exceptions
- Do NOT continue if ContentManager initialization fails
- Do NOT ignore `registry_reset_during_active` errors

---

## D. Performance Tuning

### Hot Paths and Performance Budgets

Refer to `PERFORMANCE_EXPECTATIONS.md` for detailed gates. Summary:

| Operation | Budget | Gate ID | Measurement |
|-----------|--------|---------|-------------|
| Extract entities throughput | ≥100K ops/s | GATE-TBX-P1 | ops per second |
| Extract entity set latency | p95 ≤50ms | GATE-TBX-P2 | milliseconds |
| Text normalization latency | p95 ≤10ms | GATE-TBX-P3 | milliseconds |
| Language detection latency | p95 ≤15ms | GATE-TBX-P4 | milliseconds |
| Fingerprinting throughput | ≥1M ops/s | GATE-TBX-P5 | ops per second |
| Bridge enrichment latency | p95 ≤100ms | GATE-TBX-P6 | milliseconds |

### Concurrency Tuning

**Optimal Thread Pool Sizing:**

```
CPU Cores | Extraction | Helpers | Bridge
-----------|-----------|---------|--------
    4     |     2     |    2    |   1
    8     |     4     |    2    |   2
   16     |     8     |    4    |   2
   32     |    16     |    8    |   4
```

**Memory vs. Concurrency Trade-off:**
- Each thread: ~1-2 MB stack overhead
- 8 threads: ~16 MB; 16 threads: ~32 MB
- Start conservative; increase only if CPU utilization < 60%

### Caching Opportunities

**Language Detector Cache (LRU):**
- Cache size: 10K entries (typical)
- Hit rate: 60-70% for repeated content
- TTL: 24 hours (set via `language_cache_ttl_sec`)

**Fingerprint Deduplication Cache (LRU):**
- Cache size: 50K entries (typical)
- Hit rate: 40-50% for batch operations
- TTL: 48 hours (set via `fingerprint_cache_ttl_sec`)

**Enabling Caches:**
```yaml
toolbox:
  language_cache_enabled: true
  language_cache_max_entries: 10000
  language_cache_ttl_sec: 86400        # 24 hours
  
  fingerprint_cache_enabled: true
  fingerprint_cache_max_entries: 50000
  fingerprint_cache_ttl_sec: 172800    # 48 hours
```

### Baseline Regression Procedure

**Regression Detection (TBXG-1):**

1. **Establish Baseline** (from Q3 2026 release run)
   ```bash
   # Baselines already documented in PERFORMANCE_EXPECTATIONS.md
   # P1: 125K ops/s, P2: 42ms, P3: 8ms, P4: 12ms, P5: 1.2M ops/s, P6: 78ms
   ```

2. **Run Current Benchmark**
   ```bash
   cmake --preset linux-release
   cmake --build --preset linux-release --target bench_toolbox_native_workloads
   ./build/linux-release/benchmarks/toolbox/bench_toolbox_native_workloads \
     --benchmark_out_format=csv \
     --benchmark_out=current_run.csv
   ```

3. **Calculate Regression**
   ```bash
   python3 <<'EOF'
   import csv
   
   baseline = {
     "BM_ExtractEntities_Throughput": 125000,
     "BM_ExtractEntitySet_Latency": 42,
     "BM_TextNormalization_Latency": 8,
     "BM_LanguageDetection_Latency": 12,
     "BM_ContentFingerprinting_Throughput": 1200000,
     "BM_BridgeEnrichment_Placeholder": 78
   }
   
   with open('current_run.csv') as f:
     for row in csv.DictReader(f):
       name = row['name']
       current = float(row['real_time'])
       base = baseline[name]
       regression = abs(current - base) / base * 100
       status = "PASS" if regression <= 10 else "FAIL"
       print(f"{name}: {regression:.2f}% {status}")
   EOF
   ```

4. **Alert Threshold:** Regression > 10% triggers investigation

---

## E. Monitoring & Observability

### Key Prometheus Metrics

**Layer 1: Orchestration (Extraction)**

- `toolbox_extraction_failures_total` (counter, per incident type: EX-EMPTY, EX-FAILED, EX-TIMEOUT, EX-OVERFLOW)
- `toolbox_extraction_latency_us` (histogram, microseconds)
- `toolbox_extract_empty_results_total` (counter, empty extractions)

**Recommended Alert:** If `toolbox_extraction_failures_total[EX-FAILED]` > 100/min

**Layer 2: Bridge (Content Enrichment)**

- `toolbox_bridge_failures_total` (counter, per incident type: BR-NO-TEXT, BR-WRITER, BR-TOOLBOX, BR-EMPTY)
- `toolbox_bridge_latency_us` (histogram, microseconds)

**Recommended Alert:** If `toolbox_bridge_failures_total[BR-WRITER]` > 50/min

**Layer 3: Registry (Global State)**

- `toolbox_registry_misuse_total` (counter, per incident type: REG-NOT-INIT, REG-DOUBLE, REG-RESET-ACTIVE)

**Recommended Alert:** If `toolbox_registry_misuse_total` increments (immediate critical alert)

**Layer 4: Helpers (Text Processing)**

- `toolbox_text_chunker_errors_total` (counter, per error type)
- `toolbox_text_normalizer_errors_total` (counter, per error type)
- `toolbox_language_detector_errors_total` (counter, per error type)
- `toolbox_text_quality_scorer_errors_total` (counter, per error type)

**Recommended Alert:** If any helper error counter > 50/min

### Alert Thresholds

| Metric | Threshold | Severity | Action |
|--------|-----------|----------|--------|
| `toolbox_extraction_failures_total[EX-FAILED]` | > 100/min | WARNING | Investigate extraction processor |
| `toolbox_extraction_failures_total[EX-TIMEOUT]` | > 10/min | WARNING | Check resource availability |
| `toolbox_bridge_failures_total[BR-WRITER]` | > 50/min | WARNING | Check graph/vector writer health |
| `toolbox_registry_misuse_total` | > 0 | **CRITICAL** | Immediate action: application code issue |
| `toolbox_helper_errors_total` (any) | > 50/min | WARNING | Monitor specific helper type |
| `toolbox_extraction_latency_us[p95]` | > 75ms | WARNING | Performance degradation detected |
| `toolbox_bridge_latency_us[p95]` | > 150ms | WARNING | Bridge performance degradation |

### Log Parsing Examples

**Extract error incidents from logs:**

```bash
# Find all extraction failures
grep "\[TOOLBOX\] Error: EX-" /var/log/themisdb/toolbox.log

# Find bridge write failures
grep "BR-WRITER" /var/log/themisdb/toolbox.log

# Find registry misuse
grep "REG-NOT-INIT\|REG-DOUBLE\|REG-RESET-ACTIVE" /var/log/themisdb/toolbox.log

# Count incidents by type (JSON logs)
jq -r .incident_code /var/log/themisdb/toolbox.log | sort | uniq -c
```

**Time-series analysis:**

```bash
# Incidents per hour
jq -r '.timestamp | split("T")[0]' /var/log/themisdb/toolbox.log | sort | uniq -c

# Average latency by operation type
jq -s 'group_by(.operation_type) | 
  map({operation: .[0].operation_type, 
       avg_latency: (map(.latency_us) | add / length)}) | 
  .[] | "\(.operation): \(.avg_latency | round)µs"'
```

### Dashboard Setup Guide

**Grafana Dashboard Template:**

1. **Overview Panel (Top):**
   - Current error rates (last 5 min)
   - Latency percentiles (p50/p95/p99)
   - Thread pool utilization

2. **Layer-wise Breakdown (4 rows):**
   - **Orchestration:** Extraction success/failure, latency distribution
   - **Bridge:** Writer failures, latency, entity count
   - **Registry:** Misuse incidents, initialization events
   - **Helpers:** Error counts per helper type, cache hit rates

3. **Alerting:**
   - Red background if `toolbox_registry_misuse_total` > 0
   - Yellow if extraction or bridge failure rate exceeds thresholds
   - Green for normal operation

**Example Grafana Query:**

```promql
# p95 latency for bridge operations
histogram_quantile(0.95, toolbox_bridge_latency_us)

# Daily error rate by incident type
increase(toolbox_extraction_failures_total[1d]) by (incident)

# Memory usage trend (if available)
container_memory_usage_bytes{pod="themisdb-ingestion"}
```

---

## Review / Sourcecode-Audit-Nachweis

### Betroffene Dateien im Review

- `src/toolbox/PRODUCTION_REQUIREMENTS.md`
- `src/toolbox/text_normalizer.cpp`
- `src/toolbox/language_detector.cpp`
