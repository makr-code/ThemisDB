# RUNBOOK: Scraper Ingestion — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Ingestion Platform Team Lead
**Purpose:** Triage and recover from scraper fetch failures, ingest overflow, rate-limit breaches, and long-run degradation events
**Severity:** High (scraper failures halt source seeding and may cause data freshness SLO violations)
**Estimated Duration:** 5 min – 2 hours (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB scraper ingestion module (`src/scraper/`). The module has five primary surfaces:

1. **Fetch pipeline** — URL resolution, HTTP GET, JS render, pagination (`scraper_api_client.cpp`, `scraper_js_renderer.cpp`)
2. **Quality evaluation** — relevance scoring, content extraction (`scraper_llm_evaluator.cpp`)
3. **Metadata write** — provenance-stamped write to storage backend (`scraper_metadata_writer.cpp`)
4. **Burst controller** — rate-limiting token bucket (`scraper_burst_controller.h`)
5. **Diagnostics** — incident taxonomy, fault class, severity mapping (`scraper_diagnostics.h`)

**Key Principles:**
- The scraper pipeline is fail-closed; fetch, parse, and write faults return explicit error codes, not silent skips
- Burst controller enforces token-bucket rate limits; `tryAcquire()` returning `false` is expected under burst pressure
- JS render is optional; plain HTTP fetch degrades gracefully when renderer is unavailable
- Provenance stamps are written with every result; missing stamps indicate a write-path fault

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Access to server logs with `[SCRAPER:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing `scraper_*` metrics
- [ ] Knowledge of active source catalog and crawl policy configuration
- [ ] Burst controller state accessible via `BurstCrawlController` admin API or log scan

---

## Failure Scenarios

---

### Scenario 1: Fetch Failure Storm

**Symptoms:**
- Log pattern: `[SCRAPER:FetchFailed] url=<url> reason=<reason>`
- `scraper_fetch_failures_total` counter rising steeply
- Source seeding throughput dropping below 8 000 fetch/s

**Log patterns:**
```
[SCRAPER:FetchFailed] url=<url> reason=connection_refused
[SCRAPER:FetchFailed] url=<url> reason=ssl_handshake_timeout source_id=<id>
[SCRAPER:FetchFailed] url=<url> reason=dns_resolution_failed
```

#### Step 1: Identify Failure Scope
```bash
grep '\[SCRAPER:FetchFailed\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'source_id=\S+' | sort | uniq -c | sort -rn | head -10

# Check if failure is isolated to one source or systemic
query-metrics --metric scraper_fetch_failures_total --label source_id --range 10m
```

#### Step 2: Pause Failing Sources
```bash
# Pause source(s) with > 50% failure rate
themis-admin scraper pause-source --source-id <source_id>

# Check remaining sources are healthy
themis-admin scraper source-status --all
```

#### Step 3: Validate Connectivity
```bash
curl -v --max-time 10 "<failing_url>"
# DNS check
nslookup <failing_hostname>
```

#### Step 4: Resume After Fix
```bash
themis-admin scraper resume-source --source-id <source_id>
```

---

### Scenario 2: Ingest Queue Overflow

**Symptoms:**
- Log pattern: `[SCRAPER:IngestOverflow] queue_depth=<N> max_depth=<M>`
- `scraper_ingest_queue_overflows_total` counter rising
- Write latency increasing; metadata write SLO at risk

**Log patterns:**
```
[SCRAPER:IngestOverflow] queue_depth=4096 max_depth=4096 dropped=<N>
[SCRAPER:IngestOverflow] source_id=<id> ingest_rate=<rate> consumer_lag=<ms>
```

#### Step 1: Check Queue Depth
```bash
grep '\[SCRAPER:IngestOverflow\]' /var/log/themisdb/themisdb.log | tail -20

query-metrics --metric scraper_ingest_queue_depth --range 5m
```

#### Step 2: Throttle Fetch Rate
```bash
# Reduce burst controller token rate
themis-admin config set scraper.burst_controller.tokens_per_second 500
themis-admin scraper reload-config
```

#### Step 3: Scale Write Consumers
```bash
# Increase metadata writer parallelism
themis-admin config set scraper.metadata_writer.worker_threads 8
themis-admin scraper reload-config
```

#### Step 4: Restore Normal Rate
After queue drains below 50% capacity:
```bash
themis-admin config set scraper.burst_controller.tokens_per_second 2000
themis-admin scraper reload-config
```

---

### Scenario 3: Rate-Limit Breach

**Symptoms:**
- Log pattern: `[SCRAPER:RateLimitBreach] source_id=<id> requests=<N> limit=<L>`
- External source returns HTTP 429 responses
- `scraper_rate_limit_breaches_total` counter non-zero

**Log patterns:**
```
[SCRAPER:RateLimitBreach] source_id=<id> requests_last_60s=1200 limit=1000
[SCRAPER:RateLimitBreach] source_id=<id> http_status=429 retry_after=60
```

#### Step 1: Identify Breaching Sources
```bash
grep '\[SCRAPER:RateLimitBreach\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'source_id=\S+' | sort | uniq -c | sort -rn | head -5
```

#### Step 2: Apply Per-Source Rate Cap
```bash
# Set per-source rate limit (requests/minute)
themis-admin scraper set-rate-limit --source-id <source_id> --rpm 600
```

#### Step 3: Honour Retry-After
If source returns `Retry-After` header:
```bash
# Back-off for specified duration
themis-admin scraper pause-source --source-id <source_id> --duration 60s
```

---

### Scenario 4: Long-Run Degradation

**Symptoms:**
- Log pattern: `[SCRAPER:LongRunDegradation] uptime_hours=<h> throughput_drop_pct=<N>`
- Fetch throughput trending down over multi-hour window
- Memory usage of scraper process increasing (potential leak in JS renderer)

**Log patterns:**
```
[SCRAPER:LongRunDegradation] uptime_hours=6 fetch_throughput_now=3200 baseline=8000
[SCRAPER:LongRunDegradation] js_renderer_restarts=<N> avg_restart_interval_min=<M>
```

#### Step 1: Check Process Health
```bash
query-metrics --metric scraper_fetch_ops_per_sec --range 6h

# Check JS renderer process memory
ps aux | grep scraper_js_renderer
```

#### Step 2: Restart JS Renderer (if implicated)
```bash
themis-admin scraper restart-js-renderer
# Verify recovery in metrics within 60 s
```

#### Step 3: Rolling Scraper Restart
If memory leak or degradation persists:
```bash
themis-admin scraper rolling-restart --grace-period 30s
```

---

### Scenario 5: Metadata Write Fault

**Symptoms:**
- Log pattern: `[SCRAPER:WritePathFault] url=<url> reason=<reason>`
- Write success rate dropping; provenance stamps missing from storage records
- `scraper_write_failures_total` counter rising

**Log patterns:**
```
[SCRAPER:WritePathFault] url=<url> reason=storage_unavailable
[SCRAPER:WritePathFault] url=<url> reason=provenance_stamp_missing doc_id=<id>
```

#### Step 1: Check Storage Backend
```bash
themis-admin storage health-check
grep '\[SCRAPER:WritePathFault\]' /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Enable Write Retry
```bash
themis-admin config set scraper.metadata_writer.retry_on_failure true
themis-admin config set scraper.metadata_writer.max_retries 3
themis-admin scraper reload-config
```

#### Step 3: Drain Write Buffer After Storage Recovery
```bash
# Force flush of pending write buffer
themis-admin scraper flush-write-buffer
```

---

## Alert → Runbook Mapping

| Alert Name | Log Pattern | Runbook Scenario |
|------------|-------------|------------------|
| `scraper_fetch_failure_rate_high` | `[SCRAPER:FetchFailed]` | Scenario 1 |
| `scraper_ingest_overflow` | `[SCRAPER:IngestOverflow]` | Scenario 2 |
| `scraper_rate_limit_breach` | `[SCRAPER:RateLimitBreach]` | Scenario 3 |
| `scraper_long_run_degradation` | `[SCRAPER:LongRunDegradation]` | Scenario 4 |
| `scraper_write_path_fault` | `[SCRAPER:WritePathFault]` | Scenario 5 |

---

## Escalation Path

1. **L1 (Operator):** Apply runbook steps; resolve within 30 min
2. **L2 (SRE):** Escalate if fetch failure rate remains > 20% after 15 min
3. **L3 (Ingestion Platform):** Engage for systematic source quality or write-path issues

---

## Related Documentation

- `src/scraper/ROADMAP.md` — Wave D operability items
- `include/scraper/scraper_diagnostics.h` — Incident taxonomy and fault classes
- `include/scraper/scraper_burst_controller.h` — Rate-limit contract
- `tests/integration/test_scraper_ingestion_soak.cpp` — Wave D soak tests
- `tests/scraper/test_scraper_highcardinality_stress.cpp` — Wave D stress tests
