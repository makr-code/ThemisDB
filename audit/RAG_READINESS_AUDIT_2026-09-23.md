# RAG-Readiness Audit ThemisDB (Reality-Check)

**Author:** ThemisDB Contributors  
**Created:** 2026-09-23  
**Last Updated:** 2026-09-23  
**Status:** review

## 1) Scope & Methodik

Dieser Audit bewertet die aktuelle Eignung von ThemisDB fuer LLM-RAG-Workloads auf Basis von Repository-Evidenz (Code, Modul-Dokumentation, Test-Registrierung, CI-Workflows) statt rein planerischer Aussagen.

Verpflichtend gepruefter Kontext:
- `AI_WIKI_INTEGRATION_PLAYBOOK.md`
- `ai_context/developer_llm_wiki/INDEX.md`
- `ai_context/developer_llm_wiki/MODULES_AND_APIS.md`
- `ai_context/developer_llm_wiki/BUILD_TEST_CI_AND_OPERATIONS.md`
- `ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md`
- `ai_context/developer_llm_wiki/WIKI_STATUS.json`
- `BRANCHING_STRATEGY.md` (Default-Zielbranch: `develop`)

Untersuchte RAG-kritische Module:
- `src/rag`, `src/retrieval`, `src/index`, `src/llm`, `src/ingestion`, `src/query`, `src/observability`, `src/security`

---

## 2) Ist-Zustand (evidenzbasiert)

### 2.1 Positive technische Substanz

1. **Hybrid-Fusion mit RRF ist produktiv implementiert** in `src/rag/hybrid_retriever.cpp` inkl. gewichteter RRF-Fusion (`fuseRRF`) und Konfigurationsvalidierung (`validateConfig`) (`src/rag/hybrid_retriever.cpp:104-117`, `150-164`, `213-254`).
2. **WikiIndexStore vereint BM25, HNSW, Embeddings und Persistenz-Caches** (`src/llm/wiki_index_store.cpp:6-13`, `116-123`, `125-156`, `162-182`, `195-215`).
3. **FTS-Executor mit Positions-/Token-Logik** fuer Phrase/Term-Prozesse ist implementiert (`src/query/fts_executor.cpp:55-103`, `193-237`).
4. **Observability-Basis vorhanden** (OTLP/Jaeger/Zipkin-Auswahl, Span-Lifecycle, Ringbuffer-Retention) (`src/observability/opentelemetry_tracer.cpp:50-69`, `86-121`, `171-210`).
5. **RAG-fokussierte Testregistrierung inkl. `release_critical`-Labels** existiert (`tests/rag/CMakeLists.txt:64-70`, `202-209`, `241-248`).
6. **Dedizierter LLM+RAG-CI-Workflow** ist vorhanden (`.github/workflows/build-llm-inference.yml:1-16`, `217-219`).

### 2.2 Dokumentations- und Reife-Inkonsistenzen

1. `src/retrieval/README.md` beschreibt das Modul als Scaffold/Skeleton (`src/retrieval/README.md:7-11`, `28-31`), waehrend andere Stellen fortgeschrittene Rollout-Reife behaupten (`src/retrieval/ROADMAP.md:95-149`).
2. In `src/retrieval/src/README.md` bleibt die Aussage "skeleton translation units" bestehen (`src/retrieval/src/README.md:7-9`), waehrend aktuell lediglich `lora_package.cc` als Code vorliegt (`src/retrieval/src/README.md:19`; Dateisystemstand `src/retrieval/src/lora_package.cc`).
3. Mehrere zentrale `FUTURE_ENHANCEMENTS.md` tragen alte Validierungsstaende (z. B. Query/Security), obwohl Roadmaps deutlich weiter sind (`src/query/FUTURE_ENHANCEMENTS.md:3`, `src/security/FUTURE_ENHANCEMENTS.md:3`).
4. Quarantaene in Test-CMake zeigt, dass Teile nicht im Standard-Fokuslauf sind:
   - LLM (`tests/llm/CMakeLists.txt:66-70`)
   - Index (`tests/index/CMakeLists.txt:13-20`)
   - Ingestion (`tests/ingestion/CMakeLists.txt:12-16`).

### 2.3 Wiki-/Governance-Zustand

1. `WIKI_STATUS.json` ist frisch (generated_at 2026-09-21, `source_count=7810`), aber Delta zeigt baseline-artiges Muster (`added_count=7810`, `changed_count=0`) statt klarer Drift-Signale (`ai_context/developer_llm_wiki/WIKI_STATUS.json:4-11`).
2. In der generierten Governance-Wiki-Seite steht fuer den Eintrag zu sich selbst ein aelteres Datum (`2026-09-14`) trotz Header-Datum `2026-09-21` (`ai_context/developer_llm_wiki/GOVERNANCE_AND_ROADMAP.md:1-4`, `384-390`).
3. `maintenance-docs.yml` zeigt, dass LLM-Wiki-Writeback standardmaessig **nicht** automatisch erfolgt (`llm_wiki_apply_updates=false`) und nur per `workflow_dispatch` aktiviert wird (`.github/workflows/maintenance-docs.yml:87-93`, `185-190`).

---

## 3) Reality-Check (Produktionsreife fuer LLM-RAG)

### Staerken
- Substanzielle Kernimplementierungen fuer Retrieval-Fusion, FTS, Embedding-Cache und RAG-Tests sind vorhanden.
- CI-Gates fuer LLM/RAG sind explizit und reproduzierbar beschrieben.
- Sicherheits- und Observability-Pfade sind nicht nur konzeptionell, sondern im Code sichtbar.

### Luecken
- Konsistenzproblem zwischen Modulrealitaet und Modul-Dokumentation (insb. `src/retrieval`).
- Teilweise Quarantaene zentraler Tests reduziert Vertrauen in "all-green" Aussagen.
- Fehlende durchgaengige, versionsgebundene Embedding-/Index-Lifecycle-Governance fuer echte produktive RAG-Datenzyklen.

### Risiken
- Ueberschaetzung der Reife durch roadmap-lastige Aussagen ohne durchgehend aktivierte Gates.
- Retrieval-Qualitaet kann bei Modell-/Embedding-Drift ohne strikte Versionierungs- und Reindex-Politik regressieren.
- Multi-Tenant-/Security-Grenzen im Retrieval-Pfad sind nicht als durchgaengiger End-to-End-Vertrag dokumentiert.

### Sofortiges Feasibility-Verdikt
**Verdikt:** *Bedingt produktionsfaehig fuer kontrollierte RAG-Workloads, aber noch nicht robust genug fuer breite, hochdynamische Enterprise-RAG-Szenarien ohne zusaetzliche Governance- und Qualitaetsgates.*

---

## 4) Gap-Analyse gegen Stand der Technik / Wissenschaft

Abgleich gegen heutige RAG-Best-Practices (hybrid retrieval, re-ranking, eval loops, observability, security, lifecycle governance):

1. **Retrieval Quality Loop:** Hybrid retrieval vorhanden, aber verbindliche, dauerhaft aktive Qualitaets-Gates (nDCG/Recall/MRR/Faithfulness) je Daten-/Modellversion fehlen als einheitlicher Betriebsvertrag.
2. **Indexing Lifecycle:** Persistente Caches vorhanden, aber ein strikter Embedding-Version-/Reindex-Migrationspfad mit SLA und Rollback ist nicht konsistent ueber Module dokumentiert.
3. **Hybrid + Re-Ranking Orchestration:** Komponenten sind da, aber adaptive Query-Intent-Steuerung (lexical vs dense vs graph) und hard gates fuer reranking-cost/latency fehlen als systematischer Standard.
4. **Evaluation & Regression:** Viele Tests vorhanden; jedoch deuten Quarantaenen auf Luecken im regulaeren Gate-Betrieb.
5. **Observability:** Gute Basis, aber RAG-spezifische End-to-End-SLOs (Recall+Latency+Cost gekoppelt) sind nicht als zentrales SLO-Paket operationalisiert.
6. **Security & Multi-Tenancy:** Gute Einzelbausteine (HSM/TSA/Policy), aber RAG-spezifische Datenflusskontrollen (tenant isolation vor retrieval/rerank/context assembly) brauchen klarere DoD-Ketten.

---

## 5) Konkrete Verbesserungsvorschlaege (priorisiert)

### Kurzfristig (Q4 2026) — hoher Impact, moderater Aufwand

1. **RAG Eval Contract v1 (Impact: Hoch / Effort: Mittel)**
   - Einheitlicher Eval-Vertrag ueber `rag+llm+query+observability` mit festen Datensaetzen pro Domaintyp.
   - Pflichtmetriken je Build: `Recall@10`, `nDCG@10`, `MRR@10`, `Faithfulness`, `p95 latency`, `cost/query`.
2. **Doku-Realitaetsabgleich Retrieval (Impact: Hoch / Effort: Niedrig)**
   - `src/retrieval` klarstellen: produktiver Scope vs. geplante EPIC-Surfaces; keine widerspruechlichen Reifeclaims.
3. **Embedding/Index Version Governance (Impact: Hoch / Effort: Mittel)**
   - Versionierter Vertrag: `embedding_model_id`, `embedding_dim`, `chunking_profile`, `index_schema_version`, `reindex_required`.
4. **RAG Security Guardrail Pack (Impact: Hoch / Effort: Mittel)**
   - Tenant-isolierte Retrieval-Pfade + deny-by-default bei fehlendem Policy-Kontext.

### Mittelfristig (Q1-Q2 2027) — hoher Impact, hoeherer Aufwand

5. **Adaptive Hybrid Routing + Learned Policy (Impact: Hoch / Effort: Hoch)**
   - Query-intent-basierte Gewichtung lexical/dense/graph mit Telemetrie-Feedback.
6. **Cross-Encoder Re-Ranking Gate (Impact: Mittel-Hoch / Effort: Mittel-Hoch)**
   - Re-ranking nur bei Nutzen > Kosten; budget-aware activation.
7. **Continuous Index Freshness SLA (Impact: Mittel-Hoch / Effort: Mittel)**
   - CDC/ingestion-getriebene Freshness-Ziele inkl. maximaler Staleness fuer RAG.

### Langfristig (ab Q2 2027) — strategisch

8. **Research-backed Eval Harness (Impact: Hoch / Effort: Hoch)**
   - Mehrsprachige, domain-spezifische und adversarial Sets; robuste Drift-/Bias-Reports.
9. **Cost-Latency-Quality Optimizer (Impact: Hoch / Effort: Hoch)**
   - Multi-Objective Steuerung pro Tenant/SLO.

---

## 6) KPI / Abnahmekriterien

| Vorschlag | KPI / Akzeptanzkriterium | Ziel |
|---|---|---|
| RAG Eval Contract v1 | Pflichtmetriken pro release_critical Lauf vorhanden | 100% Runs mit vollständigem Metrik-Satz |
| RAG Eval Contract v1 | Recall@10 Delta ggü. Baseline | kein Regression > 2 Prozentpunkte |
| Embedding/Index Version Governance | Reindex-Entscheidung deterministisch/versioniert | 100% index writes mit Version-Metadaten |
| Security Guardrail Pack | Unauthorized retrieval leakage | 0 bestätigte Cross-Tenant-Leaks in Gate-Tests |
| Adaptive Hybrid Routing | Qualität vs. statische Fusion | +5% nDCG@10 bei <=10% Mehrlatenz |
| Re-ranking Gate | Wirtschaftlichkeit | mindestens 80% rerank-Aufrufe mit messbarem Qualitätsgewinn |
| Freshness SLA | Ingestion-to-index delay | p95 < 5 min (konfigurierbar je Tier) |
| Observability SLO Pack | End-to-end RAG SLO compliance | >= 99% innerhalb TTFT/latency/cost budget |

---

## 7) Risiko- und Migrationshinweise

1. **Migrationsrisiko Embedding-Versionierung:** Aenderung von Chunking/Embedding kann Recall kurzfristig senken. Mit Canary-Reindex + dual-read absichern.
2. **Kostenrisiko Re-ranking/Adaptive Routing:** Nur budget-aware aktivieren; harte Cost Ceilings pro Tenant erzwingen.
3. **Betriebsrisiko durch Test-Quarantaenen:** Quarantaene-Faelle in regulierte, zeitlich begrenzte Exit-Kriterien ueberfuehren.
4. **Dokumentationsrisiko:** Widerspruechliche Modul-Reifestaende koennen Fehlentscheidungen in Rollouts erzeugen; daher Pflicht-Sync mit Roadmap/Future Enhancements.

---

## 8) Mapping zu aktualisierten `src/*/FUTURE_ENHANCEMENTS.md`

- `src/rag/FUTURE_ENHANCEMENTS.md` — RAG Eval Contract, Retrieval-Policy, Citation/Provenance Gates
- `src/retrieval/FUTURE_ENHANCEMENTS.md` — Scope-Konsolidierung, verbindlicher Delivery-Pfad fuer reale Retrieval-Surfaces
- `src/index/FUTURE_ENHANCEMENTS.md` — Hardware-basierte Parity/Latency Gates, Embedding-Version-kompatible Index-Lifecycle-Regeln
- `src/llm/FUTURE_ENHANCEMENTS.md` — Embedding/Cache-Versionvertrag, Reranker-Orchestrierung
- `src/ingestion/FUTURE_ENHANCEMENTS.md` — Chunking-/Embedding-Lifecycle und Freshness-SLA
- `src/query/FUTURE_ENHANCEMENTS.md` — Hybrid-Planung und budget-aware Reranking Trigger
- `src/observability/FUTURE_ENHANCEMENTS.md` — RAG E2E-SLO Pack und Gate-Telemetrie
- `src/security/FUTURE_ENHANCEMENTS.md` — Tenant-Isolation + Retrieval-Policy-Enforcement

---

## 9) Offene Wiki-Luecken / Dokumentationskonflikte

1. **Developer-Wiki-Synthese ist teils nur Katalog statt semantischer Zusammenfassung** (`MODULES_AND_APIS.md`, `BUILD_TEST_CI_AND_OPERATIONS.md` enthalten ueberwiegend Dateilisten mit "binary or unreadable").
2. **Datumsinkonsistenz im generierten Governance-Artefakt** (`GOVERNANCE_AND_ROADMAP.md:1-4` vs `384-390`).
3. **`src/retrieval` Doku-Konflikt** zwischen "Skeleton"-Status und fortgeschrittenen Roadmap-Claims (`src/retrieval/README.md:7-11`, `src/retrieval/src/README.md:7-9`, `src/retrieval/ROADMAP.md:95-149`).
4. **Scanner-Artefakt-Drift**: `src/retrieval/MODULE_GAPS.md` meldet fehlende `PRODUCTION_REQUIREMENTS.md`, obwohl Datei existiert (`src/retrieval/MODULE_GAPS.md:42-46`, `src/retrieval/PRODUCTION_REQUIREMENTS.md:1-16`).

