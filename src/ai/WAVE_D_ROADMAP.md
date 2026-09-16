# AI Module — Wave D Operability Hardening Roadmap

<!-- Status: IN PROGRESS | target: Q1 2027 | D1✅ D3~ -->
<!-- Wave: D — Operability Hardening -->
<!-- Branch: develop -->
<!-- Dependency: Wave A–C exit criteria confirmed -->

**Module:** `src/ai/`  
**Wave:** D — AI Module Operability Hardening  
**Target:** Q1 2027  
**Status:** 🟡 In Progress — D1 runbook published; D3 soak test created; D2/D4 pending Q1 2027  

---

## Overview

Wave D for the AI module focuses on operator visibility, runbook completeness, and
sustained-load resilience for the AI generation path, CAI safety gate, and federated
learning coordinator. Unlike Wave A–C (implementation, ML enhancements, safety), Wave D
emphasizes operational confidence: how operators diagnose, respond to, and recover from
production incidents on the AI module.

### Prerequisites

- [~] Wave A–C exit criteria confirmed (Wave C ✅; Wave A/B hardware baselines pending Q4 2026)
- [ ] Representative-hardware runner available for soak/benchmark runs
- [ ] OpenTelemetry SDK deployed across core modules (Wave D cross-cutting dependency from `docs/operability/WAVE_D_ROADMAP.md`)

---

## D1: AI Generation Operator Runbook

**Target:** Q1 2027 (early January)  
**Owner:** Platform / AI module team  
**Output:** `docs/operability/RUNBOOK_AI_GENERATION.md`

### Scope

- Incident classification for AI generation failures (validation, endpoint, transport, parse, sandbox)
- Endpoint-timeout diagnosis flow: how to identify slow endpoints, review allow-list, and adjust timeout budget
- Retry-storm prevention: detecting excessive retry-budget exhaustion and tuning `max_retries` / backoff constants
- Redacted-log interpretation: understanding `[REDACTED]` tokens in logs and how to extract diagnostics safely
- Stats struct interpretation: `requests_attempted`, `requests_succeeded`, `validation_failures`, `endpoint_failures`, `parse_failures` and their operational thresholds
- CAI safety gate latency spikes: diagnosing critic-revision loop overruns and adjusting revision round limits
- Federated aggregation failures: Byzantine-suspect node identification, aggregation round timeouts, DP budget exhaustion alerts

### Tasks

- [x] Draft runbook with incident classification taxonomy (Target: 2026-11)
- [x] Add decision tree for endpoint-timeout + retry-storm scenarios (Target: 2026-11)
- [x] Document stats-counter operational thresholds (alert rules) (Target: 2026-12)
- [x] Add CAI safety gate latency incident section (Target: 2026-12)
- [ ] Review and sign-off with SRE team (Target: Q1 2027)

---

## D2: Observability Expansion

**Target:** Q1 2027  
**Owner:** AI module + Observability team  
**Output:** Updated `src/ai/ai_plugin_generator.cpp`, `src/ai/cai_ethics_integration.cpp`, `src/llm/` LLMAQLHandler paths

### Scope

- Telemetry counters for all error classes (extend existing `Stats` struct)
- CAI safety gate per-call latency histogram (p50/p95/p99)
- Federated aggregation overhead per round (gradient communication latency)
- `generatePlugin` path: distributed trace span propagation (OpenTelemetry)
- `executeInfer` / `executeInferStreaming` / `executeRAG` / `executeChat`: trace span propagation for CAI gate and federated telemetry

### Tasks

- [ ] Extend `AIPluginGenerator::Stats` with per-error-class counters (Target: 2026-11)
- [ ] Add CAI safety gate latency histogram (CAI evaluation p95/p99) (Target: 2026-11)
- [ ] Add federated aggregation round-latency counter (Target: 2026-12)
- [ ] Integrate OpenTelemetry span propagation in `generatePlugin` (Target: Q1 2027)
- [ ] Integrate span propagation in LLMAQLHandler AI-gate paths (Target: Q1 2027)
- [ ] Verify observability output against Wave 7-9 benchmark gates (Target: Q1 2027)

---

## D3: Soak Tests (Sustained Load)

**Target:** Q1 2027 (requires representative-hardware runner)  
**Owner:** AI module + QA team  
**Output:** `tests/ai/test_ai_generation_soak_60min.cpp`

### Scope

- 60-minute sustained load on AI generation path: endpoint-stress, retry-budget-exhaustion, payload-size variations
- CAI safety gate sustained evaluation: 500 evaluations/minute for 10 minutes, verify no latency drift
- Federated aggregation soak: 10 rounds per minute for 30 minutes, verify convergence stability
- Retry-budget exhaustion test: force 100% transient failure rate, verify clean fallback and stats accuracy
- Verify: no memory leaks (RAII compliance), no stat counter overflow, no log-redaction bypass under load

### Tasks

- [x] Create `tests/ai/test_ai_generation_soak_60min.cpp` (label: `wave_d;soak;not_release_critical`) (Target: 2026-12)
- [ ] Add CAI 10-minute sustained evaluation test (Target: 2026-12)
- [ ] Add federated 30-minute round-stability test (Target: Q1 2027)
- [x] Add retry-budget exhaustion soak scenario (Target: Q1 2027)
- [ ] Execute full soak suite on representative hardware (Target: Q1 2027)
- [ ] Publish soak results as Wave D evidence artefact (Target: Q1 2027)

---

## D4: Representative-Hardware p95/p99 Baselines

**Target:** Q1 2027 (requires representative-hardware runner)  
**Owner:** AI module + Release team  
**Output:** Baseline artefacts in `audit/evidence/waves/manifests/`

### Scope

- `generatePlugin` p95/p99 latency on representative hardware (excluding mock/local endpoint)
- `validatePrompt` p99 latency (must remain in low-single-digit ms)
- CAI safety gate p95/p99 overhead per response (must remain ≤ 2.0 s)
- Federated aggregation round p95 latency (must remain ≤ 2.0 s per round)
- AI plugin generator benchmark regression gate (`benchmarks/ai/bench_ai_plugin_generator.cpp`) on hardware

### Tasks

- [ ] Set up representative-hardware benchmark job for AI module targets (Target: 2026-12)
- [ ] Capture `generatePlugin` + `validatePrompt` hardware baselines (Target: Q1 2027)
- [ ] Capture CAI safety gate hardware baselines (Target: Q1 2027)
- [ ] Capture federated aggregation hardware baselines (Target: Q1 2027)
- [ ] Bundle results in `src/ai/WAVE_D_CLOSURE_EVIDENCE_BUNDLE.md` (Target: Q1 2027)

---

## Wave D Exit Criteria (Gate to Release)

| Criterion | Target | Status |
|---|---|---|
| AI generation runbook published at `docs/operability/RUNBOOK_AI_GENERATION.md` | Q1 2027 | [ ] Pending |
| Observability expansion complete: extended Stats + CAI/federated latency histograms | Q1 2027 | [ ] Pending |
| OpenTelemetry span propagation in `generatePlugin` and LLMAQLHandler AI paths | Q1 2027 | [ ] Pending |
| AI generation 60-min soak test created and green | Q1 2027 | [ ] Pending |
| Representative-hardware p95/p99 baselines captured for AI generation + CAI gate | Q1 2027 | [ ] Pending |
| Wave D sign-off document complete with human approval | Q1 2027 | [ ] Pending |

---

## References

- `docs/operability/WAVE_D_ROADMAP.md` — cross-cutting Wave D program (Phase 2A/2B/2C)
- `docs/operability/WAVE_D_SIGN_OFF.md` — sign-off document (D1–D4 completion required)
- `src/ai/ROADMAP.md` — Wave D section
- `src/ai/WAVE_A_CLOSURE_EVIDENCE_BUNDLE.md` — Wave A closure (complete)
- `src/ai/WAVE_B_CLOSURE_EVIDENCE_BUNDLE.md` — Wave B closure (complete)
- `src/ai/WAVE_C_CLOSURE_EVIDENCE_BUNDLE.md` — Wave C closure (complete)
