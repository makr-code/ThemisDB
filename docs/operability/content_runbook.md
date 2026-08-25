# RUNBOOK: Content Module — Ingestion, Processor, and Policy Incident Response

**Audience:** Database Operators, SREs, Content Platform Team  
**Purpose:** Detect, triage, and recover from content module incidents: ingestion stalls,
processor degradation, policy surges, archive abuse, async queue deadlocks, and ingestion surges  
**Severity:** Ranges from High (processor degradation) to Critical (ingestion stall, queue deadlock)  
**Estimated Duration:** 5–30 min depending on scenario (see per-section RTO estimates)

---

## Overview

The content module handles ingestion orchestration, multi-format extraction, policy/security
validation, enrichment (OCR/LLM/embedding), and deduplication for ThemisDB. This runbook covers
the seven operator-critical failure modes identified during Wave D operability hardening.

**Key Principles:**
- All content processing gates are fail-closed: a failure at any pre-processing stage halts
  ingestion and emits a structured error code — it never silently passes through.
- Processor degradation (OCR, LLM, embedding unavailable) is expected and handled via structured
  non-silent failure states; operators should verify degradation is expected before taking action.
- Policy and security violations are normal operational events at scale; a _surge_ in violations
  indicates either a misconfiguration or an abuse campaign.
- Async worker queue depth is the primary leading indicator for ingestion health.

**Structured Error Codes and Remediation Hints:**

| Error Code | Description | Runbook Section |
|---|---|---|
| `VALIDATION_ERROR` | Payload rejected at validation stage | §2 |
| `PROCESSOR_DEGRADED` | Optional processor unavailable | §3 |
| `AMPLIFICATION_RISK` | Archive amplification safety limit reached | §4 |
| `POLICY_VIOLATION` | Policy gate rejected ingestion request | §5 |
| `ASYNC_QUEUE_STALL` | Worker queue not draining | §6 |
| `INGESTION_STALL` | Ingestion pipeline not processing | §1 |
| `MEMORY_PRESSURE` | Worker heap growth exceeds threshold | §7 |

---

## Prerequisites Checklist

- [ ] Access to content module metrics dashboard (`content_ingestion_rate`, `content_queue_depth`,
  `content_validation_errors`, `content_policy_violations`)
- [ ] Read access to structured error logs for `content_manager`, `content_validator`,
  `content_policy`, `content_security`
- [ ] Ability to query current async worker queue depth
- [ ] Familiarity with which processors are optional vs. required for your deployment
- [ ] Knowledge of active content policies and expected violation rate baseline

---

## §1 — Ingestion Pipeline Stall

**Estimated RTO:** 5–15 min  
**Severity:** Critical

### Detection

```bash
# Check current ingestion rate (expect > 0 during normal hours)
query-metrics \
  --metric content_ingestion_rate \
  --window 5m \
  --interval 1m

# Check queue depth (high depth + low drain rate = stall)
query-metrics \
  --metric content_queue_depth \
  --window 10m \
  --interval 30s
```

**Alert Thresholds:**
- ⚠ **Warning:** `content_ingestion_rate` drops > 50% from baseline for > 2 min
- ❌ **Critical:** `content_ingestion_rate` = 0 for > 5 min during expected load period

### Diagnosis

1. **Check worker process health:**
   ```bash
   check-process-health --component content_manager
   ```

2. **Check for error logs in the last 5 minutes:**
   ```bash
   collect-logs \
     --component content_manager \
     --duration 5m \
     --filter "level=error OR level=fatal"
   ```

3. **Identify the failure stage from structured error codes:**
   - `ASYNC_QUEUE_STALL` → Go to §6
   - `VALIDATION_ERROR` surge → Go to §2
   - `PROCESSOR_DEGRADED` → Go to §3
   - `POLICY_VIOLATION` surge → Go to §5

4. **If no structured errors and ingestion rate = 0:**
   - Verify upstream producers are still sending requests
   - Check content module process is alive and accepting connections
   - Check for database connectivity issues (content table accessible)

### Recovery

```bash
# If process is alive but stalled — trigger graceful restart
restart-component --component content_manager --graceful

# Verify recovery within 60 seconds
watch-metric --metric content_ingestion_rate --interval 5s --timeout 120s
```

**Success Criteria:**
- [ ] `content_ingestion_rate` returns to ≥ 80% of baseline within 5 min
- [ ] Queue depth begins draining
- [ ] No repeat stall within 10 min of recovery

---

## §2 — Validation Failure Spike

**Estimated RTO:** 10–20 min  
**Severity:** High

### Detection

```bash
# Check validation error rate
query-metrics \
  --metric content_validation_errors \
  --window 30m \
  --group-by error_class
```

**Alert Thresholds:**
- ⚠ **Warning:** `content_validation_errors` > 5% of ingestion volume
- ❌ **Critical:** `content_validation_errors` > 20% of ingestion volume

### Diagnosis

1. **Identify dominant error class:**
   ```bash
   query-metrics \
     --metric content_validation_errors \
     --window 15m \
     --group-by error_class,mime_type
   ```

2. **Common causes and identification:**

   | Error Class | Likely Cause | How to Identify |
   |---|---|---|
   | `MIME_MISMATCH` | Producers sending wrong Content-Type | Group by `producer_id` |
   | `SIZE_EXCEEDED` | Payload size > configured maximum | Check `payload_size_bytes` in logs |
   | `FORMAT_INVALID` | Corrupted or truncated documents | Sample log for affected content IDs |
   | `ENCODING_ERROR` | Non-UTF-8 text submitted as plain | Check `detected_encoding` in error context |

3. **Sample failing content IDs:**
   ```bash
   collect-logs \
     --component content_validator \
     --duration 15m \
     --filter "error_code=VALIDATION_ERROR" \
     --fields "content_id,mime_type,payload_size_bytes,error_class" \
     --limit 20
   ```

### Recovery

```bash
# For MIME_MISMATCH: identify and notify producer
# For SIZE_EXCEEDED: adjust size limit if legitimate (requires config change + restart)
# For FORMAT_INVALID: investigate producer-side corruption; reject and re-queue clean payloads

# If spike is transient (bad batch from one producer):
# Mark the batch as failed and allow downstream producers to continue
reject-content-batch --batch-id <batch-id> --reason "validation_failure_spike"
```

**Success Criteria:**
- [ ] `content_validation_errors` drops below 5% within 15 min
- [ ] Root cause producer or payload class identified
- [ ] Incident report filed if > 100 items affected

---

## §3 — Processor Degradation (OCR / LLM / Embedding Unavailable)

**Estimated RTO:** 5–10 min  
**Severity:** High (functional degradation, not complete outage)

### Detection

```bash
# Check processor health per type
query-metrics \
  --metric content_processor_availability \
  --window 10m \
  --group-by processor_type
```

**Alert Thresholds:**
- ⚠ **Warning:** Any optional processor `availability < 1.0` for > 3 min
- ❌ **Critical:** Required processor unavailable (check deployment config for required vs. optional)

### Diagnosis

1. **Check which processor is degraded:**
   ```bash
   collect-logs \
     --component content_manager \
     --duration 10m \
     --filter "error_code=PROCESSOR_DEGRADED" \
     --fields "processor_type,dependency,error_detail"
   ```

2. **Determine if optional or required:**
   - **Optional processors** (OCR, LLM embedding): Ingestion continues; items awaiting these
     processors queue or are stored without enrichment
   - **Required processors** (content validator, MIME detector): Ingestion halts → escalate

3. **For optional processor degradation:**
   ```bash
   # Check the processor's own health endpoint
   check-process-health --component ocr_service  # or llm_service, embedding_service

   # Check external dependency health (GPU, model file, external API)
   collect-logs --component ocr_service --duration 5m --filter "level=error"
   ```

### Recovery

```bash
# Restart degraded optional processor
restart-component --component ocr_service --graceful

# Monitor recovery
watch-metric --metric content_processor_availability \
  --group-by processor_type \
  --interval 10s \
  --timeout 120s

# If processor cannot recover within 5 min, activate degraded-mode ingestion policy
# (store content without OCR/LLM enrichment; mark for re-enrichment when processor recovers)
update-ingestion-policy \
  --processor-type ocr \
  --mode degrade-graceful \
  --reprocess-on-recovery true
```

**Success Criteria:**
- [ ] Processor availability returns to 1.0 within 10 min, OR
- [ ] Degraded-mode ingestion policy activated and ingestion continues without enrichment
- [ ] Re-enrichment queue populated for recovery processing

---

## §4 — Archive Amplification Incident

**Estimated RTO:** 5–10 min  
**Severity:** High (abuse mitigation; no service disruption expected)

### Detection

```bash
# Check for amplification rejection events
query-metrics \
  --metric content_security_rejections \
  --window 15m \
  --filter "error_code=AMPLIFICATION_RISK"
```

**Alert Thresholds:**
- ⚠ **Warning:** > 5 `AMPLIFICATION_RISK` rejections in 5 min
- ❌ **Critical (abuse campaign):** > 50 `AMPLIFICATION_RISK` rejections in 5 min from same producer

### Diagnosis

1. **Identify source:**
   ```bash
   collect-logs \
     --component archive_processor \
     --duration 15m \
     --filter "error_code=AMPLIFICATION_RISK" \
     --fields "content_id,producer_id,archive_size_bytes,expansion_ratio,filename"
   ```

2. **Determine if isolated incident or pattern:**
   - Single high-ratio archive from legitimate producer → Notify producer, no action
   - Repeated high-ratio archives from same producer → Rate-limit or block producer
   - Coordinated uploads from multiple producers → Escalate to security team

3. **Check current amplification limit setting:**
   ```bash
   query-config --component archive_processor --key max_expansion_ratio
   ```

### Recovery

```bash
# For legitimate large archives (e.g., bulk data import), temporarily increase limit:
# (Requires explicit sign-off from Security Lead)
update-config \
  --component archive_processor \
  --key max_expansion_ratio \
  --value <new-limit> \
  --duration 30m \
  --requires-signoff security_lead

# For abuse campaign: block producer
block-producer --producer-id <producer-id> --reason "archive_amplification_abuse"

# For isolated incident: reject specific content and notify producer
reject-content --content-id <content-id> --reason "amplification_risk_exceeded"
```

**Success Criteria:**
- [ ] `AMPLIFICATION_RISK` rejections return to baseline (< 1/min)
- [ ] Abuse producer blocked if attack pattern confirmed
- [ ] If legitimate: archive re-submitted within bounds or limit temporarily adjusted with sign-off

---

## §5 — Policy Violation Surge

**Estimated RTO:** 10–20 min  
**Severity:** High

### Detection

```bash
# Check policy violation rate
query-metrics \
  --metric content_policy_violations \
  --window 30m \
  --group-by policy_ref,violation_code
```

**Alert Thresholds:**
- ⚠ **Warning:** Policy violation rate > 10% of ingestion volume
- ❌ **Critical:** Policy violation rate > 30% of ingestion volume (likely misconfiguration)

### Diagnosis

1. **Identify the dominant policy and violation code:**
   ```bash
   collect-logs \
     --component content_policy \
     --duration 15m \
     --filter "error_code=POLICY_VIOLATION" \
     --fields "policy_ref,violation_code,content_type,producer_id" \
     --top 20
   ```

2. **Distinguish abuse from misconfiguration:**
   - **Misconfiguration indicator:** High violation rate from known-good producers, recently
     deployed policy change, violation type is `CONTENT_TYPE_DENIED` or `VERSION_MISMATCH`
   - **Abuse indicator:** High violation rate from new/unknown producers, violation type is
     `PROHIBITED_CONTENT` or `QUOTA_EXCEEDED`

3. **Verify recent policy changes:**
   ```bash
   query-policy-changelog --duration 24h --order desc --limit 10
   ```

### Recovery

```bash
# If misconfiguration (policy too strict after recent change):
# Roll back policy to last known-good version
rollback-policy --policy-ref <policy-ref> --to-version <last-good-version>

# If abuse campaign: rate-limit or block offending producers
rate-limit-producer --producer-id <producer-id> --rate 0  # block

# If legitimate policy violation surge (e.g., new content type not yet approved):
# Temporarily allow specific content type while formal approval proceeds
update-policy \
  --policy-ref <policy-ref> \
  --allow-content-type <type> \
  --duration 2h \
  --requires-signoff policy_owner
```

**Success Criteria:**
- [ ] Policy violation rate drops below 5% within 15 min
- [ ] Root cause (misconfiguration or abuse) identified and documented
- [ ] If rollback performed: post-mortem scheduled within 24 hours

---

## §6 — Async Worker Queue Deadlock

**Estimated RTO:** 10–20 min  
**Severity:** Critical

### Detection

```bash
# Check queue depth trend
query-metrics \
  --metric content_queue_depth \
  --window 20m \
  --interval 1m

# Check drain rate (should be > 0 during normal operation)
query-metrics \
  --metric content_queue_drain_rate \
  --window 10m
```

**Alert Thresholds:**
- ❌ **Critical:** `content_queue_depth` growing continuously for > 5 min with
  `content_queue_drain_rate` = 0

### Diagnosis

1. **Check worker thread status:**
   ```bash
   collect-logs \
     --component content_manager \
     --duration 10m \
     --filter "component=async_worker" \
     --fields "worker_id,state,current_item,blocked_since"
   ```

2. **Check for processor blocking the worker:**
   - A worker blocked on a degraded OCR/LLM processor that is not timing out
   - A worker waiting on a database lock while the lock holder is also waiting
   - A worker stuck in an infinite retry loop

3. **Check processor timeout configuration:**
   ```bash
   query-config --component content_manager --key processor_timeout_ms
   ```
   - Expected: 30,000 ms (30 s) for OCR, 60,000 ms (60 s) for LLM embedding
   - If unset or 0: worker can block indefinitely — this is the likely cause

### Recovery

```bash
# Force-restart the stalled worker(s)
restart-component --component content_manager --force  # Only if graceful fails

# Verify queue begins draining within 30 seconds
watch-metric --metric content_queue_drain_rate --interval 5s --timeout 60s

# Verify queue depth begins decreasing
watch-metric --metric content_queue_depth --interval 10s --timeout 300s

# If processor timeout was unset, apply fix:
update-config \
  --component content_manager \
  --key processor_timeout_ms \
  --value 30000 \
  --requires-restart true
```

**Success Criteria:**
- [ ] `content_queue_drain_rate` > 0 within 2 min of restart
- [ ] `content_queue_depth` decreasing steadily
- [ ] Processor timeout configured and verified

---

## §7 — High-Cardinality Ingestion Surge (Memory / CPU Pressure)

**Estimated RTO:** 15–30 min  
**Severity:** High

### Detection

```bash
# Check worker process memory
query-metrics \
  --metric content_worker_heap_mb \
  --window 30m \
  --interval 1m

# Check CPU utilization
query-metrics \
  --metric content_worker_cpu_pct \
  --window 10m
```

**Alert Thresholds:**
- ⚠ **Warning:** Worker heap > 500 MB or growing > 50 MB/min
- ❌ **Critical:** Worker heap > 1 GB or CPU > 90% sustained for > 5 min

### Diagnosis

1. **Identify workload source:**
   ```bash
   collect-logs \
     --component content_manager \
     --duration 15m \
     --filter "level=warn OR payload_size_bytes > 1000000" \
     --fields "content_id,producer_id,payload_size_bytes,format,processor_type"
   ```

2. **Check for deduplication table growth:**
   ```bash
   query-metrics --metric content_dedup_table_size --window 1h
   ```
   - Abnormal growth suggests a high-cardinality ingestion surge without deduplication hits

3. **Check chunker output volume:**
   ```bash
   query-metrics \
     --metric content_chunks_produced_total \
     --window 10m \
     --group-by producer_id
   ```

### Recovery

```bash
# Step 1: Identify the high-volume producer
# (from logs in diagnosis step)

# Step 2: Apply rate limiting to the surge source
rate-limit-producer \
  --producer-id <producer-id> \
  --rate 100  # items/min

# Step 3: If memory pressure is severe, flush dedup cache
flush-cache --component content_dedup_cache --keep-hot-entries 10000

# Step 4: Monitor heap recovery
watch-metric --metric content_worker_heap_mb --interval 30s --timeout 300s

# Step 5: If heap does not recover after 5 min:
restart-component --component content_manager --graceful
```

**Success Criteria:**
- [ ] Worker heap growth rate < 5 MB/min within 10 min
- [ ] CPU returns below 70% within 5 min of rate limiting
- [ ] Ingestion continues (possibly rate-limited) without complete stall

---

## Troubleshooting Quick Reference

| Symptom | Likely Cause | First Action | Runbook Section |
|---------|-------------|--------------|-----------------|
| `content_ingestion_rate` = 0 | Stall (any cause) | Check structured error logs | §1 |
| `content_validation_errors` > 20% | Bad producer batch or MIME mismatch | Group errors by `producer_id` | §2 |
| OCR/LLM results missing from content | Processor degraded | Check `content_processor_availability` | §3 |
| `AMPLIFICATION_RISK` alerts | Zip bomb / large archive uploaded | Check `expansion_ratio` in logs | §4 |
| `content_policy_violations` > 30% | Policy misconfiguration or abuse | Check policy changelog | §5 |
| `content_queue_depth` growing, drain = 0 | Worker deadlock | Check worker thread state | §6 |
| Worker heap > 1 GB | High-cardinality ingestion surge | Rate-limit top producer | §7 |

---

## Incident Report Template

```markdown
# Content Module Incident Report

## Incident Details
- **Scenario Type:** [Ingestion Stall / Validation Spike / Processor Degradation / Archive Abuse /
  Policy Surge / Queue Deadlock / Memory Pressure]
- **Start Time:** YYYY-MM-DD HH:MM:SS UTC
- **End Time / Recovery Time:** YYYY-MM-DD HH:MM:SS UTC
- **Duration:** [minutes]
- **Primary Error Code:** [VALIDATION_ERROR / PROCESSOR_DEGRADED / AMPLIFICATION_RISK /
  POLICY_VIOLATION / ASYNC_QUEUE_STALL / INGESTION_STALL / MEMORY_PRESSURE]

## Impact Assessment
- **Ingestion Volume Affected:** [items/percentage]
- **Producer(s) Affected:** [producer-ids]
- **Data Loss or Delayed Enrichment:** [yes/no; describe]

## Timeline
1. [HH:MM] Alert triggered; operators notified
2. [HH:MM] Diagnosis began
3. [HH:MM] Root cause identified
4. [HH:MM] Recovery action initiated
5. [HH:MM] Ingestion rate normalized

## Root Cause Analysis
[To be completed within 24 hours]

## Prevention
[What will prevent recurrence]

## Sign-Off
- **Operator:** [name]
- **Content Platform Lead:** [name]
- **Date:** YYYY-MM-DD
```

---

## Quick Reference — Key Metrics

```bash
# Ingestion rate
query-metrics --metric content_ingestion_rate --window 5m

# Queue health
query-metrics --metric content_queue_depth,content_queue_drain_rate --window 10m

# Validation errors by class
query-metrics --metric content_validation_errors --group-by error_class --window 15m

# Policy violations
query-metrics --metric content_policy_violations --group-by policy_ref --window 15m

# Processor availability
query-metrics --metric content_processor_availability --group-by processor_type --window 5m

# Worker memory
query-metrics --metric content_worker_heap_mb --window 30m

# Security rejections
query-metrics --metric content_security_rejections --group-by error_code --window 15m
```

---

**Runbook Version:** 1.0  
**Last Updated:** 2026-08-24  
**Owner:** Content Platform Team  
**Next Review:** 2027-02-24  
**Wave D Closure:** This runbook satisfies the Wave D operability hardening requirement for
operator-critical scenario coverage in the content module.
