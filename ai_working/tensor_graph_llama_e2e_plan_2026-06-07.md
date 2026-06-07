# Tensor-Graph -> llama.cpp End-to-End Plan (ohne Build)

Status: Proposal
Datum: 2026-06-07
Ziel: Durchgaengige Verdrahtung von Retrieval + Re-Retrieval (FLARE/TARG) ueber Tensor-Graph-Adapterauswahl bis zur aktiven llama.cpp Adapter-Umschaltung.

## 0. Umsetzungsstand (Update 2026-06-07)

- PR-1: umgesetzt
  - Optionaler AdapterCandidateProvider in AIOrchestrator
  - RAG-Pfad produziert Kandidaten/Selektion als Metadaten
  - Selektierter Adapter wird in InferenceRequest propagiert

- PR-2: umgesetzt
  - Optionaler AdapterApplyService
  - AdapterSwitchPolicy mit Cooldown, Debounce, max_switches_per_request, min_similarity_gain
  - Apply-Outcome inkl. Blockgruenden im RAG-Metadatenpfad

- PR-3: umgesetzt
  - Observability-Counter im Orchestrator-Stats-Output
  - Kontrollierter Rollback-Pfad via `force_adapter_rollback` (Test-/Control-Flag)
  - Zusetzliche Tests fuer Apply-Guardrails, Rollback und Counter
  - Default-Produktionsverdrahtung: Orchestrator bindet bei `setLLMPlugin(...)`
    automatisch einen ILLMPlugin-basierten AdapterApplyService, wenn keiner
    explizit gesetzt wurde
  - Konfigurierbarer Adapter-Path-Resolver (`setAdapterPathResolver(...)`) ist
    umgesetzt und wird in die Default-Produktionsverdrahtung uebernommen
  - Apply-Fehlerbeobachtbarkeit erweitert: `metadata.extra.adapter_apply_error_code`
    wird fuer Erfolg/Block/Fail konsistent gesetzt (u. a. `none`, `cooldown`,
    `same_adapter`, `resolver_empty_path`, `unload_failed`, `load_failed`)
  - Operability-Erweiterung: `metadata.extra.adapter_apply_error_class`
    klassifiziert Fehler als `none`, `retryable` oder `non_retryable`
  - Retry-Hardening: adapter apply kann bei `retryable` Fehlern automatisch
    erneut versucht werden (Policy: `max_retry_attempts`, `retry_backoff_ms`),
    inklusive Metadaten `adapter_apply_attempts` und `adapter_apply_retried`
  - Globale Retry-Observability: `stats()` enthaelt nun
    `rag_adapter_retry_total` und `rag_adapter_retry_success_total`
  - Exhaustion-Observability: `stats()` enthaelt
    `rag_adapter_retry_exhausted_total` sowie Request-Metadatum
    `adapter_apply_retry_exhausted`
  - Cost-Model-Integration: optionaler `IRagCostModelService` am Orchestrator,
    Default-Bridge auf `themis::DistributedQueryCostModel`, Ausgabe pro Run in
    `metadata.extra.rag_cost_estimate` (retrieval/inference/adapter/total)
  - Cost-Governance: kostenbasiertes Pre-Apply-Budget-Gate
    (`enable_cost_budget_gate`, `max_total_cost`) blockiert Adapter-Switches
    bei budgetueberschreitender Projektion (`cost_budget_exceeded`)
  - Retrieval-Cost-Steuerung: optionales Top-K-Downscaling vor Retrieval
    (`enable_cost_top_k_adaptation`, `min_top_k_under_budget`) reduziert
    effektives `top_k` budgetbasiert und schreibt
    `rag_cost_top_k_original`/`rag_cost_top_k_effective`
  - Tenant-Budget-Profile: `ctx.extra.tenant_budget_override` (oder
    `ctx.extra.tenant_budgets[tenant]`) ueberschreibt Policy-Budget; effektive
    Werte werden als `rag_cost_budget_limit_effective` und
    `rag_cost_budget_limit_source` ausgegeben
  - Prioritaetsregel festgelegt: `tenant_budget_override` hat Vorrang vor
    `tenant_budgets[tenant]`; Quelle wird explizit als
    `tenant_budget_override` bzw. `tenant_budgets.<tenant>` markiert
  - Konfig-Hardening: ungueltige Budget-Overrides (falscher Typ / <= 0)
    werden ignoriert und als
    `rag_cost_budget_override_invalid` +
    `rag_cost_budget_override_invalid_code` +
    `rag_cost_budget_override_invalid_detail` markiert
  - Einheitliche Block-Codes: Guardrail-Blockierungen schreiben nun
    `adapter_apply_block_code` (z. B. `cost_budget_exceeded`, `cooldown`,
    `same_adapter`) zusaetzlich zu `adapter_apply_block_reason`
  - API-Consumer-Sicht: `adapter_apply_block_code` wird in `raw_response`
    gespiegelt, damit Alerting ohne Metadata-Parsing moeglich ist
  - Erweiterte API-Consumer-Sicht: auch `adapter_apply_error_code` und
    `adapter_apply_error_class` werden in `raw_response` gespiegelt
  - Vollstaendige Budget-Observability fuer API-Consumer: auch
    `rag_cost_budget_override_invalid`,
    `rag_cost_budget_override_invalid_code` und
    `rag_cost_budget_override_invalid_detail` werden in `raw_response`
    gespiegelt
  - Downstream-Parsing vereinfacht: `raw_response.decision_summary` fasst
    die wichtigsten Adapter-/Budget-Entscheidungsfelder kompakt zusammen
  - Budget-Phasentransparenz: `decision_summary.cost_gate_phase` zeigt, ob
    Budget-Gating in `pre_retrieval_top_k` oder `pre_apply_switch` gegriffen hat
  - Multi-Trigger-Transparenz: `decision_summary.cost_gate_trigger_count`
    zaehlt ausgeloeste Budget-Gates pro Lauf; `cost_gate_phase` kann bei
    kombinierten Triggern auf `multi` stehen
  - Globale Cost-Gate-Phasenstatistik in `stats()`:
    `rag_cost_gate_pre_retrieval_total`,
    `rag_cost_gate_pre_apply_total`,
    `rag_cost_gate_multi_total`

## 1. Scope und Ergebnis

Dieses Dokument plant 3 PR-faehige Arbeitspakete:

1. Runtime-Orchestrierung: Re-Retrieval Trigger -> Adapter-Kandidatensuche
2. Adapter-Aktivierung: Kandidat -> GGML/llama.cpp Apply (Hot-Swap)
3. Hardening: Observability, Guardrails, Rollback, Lasttests

Nicht-Ziel in diesem Plan:

- Kein Full-Reload des Basismodells im laufenden Token-Loop
- Keine neue Legacy-Kompatibilitaetsschicht
- Kein Build/Test-Run im Rahmen dieses Planungsschritts

## 2. Ist-Luecke (kurz)

Der Code enthaelt bereits:

- FLARE/TARG Trigger in TensorRAGPipeline
- Tensor-Graph Similarity in Tensor/Training
- llama.cpp LoRA-Mechanik

Es fehlt die durchgehende Runtime-Kette:

- Trigger-Entscheidung aus Generation -> Similar-Adapter Query -> deterministische Auswahl -> map/apply in laufender Inferenz -> Regeneration des unsicheren Spans.

## 3. Zielarchitektur

1. Token-Schritt liefert Entscheidung (FLARE/TARG)
2. Bei should_retrieve=true:
   - Query-Embedding erstellen
   - Dokumente re-retrieven
   - Adapterkandidaten ueber Tensor-Graph bestimmen
3. AdapterPolicy waehlt einen Kandidaten (Tenant, Domain, Confidence, Cooldown)
4. GGML-Bridge mapped Adapter, llama.cpp setzt Scale/Apply
5. Unsicheres Segment wird mit aktualisiertem Kontext regeneriert
6. Pipeline meldet RetrievalDone, Token-Loop laeuft weiter

## 4. Arbeitspaket A (PR-1): Re-Retrieval zu Adapterkandidaten

### 4.1 Aenderungsumfang (minimal)

- Erweiterung der RAG-Orchestrierung um optionalen AdapterCandidateStep
- Verbindung von TensorRAGPipeline Entscheidung mit Adapter-Kandidatensuche
- Kein Apply in dieser PR, nur Kandidatenbestimmung + Telemetrie

### 4.2 Vorgesehene API-Erweiterungen

- Neue Struktur AdapterCandidate
  - adapter_id
  - similarity
  - source_layer
  - tenant

- Neue Struktur AdapterSelectionInput
  - session_id
  - tenant
  - query_embedding
  - top_k
  - domain_hint

- Neue Struktur AdapterSelectionResult
  - selected_adapter_id (optional)
  - candidates
  - reason

- Neue orchestrator-nahe Schnittstelle IAdapterCandidateProvider
  - selectCandidates(const AdapterSelectionInput&) -> AdapterSelectionResult

### 4.3 Akzeptanzkriterien

- Bei should_retrieve=true wird AdapterSelectionResult produziert
- Bei should_retrieve=false kein Kandidatenlauf
- Metadaten enthalten Auswahlgrund und Kandidatenanzahl
- Fallback: Fehler in Kandidatensuche blockiert Text-Retrieval nicht

## 5. Arbeitspaket B (PR-2): Kandidat zu llama.cpp Apply

### 5.1 Aenderungsumfang (minimal)

- Einfuehrung eines klaren AdapterApplyService mit atomarem Wechsel
- Verdrahtung zu bestehender GGML/LoRA Infrastruktur
- Cooldown + Debounce gegen thrashing

### 5.2 Vorgesehene API-Erweiterungen

- Neues Interface IAdapterApplyService
  - applyAdapter(adapter_id, tenant, scale) -> bool
  - currentAdapter() -> string
  - canSwitch(now_ts) -> bool

- Neue Runtime-Policy AdapterSwitchPolicy
  - min_switch_interval_ms
  - min_similarity_gain
  - max_switches_per_request

### 5.3 Akzeptanzkriterien

- Bei gueltigem Kandidat wird genau ein Apply ausgefuehrt
- Bei nicht mapbarem Kandidat wird sauber geloggt und ohne Crash fortgesetzt
- Kein doppeltes Apply desselben Adapters im Cooldown-Fenster
- Erfolg/Fehlschlag pro Apply als Metrik sichtbar

## 6. Arbeitspaket C (PR-3): Hardening und Betrieb

### 6.1 Aenderungsumfang (minimal)

- End-to-End Telemetrie und Fehlerpfade
- Rollback-Pfad bei schlechterem Verlauf nach Switch
- Produktionsnahe Lastszenarien als Testspezifikation

### 6.2 Metriken (Pflicht)

- rag_retrieval_trigger_total
- rag_reretrieval_total
- rag_adapter_candidates_total
- rag_adapter_switch_total
- rag_adapter_switch_fail_total
- rag_adapter_switch_latency_ms
- rag_adapter_switch_rollback_total

Status: implementiert im aktuellen Orchestrator-Stats-Output.

### 6.3 Akzeptanzkriterien

- Jeder Switch erzeugt Trace/Metric-Eintrag
- Rollback wird nur unter klarer Bedingung ausgeloeist (z. B. Antwortqualitaet sinkt)
- Keine ungebundene Switch-Kaskade in langen Generationen

## 7. Geplante Dateiberuehrung pro PR (minimal)

PR-1:

- src/rag/tensor_rag_pipeline.cpp
- include/rag/tensor_rag_pipeline.h
- src/llm/ai_orchestrator.cpp
- include/llm/ai_orchestrator.h

PR-2:

- src/training/adalora_tt_bridge.cpp
- include/training/adalora_tt_bridge.h
- src/storage/ggml_tensor_bridge.cpp
- include/storage/ggml_tensor_bridge.h
- src/llm/llama_wrapper.cpp

PR-3:

- src/llm/ai_orchestrator.cpp
- src/llm/llama_wrapper.cpp
- src/rag/tensor_rag_pipeline.cpp
- relevante Testdateien in tests/tensor, tests/rag, tests/llm

## 8. Risiken und Gegenmassnahmen

1. Runtime thrashing durch haeufige Adapterwechsel
   - Gegenmassnahme: Cooldown + min_similarity_gain + max_switches_per_request

2. Latenzanstieg im Token-Loop
   - Gegenmassnahme: Kandidatensuche asynchron vorbereiten, harte Timeouts

3. Tenant-Leakage bei Adapterauswahl
   - Gegenmassnahme: Tenant als Pflichtfilter in jeder Kandidatenselektion

4. Fehlerhafte Mappings
   - Gegenmassnahme: fail-closed fuer Adapterwechsel, fail-open fuer Text-RAG

## 9. Definition of Done (gesamt)

1. Re-Retrieval triggert reproduzierbar Kandidatensuche und optionalen Adapterwechsel.
2. Adapterwechsel ist observierbar, begrenzt und rollback-faehig.
3. Die Pipeline bleibt ohne Hard-Failure funktionsfaehig, auch wenn Adapterpfad fehlschlaegt.
4. Dokumentation in Tensor/RAG/Training Architektur und Roadmap ist synchron.

## 10. Offene Anschlussarbeiten

1. Echte Qualitaets-getriebene Rollback-Bedingung (statt Testflag)
  - Aktuell: kontrollierter Rollback ueber `ctx.extra.force_adapter_rollback`
  - Ziel: triggern auf messbarer Qualitaetsdegradation (z. B. Judge/Confidence)

2. Direkte Verdrahtung zum produktiven llama.cpp Hot-Swap-Backend
  - Aktuell: Default-Binding gegen ILLMPlugin `loadLoRA/unloadLoRA` ist implementiert
   - Erweitert: optionaler repository-/tenant-spezifischer Path-Resolver ist
     am Orchestrator verfügbar und im Default-Binding aktiv
   - Restziel: engeres GGML-Bridge Binding fuer deploymentspezifische
     Adapterquellen

3. End-to-End Last- und Stabilitaetstests im Build-Workflow
  - Aktuell: statische Diagnostik + Unit-Tests im Code ergänzt
  - Ziel: reproduzierbare E2E-Laeufe in CI mit p95/p99 Auswertung
