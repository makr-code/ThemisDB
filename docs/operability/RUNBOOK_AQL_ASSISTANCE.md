# RUNBOOK: AQL Assistance — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, AI/AQL Team Lead  
**Purpose:** Triage and recover from AQL assistance pipeline failures, performance degradation, and provider-level outages  
**Severity:** Medium-High (affects NL→AQL assistance; query execution continues via direct AQL submission)  
**Estimated Duration:** 5 min - 1 hour (depending on failure class)  

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB AQL assistance pipeline (`src/aql/`). The pipeline has three primary surfaces:

1. **Translation pipeline** — NL→AQL (`translateNLToAQL`, `translateNLToAQLWithConfidence`)
2. **Validation pipeline** — post-generation AQL validation (`validateAQLWithParser`)
3. **Bridge/embedding** — embedding and RAG context retrieval (`LLMAQLEmbeddingBridge`)

**Key Principles:**
- AQL assistance is an additive capability; its failure must not block direct AQL submission
- Validation mode is configurable: `FAIL_CLOSED` rejects malformed queries; `WARN_ONLY` (default) logs issues but passes the query for backward compatibility. Operators should set `FAIL_CLOSED` in production.
- Circuit breakers protect the translation path from runaway LLM provider calls
- Conversation context is bounded by token budget; eviction is expected, not an error

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Direct AQL query execution is operational (AQL assistance is non-critical path)
- [ ] Access to server logs with `[TRANSLATION:*]`, `[VALIDATION:*]`, `[BRIDGE:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing AQL assistance metrics (`aql_translation_*`, `aql_validation_*`)
- [ ] Knowledge of which LLM provider is configured (`impl_->config_`) and its health endpoint
- [ ] Circuit breaker state accessible via admin API or log scan

---

## Failure Scenarios

---

### Scenario 1: LLM Provider Unavailability (Circuit Breaker Open)

**Symptoms:**
- Logs contain `[TRANSLATION:ProviderUnavailable]` tags
- Translation requests return `LLMException` with `"Circuit breaker"` or `"temporarily unavailable"` in the message
- All NL→AQL requests fail immediately (no retries)

**Log pattern to search for:**
```
[TRANSLATION:ProviderUnavailable] NL-to-AQL: Provider unavailable (circuit breaker open): ...
```

#### Step 1: Confirm Circuit Breaker State

```bash
# Search recent logs for circuit breaker events
grep '\[TRANSLATION:ProviderUnavailable\]' /var/log/themisdb/themisdb.log | tail -20

# Check if all breakers are open (pattern: "Circuit breaker" in error messages)
grep 'Circuit breaker' /var/log/themisdb/themisdb.log | tail -20
```

Expected output when open:
```
[TRANSLATION:ProviderUnavailable] NL-to-AQL: Provider unavailable (circuit breaker open): Circuit breaker for 'translate' is OPEN
```

#### Step 2: Diagnose Root Cause

| Symptom | Likely Cause | Action |
|---------|-------------|--------|
| All breakers open simultaneously | LLM provider process down | Restart LLM provider, then reset breakers |
| Breaker for `translate` only | Translation-specific failure (CUDA OOM, model unloaded) | Check GPU/model health, then reset |
| Intermittent open/half-open cycling | Flaky provider network or model instability | Investigate provider logs, increase failure threshold |
| Breaker never closes (stuck OPEN) | Consecutive failures exceed threshold, not recovering | Manual half-open probe (Step 3) |

#### Step 3: Reset Circuit Breaker (Manual)

```bash
# Option A: Restart the ThemisDB server process to reset all in-process circuit breakers
# (no dedicated aql-assistance CLI subcommand exists; use server restart or config reload)
systemctl restart themisdb  # or equivalent for your deployment

# Option B: If provider is healthy, watch for the breaker to self-transition to half-open
# after the configured cool-down interval, then confirm via log:
grep 'circuit_breaker.*HALF_OPEN\|breaker.*half.open' /var/log/themisdb/themisdb.log | tail -5

# Verify breaker transitions to CLOSED
grep 'circuit_breaker.*CLOSED\|breaker.*success' /var/log/themisdb/themisdb.log | tail -5
```

#### Step 4: Validate Recovery

```bash
# Confirm translations are succeeding again (check for absence of ProviderUnavailable tags)
grep '\[TRANSLATION:Confidence\]' /var/log/themisdb/themisdb.log | tail -10

# Confirm [TRANSLATION:ProviderUnavailable] messages stop appearing
grep '\[TRANSLATION:ProviderUnavailable\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | tail -5
```

**Decision Point:**
- ✅ **Recovery confirmed:** Translation succeeds, no new `ProviderUnavailable` logs → incident closed
- ❌ **Provider still unavailable:** Escalate to LLM infrastructure team; consider enabling `WARN_ONLY` mode (see Scenario 2)

---

### Scenario 2: NL→AQL Translation Failure Triage

**Symptoms:**
- Individual translation requests fail with `[TRANSLATION:GenerationFailed]`
- Some translations succeed (not all-or-nothing failure like Scenario 1)
- High retry rate in metrics: `aql_translation_retry_total` rising

**Log patterns:**
```
[TRANSLATION:GenerationFailed] NL-to-AQL: Validation failed (attempt N/M): <error detail>
[TRANSLATION:GenerationFailed] NL-to-AQL: Exhausted all N retries with validation errors
[TRANSLATION:GenerationFailed] NL-to-AQL: Translation exception: <error>
[TRANSLATION:Confidence] provider=<id> confidence_score=<0.00-1.00> retries_used=<N>
```

#### Step 1: Identify Failure Pattern

```bash
# Count GenerationFailed events per minute (rate spike indicates model quality issue)
grep '\[TRANSLATION:GenerationFailed\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | cut -d: -f1,2 | sort | uniq -c | tail -20

# Extract the specific validation errors causing failures
grep 'Validation failed' /var/log/themisdb/themisdb.log | \
  sed 's/.*Validation failed.*): //' | sort | uniq -c | sort -rn | head -10
```

#### Step 2: Diagnose by Error Pattern

| Error Pattern | Likely Cause | Resolution |
|---|---|---|
| `Generated AQL failed validation: Missing RETURN` | Model not generating complete queries | Increase `max_retries` in validation config |
| `Generated AQL failed validation: injection` | Injection attempt detected | Security event — review input, block client |
| `NL-to-AQL generation exhausted retries` | Model producing persistently invalid AQL | Reload model, check few-shot examples |
| `NestedSubqueryDepthExceeded` | Query complexity too high | Inform user to simplify query |
| `CollectionNameTooLong` | Hallucinated oversized collection name | Model quality issue — check schema context |
| `Collection scope check failed` | Hallucinated collection outside schema | Schema context missing — ensure schema is injected |

#### Step 3: Increase Retry Budget (Temporary Mitigation)

```bash
# View current retry config
themisdb-admin config get aql.validation_config.max_retries

# Increase retries temporarily (if model quality is marginal)
themisdb-admin config set aql.validation_config.max_retries 5 --apply-live
```

#### Step 4: Enable WARN_ONLY Mode (Last Resort)

Only use if business continuity requires imperfect translation over zero translation:

```bash
themisdb-admin config set aql.translation_validation_mode warn_only --apply-live
# WARNING: This allows potentially invalid AQL through to the query executor.
# Revert once the underlying model/provider issue is resolved.
```

**Decision Point:**
- ✅ **Failure rate drops to baseline:** Normal operation restored → close incident
- ⚠ **Isolated failures persist:** Monitor for 10 min; if < 1% error rate, accept
- ❌ **Failure rate > 5%:** Escalate to AQL/AI team; reload model if available

---

### Scenario 3: Validation Pipeline Errors

**Symptoms:**
- Logs contain `[VALIDATION:*]` structured error tags
- Validation latency metrics spiking (`aql_validation_latency_p99_us`)
- Schema mismatch or injection detection warnings

**Log patterns:**
```
[VALIDATION:SchemaMismatch] validateAQLWithParser: schema context mismatch for collection <name>
[VALIDATION:InjectionAttempt] validateAQLWithParser: injection pattern detected in generated AQL
[VALIDATION:NestedSubqueryDepthExceeded] query depth 6 exceeds maximum allowed depth 5
[VALIDATION:CollectionNameTooLong] collection name length 150 exceeds maximum 128
```

#### Step 1: Classify the Validation Error Type

```bash
# Check for injection attempts (security-relevant — alert immediately)
grep '\[VALIDATION:InjectionAttempt\]' /var/log/themisdb/themisdb.log | tail -20

# Check for schema mismatches (usually benign schema drift)
grep '\[VALIDATION:SchemaMismatch\]' /var/log/themisdb/themisdb.log | tail -20

# Check for structural policy violations
grep '\[VALIDATION:NestedSubqueryDepthExceeded\]\|\[VALIDATION:CollectionNameTooLong\]' \
  /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Action by Error Type

**InjectionAttempt:**
1. Note the client identifier from the log line
2. Block or rate-limit the client at the API gateway
3. Review the NL query that triggered the detection
4. File a security incident if pattern appears adversarial

**SchemaMismatch:**
1. Verify the schema context being injected matches the current database schema
2. If schema changed recently, update the schema_context passed to `translateNLToAQL()`
3. Validate corrected schema in staging before applying to production

**NestedSubqueryDepthExceeded / CollectionNameTooLong:**
1. These are policy enforcement events — no operator action required
2. Return a user-facing message: "Query complexity exceeds supported limits; simplify the query"
3. Monitor for sudden spike (could indicate a client sending adversarial inputs)

#### Step 3: Verify Validation Latency Is Within Gate

```bash
# Check p99 validation latency (gate: ≤ 100 µs)
query-metrics --metric aql_validation_latency_p99_us --range 1h --window 5m

# If p99 > 100 µs, check for slow regex patterns or large schema contexts
grep '\[VALIDATION:' /var/log/themisdb/themisdb.log | \
  grep -v 'NestedSubquery\|CollectionName' | tail -30
```

**Decision Point:**
- `InjectionAttempt`: Security incident — escalate immediately to security team
- `SchemaMismatch`: Schema refresh required — coordinate with schema/DDL team
- Structural policy: Expected behavior — log and return policy error to user

---

### Scenario 4: Circuit Breaker State Management

**Objective:** Understand and manage circuit breaker state across the three AQL assistance breakers: `translate`, `validate`, `rag`.

#### Circuit Breaker States

| State | Meaning | Operator Action |
|---|---|---|
| CLOSED | Normal operation | None |
| OPEN | Provider unavailable; all calls fail immediately | Diagnose provider, reset when healthy |
| HALF_OPEN | Probe mode; one test call allowed | Allow probe to complete; do not intervene |

#### Checking All Breaker States

```bash
# All three breakers: translate, validate, rag
for breaker in translate validate rag; do
  echo "=== breaker: $breaker ==="
  grep "circuit_breaker.*${breaker}" /var/log/themisdb/themisdb.log | tail -3
done
```

#### Forcing Half-Open (Probe)

```bash
# Wait for automatic half-open transition (default: 30 s after last failure).
# No dedicated CLI exists to force this; monitor log for HALF_OPEN transition:
grep 'circuit_breaker.*HALF_OPEN\|breaker.*half.open' /var/log/themisdb/themisdb.log | tail -5
```

#### Resetting to CLOSED (Manual)

Only reset when provider health is confirmed:

```bash
# Confirm provider is healthy first
themisdb-admin llm-provider health-check --provider-id <configured-provider>

# Then restart ThemisDB to reset all in-process circuit breakers
# (no dedicated aql-assistance reset-circuit-breakers CLI subcommand exists)
systemctl restart themisdb  # or equivalent for your deployment
```

---

### Scenario 5: Performance Degradation

**Symptoms:**
- Translation latency p95 > 2 ms (gate: AG-4)
- Validation latency p95 > 100 µs (gate: AG-5)
- `[TRANSLATION:Confidence]` logs showing low confidence scores (< 0.5)

**Log patterns:**
```
[TRANSLATION:Confidence] provider=<id> confidence_score=0.42 retries_used=3
NL-to-AQL: LLM invocation attempt 3/3
```

#### Step 1: Identify Which Path Is Slow

```bash
# Translation latency trend (p95 gate: ≤ 2 ms)
query-metrics --metric aql_translation_latency_p95_ms --range 2h --window 5m

# Validation latency trend (p95 gate: ≤ 100 µs)
query-metrics --metric aql_validation_latency_p95_us --range 2h --window 5m

# Check retry rate (high retries inflate translation latency significantly)
query-metrics --metric aql_translation_retry_total_rate --range 2h --window 5m
```

#### Step 2: Correlate With System Events

```bash
# Check GPU/CPU utilization around the degradation window
query-metrics --metric gpu_utilization_pct,cpu_utilization_pct \
  --range 2h --window 5m --overlay

# Check if model reload occurred (can spike latency temporarily)
grep 'model.*load\|model.*reload' /var/log/themisdb/themisdb.log | tail -10
```

#### Step 3: Mitigations by Cause

| Cause | Detection | Mitigation |
|---|---|---|
| High retry count inflating p95 | `retries_used > 1` common in Confidence logs | Improve few-shot examples, reload model |
| Validation schema too large | Schema context > 4 KB in logs | Trim schema to relevant collections only |
| LLM model under memory pressure | GPU utilization > 98%, OOM warnings | Reduce batch size or concurrency |
| Cold model (first call after idle) | Single spike in latency metrics | Accept warm-up cost; pre-warm via periodic probe |
| Regex compilation overhead in validator | `spdlog::debug "[AQLValidator] regex compile"` repeated | Confirm static regex objects are being reused |

#### Step 4: Verify Recovery Against Gates

```bash
# Confirm p95 latency returns to expected range
query-metrics --metric aql_translation_latency_p95_ms --range 30m --window 1m
# Expected: ≤ 1.89 ms (locked baseline) with p99 ≤ 2.0 ms

query-metrics --metric aql_validation_latency_p95_us --range 30m --window 1m
# Expected: ≤ 100 µs
```

---

### Scenario 6: Conversation Context Overflow

**Symptoms:**
- Logs show context eviction events at high rate
- `aql_context_eviction_total` metric rising steeply
- Users report earlier conversation turns being "forgotten"

**Log patterns:**
```
[BRIDGE:ContextEviction] Compressing conversation history: turns=45 budget_tokens=2048 used_tokens=2250
[BRIDGE:ContextEviction] Evicted N turns; remaining=M used_tokens=K
```

#### Step 1: Check Eviction Rate

```bash
# Eviction events per minute
query-metrics --metric aql_context_eviction_total_rate --range 1h --window 5m

# Raw eviction log entries
grep '\[BRIDGE:ContextEviction\]\|evict\|Compressing conversation' \
  /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Assess Impact

Eviction is **expected behavior** — the context window is bounded by design.
Operator action is required only if:
- Eviction rate is **much higher than baseline** (spike, not steady-state)
- Users report coherence failures (context losing critical schema or query state)

#### Step 3: Increase Token Budget (If Justified)

```bash
# View current budget
themisdb-admin config get aql.conversation_context.max_history_tokens

# Increase (doubles memory pressure — validate with load test first)
themisdb-admin config set aql.conversation_context.max_history_tokens 4096 --apply-live
```

#### Step 4: Tune Compressor

If the compressor is evicting too aggressively (low similarity threshold):

```bash
# View compressor config
themisdb-admin config get aql.conversation_context.compressor_similarity_threshold

# Increase threshold to retain more semantically similar turns
themisdb-admin config set aql.conversation_context.compressor_similarity_threshold 0.7 --apply-live
```

**Decision Point:**
- ✅ **Eviction rate within baseline, users not impacted:** Steady-state, no action
- ⚠ **Eviction rate 2× baseline:** Investigate conversation length distribution; consider budget increase
- ❌ **Coherence failures confirmed:** Increase budget and reduce compressor aggressiveness

---

## Troubleshooting Quick Reference

| Symptom | Log Tag | First Action |
|---------|---------|-------------|
| All translations fail | `[TRANSLATION:ProviderUnavailable]` | Check circuit breaker state (Scenario 1) |
| Individual translations fail | `[TRANSLATION:GenerationFailed]` | Check retry count and validation errors (Scenario 2) |
| Injection detected | `[VALIDATION:InjectionAttempt]` | Security incident — block client, alert security team |
| Schema mismatch | `[VALIDATION:SchemaMismatch]` | Refresh schema context injection (Scenario 3) |
| Query too complex | `[VALIDATION:NestedSubqueryDepthExceeded]` | User-facing limit — return policy error message |
| Collection name invalid | `[VALIDATION:CollectionNameTooLong]` | User-facing limit — return policy error message |
| Translation p95 > 2 ms | `[TRANSLATION:Confidence]` retries_used > 1 | Check retry rate and model quality (Scenario 5) |
| Validation p95 > 100 µs | Latency metric spike | Check schema context size (Scenario 5) |
| Context evictions spiking | `[BRIDGE:ContextEviction]` | Assess budget, tune compressor (Scenario 6) |

---

## Evidence & Logging Checklist

After incident resolution, collect:

- [ ] `aql_translation_latency_p99.json` — p50/p95/p99 pre- and post-incident
- [ ] `aql_circuit_breaker_events.log` — Filtered breaker open/close/probe events
- [ ] `aql_validation_error_counts.json` — Error category distribution during incident window
- [ ] `aql_confidence_score_distribution.csv` — Confidence scores during incident (low scores indicate model quality issue)
- [ ] `aql_context_eviction_rate.csv` — Eviction rate over incident window

Archive in: `evidence/aql-assistance-incidents/<date>-<issue-id>/`

---

## Quick Reference: Diagnostic Commands

```bash
# ── Circuit breaker health ──────────────────────────────────────────────────
grep 'circuit_breaker\|TRANSLATION:ProviderUnavailable' \
  /var/log/themisdb/themisdb.log | tail -30

# ── Translation failures ────────────────────────────────────────────────────
grep '\[TRANSLATION:GenerationFailed\]' /var/log/themisdb/themisdb.log | tail -20

# ── Confidence and retry diagnostics ────────────────────────────────────────
grep '\[TRANSLATION:Confidence\]' /var/log/themisdb/themisdb.log | tail -20

# ── Validation errors ───────────────────────────────────────────────────────
grep '\[VALIDATION:' /var/log/themisdb/themisdb.log | tail -30

# ── Bridge / context eviction ───────────────────────────────────────────────
grep '\[BRIDGE:' /var/log/themisdb/themisdb.log | tail -20

# ── Combined AQL assistance health check ────────────────────────────────────
# (no dedicated aql-assistance health-check CLI subcommand exists; use log scan above)

# ── Live metric snapshot ────────────────────────────────────────────────────
query-metrics \
  --metric aql_translation_latency_p99_ms,aql_validation_latency_p99_us,\
aql_translation_retry_total_rate,aql_context_eviction_total_rate \
  --range 30m --window 1m
```

---

## Wave D — D1 Distributed Trace Span Cross-Links

> **Wave D Phase 2A dependency:** The trace span annotations below reference the `DistributedTraceSpan`
> framework planned in `docs/operability/WAVE_D_ROADMAP.md` §2A. Until Phase 2A implementation
> completes (Target: Q1 2027), the listed span names are reference identifiers for future
> instrumentation.

### AQL Assistance D1 Trace Spans

When the Phase 2A tracing SDK is available, the following operator actions map to trace spans:

| Pipeline Surface | D1 Span Name | Baggage Keys | Notes |
|---|---|---|---|
| NL→AQL translation | `aql.translate_nl_to_aql` | `provider_id`, `retries_used`, `confidence_score` | Includes retry child spans |
| Validation | `aql.validate_with_parser` | `aql_length_chars`, `schema_collections_count`, `error_category` | Error status set on failure |
| Bridge/RAG | `aql.embedding_bridge.execute` | `collection`, `retrieved_docs`, `latency_ms` | Child span of translate |
| Circuit breaker open | `aql.circuit_breaker.trip` | `breaker_name`, `failure_count`, `state` | Status ERROR |
| Context eviction | `aql.context.evict` | `turns_evicted`, `tokens_freed`, `budget_tokens` | Informational |

### Querying Trace Spans (Phase 2A onwards)

```bash
# Find all translation traces with low confidence scores
otel-query --service aql_assistance --operation aql.translate_nl_to_aql \
  --baggage "confidence_score<0.5" --range 1h

# Find all validation failures by error category
otel-query --service aql_assistance --operation aql.validate_with_parser \
  --status ERROR --range 24h --include-baggage

# Find circuit breaker trip events
otel-query --service aql_assistance --operation aql.circuit_breaker.trip \
  --status ERROR --range 7d

# Cross-reference confidence with retry count
otel-metrics-join \
  --trace-operation aql.translate_nl_to_aql \
  --metric aql_translation_retry_total_rate \
  --window 5m
```

### Phase 2A Instrumentation Targets

Once Phase 2A is implemented, add trace points in:

- `src/aql/llm_aql_handler.cpp`: Wrap `translateNLToAQL()` and `validateAQLWithParser()` in
  `DistributedTraceSpan` with baggage `provider_id`, `retries_used`, `confidence_score`
- `src/aql/llm_aql_embedding_bridge.cpp`: Wrap `execute()` and fallback paths in
  `DistributedTraceSpan` with baggage `collection`, `retrieved_docs`, `error_category`
- `src/aql/aql_query_validator.cpp`: Add span events for policy violations
  (`NestedSubqueryDepthExceeded`, `CollectionNameTooLong`, `InjectionAttempt`)

**Related Wave D documents:**
- `docs/operability/WAVE_D_ROADMAP.md` §2A — DistributedTraceSpan implementation plan
- `docs/operability/PHASE2A_DISTRIBUTED_TRACING_VERIFICATION.md` — Acceptance gate W4A-TRACE-01
- `src/aql/ROADMAP.md` §Wave D — Operability dependency notes

---

**Runbook Version:** 1.0  
**Last Updated:** 2026-09-16  
**Owner:** AI/AQL Team, Operations Team  
**Next Review:** 2027-03-01 (post-Wave D Phase 2A delivery)
