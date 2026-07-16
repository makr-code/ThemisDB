---
type: enhancement
labels: ["type:enhancement", "module:training", "module:distributed_knowledge", "priority:high", "status:open", "queue/copilot"]
milestone: v2.0.0
parent: IMPL-EPIC-A
session: S-4
---

# [IMPL-A3] LoRA Foundation: Federation Bridges (exportGradient / applyGlobalDelta / FEDERATED_ROUND_START)

## Aufgabe

Die drei fehlenden Brücken zwischen Paper 1 (LoRA Training) und Paper 3
(Distributed Knowledge) implementieren: `IncrementalLoRATrainer` bekommt
`exportGradient()` und `applyGlobalDelta()`, und `ContinuousLearningOrchestrator`
bekommt `TriggerEvent::FEDERATED_ROUND_START` als fünften Loop-Trigger.
Ohne diese Brücken kann DK-3 (Federated LoRA) nicht funktionieren.

## Abgrenzung

| In Scope | Out of Scope |
|---|---|
| `IncrementalLoRATrainer::exportGradient(round)` | Neues Gewichtsformat |
| `IncrementalLoRATrainer::applyGlobalDelta(delta)` | DP-Noise-Berechnung (liegt in LoRAFederationCoordinator) |
| `TriggerEvent::FEDERATED_ROUND_START` im Orchestrator | Admin-API für manuellen Trigger (→ DK-7) |
| `LoopCompletionEvent` feuert FEDERATED_ROUND_START nach Loop-4 | Scheduler-Integration |
| 7 neue Tests (Trainer: 5, Orchestrator: 2) | Performance-Benchmarks |

## Idee / Konzept

`IncrementalLoRATrainer` akkumuliert intern bereits Gewichts-Deltas
(für die Versionierung im `AdapterRegistry`). `exportGradient()` liest
dieses akkumulierte Delta aus ohne es zurückzusetzen — erst `applyGlobalDelta()`
schreibt neue Informationen zurück.

```
Normaler Ablauf (Paper 1, Loop 1–4):
  train() → Gewicht w_lokal ändert sich um Δw_lokal
  exportGradient() → gibt Δw_lokal seit letztem Export zurück + reset Akkumulator
  [via LoRAFederationCoordinator: FedAvg + DP]
  applyGlobalDelta(Δw_global) → w_lokal += lr * Δw_global

Trigger-Kette (Paper 3, FEDERATED_ROUND_START):
  Loop-4 abgeschlossen → LoopCompletionHandler feuert
  → ContinuousLearningOrchestrator.triggerEvent(FEDERATED_ROUND_START)
  → orchestrator ruft exportGradient() auf eigeknüpftem Trainer auf
  → coordinator.submitGradient(gradient)
```

### Datenschutz-Invariante für exportGradient()

Das exportierte Delta **darf keine** Rohtexte aus Trainingsdokumenten enthalten.
`data` ist eine Map von Layer-Namen auf Float-Deltas:
```json
{"lora_A_layer_0": 0.0012, "lora_B_layer_0": -0.0034, ...}
```
Kein Schlüssel enthält Query-Text, kein Wert enthält Text. Diese Invariante
wird in DK-6 Scenario 5 (Privacy Test) verifiziert.

## Technische Details

### exportGradient() — IncrementalLoRATrainer

```cpp
// Neue Methode
EncryptedGradient exportGradient(uint64_t federation_round);
```

**Verhalten:**
- `data`: JSON-Map `{layer_name → Σ(weight_delta) / update_count}` — normalisiertes Delta
- `sample_count`: Anzahl Trainingssamples seit letztem `exportGradient()` oder Trainer-Start
- `shard_id`: aus injizierter Shard-ID (DI-Setter `setShardId(string)`)
- Wirft `std::runtime_error("no training since last export")` wenn `update_count == 0`
- Setzt internen Delta-Akkumulator auf 0 nach Export

**Akkumulator-Implementierung:**
```cpp
// Existierende Mitglieder nutzen/ergänzen:
std::unordered_map<std::string, double> gradient_accumulator_;
size_t gradient_update_count_ = 0;
// In train(): gradient_accumulator_[layer] += delta; gradient_update_count_++;
```

### applyGlobalDelta() — IncrementalLoRATrainer

```cpp
void applyGlobalDelta(const GlobalAdapterDelta& delta);
```

**Verhalten:**
- Iteriert über `delta.delta` Map
- Für jeden bekannten Layer-Namen: `local_weight[layer] += learning_rate * delta.delta[layer]`
- Unbekannte Layer-Namen im Delta: ignoriert (kein Fehler — Forward-compatible)
- Schreibt `DecisionRecord{decision_type="FEDERATED_DELTA_APPLIED", ...}` in `AIDecisionAuditor`
- `learning_rate`: konfigurierbar via `setFederatedLearningRate(double)`, default `0.01`

### FEDERATED_ROUND_START — ContinuousLearningOrchestrator

```cpp
// Erweiterung des bestehenden TriggerEvent-Enums (oder neues Enum)
enum class TriggerEvent {
    // ... bestehende Events ...
    FEDERATED_ROUND_START  // neu: nach Loop-4-Abschluss
};
```

**Automatische Auslösung:**
```cpp
// In LoopCompletionHandler für LOOP_4_RLAIF:
if (result.success && result.guardrail_passed) {
    triggerEvent(TriggerEvent::FEDERATED_ROUND_START);
}
```

**Handler-Implementierung:**
```cpp
void handleFederatedRoundStart() {
    if (!trainer_ || !coordinator_) return;
    auto gradient = trainer_->exportGradient(coordinator_->currentRound());
    coordinator_->submitGradient(gradient);
}
```

DI-Setter: `setFederationCoordinator(shared_ptr<ILoRAFederationCoordinator>)`

### Neue Tests

**tests/test_incremental_lora_trainer.cpp:**
- `ILT-EG-01` exportGradient() nach mindestens einem train()-Aufruf gibt non-empty data zurück
- `ILT-EG-02` exportGradient() wirft wenn kein Training seit letztem Export
- `ILT-EG-03` exportGradient() setzt Akkumulator zurück (zweiter Export direkt danach wirft)
- `ILT-AG-01` applyGlobalDelta() verändert lokale Gewichte messbar (mock Layer-Map)
- `ILT-AG-02` applyGlobalDelta() mit unbekannten Layer-Namen wirft nicht

**tests/test_continuous_learning_orchestrator.cpp:**
- `CLO-FED-01` FEDERATED_ROUND_START triggert exportGradient + submitGradient in korrekter Reihenfolge
- `CLO-FED-02` FEDERATED_ROUND_START feuert nur nach Loop-4 mit guardrail_passed=true

## Abhängigkeiten

- **Vorbedingung:** IMPL-A2 (LoopPhase + LoopCompletionHandler vorhanden)
- **Blockiert:** DK-3 (`LoRAFederationCoordinator` + Trainer vollständig verdrahtbar)

## Erfolgskriterien

- [ ] `IncrementalLoRATrainer::exportGradient(round)` vorhanden
- [ ] `exportGradient()` gibt `EncryptedGradient` mit non-empty `data` zurück nach Training
- [ ] `exportGradient()` wirft `std::runtime_error` ohne vorheriges Training
- [ ] `exportGradient()` setzt internen Akkumulator zurück
- [ ] `IncrementalLoRATrainer::applyGlobalDelta(delta)` vorhanden
- [ ] `applyGlobalDelta()` verändert lokale Gewichte nachweisbar
- [ ] `applyGlobalDelta()` schreibt `FEDERATED_DELTA_APPLIED` in `AIDecisionAuditor`
- [ ] `TriggerEvent::FEDERATED_ROUND_START` vorhanden und kompilierbar
- [ ] `FEDERATED_ROUND_START` feuert nur wenn `guardrail_passed = true`
- [ ] `setFederationCoordinator()` DI-Setter vorhanden
- [ ] 7 neue Tests grün
- [ ] Keine Regressions in bestehenden Trainer-Tests

## Definition of Done

Mock-Trainer auf 2 Shards: `exportGradient()` → `submitGradient()` →
`applyGlobalDelta()` Zyklus vollständig ohne Fehler. `AIDecisionAuditor`
enthält `FEDERATED_DELTA_APPLIED` Eintrag.
