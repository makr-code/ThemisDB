# RUNBOOK: AI Generation — Incident Response & Operator Guide

**Author:** ThemisDB Platform Team  
**Created:** 2026-08-24  
**Last Updated:** 2026-09-16  
**Status:** active  

**Audience:** Database Operators, SREs, AI/LLM Integration Team  
**Purpose:** Diagnose and recover from AI plugin generation failures, endpoint issues,
retry storms, CAI safety gate latency spikes, and federated aggregation failures  
**Severity:** Medium–High (affects AI plugin generation workloads; core database unaffected)  
**Estimated Duration:** 5–30 min (diagnosis + recovery)  
**Module:** `src/ai/` — `AIPluginGenerator`, `CAIEthicsIntegration`  

---

## Overview

This runbook guides operators through diagnosing and resolving production incidents
on the ThemisDB AI generation path. The AI generation path covers:

1. **Plugin Generation** (`generatePlugin`) — LLM endpoint invocation, validation, retry
2. **CAI Safety Gate** (`CAIEthicsIntegration`) — critic-revision loop, ethical evaluation
3. **Federated Aggregation** — gradient aggregation rounds, DP budget, Byzantine detection

**Key Principles:**
- The AI path is **fail-closed**: all error conditions return structured errors; no partial success
- Retry logic is **bounded**: max 3 attempts with exponential backoff (100/200/400 ms)
- Logs are **redacted**: sensitive fields truncated at 120 chars; never log raw LLM output
- The core database is **unaffected** by AI generation failures

---

## Prerequisites Checklist

- [ ] Access to AI module logs (spdlog WARN/ERROR level output from `AIPluginGenerator`)
- [ ] Stats counter access (via `AIPluginGenerator::getStats()` or metrics endpoint)
- [ ] Knowledge of configured endpoint allow-list (`Config::allowed_endpoints`)
- [ ] Access to CAI safety gate configuration (`CAIConfig::max_revision_rounds`, latency budget)
- [ ] Escalation contact for LLM endpoint team (if external endpoint)

---

## Stats Counter Reference

The `AIPluginGenerator::Stats` struct exposes 7 observable counters. Use these as
the primary triage signal before inspecting logs.

| Counter | Meaning | Alert Threshold |
|---|---|---|
| `successes` | Successful `generatePlugin()` completions | Baseline: monitor for drops |
| `validation_errors` | Prompt validation failures (bad input from caller) | > 5% of attempts → caller issue |
| `transport_errors` | CURL transport failures after all retries exhausted | Any sustained spike → endpoint/network issue |
| `http_errors` | Non-2xx HTTP responses (not retried) | Any sustained spike → endpoint policy/auth issue |
| `parse_errors` | JSON parse or schema type errors in LLM response | Any spike → LLM response format change |
| `safety_rejections` | C1 CAI safety gate rejections | Monitor; sustained spike may indicate adversarial inputs |
| `sandbox_rejections` | Sandbox artifact gate rejections | Any spike → generated code fails verification |

---

## Incident Taxonomy

### Type 1: Validation Failure (caller-side)

**Symptom:** High `validation_errors` counter; requests rejected before LLM call  
**Root Cause:** Caller sending malformed prompts (description too long, oversized capability/dependency lists, invalid tokens, control characters)  
**Action:**
1. Inspect log lines at WARN level: `"Prompt validation failed"`
2. Identify the failing field (description length, token count, format)
3. Fix the caller's prompt construction — no operator change required on the server side
4. Verify: `validation_errors` stops growing after caller fix

---

### Type 2: Endpoint Timeout / Transport Failure

**Symptom:** High `transport_errors`; requests timing out or connection refused  
**Root Cause:** LLM endpoint unreachable, slow, or network path degraded

**Decision Tree:**

```
transport_errors spiking?
│
├── Is the endpoint in the allow-list? ──NO──> Caller is targeting non-allowed endpoint
│                                              Fix: update Config::allowed_endpoints
│
├── YES → Check endpoint reachability:
│   curl -s --max-time 10 <configured_endpoint>/health
│
│   ├── TIMEOUT/REFUSED → Endpoint is down
│   │   Action: Switch to backup endpoint (update allow-list + endpoint config)
│   │   Verify: transport_errors stop growing within 2-3 minutes
│   │
│   └── HTTP 200 → Endpoint up but slow
│       Action: Check endpoint latency → may need to increase request timeout
│       Check: is retry storm in progress? (see Type 3)
```

**Recovery Steps:**
1. Identify configured endpoint from `AIPluginGenerator::Config`
2. Test endpoint reachability with a direct HTTP probe
3. If endpoint down: switch to fallback endpoint (update config + hot reload if supported)
4. If endpoint slow: check current retry budget; tune `request_timeout_ms` if latency spike is transient
5. Verify: `transport_errors` counter stabilizes; `successes` resume

---

### Type 3: Retry Storm

**Symptom:** `transport_errors` counter growing rapidly; high CPU from retry loops;
large number of in-flight requests to the LLM endpoint  
**Root Cause:** LLM endpoint partially unavailable; all requests hitting max retries (3 × 100/200/400 ms)

**Detection:**
```bash
# Check retry rate in logs (look for "Endpoint attempt N failed, retrying" at WARN level)
grep "Endpoint attempt" /var/log/themisdb/ai.log | tail -50

# Check if retry budget exhausted consistently:
grep "All N retry attempts failed" /var/log/themisdb/ai.log | wc -l
```

**Action:**
1. If > 50% of requests hitting max retries → endpoint is effectively down (treat as Type 2)
2. If < 20% hitting max retries → transient intermittent failures; monitor
3. To reduce retry pressure: temporarily reduce incoming generation request rate via caller throttling
4. **Do not increase max retries without review** — this extends per-request latency and can cascade

**Recovery Steps:**
1. Confirm endpoint health (Type 2 check)
2. If endpoint recovers: retry storm self-resolves within ~400 ms (max backoff)
3. If endpoint remains degraded: apply caller-side circuit breaker / rate limit
4. Verify: `transport_errors` rate drops; `successes` resume

---

### Type 4: HTTP Error (Non-2xx)

**Symptom:** High `http_errors`; `transport_errors` stable (connection OK)  
**Root Cause:** LLM endpoint returning 4xx/5xx responses

| HTTP Status | Likely Cause | Action |
|---|---|---|
| 401 / 403 | Auth token expired or wrong API key | Rotate/update API credential in Config |
| 400 | Malformed request body | Check LLM endpoint API version; inspect sanitized request log |
| 429 | Rate limit exceeded | Apply caller-side rate limiting; check quota |
| 500 / 502 / 503 | Endpoint internal error | Treat as endpoint down (Type 2) |

**Note:** Non-2xx responses are **not retried** (by design). Each 4xx/5xx counts once.

---

### Type 5: LLM Response Parse Failure

**Symptom:** High `parse_errors`; requests succeed transport but fail at JSON parse / schema validation  
**Root Cause:** LLM endpoint changed response format; response truncated; malformed JSON

**Detection:**
```bash
grep "Failed to parse LLM response" /var/log/themisdb/ai.log | tail -20
grep "Response schema validation failed" /var/log/themisdb/ai.log | tail -20
```

**Action:**
1. Check if a recent LLM endpoint update was deployed
2. Compare expected schema: `{ "generated_plugin": { "code": "...", "manifest": {...} } }`
3. If endpoint API version changed: update the response parser in `ai_plugin_generator.cpp`
4. Short-term mitigation: route to a pinned/stable endpoint version

---

### Type 6: CAI Safety Gate Latency Spike

**Symptom:** `generatePlugin` P95/P99 latency increasing; CAI critic-revision loop running multiple rounds  
**Root Cause:** Input content requiring multiple revision rounds (max 2); LLM-as-critic slow response

**Normal behavior:**
- CAI gate runs 1–2 critic-revision rounds (max 2)
- Per-round latency budget: ≤ 2.0 s total overhead
- Latency spike = critic LLM responding slowly or prompt triggering max rounds

**Action:**
1. Check if `safety_rejections` is also increasing (both up = adversarial inputs; only latency = critic LLM slow)
2. Check critic LLM endpoint health (same Type 2 steps above)
3. If adversarial inputs suspected: review prompt content patterns; consider input rate limiting
4. If critic LLM is overloaded: temporarily increase CAI gate timeout budget in `CAIConfig`
5. As last resort: disable CAI gate (`CAIConfig::enabled = false`) — **requires human approval**

**Thresholds:**
| Metric | Normal | Warning | Critical |
|---|---|---|---|
| CAI gate P99 latency | ≤ 500 ms | 500 ms – 2.0 s | > 2.0 s |
| Revision rounds per call | 0–1 | 2 | Always 2 |
| False positive rate | ≤ 5% | 5–10% | > 10% |

---

### Type 7: Federated Aggregation Failure

**Symptom:** Federated telemetry hooks failing; aggregation round timeouts; federated coordinator errors  
**Root Cause:** Byzantine-suspect node detected; aggregation round timeout; DP budget exhausted

**Byzantine Node Detection:**
```
federated aggregation failure in logs?
│
├── "Byzantine gradient detected from node X" → Node X suspected malicious/faulty
│   Action: Inspect node X health; exclude from next aggregation round
│
├── "Aggregation round timed out after N ms" → Slow nodes / network partition
│   Action: Reduce aggregation quorum; increase round timeout budget
│
└── "DP budget exhausted for this training epoch" → epsilon budget used up
    Action: Wait for next training epoch to reset; review DP tuning
```

**Recovery Steps:**
1. Identify failing node from coordinator logs
2. Exclude Byzantine-suspect node from next round via coordinator configuration
3. Verify: aggregation resumes within 1–2 rounds (Byzantine-robust median aggregation tolerates minority failures)
4. If budget exhausted: confirm this is expected per-epoch behavior; escalate if unexpected

---

## Redacted Log Interpretation

AI module logs truncate sensitive content at 120 characters and append `[…]`.

**How to read truncated logs:**
- `"Prompt description: [TRUNCATED at 120 chars][…]"` — normal; full content not needed for diagnosis
- `"LLM response: [REDACTED]"` — by design; raw LLM output never appears in logs
- `"Endpoint: [REDACTED_URL]"` — configured endpoint; check `Config::allowed_endpoints`

**What to do if you need full content for diagnosis:**
1. Enable debug capture in a non-production environment only
2. Never enable verbose LLM output logging in production (security/privacy policy)
3. Use Stats counters and error type classification instead of raw content inspection

---

## Stats-Counter Operational Thresholds and Alert Rules

| Counter | Alert Condition | Severity | Action |
|---|---|---|---|
| `transport_errors` | > 0 for > 60 s sustained | HIGH | Endpoint health check (Type 2) |
| `http_errors` | > 0 for > 60 s sustained | HIGH | Check auth/rate-limit (Type 4) |
| `parse_errors` | Any occurrence | MEDIUM | Check endpoint API version (Type 5) |
| `safety_rejections` | > 10% of total calls | MEDIUM | Review input patterns (Type 6) |
| `sandbox_rejections` | Any occurrence | MEDIUM | Check generated code quality |
| `validation_errors` | > 5% of total calls | LOW | Caller-side issue (Type 1) |
| `successes` | Drop > 50% vs baseline | HIGH | Composite — check all counters |

---

## Troubleshooting Table

| Symptom | Likely Cause | Counter Signal | Action |
|---|---|---|---|
| Requests returning ERR_VALIDATION_FAILED immediately | Bad prompt from caller | `validation_errors` up | Fix caller input (Type 1) |
| Requests timing out, no HTTP response | Endpoint down / unreachable | `transport_errors` up | Endpoint health check (Type 2) |
| Requests failing after 3 retries | Endpoint intermittent | `transport_errors` up | Retry storm check (Type 3) |
| Requests returning HTTP 401/403 | Auth failure | `http_errors` up | Rotate credential (Type 4) |
| Requests returning HTTP 429 | Rate limit | `http_errors` up | Rate-limit caller (Type 4) |
| Requests succeeding transport but failing with parse error | Schema change in LLM response | `parse_errors` up | Check endpoint API version (Type 5) |
| High P95/P99 latency on `generatePlugin` | CAI gate slow or retry overhead | `safety_rejections` or latency spike | CAI gate check (Type 6) |
| Federated coordinator errors in logs | Byzantine node / DP budget | Coordinator log patterns | Federated triage (Type 7) |
| `successes` drops to 0, all other counters 0 | Constructor/init failure or allow-list blocks all endpoints | — | Check `Config::allowed_endpoints`; check startup logs |

---

## Rollback / Emergency Disable Procedures

### Disable CAI Safety Gate (emergency only — requires human approval)

```cpp
// In AIPluginGenerator::Config:
config.enable_cai_gate = false;  // Disable C1 safety evaluation
// REQUIRES: human approval; document in incident log
```

### Disable Sandbox Gate (if blocking legitimate generation)

```cpp
config.enable_sandbox_gate = false;  // Disable artifact verification
// Use only if sandbox filesystem is unavailable in production environment
```

### Switch to Fallback Endpoint

```cpp
config.llm_endpoint = "https://fallback-endpoint.example.com/generate";
config.allowed_endpoints = {"https://fallback-endpoint.example.com"};
// Verify allow-list includes new endpoint before switching
```

---

## Evidence and Logging Checklist

When escalating an AI generation incident:

- [ ] `AIPluginGenerator::Stats` snapshot at time of incident
- [ ] Relevant WARN/ERROR log lines from `ai.log` (no raw LLM content)
- [ ] HTTP status code distribution (from `http_errors` and logs)
- [ ] Endpoint reachability probe result
- [ ] CAI gate configuration (`max_revision_rounds`, `enabled` flag)
- [ ] Duration of the incident (start timestamp → resolution)
- [ ] Any recent configuration changes (endpoint, allow-list, CAI config)

---

## References

- `src/ai/ROADMAP.md` — Wave D section (D1 runbook)
- `src/ai/WAVE_D_ROADMAP.md` — full Wave D plan
- `src/ai/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` — Wave A reliability evidence
- `include/ai/ai_plugin_generator.h` — Config struct, Stats struct, API contract
- `include/ai/cai_ethics_integration.h` — CAIConfig, EthicsEvaluator API
- `docs/operability/WAVE_D_SIGN_OFF.md` — Batch D5 (AI runbook sign-off)
