# SSM / Hybrid-Transformer / Infini-attention / Agentic Memory — Rollout-Plan

<!-- Status: proposed | branch: copilot/themisdb-ssm-analysis -->
<!-- Links: ssm-hybrid-analysis.md · ../../ROADMAP.md · ../../FUTURE_ENHANCEMENTS.md -->

**Dokument-Typ:** Phasenweiser Umsetzungsplan (technisch, nicht Marketingtext)
**Scope:** ThemisDB LLM/Inference/Context/Memory-Stack
**Stand:** 2026-07-22
**Ziel-Branch:** `develop`
**Voraussetzung:** `ssm-hybrid-analysis.md` gelesen und Gap-Inventar bestätigt

> **Assumptions-Marker:** Aussagen ohne Repository-Fundament sind mit `[ASSUMPTION]` markiert.
> **Proposed-Marker:** Neue Dateipfade/Interfaces sind mit `[PROPOSED]` markiert.

---

## Inhaltsverzeichnis

1. [Rollout-Übersicht](#1-rollout-übersicht)
2. [Phase 0: Voraussetzungen und Hygiene](#2-phase-0-voraussetzungen-und-hygiene)
3. [Phase 1: POC — Plugin-Infrastruktur und State-Skeleton](#3-phase-1-poc--plugin-infrastruktur-und-state-skeleton)
4. [Phase 2: Beta — Infini-attention und NVFP4 KV-Quantisierung](#4-phase-2-beta--infini-attention-und-nvfp4-kv-quantisierung)
5. [Phase 3: Beta+ — Hybrid-Routing und Agentic Memory Layer](#5-phase-3-beta--hybrid-routing-und-agentic-memory-layer)
6. [Phase 4: GA-Hardening](#6-phase-4-ga-hardening)
7. [Rollback-Strategie](#7-rollback-strategie)
8. [Abhängigkeiten und Sequenzierung](#8-abhängigkeiten-und-sequenzierung)
9. [ROADMAP.md Einträge (proposed)](#9-roadmapmd-einträge-proposed)

---

## 1. Rollout-Übersicht

```
Phase 0  (2026-Q3)  Voraussetzungen: FA3-Verifikation, NVFP4-Capability-Check
Phase 1  (2026-Q3)  POC: SSM-Plugin-Infrastruktur, State-Skeleton, Drift-Metriken
Phase 2  (2026-Q4)  Beta: Infini-attention (CPU→CUDA), NVFP4 KV-Quant, L2-Episodic
Phase 3  (2027-Q1)  Beta+: Hybrid-Router, vollständiger Agentic Memory Layer
Phase 4  (2027-Q2)  GA-Hardening: Benchmarks, Chaos-Tests, Security-Review, Docs
```

### Aufwand/Nutzen-Priorisierung

| Aufgabe | Aufwand | Nutzen | Priorität |
|---|---|---|---|
| NVFP4 KV-Cache-Quantisierung (`PagedKVCache::Config`) | Gering | ✅ Sofort: 2× Sequenzlänge bei gleichem VRAM | **P0 — sofort** |
| FA3 SM90 Kernel-Verifikation | Gering | ✅ 1.5–2× Durchsatz (wenn nicht bereits aktiv) | **P0 — sofort** |
| SSM-Plugin-Interface (`ILLMPlugin`-Erweiterung) | Mittel | Architektur-Grundlage für alle weiteren Phasen | **P1** |
| `SSMStateStore` Interface + Skeleton | Mittel | State-Persistenz-Fundament | **P1** |
| Infini-attention CPU-Fallback | Mittel | Datenfluss-Validierung ohne CUDA-Aufwand | **P1** |
| Drift-Metriken Prometheus | Gering | Observability-Basis | **P1** |
| Infini-attention CUDA-Kernel | Hoch | Production-VRAM-Effizienz | **P2** |
| L2 Episodic Memory Komprimierung | Mittel | Agentic Memory L1→L2 Rotation | **P2** |
| Hybrid-Context-Router | Mittel | Intelligente Architekturwahl per Request | **P3** |
| Agentic Memory L1+L2+L3 Integration | Hoch | Vollständiges Gehirn-Modell | **P3** |

---

## 2. Phase 0: Voraussetzungen und Hygiene

**Ziel:** Sicherstellen, dass bestehende Infrastruktur korrekt für die weiteren
Phasen aufgestellt ist. Keine neuen Features.

**Aufwand:** 1–2 Wochen
**Branch:** `develop`

### 2.1 Deliverables

- [ ] **P0-D01:** FA3 Kernel-Audit
  - Datei: `src/llm/attention/cuda/flash_attention_cuda.cu`
  - Prüfen ob SM90-Pfad TMA/WGMMA-Instruktionen nutzt (FA3) oder FA2-Kernels auf SM90 ausführt
  - Dokumentieren in `src/llm/attention/FLASH_ATTENTION_VERSION_EVIDENCE.md`
  - Akzeptanzkriterium: Klarheit über aktuellen FA-Versionsstand auf SM90

- [ ] **P0-D02:** NVFP4 Capability-Assessment
  - Datei: `include/llm/paged_kv_cache.h`, `include/llm/mixed_precision_inference.h`
  - Evaluieren ob llama.cpp-Backend KV-Cache-Quantisierung auf 4-Bit unterstützt
  - Evidenz: `src/llm/NVFP4_KV_CACHE_CAPABILITY_ASSESSMENT.md`
  - Dokumentieren als Entscheidungsgrundlage für P2-D01

- [ ] **P0-D03:** GGUF / llama.cpp SSM-Ökosystem-Status
  - Prüfen ob `llama.cpp` (via `src/llm/llama_wrapper.cpp`) GGUF-Mamba-Modelle unterstützt
  - Evidenz: `docs/architecture/ssm-gguf-mamba-status.md`
  - Dokumentieren: Modell-Liste, `llama_model_params`-Flags, bekannte Einschränkungen
  - Security/Governance prüfen: ThemisDB bleibt System-of-Record; RocksDB nur interne Persistenz
  - Security/Governance prüfen: Tenant-Isolation, Zugriffskontrolle und Auditierbarkeit für SSM-State
  - Akzeptanzkriterium: Entweder Modell verfügbar oder Stub-Plan bestätigt

- [ ] **P0-D04:** Baseline-Benchmark
  - Sequenzlängen: 512, 2048, 8192 Token; 10 parallele Sessions; Seed=42
  - Metriken: `latency_p99`, `vram_footprint`, `throughput_tokens_per_second`
  - Tool: Google Benchmark (kompatibel mit `benchmarks/bench_fixtures.h`)
  - Ergebnisse als Datei `benchmarks/ssm_baseline/baseline_phase0.json`

### 2.2 Akzeptanzgates Phase 0

| Gate | Bedingung |
|---|---|
| P0-GATE-01 | FA3-Audit-Dokument vorhanden mit FA-Version-Klarheit |
| P0-GATE-02 | GGUF-SSM-Status dokumentiert (verfügbar ODER Stub-Plan) |
| P0-GATE-02b | Mamba Security/Governance dokumentiert (ThemisDB SoR, RocksDB intern) |
| P0-GATE-03 | Baseline-Benchmark-JSON committed |
| P0-GATE-04 | `ctest -L release_critical` grün |

---

## 3. Phase 1: POC — Plugin-Infrastruktur und State-Skeleton

**Ziel:** Minimaler Proof-of-Concept. SSM-Backend als `ILLMPlugin` registrierbar.
`SSMStateStore`-Interface mit In-Memory-Implementierung. Infini-attention CPU-Fallback.
Drift-Metriken sichtbar. **Kein Produktions-Einsatz.**

**Aufwand:** 3–4 Wochen
**Branch:** `develop`
**Abhängigkeit:** Phase 0 abgeschlossen

### 3.1 Deliverables

- [ ] **P1-D01:** `ISSMPlugin` Interface [PROPOSED]
  - Datei: `include/llm/i_ssm_plugin.h` [PROPOSED]
  - Erbt von `ILLMPlugin` (`include/llm/llm_plugin_interface.h`)
  - Ergänzt: `updateState(token_batch)`, `getStateSnapshot()`, `restoreState(snapshot)`
  - Integrationspunkt: `LLMPluginManager::registerPlugin()` bleibt unverändert
  - **Human Design Review Gate (verbindlich vor Implementierung):**
    - Entscheidung `SSMStateStore` Verteilstrategie für L5: `replication` vs `shard_partitioned`
    - Failure-Semantik bei Cross-Shard-State-Loss (fail-closed/fail-open) festlegen
    - Eigentümer und Migrationspfad für spätere Persistenz (`RocksDB`/Shard-Storage) festhalten

  ```cpp
  // PROPOSED INTERFACE (not yet implemented)
  // Integration: extends ILLMPlugin from include/llm/llm_plugin_interface.h
  namespace themis::llm {
  struct ISSMPlugin : public ILLMPlugin {
      /// Update SSM hidden state with new token batch.
      virtual bool updateState(const std::vector<int32_t>& tokens) = 0;
      /// Return serialized hidden state for checkpointing.
      virtual std::vector<uint8_t> getStateSnapshot() const = 0;
      /// Restore hidden state from checkpoint.
      virtual bool restoreState(const std::vector<uint8_t>& snapshot) = 0;
      /// Reset state to uninitialized.
      virtual void resetState() = 0;
  };
  } // namespace themis::llm
  ```

- [ ] **P1-D02:** `SSMStateStore` Interface + In-Memory-Implementierung [PROPOSED]
  - Header: `include/llm/ssm_state_store.h` [PROPOSED]
  - Implementation: `src/llm/ssm_state_store.cpp` [PROPOSED]
  - Muster: analog zu `IKVStateSerializer` (`include/llm/kv_prefix_transfer_manager.h`)
  - HLC-Bindung: `SSMStateSnapshot.snapshot_ts` trägt `HLCTimestamp` aus `LLMQueryContext`
  - Enthält: `checkpoint()`, `resume()`, `invalidate()`, `compact()`

- [ ] **P1-D03:** Synthetischer SSM-Stub-Plugin [PROPOSED]
  - Datei: `src/llm/ssm_stub_plugin.cpp` [PROPOSED]
  - Implementiert `ISSMPlugin` mit festem Zufalls-Hidden-State (Seed=42)
  - Zweck: Datenfluss-Validierung ohne echtes Mamba-Modell

  ```cpp
  // STUB/SIMULATION NOTE:
  // Purpose: Validate SSM plugin dataflow and state serialization without a real Mamba model
  // Activation: Only when THEMIS_SSM_STUB_MODE=1 build flag is set
  // Production Delta: Uses fixed random state; no real sequence processing
  // Removal Plan: Replace with real Mamba ISSMPlugin implementation in Phase 2
  ```

- [ ] **P1-D04:** Infini-attention CPU-Fallback [PROPOSED]
  - Datei: `src/llm/attention/infini_attention_cpu.cpp` [PROPOSED]
  - Backend-Enum: `Backend::INFINI_COMPRESSIVE` zu `flash_attention.h` hinzufügen
  - Implementiert: assoziatives Gedächtnisschreiben/-lesen ohne GPU-Kernel
  - Aktivierung: `Backend::INFINI_COMPRESSIVE` mit `CPU`-Fallback wenn CUDA nicht verfügbar

- [ ] **P1-D05:** Drift-Metriken [PROPOSED]
  - Datei: `src/llm/grafana_metrics.cpp` (bestehend, erweitern)
  - Neue Metriken: `themis_factual_drift_score`, `themis_ssm_state_checkpoints_total`
  - `themis_hybrid_router_decision{path}` als Zähler für Routing-Entscheidungen

- [ ] **P1-D06:** `ContextQualityBudget` Erweiterung [PROPOSED]
  - Datei: `include/llm/context_window_budget.h` (bestehend, erweitern)
  - Ergänzt: `ContextQualityMetrics` Struct mit `state_retention_score`,
    `factual_drift_estimate`, `tokens_since_last_retrieval`

- [ ] **P1-D07:** Unit-Tests Phase 1 [PROPOSED]
  - Datei: `tests/llm/test_ssm_plugin_interface.cpp` [PROPOSED]
  - Tests: Plugin-Registrierung, State-Roundtrip, HLC-Bindung, Infini-CPU-Fallback
  - CTest-Target: `module_llm_test_ssm_plugin_interface_focused` (Muster: `tests/llm/CMakeLists.txt`)

- [ ] **P1-D08:** Mamba Governance Contract [PROPOSED]
  - Dokumentiert Security-/Governance-Grenzen für Mamba-State-Lifecycle
  - ThemisDB bleibt Datenautorität; RocksDB nur als interner Persistenzbaustein
  - Audit-Events + Tenant-Boundaries als verbindliche Integrationskriterien

### 3.2 Nicht im Scope Phase 1

- Echtes Mamba/SSM-Modell (Phase 2)
- CUDA-Kernel für Infini-attention (Phase 2)
- Hybrid-Routing (Phase 3)
- L2 Episodic Memory Komprimierung (Phase 2)

### 3.3 Akzeptanzgates Phase 1

| Gate | Bedingung | Nachweis |
|---|---|---|
| P1-GATE-01 | SSM-Stub via `LLMPluginManager` registrierbar | Unit-Test P1-D07 |
| P1-GATE-02 | `SSMStateSnapshot` Round-Trip verlustfrei | Unit-Test P1-D07 |
| P1-GATE-03 | HLC `snapshot_ts` in State-Snapshot enthalten | Integration-Test |
| P1-GATE-04 | Infini-CPU-Fallback: korrekte Matrix-Updates | Unit-Test P1-D07 |
| P1-GATE-05 | Drift-Metrik Prometheus-Export sichtbar | Smoke-Test |
| P1-GATE-06 | `latency_p99` ≤ +5 % vs. P0-Baseline | Benchmark P0-D04 |
| P1-GATE-07 | Mamba Governance Contract approved (Security + SoR/RocksDB constraints) | Review-Protokoll |
| P1-GATE-08 | `ctest -L release_critical` vollständig grün | CI-Gate |

---

## 4. Phase 2: Beta — Infini-attention und NVFP4 KV-Quantisierung

**Ziel:** Produktionsnahe Implementierung für Infini-attention (CUDA).
NVFP4 KV-Cache-Quantisierung für sofortige Sequenzlängen-Verdopplung.
L2 Episodic Memory Komprimierung als erster Agentic-Memory-Übergang.

**Aufwand:** 4–6 Wochen
**Branch:** `develop`
**Abhängigkeit:** Phase 1 Gates bestanden; P0-D02 NVFP4-Assessment positiv

### 4.1 Deliverables

- [ ] **P2-D01:** NVFP4 KV-Cache-Quantisierung
  - Datei: `include/llm/paged_kv_cache.h` (erweitern) + `src/llm/paged_kv_cache.cpp`
  - Ergänzt: `PagedKVCache::Config::kv_quantization` (`FP16`/`INT8`/`NVFP4`)
  - Datei: `include/llm/mixed_precision_inference.h` (erweitern)
  - Aktivierung: compile-time flag `THEMIS_ENABLE_KV_QUANT`
  - Tests: FP16 vs NVFP4 Genauigkeitsdelta ≤ 1 %

- [ ] **P2-D02:** Infini-attention CUDA-Kernel
  - Datei: `src/llm/attention/cuda/infini_attention_cuda.cu` [PROPOSED]
  - `Backend::INFINI_COMPRESSIVE` auf SM90/SM86 implementiert
  - Kompressive Gedächtnismatrix `M` in VRAM; assoziatives Schreiben/Lesen
  - Integration: `FlashAttentionFactory::create()` gibt `InfiniAttentionCUDA` zurück
  - Tests: Numerische Validierung gegen CPU-Fallback (P1-D04)

- [ ] **P2-D03:** L2 Episodic Memory — `AQLConversationContext` Komprimierung
  - Datei: `src/aql/aql_conversation_context.cpp` (erweitern)
  - Neuer `IHistoryCompressor` Hook [PROPOSED]: bei History-Overflow statt FIFO-Drop
  - Default-Implementierung: Extractive Summarization via LLM (nutzt `PromptManager`)
  - Speichert Episodic-Summary in `LLMInteractionStore` (`include/llm/llm_interaction_store.h`)
  - Aktivierung: `AQLConversationContext::Config::enable_episodic_compaction = false` (default)

- [ ] **P2-D04:** SSM-State RocksDB-Persistierung (optional, wenn P0-D03 positiv)
  - Datei: `src/llm/ssm_state_rocksdb_store.cpp` [PROPOSED]
  - Implementiert `ISSMStateStore` mit RocksDB-Backend
  - Nutzt `HLCTimestamp` als Schlüssel-Präfix für MVCC-kompatible Isolation

- [ ] **P2-D05:** `KnowledgeGapDetector` SSM-Drift-Signal
  - Datei: `include/rag/agentic_rag.h` (erweitern), `src/rag/agentic_rag.cpp`
  - Ergänzt: `KnowledgeGapConfig::ssm_drift_threshold` Parameter
  - Wenn `factual_drift_estimate > ssm_drift_threshold` → forciert RAG-Iteration

- [ ] **P2-D06:** Tests und Benchmarks Phase 2 [PROPOSED]
  - `tests/llm/test_nvfp4_kv_quantization.cpp` — Genauigkeitsdelta-Tests
  - `tests/llm/test_infini_attention.cpp` — CUDA-vs-CPU Numerik-Validierung
  - `tests/aql/test_episodic_compaction.cpp` — L2-Komprimierungs-Roundtrip
  - Benchmark: `benchmarks/ssm_baseline/phase2_long_context.json` [PROPOSED]

### 4.2 Akzeptanzgates Phase 2

| Gate | Bedingung | Nachweis |
|---|---|---|
| P2-GATE-01 | NVFP4 Genauigkeitsdelta ≤ 1 % vs FP16 | P2-D06 Test |
| P2-GATE-02 | Infini-CUDA numerisch konsistent mit CPU-Fallback (P1-D04) | P2-D06 Test |
| P2-GATE-03 | L2-Komprimierung erhält semantische Ähnlichkeit ≥ 0.85 | P2-D06 Test |
| P2-GATE-04 | VRAM-Footprint mit NVFP4 ≤ 55 % vs FP16 bei gleicher Sequenz | Benchmark |
| P2-GATE-05 | KnowledgeGapDetector triggert bei Drift > Threshold | Integration-Test |
| P2-GATE-06 | `ctest -L release_critical` vollständig grün | CI-Gate |

---

## 5. Phase 3: Beta+ — Hybrid-Routing und Agentic Memory Layer

**Ziel:** Vollständiges Agentic Memory Modell (L1+L2+L3+L4). Intelligenter
`HybridContextRouter` wählt Architekturpfad per Request. Reasoning-Density-Telemetrie.

**Aufwand:** 4–6 Wochen
**Branch:** `develop`
**Abhängigkeit:** Phase 2 Gates bestanden

### 5.1 Deliverables

- [ ] **P3-D01:** `HybridContextRouter` [PROPOSED]
  - Datei: `include/llm/hybrid_context_router.h` [PROPOSED]
  - Integrationspunkt: vor `LLMAQLHandler::executeInfer()` in `src/aql/llm_aql_handler.cpp`
  - Routing-Logik:
    ```
    if (session_token_count ≤ short_ctx_threshold):
        → Transformer Path (existing llamacpp_inference_engine)
    elif (infini_attention_enabled AND context_quality_ok):
        → Infini-attention Path
    elif (ssm_plugin_available):
        → Hybrid SSM Path
    else:
        → Transformer Path (fallback)
    ```
  - Konfigurierbar via `InferenceEngineEnhanced::Config` (bestehend, erweitern)

- [ ] **P3-D02:** Agentic Memory Layer — L1→L2→L3 Rotation
  - L1→L2: `AQLConversationContext` Episodic-Compaction (P2-D03) + SSM-State-Update
  - L2→L3: Drift-triggered RAG-Refresh (`KnowledgeGapDetector` Signal aus P2-D05)
  - L3→L1: Präzisions-Retrieval via `AgenticRAG` (bestehend)
  - Konfigurations-Zentrale: `AgenticRAGConfig` (bestehend, minimale Erweiterung)

- [ ] **P3-D03:** Reasoning-Density-Telemetrie
  - Datei: `src/llm/grafana_metrics.cpp` (erweitern)
  - `themis_reasoning_density_score` Histogram
  - `themis_memory_rotation_l1_to_l2_total` Counter
  - Dashboard-Template: `docs/observability/ssm-agentic-memory-dashboard.json` [PROPOSED]

- [ ] **P3-D04:** `ContextQualityBudget` als vollständiger Ersatz [PROPOSED]
  - `include/llm/context_window_budget.h`: `ContextQualityMetrics` vollständig integriert
  - `ContextWindowBudget` bleibt backward-kompatibel (keine Breaking Changes)
  - `compute()` Factory ergänzt um State-Quality-Input

- [ ] **P3-D05:** Sektor-spezifische RAG-Konfigurationen [PROPOSED]
  - `AgenticRAGFactory::createHighPrecision()` — für Medizin/Recht (kleinere Segmente, 100 % Precision)
  - `AgenticRAGFactory::createFinance()` — Lupe-Modus, hohe `quality_threshold`
  - Basiert auf `AgenticRAGFactory::createAggressive()` (bestehend) als Vorlage

- [ ] **P3-D06:** Tests Phase 3 [PROPOSED]
  - `tests/llm/test_hybrid_context_router.cpp` — Routing-Logik-Tests
  - `tests/rag/test_agentic_memory_rotation.cpp` — L1→L2→L3 End-to-End
  - `tests/llm/test_reasoning_density.cpp` — Metrik-Validierung

### 5.2 Akzeptanzgates Phase 3

| Gate | Bedingung | Nachweis |
|---|---|---|
| P3-GATE-01 | Router wählt korrekte Architektur per Routing-Logik | P3-D06 Test |
| P3-GATE-02 | Agentic Memory L1→L2→L3 Rotation vollständig ohne Datenverlust | P3-D06 Test |
| P3-GATE-03 | Reasoning-Density-Metrik korreliert mit QA-Qualität | P3-D06 Test |
| P3-GATE-04 | Transformer-Baseline unverändert bei Router-Entscheidung → Transformer | Regression |
| P3-GATE-05 | `ctest -L release_critical` vollständig grün | CI-Gate |

---

## 6. Phase 4: GA-Hardening

**Ziel:** Release-Reife für alle Phase-1-3-Features. Security-Review, Chaos-Tests,
vollständige Dokumentation, ROADMAP.md-Update.

**Aufwand:** 3–4 Wochen
**Branch:** `develop`

### 6.1 Deliverables

- [ ] **P4-D01:** Wave-8-kompatible Benchmarks [PROPOSED]
  - Benchmarks nach `benchmarks/wave7/`-Muster (Seed=42, `kW7CanonicalSeed`)
  - Gates: Latenz-p99, Throughput, VRAM-Effizienz, Reasoning-Density
  - Vergleich: Baseline (Phase 0) vs. alle Architekturpfade

- [ ] **P4-D02:** Chaos- und Resilience-Tests [PROPOSED]
  - SSM-State-Korruption: invalidate + resume Roundtrip
  - Infini-attention Backend-Failure: CPU-Fallback aktiviert
  - NVFP4-Quantisierungsfehler: FP16-Fallback-Path
  - Anlehnung an bestehende Wave-6-Tests (`w6c_failure_injection_recovery_test.cpp`)

- [ ] **P4-D03:** Security-Review
  - SSM-State-Serialisierung: keine Credential-Leaks im State-Snapshot
  - Infini-attention Kompressionsmatrix: keine Cross-Session-Datenlecks
  - `SSMStateStore` Zugriffskontrolle: Tenant-Isolation

- [ ] **P4-D04:** Doxygen-API-Dokumentation
  - Alle neuen `[PROPOSED]`-Header vollständig mit `@brief`, `@param`, `@return`, `@throws`
  - `include/llm/i_ssm_plugin.h`, `include/llm/ssm_state_store.h`,
    `include/llm/hybrid_context_router.h`

- [ ] **P4-D05:** ROADMAP.md-Update
  - Alle Phase-1-3-Tasks als `[x]` markieren
  - Neue Items für zukünftige Arbeit (Ring Attention, Multi-Tenant-State) eintragen

- [ ] **P4-D06:** Observability-Dokumentation
  - `docs/observability/ssm-agentic-memory-dashboard.json` vollständig
  - `docs/architecture/ssm-hybrid-analysis.md` Status auf `current` aktualisieren

### 6.2 GA-Akzeptanzgates

| Gate | Bedingung |
|---|---|
| P4-GATE-01 | Alle Wave-8-Benchmark-Gates bestanden |
| P4-GATE-02 | Security-Review ohne High/Critical-Findings |
| P4-GATE-03 | Chaos-Test-Suite: alle Recovery-Szenarien grün |
| P4-GATE-04 | Doxygen-Coverage: alle neuen Public-Headers 100 % |
| P4-GATE-05 | `ctest -L release_critical` vollständig grün |
| P4-GATE-06 | Human-Review und Sign-Off (`docs/governance/GA_PROMOTION_SIGN_OFF.md`) |

---

## 7. Rollback-Strategie

Alle neuen Features werden hinter **Feature-Flags** implementiert:

| Feature | Build-Flag | Runtime-Config | Default |
|---|---|---|---|
| SSM-Plugin | `THEMIS_ENABLE_SSM_PLUGIN` | `ssm_plugin_enabled: false` | Off |
| Infini-attention CPU | `THEMIS_ENABLE_INFINI_CPU` | `infini_attention_mode: disabled` | Off |
| Infini-attention CUDA | `THEMIS_ENABLE_INFINI_CUDA` | `infini_attention_mode: disabled` | Off |
| NVFP4 KV-Quant | `THEMIS_ENABLE_KV_QUANT` | `kv_quantization_bits: 0` | Off |
| SSM-State Stub | `THEMIS_SSM_STUB_MODE` | — | Off |
| Hybrid-Router | `THEMIS_ENABLE_HYBRID_ROUTER` | `hybrid_routing: false` | Off |
| L2 Episodic Compaction | — | `enable_episodic_compaction: false` | Off |

**Rollback-Prozedur:**
1. Feature-Flag in Konfiguration deaktivieren
2. Bestehende `llamacpp_inference_engine.cpp` Pfade sind unberührt
3. `SSMStateStore` Daten bleiben erhalten (kein Datenverlust)
4. Kein Code-Revert erforderlich bei korrekter Flag-Umsetzung

**Kritische Invariante:** Die Plugin-Registrierung via `LLMPluginManager` ist
additiv — bestehende Plugins (llama.cpp) sind nicht betroffen.

---

## 8. Abhängigkeiten und Sequenzierung

```
P0: Verifikation und Baseline
 │
 ├──► P1: Plugin-Infrastruktur (blockiert P2, P3)
 │         │
 │         ├──► P2-D01: NVFP4 (unabhängig, kann parallel zu P1 starten)
 │         │
 │         └──► P2-D02: Infini CUDA (braucht P1-D04 CPU-Fallback als Basis)
 │                   │
 │                   └──► P2-D03: L2 Episodic (braucht P2-D02 Infini als Kontext)
 │                              │
 │                              └──► P3-D01: Hybrid-Router (braucht P1+P2)
 │                                        │
 │                                        └──► P3-D02: Agentic Memory (braucht P3-D01)
 │                                                   │
 │                                                   └──► P4: GA-Hardening
 │
 ├──► L5a: RAID-Sharding Gate (blockiert P3+ produktiv)
 │         │
 │         ├──► Phase-C Thread-Safety/Lock-Ordering Gates grün
 │         ├──► Cross-Shard-Execution stabil (`RemoteExecutor`-Pfad)
 │         └──► Erst danach: produktives P3 Hybrid-Routing + Cross-Shard-Decoding
 │
 ├──► L5b: LoRA/AdaLoRA Federation-Hardening
 │         │
 │         ├──► Cross-Shard Importance-Aggregation (AdaLoRA Rank-Awareness)
 │         └──► Federationspfad mit SSM-State-Distribution-Entscheidung aus P1-D01 ausrichten
 │
 └──► P2-D05: KnowledgeGapDetector Drift-Signal (kann früh parallel starten)
```

### Kritische Blocker (L5)

- **L5a ist ein harter Blocker für Phase 3+ in Produktion:** Ohne Phase-C-Gates im Sharding-System
  (Thread-Safety, Lock-Ordering, Cross-Shard-Stabilität) bleiben Hybrid-Router, Cross-Shard
  Speculative Decoding und KV-Prefix-Sharing auf nicht-produktive Pfade begrenzt.
- **L5b ist ein funktionaler Blocker für verteiltes AdaLoRA-Wissen:** Ohne shard-übergreifende
  Importance-Aggregation verbleiben Rank-Entscheidungen lokal und verhindern konsistente
  Föderationsqualität für SSM-gekoppelte Workloads.

### Parallelisierbare Aufgaben

| Aufgaben | Parallel möglich |
|---|---|
| P0-D01 (FA3-Audit) + P0-D02 (NVFP4-Assessment) + P0-D03 (GGUF-Status) | ✅ Vollständig parallel |
| P1-D05 (Drift-Metriken) + P1-D04 (Infini-CPU) | ✅ Parallel nach P1-D01 |
| P2-D01 (NVFP4) kann nach P0 sofort starten | ✅ Parallel zu P1 |
| P2-D05 (KnowledgeGap) kann nach P1-D05 starten | ✅ Parallel zu P2-D02 |
| L5a Vorarbeit (Phase-C Thread-Safety-Gates) + P2-D01 (NVFP4) | ✅ Parallel nach P0 |
| L5b AdaLoRA Federation-Hardening + P1 Design-Review-Artefakte | ✅ Parallel zu P1/P2 |

---

## 9. ROADMAP.md Einträge (proposed)

Die folgenden Einträge sind als Ergänzung zu `ROADMAP.md` vorgeschlagen:

```markdown
### SSM / Hybrid / Infini-attention / Agentic Memory (Target: 2026-Q3 – 2027-Q2)

*Defined in: docs/architecture/ssm-hybrid-analysis.md · docs/architecture/ssm-hybrid-rollout-plan.md*

#### Phase 0: Voraussetzungen (Q3 2026)
- [ ] P0-D01: FA3 SM90-Kernel-Audit (`src/llm/attention/cuda/`) (Target: Q3 2026)
- [ ] P0-D02: NVFP4 KV-Cache Capability-Assessment (Target: Q3 2026)
- [ ] P0-D03: GGUF SSM-Ökosystem-Status (Target: Q3 2026)
- [ ] P0-D04: Baseline-Benchmarks (Target: Q3 2026)

#### Phase 1: POC (Q3 2026)
- [ ] P1-D01: `ISSMPlugin` Interface (`include/llm/i_ssm_plugin.h`) (Target: Q3 2026)
- [ ] P1-D02: `SSMStateStore` Interface + In-Memory-Impl (Target: Q3 2026)
- [ ] P1-D03: SSM-Stub-Plugin für Datenfluss-Validierung (Target: Q3 2026)
- [ ] P1-D04: Infini-attention CPU-Fallback (Target: Q3 2026)
- [ ] P1-D05: Drift-Metriken Prometheus (Target: Q3 2026)
- [ ] P1-D06: `ContextQualityBudget` Erweiterung (Target: Q3 2026)
- [ ] P1-D07: Unit-Tests Phase 1 (Target: Q3 2026)
- [ ] P1-D08: Mamba Governance Contract (Security + SoR/RocksDB constraints) (Target: Q3 2026)
- [ ] P1-L5-01: Human Design Review `SSMStateStore` Distribution (`replication` vs `shard_partitioned`) (Target: Q3 2026)

#### Phase 2: Beta (Q4 2026)
- [ ] P2-D01: NVFP4 KV-Cache-Quantisierung (`PagedKVCache::Config`) (Target: Q4 2026)
- [ ] P2-D02: Infini-attention CUDA-Kernel (Target: Q4 2026)
- [ ] P2-D03: L2 Episodic Memory Komprimierung (`AQLConversationContext`) (Target: Q4 2026)
- [ ] P2-D04: SSM-State RocksDB-Persistierung (Target: Q4 2026)
- [ ] P2-D05: KnowledgeGapDetector SSM-Drift-Signal (Target: Q4 2026)
- [ ] P2-D06: Tests und Benchmarks Phase 2 (Target: Q4 2026)
- [ ] P2-L5-01: LoRA Federation um AdaLoRA Rank-Awareness + Cross-Shard-Importance erweitern (Target: Q4 2026)
- [ ] P2-L5-02: LLM-RAID Phase-1 Routing-Pfad (`LLMAQLHandler` + AdaptiveShardRouter Domain-Routing) verifizieren/härten (Target: Q4 2026)

#### Phase 3: Beta+ (Q1 2027)
- [ ] P3-D01: `HybridContextRouter` (Target: Q1 2027)
- [ ] P3-D02: Agentic Memory Layer L1+L2+L3 Rotation (Target: Q1 2027)
- [ ] P3-D03: Reasoning-Density-Telemetrie (Target: Q1 2027)
- [ ] P3-D04: `ContextQualityBudget` vollständig (Target: Q1 2027)
- [ ] P3-D05: Sektor-spezifische RAG-Konfigurationen (Target: Q1 2027)
- [ ] P3-L5-01: Produktionsfreigabe nur mit bestandenem L5a Sharding Phase-C Gate (Target: Q1 2027)

#### Phase 4: GA-Hardening (Q2 2027)
- [ ] P4-D01: Wave-8-kompatible Benchmarks (Target: Q2 2027)
- [ ] P4-D02: Chaos- und Resilience-Tests (Target: Q2 2027)
- [ ] P4-D03: Security-Review (Target: Q2 2027)
- [ ] P4-D04: Doxygen-Dokumentation (Target: Q2 2027)
- [ ] P4-D06: Observability-Dokumentation (Target: Q2 2027)
```
