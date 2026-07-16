---
type: enhancement
labels: ["type:enhancement", "module:distributed_knowledge", "module:training", "priority:high", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: DK-0-EPIC
session: 4
---

# [DK-3] distributed_knowledge: Layer B — Federated LoRA Aggregation

## Aufgabe

Verbinde `LoRAFederationCoordinator` mit der bestehenden Trainings-Pipeline:
`IncrementalLoRATrainer` bekommt zwei neue Hooks (`exportGradient` / `applyGlobalDelta`)
und `ContinuousLearningOrchestrator` erhält ein neues Trigger-Event
`FEDERATED_ROUND_START`. Damit fließen DP-geschützte Gradienten zwischen Shards —
das ist der **Kern der verteilten Intelligenz**.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `IncrementalLoRATrainer::exportGradient(round)` | Neues ML-Modell oder Gewichtsformat |
| `IncrementalLoRATrainer::applyGlobalDelta(delta)` | GPU-Beschleunigung der Aggregation (→ Future) |
| `ContinuousLearningOrchestrator::TriggerEvent::FEDERATED_ROUND_START` | Änderung des DP-Algorithmus |
| Callback: `coordinator → applyGlobalDelta()` auf allen registrierten Shards | Admin-API (→ DK-7) |
| 7 neue Tests (Trainer: 5, Orchestrator: 2) | Performance-Benchmarks (→ DK-8) |

## Idee / Konzept

`IncrementalLoRATrainer` speichert bereits Gewichts-Deltas zwischen Trainingsläufen
(für Versionierung). `exportGradient()` liest dieses Delta aus und verpackt es als
`EncryptedGradient`. `applyGlobalDelta()` addiert den aggregierten globalen Delta
auf die lokalen Gewichte — identisch zur FedAvg-Update-Regel:

```
w_lokal_neu = w_lokal_alt + learning_rate * global_delta
```

Das `FEDERATED_ROUND_START`-Event im `ContinuousLearningOrchestrator` triggert
nach Loop-4-Abschluss (wöchentlich) automatisch den Export → Submission → Aggregation
→ Apply-Zyklus.

```
Loop 4 abgeschlossen (wöchentlich)
  ↓ ContinuousLearningOrchestrator fires FEDERATED_ROUND_START
  ↓ alle Shards: IncrementalLoRATrainer.exportGradient(round)
  ↓ alle Shards: coordinator.submitGradient(gradient)
  ↓ [nach min_participants Submissions]
  ↓ LoRAFederationCoordinator.doAggregation() + DP-Noise
  ↓ GlobalAdapterDelta via callback an alle Shards
  ↓ alle Shards: IncrementalLoRATrainer.applyGlobalDelta(delta)
```

### Differential Privacy — Warum es sicher ist

Der Gaussian-Mechanismus garantiert `(ε=0.1, δ=1e-5)-DP`:
- `σ = 1.0 · √(2·ln(1.25/1e-5)) / 0.1 ≈ 46.7`
- Für typische LoRA-Rank-8-Gradienten (‖Δw‖ ≈ 0.01) ist der SNR > 1 — das Rauschen
  überdeckt individuelle Shard-Beiträge, schadet aber dem Lernsignal kaum
  (Hypothese RQ-DK-2, empirisch zu validieren in DK-6)

## Technische Details

### Sub-Issue 3a — IncrementalLoRATrainer Hooks

**Datei:** `include/training/incremental_lora_trainer.h` + entsprechende `.cpp`

```cpp
// Exportiert das akkumulierte Gewichts-Delta seit dem letzten export- oder apply-Aufruf
EncryptedGradient exportGradient(uint64_t federation_round);

// Addiert globales Delta auf lokale Gewichte; schreibt Delta-Version ins Audit-Log
void applyGlobalDelta(const GlobalAdapterDelta& delta);
```

**`exportGradient()` Verhalten:**
- `data`: JSON-Map `{layer_name → weight_delta_sum / update_count}`
  (normalisiertes Delta, nicht rohe Gewichte)
- `sample_count`: Anzahl Trainingssamples seit letztem Export
- Wirft `std::runtime_error` wenn seit letztem Export kein Training stattfand
- Reset interner Akkumulator nach Export

**`applyGlobalDelta()` Verhalten:**
- Addiert `delta.delta[layer_name]` auf lokale Gewichte skaliert mit `learning_rate`
- Schreibt `{delta.version, delta.participants, applied_at}` in `AIDecisionAuditor`
- Thread-safe (bestehender Trainer-Mutex)

### Sub-Issue 3b — ContinuousLearningOrchestrator Trigger

**Datei:** `include/rag/continuous_learning_orchestrator.h`

```cpp
enum class TriggerEvent {
    // ... bestehende Events ...
    FEDERATED_ROUND_START   // neu: nach Loop-4-Abschluss
};
```

**Behavior bei `FEDERATED_ROUND_START`:**
1. Ruft `trainer.exportGradient(coordinator.currentRound())` auf
2. Ruft `coordinator.submitGradient(gradient)` auf
3. Registriert `applyGlobalDelta`-Callback wenn noch nicht registriert

**Registrierung der Shards beim Coordinator:**
```cpp
coordinator.setGlobalDeltaCallback([&trainer](const GlobalAdapterDelta& delta) {
    trainer.applyGlobalDelta(delta);
});
```

### Neue Tests

**`tests/test_incremental_lora_trainer.cpp`:**
- `ILT-FED-01` exportGradient() nach mindestens einem Training-Pass gibt non-empty data zurück
- `ILT-FED-02` exportGradient() wirft wenn kein Training seit letztem Export
- `ILT-FED-03` applyGlobalDelta() verändert lokale Gewichte nachweisbar
- `ILT-FED-04` applyGlobalDelta() mit leerem delta ist no-op (keine Fehler)
- `ILT-FED-05` exportGradient() enthält keine Roh-Trainingsdaten (nur Deltas)

**`tests/test_continuous_learning_orchestrator.cpp`:**
- `CLO-FED-01` FEDERATED_ROUND_START feuert exportGradient + submitGradient
- `CLO-FED-02` applyGlobalDelta-Callback wird nach Aggregation aufgerufen

## Abhängigkeiten

- **Vorbedingung:** DK-1 (Tests laufen), DK-2 (Gossip-Transport für Delta-Broadcast)
- **Blockiert:** DK-6 (Layer-B-Szenario in Integrationstest), DK-7 (Audit-Log)

## Erfolgskriterien

- [ ] `IncrementalLoRATrainer::exportGradient(round)` vorhanden und dokumentiert
- [ ] `exportGradient()` gibt `EncryptedGradient` mit non-empty `data` nach Training zurück
- [ ] `exportGradient()` wirft `std::runtime_error` ohne vorheriges Training
- [ ] `IncrementalLoRATrainer::applyGlobalDelta(delta)` verändert lokale Gewichte
- [ ] `applyGlobalDelta()` schreibt Version-Eintrag in `AIDecisionAuditor`
- [ ] `TriggerEvent::FEDERATED_ROUND_START` im `ContinuousLearningOrchestrator` vorhanden
- [ ] `FEDERATED_ROUND_START` triggert Export und Submit in korrekter Reihenfolge
- [ ] 5 Trainer-Tests grün, 2 Orchestrator-Tests grün
- [ ] `EncryptedGradient::data` enthält keine Rohtexte oder Original-Dokumente
- [ ] Keine Regressions in bestehenden Trainer-Tests

## Definition of Done

`IncrementalLoRATrainer` auf 3 In-Process-Mock-Shards: Ein vollständiger
Federation-Zyklus (Export → Aggregation + DP → Apply) läuft durch ohne Fehler.
Der globale Delta ist in jedem Shard's `AIDecisionAuditor` protokolliert.
