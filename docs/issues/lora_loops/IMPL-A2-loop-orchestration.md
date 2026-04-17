---
type: enhancement
labels: ["type:enhancement", "module:training", "module:rag", "priority:high", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: IMPL-EPIC-A
session: S-3
---

# [IMPL-A2] LoRA Foundation: Loop 1–4 Explicit Orchestration

## Aufgabe

Das `THEMISDB_LORA_RESEARCH_PAPER.md` §5 definiert vier Lernschleifen
(Loops 1–4) als das Herzstück des autonomen Optimierungssystems.
`ContinuousLearningOrchestrator` existiert, aber die vier Loops sind darin
nicht explizit als named states/phases implementiert — sie müssen als
klar benannte, zustandsbasierte Orchestrierungsphasen modelliert werden,
damit spätere Erweiterungen (FEDERATED_ROUND_START in IMPL-A3) einen
definierten Anknüpfungspunkt haben.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| Loop 1: HNSW/BaoOptimizer automatisches LoRA-Retraining | Neue HNSW-Implementierung |
| Loop 2: WorkloadAdaptiveOptimizer Feedback-Integration | Änderungen am WorkloadAdaptiveOptimizer |
| Loop 3: Autonomous Schema/Index Advisor | Online-DDL-Execution (Advisory-only) |
| Loop 4: RLAIF Constitutional AI semi-autonome Schleife | Neue RLAIF-Algorithmen |
| `LoopState` Enum + `currentLoop()` + `advanceLoop()` API | Vollständige UI für Loop-Status |
| `LoopCompletionEvent` (für IMPL-A3: FEDERATED_ROUND_START) | Persistenz der Loop-States über Neustarts |

## Idee / Konzept

```
Loop 1 — HNSW/Query (täglich, vollautomatisch):
  Signal:  HNSWParameterTuner.recall_trend < 0.93 ODER BaoOptimizer.miss_rate > 0.15
  Action:  IncrementalLoRATrainer.runIncremental(dataset, loop=1)
  Guardrail: ECE < 0.05 AND hot_coverage >= 0.85
  Output:  neuer Adapter v(n+1) in AdapterRegistry

Loop 2 — Workload (wöchentlich, vollautomatisch):
  Signal:  WorkloadAdaptiveOptimizer.profile_drift > threshold
  Action:  IncrementalLoRATrainer.runIncremental(workload_samples, loop=2)
  Guardrail: kein Rückschritt in avg_speedup
  Output:  WorkloadProfile-Update in TenantManager

Loop 3 — Schema/Index (wöchentlich, Advisory-only):
  Signal:  IndexSuggestionEngine.pending_suggestions > 0 ODER DocumentSchemaEvolution.drift
  Action:  Adapter-Finetuning auf Schema-Samples + Report an AIDecisionAuditor
  Guardrail: DBA-Review-Pflicht vor jeder DDL
  Output:  DecisionRecord in AIDecisionAuditor

Loop 4 — RLAIF (monatlich, semi-autonom):
  Signal:  FeedbackCollector.new_entries >= 100 ODER manuell
  Action:  RLAIFTrainer.trainFromPreferencePairs() + DBA-Akzeptanzcheck
  Guardrail: DBA-Akzeptanzrate >= 0.75 über letzte 30 Decisions
  Output:  ConstitutionalAI-Update, Adapter-Promotion wenn >= 0.75
```

## Technische Details

### LoopState in ContinuousLearningOrchestrator

```cpp
enum class LoopPhase {
    LOOP_1_HNSW_QUERY    = 1,
    LOOP_2_WORKLOAD      = 2,
    LOOP_3_SCHEMA_INDEX  = 3,
    LOOP_4_RLAIF         = 4,
    IDLE                 = 0
};

// Neue öffentliche APIs
LoopPhase currentLoop() const;
void triggerLoop(LoopPhase phase);
void registerLoopCompletionHandler(
    LoopPhase phase,
    std::function<void(LoopPhase, const LoopResult&)> handler
);

struct LoopResult {
    LoopPhase  phase;
    bool       success;
    bool       guardrail_passed;
    std::string adapter_version;   // neu registrierte Version (wenn applicable)
    double     metric_delta;       // Δ(primary_metric) dieser Runde
};
```

### Guardrail-Integration

Bestehende Guardrail-Logik (`ECE`, `hot_coverage`, `dba_acceptance_rate`)
wird in `shouldAutonomouslyApply(result)` gekapselt und von jedem Loop
vor dem Adapter-Commit aufgerufen — dokumentiert in `THEMISDB_LORA_METRICS_AND_OVERVIEW.md §5`.

### Loop-Trigger-Signale

| Loop | Signal-Quelle | Threshold | Getter |
|---|---|---|---|
| L1 | `HNSWParameterTuner::recallTrend()` | < 0.93 | bestehend |
| L1 | `BaoOptimizer::getMissRate()` | > 0.15 | neu — kleiner Accessor |
| L2 | `WorkloadAdaptiveOptimizer::getProfileDrift()` | > 0.1 | neu — kleiner Accessor |
| L3 | `IndexSuggestionEngine::pendingSuggestions()` | > 0 | bestehend |
| L4 | `FeedbackCollector::newEntryCount()` | >= 100 | neu — kleiner Accessor |

## Abhängigkeiten

- **Vorbedingung:** IMPL-A1 (Dataset vorhanden für Loop-1-Training)
- **Blockiert:** IMPL-A3 (`LoopCompletionEvent` ist Anknüpfungspunkt für `FEDERATED_ROUND_START`)

## Erfolgskriterien

- [ ] `LoopPhase` Enum in `ContinuousLearningOrchestrator` vorhanden
- [ ] `currentLoop()` gibt korrekte aktive Phase zurück
- [ ] `triggerLoop(LOOP_1_HNSW_QUERY)` startet `IncrementalLoRATrainer` mit korrekten Parametern
- [ ] `triggerLoop(LOOP_4_RLAIF)` startet `RLAIFTrainer::trainFromPreferencePairs()`
- [ ] `registerLoopCompletionHandler()` wird nach Abschluss jedes Loops aufgerufen
- [ ] Guardrail-Check verhindert Adapter-Commit wenn `ECE >= 0.05`
- [ ] `LoopResult::guardrail_passed = false` wird korrekt gesetzt und geloggt
- [ ] `BaoOptimizer::getMissRate()` neuer Accessor vorhanden
- [ ] `WorkloadAdaptiveOptimizer::getProfileDrift()` neuer Accessor vorhanden
- [ ] `FeedbackCollector::newEntryCount()` neuer Accessor vorhanden
- [ ] 10 neue Tests in `tests/test_continuous_learning_orchestrator.cpp`
- [ ] Keine Regressions in bestehenden Orchestrator-Tests

## Definition of Done

`triggerLoop(LOOP_1_HNSW_QUERY)` startet, der Guardrail-Check schlägt
fehl wenn `ECE >= 0.05` (simuliert im Test), und der
`LoopCompletionHandler` wird mit `LoopResult::success=false` aufgerufen.
