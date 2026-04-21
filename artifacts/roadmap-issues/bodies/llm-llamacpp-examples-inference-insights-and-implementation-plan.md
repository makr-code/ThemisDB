## Summary
Auswertung der `llama.cpp/examples` zeigt mehrere direkt nutzbare Hebel fuer ThemisDB-Inferencing:
- Continuous Batching mit adaptivem Retry
- KV-Prefix-Sharing fuer gemeinsame Systemprompts
- Prompt Lookup Decoding (n-Gram basiert) als pragmatischer Speculation-Einstieg
- Telemetrie-gestuetztes Auto-Tuning von Batch/Parallelitaet/Draft

Diese Issue beschreibt die produktive Umsetzung in Phasen inkl. Metriken, Tests und Rollout-Gates.

## Problem
Aktuell ist die Inference-Pipeline in Lastspitzen und bei heterogenen Promptlaengen nicht optimal auf Durchsatz, P95-Latenz und KV-Cache-Effizienz abgestimmt. Vorhandene LLM-Bausteine im Repo ermoeglichen die Integration, aber es fehlt ein abgestimmter End-to-End-Plan fuer:
- Scheduler-Verhalten unter Parallelitaet
- KV-Lifecycle bei Prefix-Sharing und partiellen Fehlschlaegen
- Speculation-Light (Lookup) vor komplexer Draft-Modell-Architektur

## Goals
- Durchsatz verbessern ohne Qualitaetsregression.
- P95-Latenz stabilisieren/verbessern fuer parallele Requests.
- KV-Cache-Kosten pro Request senken.
- Messbare Betriebsmetriken fuer automatische Parametertuning-Entscheidungen bereitstellen.

## Non-Goals
- Kein sofortiger produktiver Rollout von tree-based Lookahead in Phase 1.
- Kein harter Wechsel auf Draft-Modell-Speculation ohne Lookup-Pilotdaten.

## Scope
Betroffene Bereiche (Initial):
- `include/llm/continuous_batch_scheduler.h`
- `include/llm/paged_kv_cache*.h`
- `include/llm/llm_prefix_cache.h`
- `include/llm/speculative_decoder.h`
- `src/llm/*` (Scheduler-/KV-/Speculation-Implementierung)
- `benchmarks/bench_llm_inference_performance.cpp`
- Observability (`grafana`, `prometheus` Regeln/Panel)

## Reference Analysis
Technik-Quellen aus `llama.cpp/examples`:
- `batched`, `parallel`, `lookahead`, `lookup`, `speculative`, `speculative-simple`, `save-load-state`, `embedding`
- Detaillierte Notiz im Repo: `include/llm/LLAMACPP_EXAMPLES_INFERENCE_INSIGHTS.md`

## Implementation Phases

### Phase 1: Design / API-Vertrag
- [ ] API fuer Continuous Batching finalisieren (Target: Q2 2026)
- [ ] KV-Lifecycle-Vertrag fuer `seq_cp/seq_rm/seq_keep`-Aequivalente definieren (Target: Q2 2026)
- [ ] Metrik-Schema fuer `accept_rate`, `drafted_tokens`, `cache_miss`, `batch_retry_count` festlegen (Target: Q2 2026)
- [ ] Feature-Flags fuer `lookup_decoding` und `adaptive_batch_retry` spezifizieren (Target: Q2 2026)

### Phase 2: Core-Implementierung
- [ ] Continuous-Batching Scheduler mit Chunking + adaptive Halbierung bei Decode-Fehlern implementieren (Target: Q2 2026)
- [ ] Prefix-Sharing fuer gemeinsame Systemprompts in KV integrieren (Target: Q2 2026)
- [ ] Lookup-Decoding (n-Gram cache context/static/dynamic) als optionalen Pfad implementieren (Target: Q2 2026)
- [ ] Persistenzpfad fuer dynamischen Lookup-Cache implementieren (Target: Q2 2026)

### Phase 3: Fehlerbehandlung & Edge Cases
- [ ] Guards fuer `n_ctx`/KV-Budget vor Decode einbauen (Target: Q2 2026)
- [ ] Saubere Recovery bei partiell fehlgeschlagenen Batches sicherstellen (Target: Q2 2026)
- [ ] Determinismus-Guards fuer feste Seeds und Replay-Runs absichern (Target: Q2 2026)
- [ ] Tenant-Isolation fuer Lookup-Caches spezifizieren/umsetzen (Target: Q3 2026)

### Phase 4: Tests
- [ ] Unit-Tests fuer Scheduler-Chunking, Retry-Downshift, KV-Cleanup (Target: Q2 2026)
- [ ] Integrationstests fuer 1/4/8 parallele Requests und gemischte Promptlaengen (Target: Q2 2026)
- [ ] Replay-Tests fuer deterministische Outputs unter festen Seeds (Target: Q2 2026)
- [ ] Lasttest 24h fuer Speicherstabilitaet/Lecks/KV-Konsistenz (Target: Q3 2026)

### Phase 5: Performance/Hardening
- [ ] Auto-Tuning-Policy fuer `n_batch`, `n_parallel`, `n_draft` auf Basis Live-Metriken implementieren (Target: Q3 2026)
- [ ] Benchmark-Suite Baseline vs. Lookup spezifizieren und in CI integrieren (Target: Q3 2026)
- [ ] Rollout-Gates fuer P95/P99 + Throughput + Error-Budget definieren (Target: Q3 2026)

### Phase 6: Dokumentation & Abnahme
- [ ] Betriebsdoku fuer Feature-Flags, Sizing, Tuning, Failure-Modes erstellen (Target: Q3 2026)
- [ ] Dashboard-Runbook fuer Diagnose (Cache-Miss, Retry, Acceptance) erstellen (Target: Q3 2026)
- [ ] Abnahmebericht inkl. KPI-Vergleich gegen Baseline publizieren (Target: Q3 2026)

## Acceptance Criteria
- Durchsatz: >= 20% Token/s Steigerung bei 8 parallelen Requests gegenueber Baseline.
- Latenz: >= 15% Verbesserung bei P95 fuer gemischte Request-Profile.
- Stabilitaet: Keine KV-Corruption und kein Memory-Leak im 24h-Langlauftest.
- Qualitaet: Keine Regression in deterministischen Replay-Tests (feste Seeds).
- Observability: Alle neuen Metriken in Prometheus + Grafana sichtbar und alarmierbar.

## Proposed Initial Parameters
- `n_parallel`: 4-8
- `n_batch`: konservativ starten, dann adaptiv skalieren
- `n_draft`: 8-16
- `ngram_min=2`, `ngram_max=4`

## Risks
- Hoehere Komplexitaet im KV-Lifecycle kann zu schwer reproduzierbaren Fehlern fuehren.
- Lookup-Decoding kann bei ungeeigneten Workloads niedrige Acceptance liefern.
- Aggressive Batch-Groessen koennen Latenzspitzen und Retry-Kaskaden erzeugen.

## Mitigations
- Feature-Flags mit schrittweisem Rollout.
- Per-tenant Canary und KPI-Schwellen als Hard-Gate.
- Strikte Telemetrie und Replay-Faelle fuer Regressionserkennung.

## Deliverables
- Produktionsreife Scheduler- und KV-Anpassungen
- Lookup-Decoding Integration (flag-gated)
- Test- und Benchmark-Abdeckung
- Betriebsdokumentation + Dashboarding
